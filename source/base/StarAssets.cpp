#include "StarAssets.hpp"
#include "StarAssetPath.hpp"
#include "StarFile.hpp"
#include "StarTime.hpp"
#include "StarDirectoryAssetSource.hpp"
#include "StarPackedAssetSource.hpp"
#include "StarMemoryAssetSource.hpp"
#include "StarJsonBuilder.hpp"
#include "StarJsonExtra.hpp"
#include "StarJsonPatch.hpp"
#include "StarIterator.hpp"
#include "StarImageProcessing.hpp"
#include "StarLogging.hpp"
#include "StarRandom.hpp"
#include "StarFont.hpp"
#include "StarAudio.hpp"
#include "StarCasting.hpp"
#include "StarLexicalCast.hpp"
#include "StarSha256.hpp"
#include "StarDataStreamDevices.hpp"
#include "StarLua.hpp"
#include "StarLuaConverters.hpp"
#include "StarImageLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"

namespace Star {

static void validateBasePath(std::string_view const& basePath) {
  if (basePath.empty() || basePath[0] != '/')
    throw AssetException(strf("Path '{}' must be absolute", basePath));

  bool first = true;
  bool slashed = true;
  bool dotted = false;
  for (auto c : basePath) {
    if (c == '/') {
      if (!first) {
        if (slashed)
          throw AssetException(strf("Path '{}' contains consecutive //, not allowed", basePath));
        else if (dotted)
          throw AssetException(strf("Path '{}' '.' and '..' not allowed", basePath));
      }
      slashed = true;
      dotted = false;
    } else if (c == ':') {
      if (slashed)
        throw AssetException(strf("Path '{}' has ':' after directory", basePath));
      break;
    } else if (c == '?') {
      if (slashed)
        throw AssetException(strf("Path '{}' has '?' after directory", basePath));
      break;
    } else {
      slashed = false;
      dotted = c == '.';
    }
    first = false;
  }
  if (slashed)
    throw AssetException(strf("Path '{}' cannot be a file", basePath));
}

static void validatePath(AssetPath const& components, bool canContainSubPath, bool canContainDirectives) {
  validateBasePath(components.basePath.utf8());

  if (!canContainSubPath && components.subPath)
    throw AssetException::format("Path '{}' cannot contain sub-path", components);
  if (!canContainDirectives && !components.directives.empty())
    throw AssetException::format("Path '{}' cannot contain directives", components);
}

static void validatePath(StringView const& path, bool canContainSubPath, bool canContainDirectives) {
  std::string_view const& str = path.utf8();

  size_t end = str.find_first_of(":?");
  auto basePath = str.substr(0, end);
  validateBasePath(basePath);

  bool subPath = false;
  if (str[end] == ':') {
    size_t beg = end + 1;
    if (beg != str.size()) {
      end = str.find_first_of('?', beg);
      if (end == NPos && beg + 1 != str.size())
        subPath = true;
      else if (size_t len = end - beg)
        subPath = true;
    }
  }

  if (subPath)
    throw AssetException::format("Path '{}' cannot contain sub-path", path);

  if (end != NPos && str[end] == '?' && !canContainDirectives)
    throw AssetException::format("Path '{}' cannot contain directives", path);
}

Maybe<RectU> FramesSpecification::getRect(String const& frame) const {
  if (auto alias = aliases.ptr(frame)) {
    return frames.get(*alias);
  } else {
    return frames.maybe(frame);
  }
}

Assets::Assets(Settings settings, StringList assetSources) {
  const char* const AssetsPatchSuffix = ".patch";
  const char* const AssetsLuaPatchSuffix = ".patch.lua";
  const char* const AssetsPlutoPatchSuffix = ".patch.pluto";

  m_settings = std::move(settings);
  m_stopThreads = false;
  m_assetSources = std::move(assetSources);

  // OpenStarbound: Run any asset loading scripts, then load any modified assets and asset patches (both JSON and Lua).
  auto luaEngine = LuaEngine::create();
  m_luaEngine = luaEngine;
  luaEngine->tuneAutoGarbageCollection(m_settings.luaGcPause, m_settings.luaGcStepMultiplier);

  auto pushGlobalContext = [&luaEngine](String const& name, LuaCallbacks && callbacks) {
    auto table = luaEngine->createTable();
    for (auto const& p : callbacks.callbacks())
      table.set(p.first, luaEngine->createWrappedFunction(p.second));
    luaEngine->setGlobal(name, table);
  };

  auto makeBaseAssetCallbacks = [this](bool addMetadataCallback) {
    LuaCallbacks callbacks;
    callbacks.registerCallbackWithSignature<Json, String>("json", bind(&Assets::json, this, _1));

    callbacks.registerCallback("byExtension", [this](String const& ext) -> StringList {
      StringList assetList{};
      for (auto& asset : scanExtension(ext))
        assetList.append(asset);
      return assetList;
    });

    callbacks.registerCallback("bytes", [this](String const& path) -> String {
      auto assetBytes = bytes(path);
      return String(assetBytes->ptr(), assetBytes->size());
    });

    callbacks.registerCallback("rawBytes", [this](String const& path) -> ByteArray {
      auto assetBytes = bytes(path);
      return *assetBytes;
    });

    callbacks.registerCallback("newRawBytes", [this]() -> ByteArray {
      return ByteArray();
    });

    callbacks.registerCallback("image", [this](String const& path) -> Image {
      auto assetImage = image(path);
      if (assetImage->bytesPerPixel() == 3)
        return assetImage->convert(PixelFormat::RGBA32);
      else
        return *assetImage;
    });

    callbacks.registerCallback("frames", [this](String const& path) -> Json {
      if (auto frames = imageFrames(path))
      return JsonObject{
        {"aliases", jsonFromMap(frames->aliases)},
        {"frames", jsonFromMapV(frames->frames, jsonFromRectU)},
        {"file", frames->framesFile}
      };
      return Json();
    });

    callbacks.registerCallback("newImage", [this](Vec2U const& size) -> Image {
      return Image::filled(size, Vec4B::filled(0));
    });

    callbacks.registerCallback("scan", [this](Maybe<String> const& a, Maybe<String> const& b) -> StringList {
      return b ? scan(a.value(), *b) : scan(a.value());
    });

    // FezzedOne: Checks if an asset exists. Should be case-insensitive and will return newly added files.
    // Won't return any assets that haven't yet been loaded.
    callbacks.registerCallback("exists", [this](Maybe<String> const& path) -> bool {
      if (path)
        return this->assetExists(*path);
      return false;
    });

    // FezzedOne: Lists all patches for a given asset. Returns `nil` if that asset doesn't (yet) exist.
    // Won't return any patches that haven't yet been loaded.
    auto patchSources = [this](String const& path, Maybe<bool> returnFilePaths) -> Json {
      if (returnFilePaths && *returnFilePaths) {
        if (auto assetDescriptor = this->assetDescriptor(path)) {
          JsonArray returnValue{};
          auto& patchSources = assetDescriptor->patchSources;
          returnValue.reserve(patchSources.size());
          for (auto& sourcePair : patchSources) {
            auto sourcePath = this->assetSourcePath(sourcePair.second);
            returnValue.emplaceAppend(JsonArray{sourcePath ? *sourcePath : Json(), sourcePair.first});
          }
          return returnValue;
        }
        return Json();
      } else {
        JsonArray patchSourceList{};
        if (auto file = this->assetDescriptor(path)) {
          for (auto patchSource : file->patchSources) {
            patchSourceList.append(patchSource.first);
          }
          return patchSourceList;
        }
        return Json();
      }
    };

    callbacks.registerCallback("patches", patchSources);
    callbacks.registerCallback("patchSources", patchSources);

    // FezzedOne: Lists all asset source file paths, regardless of whether they're loaded yet.
    callbacks.registerCallback("sources", [this]() -> StringList {
      return this->assetSources();
    });

    callbacks.registerCallback("sourcePaths", [this](String const& path) -> Maybe<StringList> {
      try {
        return this->assetSourcePaths(path);
      } catch (MapException const &e) {
        return {};
      }
      return {};
    });

    // Added WereTech's `assets.origin` from OpenStarbound.
    callbacks.registerCallback("source", [this](String const& path) -> Maybe<String> {
      if (this->assetDescriptor(path))
        return this->assetSource(path);
      return {};
    });

    callbacks.registerCallback("origin", [this](String const& path) -> Maybe<String> {
      if (this->assetDescriptor(path))
        return this->assetSource(path);
      return {};
    });

    if (addMetadataCallback) {
      callbacks.registerCallback("sourceMetadata", [this](String const& path) -> Json {
        if (!m_assetSourcePaths.hasLeftValue(path)) return Json();
        return this->assetSourceMetadata(path);
      });
    }

    return callbacks;
  };

  pushGlobalContext("sb", LuaBindings::makeUtilityCallbacks());
  pushGlobalContext("xsb", LuaBindings::makeXsbCallbacks());
  pushGlobalContext("assets", makeBaseAssetCallbacks(true));

  List<pair<String, AssetSourcePtr>> sources;

  auto addSource = [&](String const& sourcePath, AssetSourcePtr source) {
    m_assetSourcePaths.add(sourcePath, source);

    for (auto const& filename : source->assetPaths()) {
      if (filename.contains(AssetsPatchSuffix, String::CaseInsensitive)) {
        if (filename.endsWith(AssetsPatchSuffix, String::CaseInsensitive)) {
          auto targetPatchFile = filename.substr(0, filename.size() - strlen(AssetsPatchSuffix));
          if (auto p = m_files.ptr(targetPatchFile))
            p->patchSources.append({filename, source});
        } else if (filename.endsWith(AssetsLuaPatchSuffix, String::CaseInsensitive)) {
          auto targetPatchFile = filename.substr(0, filename.size() - strlen(AssetsLuaPatchSuffix));
          if (auto p = m_files.ptr(targetPatchFile))
            p->patchSources.append({filename, source});
        } else if (filename.endsWith(AssetsPlutoPatchSuffix, String::CaseInsensitive)) {
          auto targetPatchFile = filename.substr(0, filename.size() - strlen(AssetsPlutoPatchSuffix));
          if (auto p = m_files.ptr(targetPatchFile))
            p->patchSources.append({filename, source});
        } else {
          for (int i = 0; i < 10; i++) {
            if (filename.endsWith(AssetsPatchSuffix + toString(i), String::CaseInsensitive)) {
              auto targetPatchFile = filename.substr(0, filename.size() - strlen(AssetsPatchSuffix) + 1);
              if (auto p = m_files.ptr(targetPatchFile))
                p->patchSources.append({filename, source});
              break;
            }
          }
        }
      }

      auto& descriptor = m_files[filename];
      descriptor.sourceName = filename;
      descriptor.source = source;
      m_filesByExtension[AssetPath::extension(filename).toLower()].insert(filename);
    }
  };

  auto runLoadScripts = [&](String const& groupName, String const& sourcePath, AssetSourcePtr source, List<pair<String, AssetSourcePtr>> const& sources) {
    // Runs any load scripts specified in the asset source's `_metadata` file. Not to be confused with `.patch.lua` scripts!
    auto metadata = source->metadata();
    if (auto scripts = metadata.ptr("scripts")) {
      if (auto scriptGroup = scripts->optArray(groupName)) {
        auto sourceName = metadata.value("name", File::baseName(sourcePath));

        String sourceNameStr = strf("{}", sourceName);
        if (sourceName.type() == Json::Type::String) sourceNameStr = sourceName.toString();

        String memoryName = strf("{}::{}", sourceNameStr, groupName);
        // FezzedOne: Add all metadata keys from the base asset source to any memory asset source derived from it, then change the name accordingly.
        JsonObject memoryMetadata = source->metadata(); // { {"name", memoryName} };
        memoryMetadata.set("name", memoryName);
        auto memoryAssets = make_shared<MemoryAssetSource>(memoryName, memoryMetadata);

        Logger::info("Running {} preprocessing scripts for asset source '{}': {}", groupName == "onLoad" ? "on-load" : "post-load", sourceNameStr, *scriptGroup);

        try {
          auto context = luaEngine->createContext();

          /* FezzedOne: Inlined `decorateLuaContext` to fix an optimisation-related segfault on some builds. */
          if (memoryAssets) {
            // Kae: Re-add the assets callbacks with more callbacks.
            context.remove("assets");
            auto callbacks = makeBaseAssetCallbacks(false);
            callbacks.registerCallback("add", [this, &source, &memoryAssets](LuaEngine& engine, String const& path, LuaValue const& data) {
              ByteArray bytes;
              Image imageCopy;
              bool isImage = false;
              if (auto str = engine.luaMaybeTo<String>(data))
                bytes = ByteArray(str->utf8Ptr(), str->utf8Size());
              else if (auto image = engine.luaMaybeTo<Image>(data)) {
                imageCopy = *image;
                isImage = true;
              } else if (auto rawBytes = engine.luaMaybeTo<ByteArray>(data)) {
                bytes = *rawBytes;
              } else {
                auto json = engine.luaTo<Json>(data).repr();
                bytes = ByteArray(json.utf8Ptr(), json.utf8Size());
              }
              if (isImage)
                memoryAssets->set(path, imageCopy);
              else
                memoryAssets->set(path, bytes);

              // FezzedOne: Make sure the asset gets added to the overall file list.
              if (m_files.ptr(path)) {
                auto& descriptor = m_files[path];
                descriptor.sourceName = path;
                descriptor.source = memoryAssets;
              } else {
                m_files[path] = { // FezzedOne: Fixed MSVC compatibility.
                  path,
                  memoryAssets,
                  {},
                };
              }
              m_filesByExtension[AssetPath::extension(path).toLower()].insert(path);
            });

            callbacks.registerCallback("patch", [this, &memoryAssets](String const& path, String const& patchPath) -> bool {
              if (auto file = m_files.ptr(path)) {
                if (memoryAssets->contains(patchPath)) {
                  file->patchSources.append(make_pair(patchPath, memoryAssets));
                  return true;
                } else {
                  if (auto asset = m_files.ptr(patchPath)) {
                    file->patchSources.append(make_pair(patchPath, asset->source));
                    return true;
                  }
                }
              }
              return false;
            });

            callbacks.registerCallback("erase", [this](String const& path) -> bool {
              bool erased = m_files.erase(path);
              if (erased)
                m_filesByExtension[AssetPath::extension(path).toLower()].erase(path);
              return erased;
            });

            // FezzedOne: Lists all already *loaded* asset source file paths. Useful in on-load scripts. Does not list memory asset sources.
            callbacks.registerCallback("loadedSources", [&, this]() -> StringList {
              StringList loadedSources{};
              for (auto loadedSource : sources) {
                loadedSources.append(loadedSource.first);
              }
              return loadedSources;
            });

            // FezzedOne: Returns metadata for a given *loaded* asset source, or `nil`. Memory asset sources will return `nil`.
            callbacks.registerCallback("sourceMetadata", [&, this](String const& sourcePath) -> Json {
              Json metadata = Json();
              for (auto loadedSource : sources) {
                if (loadedSource.first == sourcePath) {
                  metadata = loadedSource.second->metadata();
                  break;
                }
              }
              return metadata;
            });

            context.setCallbacks("assets", callbacks);
          }
          /* End of inlined lambda. */

          for (auto& jPath : *scriptGroup) {
            const String path = jPath.toString();
            ByteArray script;
            // FezzedOne: Made preprocessor scripts replaceable before they're run.
            const String frontLoadPath = strf("{}.frontload", path);
            if (m_files.contains(frontLoadPath)) {
              const Json sourceName = m_files.get(frontLoadPath).source->metadata().value("name", "<no name>");
              Logger::info("Using replacement '{}' script from '{}'",
                frontLoadPath,
                sourceName.isType(Json::Type::String) ? sourceName.toString() : sourceName.repr());
              script = this->read(frontLoadPath);
            } else {
              script = source->read(path);
            }
            context.load(script, path);
          }
        } catch (LuaException const& e) {
          Logger::error("Exception while running {} scripts from asset source '{}': {}", groupName, sourcePath, e.what());
        }
        if (!memoryAssets->empty())
          addSource(strf("{}::{}", sourcePath, groupName), memoryAssets);
      }
    }
    // Kae: Clear any cached files that may have been added by preprocessing scripts, as they may no longer be valid.
    m_framesSpecifications.clear();
    m_assetsCache.clear();
  };

  for (auto& sourcePath : m_assetSources) {
    Logger::info("Loading assets from: '{}'", sourcePath);
    AssetSourcePtr source;
    if (File::isDirectory(sourcePath))
      source = std::make_shared<DirectoryAssetSource>(sourcePath, m_settings.pathIgnore);
    else
      source = std::make_shared<PackedAssetSource>(sourcePath);

    addSource(sourcePath, source);
    sources.append(make_pair(sourcePath, source));

    runLoadScripts("onLoad", sourcePath, source, sources);
  }

  for (auto& pair : sources)
    runLoadScripts("postLoad", pair.first, pair.second, sources);

  Sha256Hasher digest;

  for (auto const& assetPath : m_files.keys().sorted()) {
    bool digestFile = true;
    for (auto const& pattern : m_settings.digestIgnore) {
      if (assetPath.regexMatch(pattern, false, false)) {
        digestFile = false;
        break;
      }
    }

    auto const& descriptor = m_files.get(assetPath);

    if (digestFile) {
      digest.push(assetPath);
      digest.push(DataStreamBuffer::serialize(descriptor.source->open(descriptor.sourceName)->size()));
      for (auto const& pair : descriptor.patchSources)
        digest.push(DataStreamBuffer::serialize(pair.second->open(pair.first)->size()));
    }
  }

  m_digest = digest.compute();

  for (auto const& filename : m_files.keys())
    m_filesByExtension[AssetPath::extension(filename).toLower()].add(filename);

  int workerPoolSize = m_settings.workerPoolSize;
  for (int i = 0; i < workerPoolSize; i++)
    m_workerThreads.append(Thread::invoke("Assets::workerMain", mem_fn(&Assets::workerMain), this));
}

Assets::~Assets() {
  m_stopThreads = true;

  {
    // Should lock associated mutex to prevent loss of wakeups,
    MutexLocker locker(m_assetsMutex);
    // Notify all worker threads to allow them to stop
    m_assetsQueued.broadcast();
  }

  // Join them all
  m_workerThreads.clear();
}

StringList Assets::assetSources() const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_assetSources;
}

