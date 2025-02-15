#include "StarVersioningDatabase.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarFormat.hpp"
#include "StarLexicalCast.hpp"
#include "StarFile.hpp"
#include "StarLogging.hpp"
#include "StarWorldLuaBindings.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarAssets.hpp"
#include "StarStoredFunctions.hpp"
#include "StarNpcDatabase.hpp"
#include "StarRoot.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

char const* const VersionedJson::Magic = "SBVJ01";
size_t const VersionedJson::MagicStringSize = 6;

VersionedJson VersionedJson::readFile(String const& filename) {
  DataStreamIODevice ds(File::open(filename, IOMode::Read));

  if (ds.readBytes(MagicStringSize) != ByteArray(Magic, MagicStringSize))
    throw IOException(strf("Wrong magic bytes at start of versioned json file, expected '{}'", Magic));

  return ds.read<VersionedJson>();
}

void VersionedJson::writeFile(VersionedJson const& versionedJson, String const& filename) {
  DataStreamBuffer ds;
  ds.writeData(Magic, MagicStringSize);
  ds.write(versionedJson);
  File::overwriteFileWithRename(ds.takeData(), filename);
}

Json VersionedJson::toJson() const {
  return JsonObject{
    {"id", identifier},
    {"version", version},
    {"content", content}
  };
}

VersionedJson VersionedJson::fromJson(Json const& source) {
  // Old versions of VersionedJson used '__' to distinguish between actual
  // content and versioned content, but this is no longer necessary or
  // relevant.
  auto id = source.optString("id").orMaybe(source.optString("__id"));
  auto version = source.optUInt("version").orMaybe(source.optUInt("__version"));
  auto content = source.opt("content").orMaybe(source.opt("__content"));
  return {*id, (VersionNumber)*version, *content};
}

bool VersionedJson::empty() const {
  return content.isNull();
}

void VersionedJson::expectIdentifier(String const& expectedIdentifier) const {
  if (identifier != expectedIdentifier)
    throw VersionedJsonException::format("VersionedJson identifier mismatch, expected '{}' but got '{}'", expectedIdentifier, identifier);
}

DataStream& operator>>(DataStream& ds, VersionedJson& versionedJson) {
  ds.read(versionedJson.identifier);
  // This is a holdover from when the version number was optional in
  // VersionedJson.  We should convert versioned json binary files and the
  // celestial chunk database and world storage to a new format eventually
  versionedJson.version = ds.read<Maybe<VersionNumber>>().value();
  ds.read(versionedJson.content);

  return ds;
}

DataStream& operator<<(DataStream& ds, VersionedJson const& versionedJson) {
  ds.write(versionedJson.identifier);
  ds.write(Maybe<VersionNumber>(versionedJson.version));
  ds.write(versionedJson.content);

  return ds;
}

VersioningDatabase::VersioningDatabase() {
  auto assets = Root::singleton().assets();
  auto versioningConfig = assets->json("/versioning.config");
  m_luaRoot.tuneAutoGarbageCollection(versioningConfig.optFloat("luaGcPause").value(1.2f),
    versioningConfig.optFloat("luaGcStepMultiplier").value(1.2f));

  for (auto const& pair : versioningConfig.iterateObject()) {
    if (pair.first != "luaGcPause" && pair.first != "luaGcStepMultiplier")
      m_currentVersions[pair.first] = pair.second.toUInt();
  }

  for (auto const& scriptFile : assets->scan("/versioning/", ".lua")) {
    try {
      auto scriptParts = File::baseName(scriptFile).splitAny("_.");
      if (scriptParts.size() != 4)
        throw VersioningDatabaseException::format("Script file '{}' filename not of the form <identifier>_<fromversion>_<toversion>.lua", scriptFile);

      String identifier = scriptParts.at(0);
      VersionNumber fromVersion = lexicalCast<VersionNumber>(scriptParts.at(1));
      VersionNumber toVersion = lexicalCast<VersionNumber>(scriptParts.at(2));

      m_versionUpdateScripts[identifier.toLower()].append({scriptFile, fromVersion, toVersion});
    } catch (StarException const&) {
      throw VersioningDatabaseException::format("Error parsing version information from versioning script '{}'", scriptFile);
    }
  }

  for (auto const& scriptFile : assets->scan("/versioning/", ".pluto")) {
    try {
      auto scriptParts = File::baseName(scriptFile).splitAny("_.");
      if (scriptParts.size() != 4)
        throw VersioningDatabaseException::format("Script file '{}' filename not of the form <identifier>_<fromversion>_<toversion>.pluto", scriptFile);

      String identifier = scriptParts.at(0);
      VersionNumber fromVersion = lexicalCast<VersionNumber>(scriptParts.at(1));
      VersionNumber toVersion = lexicalCast<VersionNumber>(scriptParts.at(2));

      m_versionUpdateScripts[identifier.toLower()].append({scriptFile, fromVersion, toVersion});
    } catch (StarException const&) {
      throw VersioningDatabaseException::format("Error parsing version information from versioning script '{}'", scriptFile);
    }
  }

  // Sort each set of update scripts first by fromVersion, and then in
  // *reverse* order of toVersion.  This way, the first matching script for a
  // given fromVersion should take the json to the *furthest* toVersion.
  for (auto& pair : m_versionUpdateScripts) {
    pair.second.sort([](VersionUpdateScript const& lhs, VersionUpdateScript const& rhs) {
      if (lhs.fromVersion != rhs.fromVersion)
        return lhs.fromVersion < rhs.fromVersion;
      else
        return lhs.toVersion < rhs.toVersion;
    });
  }
}