// FezzedOne: Added this missing method.
StringList Assets::assetPatchSources(String const &path) const
{
  MutexLocker assetsLocker(m_assetsMutex);
  if (auto descriptor = m_files.ptr(path)) {
    StringList patchSources = {};
    for (auto const &pair : descriptor->patchSources) {
      patchSources.append(m_assetSourcePaths.getLeft(pair.second));
    }
    return patchSources;
  } else {
    return StringList{};
  }
}

JsonObject Assets::assetSourceMetadata(String const& sourceName) const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_assetSourcePaths.getRight(sourceName)->metadata();
}

StringList Assets::assetSourcePaths(String const &sourceName) const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_assetSourcePaths.getRight(sourceName)->assetPaths();
}

ByteArray Assets::digest() const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_digest;
}

bool Assets::assetExists(String const& path) const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_files.contains(path);
}

Maybe<Assets::AssetFileDescriptor> Assets::assetDescriptor(String const& path) const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_files.maybe(path);
}

String Assets::assetSource(String const& path) const {
  MutexLocker assetsLocker(m_assetsMutex);
  if (auto p = m_files.ptr(path))
    return m_assetSourcePaths.getLeft(p->source);
  throw AssetException(strf("No such asset '{}'", path));
}

Maybe<String> Assets::assetSourcePath(AssetSourcePtr const& source) const {
  MutexLocker assetsLocker(m_assetsMutex);
  return m_assetSourcePaths.maybeLeft(source);
}

StringList Assets::scan(String const& suffix) const {
  if (suffix.beginsWith(".") && !suffix.substr(1).hasChar('.')) {
    return scanExtension(suffix).values();
  } else if (suffix.empty()) {
    return m_files.keys();
  } else {
    StringList result;
    for (auto const& fileEntry : m_files) {
      String const& file = fileEntry.first;
      if (file.endsWith(suffix, String::CaseInsensitive))
        result.append(file);
    }

    return result;
  }
}

StringList Assets::scan(String const& prefix, String const& suffix) const {
  StringList result;
  if (suffix.beginsWith(".") && !suffix.substr(1).hasChar('.')) {
    auto filesWithExtension = scanExtension(suffix);
    for (auto const& file : filesWithExtension) {
      if (file.beginsWith(prefix, String::CaseInsensitive))
        result.append(file);
    }
  } else {
    for (auto const& fileEntry : m_files) {
      String const& file = fileEntry.first;
      if (file.beginsWith(prefix, String::CaseInsensitive) && file.endsWith(suffix, String::CaseInsensitive))
        result.append(file);
    }
  }
  return result;
}

const CaseInsensitiveStringSet NullExtensionScan;

CaseInsensitiveStringSet Assets::scanExtension(String const& extension) const {
  auto find = m_filesByExtension.find(extension.beginsWith(".") ? extension.substr(1) : extension);
  return find != m_filesByExtension.end() ? find->second : NullExtensionScan;
}