VersionedJson VersioningDatabase::makeCurrentVersionedJson(String const& identifier, Json const& content) const {
  RecursiveMutexLocker locker(m_mutex);
  return VersionedJson{identifier, m_currentVersions.get(identifier), content};
}

bool VersioningDatabase::versionedJsonCurrent(VersionedJson const& versionedJson) const {
  RecursiveMutexLocker locker(m_mutex);
  return versionedJson.version == m_currentVersions.get(versionedJson.identifier);
}

VersionedJson VersioningDatabase::updateVersionedJson(VersionedJson const& versionedJson) const {
  RecursiveMutexLocker locker(m_mutex);

  auto& root = Root::singleton();
  CelestialMasterDatabase celestialDatabase;

  VersionedJson result = versionedJson;
  Maybe<VersionNumber> targetVersion = m_currentVersions.maybe(versionedJson.identifier);
  if (!targetVersion)
    throw VersioningDatabaseException::format("Versioned JSON has an unregistered identifier '{}'", versionedJson.identifier);

  LuaCallbacks celestialCallbacks;
  celestialCallbacks.registerCallback("parameters", [&celestialDatabase](Json const& coord) {
      return celestialDatabase.parameters(CelestialCoordinate(coord))->diskStore();
    });

  try {
    for (auto const& updateScript : m_versionUpdateScripts.value(versionedJson.identifier.toLower())) {
      if (result.version >= *targetVersion)
        break;

      if (updateScript.fromVersion == result.version) {
        auto scriptContext = m_luaRoot.createContext();
        scriptContext.load(*root.assets()->bytes(updateScript.script), updateScript.script);
        scriptContext.setCallbacks("root", LuaBindings::makeRootCallbacks());
        scriptContext.setCallbacks("xsb", LuaBindings::makeXsbCallbacks());
        scriptContext.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
        scriptContext.setCallbacks("celestial", celestialCallbacks);
        scriptContext.setCallbacks("versioning", makeVersioningCallbacks());

        result.content = scriptContext.invokePath<Json>("update", result.content);
        if (!result.content) {
          throw VersioningDatabaseException::format(
              "Could not bring versionedJson with identifier '{}' and version {} forward to current version of {}, conversion script from {} to {} returned null (un-upgradeable)",
              versionedJson.identifier, result.version, targetVersion, updateScript.fromVersion, updateScript.toVersion);
        }
        Logger::debug("Brought versionedJson '{}' from version {} to {}",
            versionedJson.identifier, result.version, updateScript.toVersion);
        result.version = updateScript.toVersion;
        // m_luaRoot.collectGarbage();
      }
    }
  } catch (std::exception const& e) {
    // m_luaRoot.collectGarbage();
    throw VersioningDatabaseException(strf("Could not bring versionedJson with identifier '{}' and version {} forward to current version of {}",
            versionedJson.identifier, result.version, targetVersion), e);
  }

  if (result.version > *targetVersion) {
    throw VersioningDatabaseException::format(
        "VersionedJson with identifier '{}' and version {} is newer than current version of {}, cannot load",
        versionedJson.identifier, result.version, targetVersion);
  }

  if (result.version != *targetVersion) {
    throw VersioningDatabaseException::format(
        "Could not bring VersionedJson with identifier '{}' and version {} forward to current version of {}, best version was {}",
        versionedJson.identifier, result.version, targetVersion, result.version);
  }

  return result;
}

Json VersioningDatabase::loadVersionedJson(VersionedJson const& versionedJson, String const& expectedIdentifier) const {
  versionedJson.expectIdentifier(expectedIdentifier);
  if (versionedJsonCurrent(versionedJson))
    return versionedJson.content;
  return updateVersionedJson(versionedJson).content;
}

LuaCallbacks VersioningDatabase::makeVersioningCallbacks() const {
  LuaCallbacks versioningCallbacks;

  versioningCallbacks.registerCallback("loadVersionedJson", [this](String const& storagePath) {
      try {
        auto& root = Root::singleton();
        String filePath = File::fullPath(root.toStoragePath(storagePath));
        String basePath = File::fullPath(root.toStoragePath("."));
        if (!filePath.beginsWith(basePath))
          throw VersioningDatabaseException::format(
              "Cannot load external VersionedJson outside of the Root storage path");
        auto loadedJson = VersionedJson::readFile(filePath);
        return updateVersionedJson(loadedJson).content;
      } catch (IOException const& e) {
        Logger::debug(
            "Unable to load versioned JSON file {} in versioning script: {}", storagePath, outputException(e, false));
        return Json();
      }
    });

  return versioningCallbacks;
}

}