Json Assets::json(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, true, false);

  return as<JsonData>(getAsset(AssetId{AssetType::Json, std::move(components)}))->json;
}

Json Assets::fetchJson(Json const& v, String const& dir) const {
  if (v.isType(Json::Type::String))
    return Assets::json(AssetPath::relativeTo(dir, v.toString()));
  else
    return v;
}

void Assets::queueJsons(StringList const& paths) const {
  queueAssets(paths.transformed([](String const& path) {
    auto components = AssetPath::split(path);
    validatePath(components, true, false);

    return AssetId{AssetType::Json, {components.basePath, {}, {}}};
  }));
}

void Assets::queueJsons(CaseInsensitiveStringSet const& paths) const {
  MutexLocker assetsLocker(m_assetsMutex);
  for (String const& path : paths) {
    auto components = AssetPath::split(path);
    validatePath(components, true, false);

    queueAsset(AssetId{AssetType::Json, {components.basePath, {}, {}}});
  };
}

ImageConstPtr Assets::image(AssetPath const& path) const {
  validatePath(path, true, true);

  return as<ImageData>(getAsset(AssetId{AssetType::Image, path}))->image;
}

void Assets::queueImages(StringList const& paths) const {
  queueAssets(paths.transformed([](String const& path) {
    auto components = AssetPath::split(path);
    validatePath(components, true, true);

    return AssetId{AssetType::Image, std::move(components)};
  }));
}

void Assets::queueImages(CaseInsensitiveStringSet const& paths) const {
  MutexLocker assetsLocker(m_assetsMutex);
  for (String const& path : paths) {
    auto components = AssetPath::split(path);
    validatePath(components, true, true);

    queueAsset(AssetId{AssetType::Image, std::move(components)});
  };
}

ImageConstPtr Assets::tryImage(AssetPath const& path) const {
  validatePath(path, true, true);

  if (auto imageData = as<ImageData>(tryAsset(AssetId{AssetType::Image, path})))
    return imageData->image;
  else
    return {};
}

FramesSpecificationConstPtr Assets::imageFrames(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  MutexLocker assetsLocker(m_assetsMutex);
  return bestFramesSpecification(path);
}

AudioConstPtr Assets::audio(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  return as<AudioData>(getAsset(AssetId{AssetType::Audio, std::move(components)}))->audio;
}

void Assets::queueAudios(StringList const& paths) const {
  queueAssets(paths.transformed([](String const& path) {
    const auto components = AssetPath::split(path);
    validatePath(components, false, false);

    return AssetId{AssetType::Audio, std::move(components)};
  }));
}

void Assets::queueAudios(CaseInsensitiveStringSet const& paths) const {
  MutexLocker assetsLocker(m_assetsMutex);
  for (String const& path : paths) {
    auto components = AssetPath::split(path);
    validatePath(components, false, true);

    queueAsset(AssetId{AssetType::Audio, std::move(components)});
  };
}

AudioConstPtr Assets::tryAudio(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  if (auto audioData = as<AudioData>(tryAsset(AssetId{AssetType::Audio, std::move(components)})))
    return audioData->audio;
  else
    return {};
}

FontConstPtr Assets::font(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  return as<FontData>(getAsset(AssetId{AssetType::Font, std::move(components)}))->font;
}

ByteArrayConstPtr Assets::bytes(String const& path) const {
  auto components = AssetPath::split(path);
  validatePath(components, false, false);

  return as<BytesData>(getAsset(AssetId{AssetType::Bytes, std::move(components)}))->bytes;
}

IODevicePtr Assets::openFile(String const& path) const {
  return open(path);
}

void Assets::clearCache() {
  MutexLocker assetsLocker(m_assetsMutex);

  // Clear all assets that are not queued or broken.
  auto it = makeSMutableMapIterator(m_assetsCache);
  while (it.hasNext()) {
    auto const& pair = it.next();
    // Don't clean up queued, persistent, or broken assets.
    if (pair.second && !pair.second->shouldPersist() && !m_queue.contains(pair.first))
      it.remove();
  }
}

void Assets::cleanup() {
  MutexLocker assetsLocker(m_assetsMutex);

  double time = Time::monotonicTime();

  auto it = makeSMutableMapIterator(m_assetsCache);
  while (it.hasNext()) {
    auto pair = it.next();
    // Don't clean up broken assets or queued assets.
    if (pair.second && !m_queue.contains(pair.first)) {
      double liveTime = time - pair.second->time;
      if (liveTime > m_settings.assetTimeToLive) {
        // If the asset should persist, just refresh the access time.
        if (pair.second->shouldPersist())
          pair.second->time = time;
        else
          it.remove();
      }
    }
  }
}

bool Assets::AssetId::operator==(AssetId const& assetId) const {
  return tie(type, path) == tie(assetId.type, assetId.path);
}

size_t Assets::AssetIdHash::operator()(AssetId const& id) const {
  return hashOf(id.type, id.path.basePath, id.path.subPath, id.path.directives);
}

bool Assets::JsonData::shouldPersist() const {
  return !json.unique();
}

bool Assets::ImageData::shouldPersist() const {
  return !alias && !image.unique();
}

bool Assets::AudioData::shouldPersist() const {
  return !audio.unique();
}

bool Assets::FontData::shouldPersist() const {
  return !font.unique();
}

bool Assets::BytesData::shouldPersist() const {
  return !bytes.unique();
}

FramesSpecification Assets::parseFramesSpecification(Json const& frameConfig, String path) {
  FramesSpecification framesSpecification;

  framesSpecification.framesFile = std::move(path);

  if (frameConfig.contains("frameList")) {
    for (auto const& pair : frameConfig.get("frameList").toObject()) {
      String frameName = pair.first;
      RectU rect = RectU(jsonToRectI(pair.second));
      if (rect.isEmpty())
        throw AssetException(
            strf("Empty rect in frame specification in image {} frame {}", framesSpecification.framesFile, frameName));
      else
        framesSpecification.frames[frameName] = rect;
    }
  }

  if (frameConfig.contains("frameGrid")) {
    auto grid = frameConfig.get("frameGrid").toObject();

    Vec2U begin(jsonToVec2I(grid.value("begin", jsonFromVec2I(Vec2I()))));
    Vec2U size(jsonToVec2I(grid.get("size")));
    Vec2U dimensions(jsonToVec2I(grid.get("dimensions")));

    if (dimensions[0] == 0 || dimensions[1] == 0)
      throw AssetException(strf("Image {} \"dimensions\" in frameGrid cannot be zero", framesSpecification.framesFile));

    if (grid.contains("names")) {
      auto nameList = grid.get("names");
      for (size_t y = 0; y < nameList.size(); ++y) {
        if (y >= dimensions[1])
          throw AssetException(strf("Image {} row {} is out of bounds for y-dimension {}",
              framesSpecification.framesFile,
              y + 1,
              dimensions[1]));
        auto rowList = nameList.get(y);
        if (rowList.isNull())
          continue;
        for (unsigned x = 0; x < rowList.size(); ++x) {
          if (x >= dimensions[0])
            throw AssetException(strf("Image {} column {} is out of bounds for x-dimension {}",
                framesSpecification.framesFile,
                x + 1,
                dimensions[0]));

          auto frame = rowList.get(x);
          if (frame.isNull())
            continue;
          auto frameName = frame.toString();
          if (!frameName.empty())
            framesSpecification.frames[frameName] =
                RectU::withSize(Vec2U(begin[0] + x * size[0], begin[1] + y * size[1]), size);
        }
      }
    } else {
      // If "names" not specified, use auto naming algorithm
      for (size_t y = 0; y < dimensions[1]; ++y)
        for (size_t x = 0; x < dimensions[0]; ++x)
          framesSpecification.frames[toString(y * dimensions[0] + x)] =
              RectU::withSize(Vec2U(begin[0] + x * size[0], begin[1] + y * size[1]), size);
    }
  }

  if (auto aliasesConfig = frameConfig.opt("aliases")) {
    auto aliases = aliasesConfig->objectPtr();
    for (auto const& pair : *aliases) {
      String const& key = pair.first;
      String value = pair.second.toString();

      // Resolve aliases to aliases by checking to see if the alias value in
      // the alias map itself.  Don't do this more than aliases.size() times to
      // avoid infinite cycles.
      for (size_t i = 0; i <= aliases->size(); ++i) {
        auto it = aliases->find(value);
        if (it != aliases->end()) {
          if (i == aliases->size())
            throw AssetException(strf("Infinite alias loop detected for alias '{}'", key));

          value = it->second.toString();
        } else {
          break;
        }
      }

      if (!framesSpecification.frames.contains(value))
        throw AssetException(strf("No such frame '{}' found for alias '{}'", value, key));
      framesSpecification.aliases[key] = std::move(value);
    }
  }

  return framesSpecification;
}

void Assets::queueAssets(List<AssetId> const& assetIds) const {
  MutexLocker assetsLocker(m_assetsMutex);

  for (auto const& id : assetIds)
    queueAsset(id);
}

void Assets::queueAsset(AssetId const& assetId) const {
  auto i = m_assetsCache.find(assetId);
  if (i != m_assetsCache.end()) {
    if (i->second)
      freshen(i->second);
  } else {
    auto j = m_queue.find(assetId);
    if (j == m_queue.end()) {
      m_queue[assetId] = QueuePriority::Load;
      m_assetsQueued.signal();
    }
  }
}

shared_ptr<Assets::AssetData> Assets::tryAsset(AssetId const& id) const {
  MutexLocker assetsLocker(m_assetsMutex);

  auto i = m_assetsCache.find(id);
  if (i != m_assetsCache.end()) {
    if (i->second) {
      freshen(i->second);
      return i->second;
    } else {
      throw AssetException::format("Error loading asset {}", id.path);
    }
  } else {
    auto j = m_queue.find(id);
    if (j == m_queue.end()) {
      m_queue[id] = QueuePriority::Load;
      m_assetsQueued.signal();
    }
    return {};
  }
}

shared_ptr<Assets::AssetData> Assets::getAsset(AssetId const& id) const {
  MutexLocker assetsLocker(m_assetsMutex);

  while (true) {
    auto j = m_assetsCache.find(id);
    if (j != m_assetsCache.end()) {
      if (j->second) {
        auto asset = j->second;
        freshen(asset);
        return asset;
      } else {
        throw AssetException::format("Error loading asset {}", id.path);
      }
    } else {
      // Try to load the asset in-thread, if we cannot, then the asset has been
      // queued so wait for a worker thread to finish it.
      if (!doLoad(id))
        m_assetsDone.wait(m_assetsMutex);
    }
  }
}

void Assets::workerMain() {
  while (true) {
    if (m_stopThreads)
      break;

    MutexLocker assetsLocker(m_assetsMutex);

    AssetId assetId;
    QueuePriority queuePriority = QueuePriority::None;

    // Find the highest priority queue entry
    for (auto const& pair : m_queue) {
      if (pair.second == QueuePriority::Load || pair.second == QueuePriority::PostProcess) {
        assetId = pair.first;
        queuePriority = pair.second;
        if (pair.second == QueuePriority::Load)
          break;
      }
    }

    if (queuePriority != QueuePriority::Load && queuePriority != QueuePriority::PostProcess) {
      // Nothing in the queue that needs work
      m_assetsQueued.wait(m_assetsMutex);
      continue;
    }

    bool workIsBlocking;
    if (queuePriority == QueuePriority::PostProcess)
      workIsBlocking = !doPost(assetId);
    else
      workIsBlocking = !doLoad(assetId);

    if (workIsBlocking) {
      // We are blocking on some sort of busy asset, so need to wait on
      // something to complete here, rather than spinning and burning cpu.
      m_assetsDone.wait(m_assetsMutex);
      continue;
    }

    // After processing an asset, unlock the main asset mutex and yield so we
    // don't starve other threads.
    assetsLocker.unlock();
    Thread::yield();
  }
}

template <typename Function>
decltype(auto) Assets::unlockDuring(Function f) const {
  m_assetsMutex.unlock();
  try {
    auto r = f();
    m_assetsMutex.lock();
    return r;
  } catch (...) {
    m_assetsMutex.lock();
    throw;
  }
}

FramesSpecificationConstPtr Assets::bestFramesSpecification(String const& image) const {
  if (auto framesSpecification = m_framesSpecifications.maybe(image)) {
    return *framesSpecification;
  }

  String framesFile;

  if (auto bestFramesFile = m_bestFramesFiles.maybe(image)) {
    framesFile = *bestFramesFile;

  } else {
    String searchPath = AssetPath::directory(image);
    String filePrefix = AssetPath::filename(image);
    filePrefix = filePrefix.substr(0, filePrefix.findLast('.'));

    auto subdir = [](String const& dir) -> String {
      auto dirsplit = dir.substr(0, dir.size() - 1).rsplit("/", 1);
      if (dirsplit.size() < 2)
        return "";
      else
        return dirsplit[0] + "/";
    };

    Maybe<String> foundFramesFile;

    // look for <full-path-minus-extension>.frames or default.frames up to root
    while (!searchPath.empty()) {
      String framesPath = searchPath + filePrefix + ".frames";
      if (m_files.contains(framesPath)) {
        foundFramesFile = framesPath;
        break;
      }

      framesPath = searchPath + "default.frames";
      if (m_files.contains(framesPath)) {
        foundFramesFile = framesPath;
        break;
      }

      searchPath = subdir(searchPath);
    }

    if (foundFramesFile) {
      framesFile = foundFramesFile.take();
      m_bestFramesFiles[image] = framesFile;

    } else {
      return {};
    }
  }

  auto framesSpecification = unlockDuring([&]() {
      return make_shared<FramesSpecification>(parseFramesSpecification(readJson(framesFile), framesFile));
    });
  m_framesSpecifications[image] = framesSpecification;

  return framesSpecification;
}

IODevicePtr Assets::open(String const& path) const {
  if (auto p = m_files.ptr(path))
    return p->source->open(p->sourceName);
  throw AssetException(strf("No such asset '{}'", path));
}

ByteArray Assets::read(String const& path) const {
  if (auto p = m_files.ptr(path))
    return p->source->read(p->sourceName);
  throw AssetException(strf("No such asset '{}'", path));
}

ImageConstPtr Assets::readImage(String const& path) const {
  if (auto p = m_files.ptr(path)) {
    ImageConstPtr image;
    if (auto memorySource = as<MemoryAssetSource>(p->source)) {
      image = memorySource->image(p->sourceName);
    }
    if (!image) {
      image = make_shared<Image>(Image::readPng(p->source->open(p->sourceName)));
    }

    // From OpenStarbound: Image patching code.
    if (!p->patchSources.empty()) {
      RecursiveMutexLocker luaLocker(m_luaMutex);
      LuaEngine* luaEngine = as<LuaEngine>(m_luaEngine.get());
      LuaValue result = luaEngine->createUserData(*image);
      luaLocker.unlock();
      // For a given image `imageAsset.png`, runs any `imageAsset.png.patch.lua` and `imageAsset.png.patch.pluto` scripts.
      for (auto const& pair : p->patchSources) {
        auto& patchPath = pair.first;
        auto& patchSource = pair.second;
        auto patchStream = patchSource->read(patchPath);
        if (patchPath.endsWith(".lua") || patchPath.endsWith(".pluto")) {
          luaLocker.lock();
          LuaContextPtr& context = m_patchContexts[patchPath];
          if (!context) {
            context = make_shared<LuaContext>(luaEngine->createContext());
            context->load(patchStream, patchPath);
          }
          auto newResult = context->invokePath<LuaValue>("patch", result, path);
          if (!newResult.is<LuaNilType>()) {
            if (auto ud = newResult.ptr<LuaUserData>()) {
              if (ud->is<Image>())
                result = std::move(newResult);
              else
                Logger::error("Patch '{}' for image '{}' returned a non-Image userdata value, ignoring", patchPath, path);
            } else {
              Logger::error("Patch '{}' for image '{}' returned a non-Image value, ignoring", patchPath, path);
            }
          }
          luaLocker.unlock();
        } else {
          Logger::error("Patch '{}' for image '{}' isn't a Pluto/Lua script, ignoring", patchPath, path);
        }
      }
      image = make_shared<Image>(result.get<LuaUserData>().get<Image>());
    }

    return image;
  }
  throw AssetException(strf("No such asset '{}'", path));
}


// WasabiRaptor's recursive patch checking code, «downstreamed» from OpenStarbound.
Json Assets::checkPatchArray(String const& path, AssetSourcePtr const& source, Json const result, JsonArray const patchData) const {
  auto newResult = result;
  for (auto const& patch : patchData) {
    switch(patch.type()){
      case Json::Type::Array: // If the patch is an array, go down recursively until objects are found.
        try {
          newResult = checkPatchArray(path, source, newResult, patch.toArray());
        } catch (JsonPatchTestFail const& e) {
          Logger::debug("Patch test failure from file {} in source: '{}' at '{}'. Caused by: {}", path, source->metadata().value("name", ""), m_assetSourcePaths.getLeft(source), e.what());
        } catch (JsonPatchException const& e) {
          Logger::error("Could not apply patch from file {} in source: '{}' at '{}'.  Caused by: {}", path, source->metadata().value("name", ""), m_assetSourcePaths.getLeft(source), e.what());
        }
        break;
      case Json::Type::Object: // If it's an object, check for operations.
        newResult = JsonPatching::applyOperation(newResult, patch);
        break;
      default:
        throw JsonPatchException(strf("Patch data is wrong type: {}", Json::typeName(patch.type())));
        break;
    }
  }
  return newResult;
}

Json Assets::readJson(String const& path) const {
  ByteArray streamData = read(path);
  try {
    Json result = inputUtf8Json(streamData.begin(), streamData.end(), false);
    // FezzedOne: Minor optimisation.
    auto& patchSources = m_files.get(path).patchSources;
    if (patchSources.empty()) return result;
    for (auto const& pair : patchSources) {
      auto& patchPath = pair.first;
      auto& patchSource = pair.second;
      auto patchStream = patchSource->read(patchPath);
      // FezzedOne: Patches that return an invalid result or throw errors are now ignored, allowing the patched asset file to load.
      if (patchPath.endsWith(".lua") || patchPath.endsWith(".pluto")) {
        RecursiveMutexLocker luaLocker(m_luaMutex);
        // Kae: i don't like that lock. perhaps have a LuaEngine and patch context cache per worker thread later on?
        LuaContextPtr& context = m_patchContexts[patchPath];
        Json newResult = Json();
        bool startedNewContext = false;
        if (!context) {
          context = make_shared<LuaContext>(as<LuaEngine>(m_luaEngine.get())->createContext());
          startedNewContext = true;
        }
        try {
          if (startedNewContext)
            context->load(patchStream, patchPath);
          newResult = context->invokePath<Json>("patch", result, path);
        } catch (std::exception const& e) {
          Logger::error("Ignored failed Pluto/Lua patch from file {} in source: '{}' at '{}'. Caused by: {}",
            patchPath, patchSource->metadata().value("name", ""), m_assetSourcePaths.getLeft(patchSource), e.what());
        }
        if (newResult.isType(Json::Type::Array) || newResult.isType(Json::Type::Object))
            result = std::move(newResult);
      } else {
        Json newPatchResult = Json();
        try {
          auto patchJson = inputUtf8Json(patchStream.begin(), patchStream.end(), false);
          if (patchJson.isType(Json::Type::Array)) {
            auto patchData = patchJson.toArray();
            Json newResult = Json();
            try {
              newResult = checkPatchArray(patchPath, patchSource, result, patchData);
            } catch (JsonPatchTestFail const& e) {
              Logger::debug("Ignored patch from file {} in source: '{}' at '{}' due to test failure. Caused by: {}",
                patchPath, patchSource->metadata().value("name", ""), m_assetSourcePaths.getLeft(patchSource), e.what());
            } catch (JsonPatchException const& e) {
              Logger::error("Ignored failed patch from file {} in source: '{}' at '{}'. Caused by: {}",
                patchPath, patchSource->metadata().value("name", ""), m_assetSourcePaths.getLeft(patchSource), e.what());
            }
            if (newResult.isType(Json::Type::Array) || newResult.isType(Json::Type::Object))
              newPatchResult = std::move(newResult);
          } else if (patchJson.isType(Json::Type::Object)) { // Kae: Do a good ol' json merge instead if the .patch file is a Json object
            newPatchResult = jsonMergeNull(result, patchJson.toObject());
          }
        } catch (std::exception const& e) {
          Logger::error("Cannot parse JSON patch from file {} in source: '{}' at '{}'. Patch file ignored. Caused by: {}",
            patchPath, patchSource->metadata().value("name", ""), m_assetSourcePaths.getLeft(patchSource), e.what());
        }
        if (newPatchResult.isType(Json::Type::Array) || newPatchResult.isType(Json::Type::Object))
          result = std::move(newPatchResult);
      }
    }
    return result;
  } catch (std::exception const& e) {
    throw JsonParsingException(strf("Cannot parse JSON file: {}", path), e);
  }
}

bool Assets::doLoad(AssetId const& id) const {
  try {
    // loadAsset automatically manages the queue and freshens the asset
    // data.
    return (bool)loadAsset(id);
  } catch (std::exception const& e) {
    Logger::error("Exception caught loading asset: {}, {}", id.path, outputException(e, true));
  } catch (...) {
    Logger::error("Unknown exception caught loading asset: {}", id.path);
  }

  // There was an exception, remove the asset from the queue and fill the cache
  // with null so that getAsset will throw.
  m_assetsCache[id] = {};
  m_assetsDone.broadcast();
  m_queue.remove(id);
  return true;
}

bool Assets::doPost(AssetId const& id) const {
  shared_ptr<AssetData> assetData;
  try {
    assetData = m_assetsCache.get(id);
    if (id.type == AssetType::Audio)
      assetData = postProcessAudio(assetData);
  } catch (std::exception const& e) {
    Logger::error("Exception caught post-processing asset: {}, {}", id.path, outputException(e, true));
  } catch (...) {
    Logger::error("Unknown exception caught post-processing asset: {}", id.path);
  }

  m_queue.remove(id);
  if (assetData) {
    assetData->needsPostProcessing = false;
    m_assetsCache[id] = assetData;
    freshen(assetData);
    m_assetsDone.broadcast();
  }

  return true;
}

shared_ptr<Assets::AssetData> Assets::loadAsset(AssetId const& id) const {
  if (auto asset = m_assetsCache.value(id))
    return asset;

  if (m_queue.value(id, QueuePriority::None) == QueuePriority::Working)
    return {};

  try {
    m_queue[id] = QueuePriority::Working;
    shared_ptr<AssetData> assetData;

    try {
      if (id.type == AssetType::Json) {
        assetData = loadJson(id.path);
      } else if (id.type == AssetType::Image) {
        assetData = loadImage(id.path);
      } else if (id.type == AssetType::Audio) {
        assetData = loadAudio(id.path);
      } else if (id.type == AssetType::Font) {
        assetData = loadFont(id.path);
      } else if (id.type == AssetType::Bytes) {
        assetData = loadBytes(id.path);
      }

    } catch (StarException const& e) {
      if (id.type == AssetType::Image && m_settings.missingImage) {
        Logger::error("Could not load image asset '{}', using placeholder default.\n{}", id.path, outputException(e, false));
        assetData = loadImage({*m_settings.missingImage, {}, {}});
      } else if (id.type == AssetType::Audio && m_settings.missingAudio) {
        Logger::error("Could not load audio asset '{}', using placeholder default.\n{}", id.path, outputException(e, false));
        assetData = loadAudio({*m_settings.missingAudio, {}, {}});
      } else {
        throw;
      }
    }

    if (assetData) {
      if (assetData->needsPostProcessing)
        m_queue[id] = QueuePriority::PostProcess;
      else
        m_queue.remove(id);
      m_assetsCache[id] = assetData;
      m_assetsDone.broadcast();
      freshen(assetData);

    } else {
      // We have failed to load an asset because it depends on an asset
      // currently being worked on.  Mark it as needing loading and move it to
      // the end of the queue.
      m_queue[id] = QueuePriority::Load;
      m_assetsQueued.signal();
      m_queue.toBack(id);
    }

    return assetData;

  } catch (...) {
    m_queue.remove(id);
    m_assetsCache[id] = {};
    m_assetsDone.broadcast();
    throw;
  }
}

shared_ptr<Assets::AssetData> Assets::loadJson(AssetPath const& path) const {
  Json json;

  if (path.subPath) {
    auto topJson =
      as<JsonData>(loadAsset(AssetId{AssetType::Json, {path.basePath, {}, {}}}));
    if (!topJson)
      return {};

    try {
      auto newData = make_shared<JsonData>();
      newData->json = topJson->json.query(*path.subPath);
      return newData;
    } catch (StarException const& e) {
      throw AssetException(strf("Could not read JSON value {}", path), e);
    }
  } else {
    return unlockDuring([&]() {
      try {
        auto newData = make_shared<JsonData>();
        newData->json = readJson(path.basePath);
        return newData;
      } catch (StarException const& e) {
        throw AssetException(strf("Could not read JSON asset {}", path), e);
      }
    });
  }
}

shared_ptr<Assets::AssetData> Assets::loadImage(AssetPath const& path) const {
  validatePath(path, true, true);
  if (!path.directives.empty()) {
    shared_ptr<ImageData> source =
        as<ImageData>(loadAsset(AssetId{AssetType::Image, {path.basePath, path.subPath, {}}}));
    if (!source)
      return {};
    StringMap<ImageConstPtr> references;
    StringList referencePaths;

    for (auto& directives : path.directives.list())
      directives.loadOperations();

    path.directives.forEach([&](auto const& entry, Directives const& directives) {
      addImageOperationReferences(entry.operation, referencePaths);
    }); // TODO: This can definitely be better, was changed quickly to support the new Directives.


    for (auto const& ref : referencePaths) {
      auto components = AssetPath::split(ref);
      validatePath(components, true, false);
      auto refImage = as<ImageData>(loadAsset(AssetId{AssetType::Image, std::move(components)}));
      if (!refImage)
        return {};
      references[ref] = refImage->image;
    }

    return unlockDuring([&]() {
      auto newData = make_shared<ImageData>();
      Image newImage = *source->image;
      path.directives.forEach([&](auto const& entry, Directives const& directives) {
        if (auto error = entry.operation.template ptr<ErrorImageOperation>())
          std::rethrow_exception(error->exception);
        else
          processImageOperation(entry.operation, newImage, [&](String const& ref) { return references.get(ref).get(); });
      });
      newData->image = make_shared<Image>(std::move(newImage));
      return newData;
    });

  } else if (path.subPath) {
    auto imageData = as<ImageData>(loadAsset(AssetId{AssetType::Image, {path.basePath, {}, {}}}));
    if (!imageData)
      return {};

    // Base image must have frames data associated with it.
    if (!imageData->frames)
      throw AssetException::format("No associated frames file found for image '{}' while resolving image frame '{}'", path.basePath, path);

    if (auto alias = imageData->frames->aliases.ptr(*path.subPath)) {
      imageData = as<ImageData>(loadAsset(AssetId{AssetType::Image, {path.basePath, *alias, path.directives}}));
      if (!imageData)
        return {};

      auto newData = make_shared<ImageData>();
      newData->image = imageData->image;
      newData->alias = true;
      return newData;

    } else {
      auto frameRect = imageData->frames->frames.ptr(*path.subPath);
      if (!frameRect)
        throw AssetException(strf("No such frame {} in frames spec {}", *path.subPath, imageData->frames->framesFile));

      return unlockDuring([&]() {
        // Need to flip frame coordinates because frame configs assume top
        // down image coordinates
        auto newData = make_shared<ImageData>();
        newData->image = make_shared<Image>(imageData->image->subImage(
            Vec2U(frameRect->xMin(), imageData->image->height() - frameRect->yMax()), frameRect->size()));
        return newData;
      });
    }

  } else {
    auto imageData = make_shared<ImageData>();
    imageData->image = unlockDuring([&]() {
      return readImage(path.basePath);
    });
    imageData->frames = bestFramesSpecification(path.basePath);

    return imageData;
  }
}

shared_ptr<Assets::AssetData> Assets::loadAudio(AssetPath const& path) const {
  return unlockDuring([&]() {
    auto newData = make_shared<AudioData>();
    newData->audio = make_shared<Audio>(open(path.basePath));
    newData->needsPostProcessing = newData->audio->compressed();
    return newData;
  });
}

shared_ptr<Assets::AssetData> Assets::loadFont(AssetPath const& path) const {
  return unlockDuring([&]() {
    auto newData = make_shared<FontData>();
    newData->font = Font::loadTrueTypeFont(make_shared<ByteArray>(read(path.basePath)));
    return newData;
  });
}

shared_ptr<Assets::AssetData> Assets::loadBytes(AssetPath const& path) const {
  return unlockDuring([&]() {
    auto newData = make_shared<BytesData>();
    newData->bytes = make_shared<ByteArray>(read(path.basePath));
    return newData;
  });
}

shared_ptr<Assets::AssetData> Assets::postProcessAudio(shared_ptr<AssetData> const& original) const {
  return unlockDuring([&]() -> shared_ptr<AssetData> {
    if (auto audioData = as<AudioData>(original)) {
      if (audioData->audio->totalTime() < m_settings.audioDecompressLimit) {
        auto audio = make_shared<Audio>(*audioData->audio);
        audio->uncompress();

        auto newData = make_shared<AudioData>();
        newData->audio = audio;
        return newData;
      } else {
        return audioData;
      }
    } else {
      return {};
    }
  });
}

void Assets::freshen(shared_ptr<AssetData> const& asset) const {
  asset->time = Time::monotonicTime();
}

}
