#include "StarRootLoader.hpp"
#include "StarJsonExtra.hpp"
#include "StarLexicalCast.hpp"

#ifdef STAR_SYSTEM_LINUX
#include <regex>
#endif

namespace Star {

Json const BaseAssetsSettings = Json::parseJson(R"JSON(
    {
      "assetTimeToLive" : 30,

      // In seconds, audio less than this long will be decompressed in memory.
      "audioDecompressLimit" : 4.0,

      "workerPoolSize" : 2,

      "pathIgnore" : [
        "/\\.",
        "/~",
        "thumbs\\.db$",
        "\\.bak$",
        "\\.tmp$",
        "\\.zip$",
        "\\.orig$",
        "\\.fail$",
        "\\.psd$",
        "\\.tmx$"
      ],

      "digestIgnore" : [
        "\\.ogg$",
        "\\.wav$",
        "\\.abc$"
      ],

      "luaGcPause" : 1.2,
      "luaGcStepMultiplier" : 2.0
    }
  )JSON");

Json const BaseDefaultConfiguration = Json::parseJson(R"JSON(
    {
      "configurationVersion" : {
        "basic" : 2
      },

      "gameServerPort" : 21025,
      "gameServerBind" : "::",

      "serverUsers" : {},
      "allowAnonymousConnections" : true,

      "bannedUuids" : [],
      "bannedIPs" : [],

      "serverName" : "An xStarbound Server",
      "maxPlayers" : 8,
      "maxTeamSize" : 4,
      "serverFidelity" : "automatic",

      "checkAssetsDigest" : false,

      "safeScripts" : true,
      "scriptRecursionLimit" : 100,
      "scriptInstructionLimit" : 10000000,
      "scriptProfilingEnabled" : false,
      "scriptInstructionMeasureInterval" : 10000,

      "allowAdminCommands" : true,
      "allowAdminCommandsFromAnyone" : false,
      "anonymousConnectionsAreAdmin" : false,

      "buildPermissionSettings" : {
        "enabled" : false,
        "guestWorldSpawnsAllowed" : true,
        "guestBuildingAllowed" : true,
        "accountClaimsEnabled" : false,
        "disallowServerGriefingWhenOwned" : false,
        "containerModificationProtection" : false,
        "containerOpenProtection" : false
      },

      "clientP2PJoinable" : true,
      "clientIPJoinable" : false,

      "clearUniverseFiles" : false,
      "clearPlayerFiles" : false,
      "playerBackupFileCount" : 3,

      "tutorialMessages" : true,

      "interactiveHighlight" : true,

      "monochromeLighting" : false,

      "crafting" : {
        "filterHaveMaterials" : false
      },

      "inventory" : {
        "pickupToActionBar" : true
      }
    }
  )JSON");

RootLoader::RootLoader(Defaults defaults) {
  String baseConfigFile;
  Maybe<String> userConfigFile;

  // FezzedOne: Workaround to ensure `-bootconfig` is usable with the bundled launch scripts.
  addParameter("bootconfig", "bootconfig", Multiple,
      strf("Boot time configuration file, defaults to xsbinit.config; only the last specified configuration file is used"));
  addParameter("logfile", "logfile", Optional,
      strf("Log to the given logfile relative to the root directory, defaults to {}",
          defaults.logFile ? *defaults.logFile : "no log file"));
  addParameter("loglevel", "level", Optional,
      strf("Sets the logging level (debug|info|warn|error), defaults to {}",
          LogLevelNames.getRight(defaults.logLevel)));
  addSwitch("quiet", strf("Do not log to stdout, defaults to {}", defaults.quiet));
  addSwitch("verbose", strf("Log to stdout, defaults to {}", !defaults.quiet));
  addSwitch("runtimeconfig",
      strf("Sets the path to the runtime configuration storage file relative to root directory, defaults to {}", /* FezzedOne: Fixed spelling error. */
          defaults.runtimeConfigFile ? *defaults.runtimeConfigFile : "no storage file"));
  addSwitch("noworkshop", strf("Do not load Steam Workshop content"));
  m_defaults = std::move(defaults);
}

pair<Root::Settings, RootLoader::Options> RootLoader::parseOrDie(
    StringList const& cmdLineArguments) const {
  auto options = VersionOptionParser::parseOrDie(cmdLineArguments);
  return {rootSettingsForOptions(options), options};
}

pair<RootUPtr, RootLoader::Options> RootLoader::initOrDie(StringList const& cmdLineArguments) const {
  auto p = parseOrDie(cmdLineArguments);
  auto root = make_unique<Root>(p.first);
  return {std::move(root), p.second};
}

pair<Root::Settings, RootLoader::Options> RootLoader::commandParseOrDie(int argc, char** argv) {
  auto options = VersionOptionParser::commandParseOrDie(argc, argv);
  return {rootSettingsForOptions(options), options};
}

pair<RootUPtr, RootLoader::Options> RootLoader::commandInitOrDie(int argc, char** argv) {
  auto p = commandParseOrDie(argc, argv);
  auto root = make_unique<Root>(p.first);
  return {std::move(root), p.second};
}

Root::Settings RootLoader::rootSettingsForOptions(Options const& options) const {
  try {
    const String configFileName = "xsbinit.config";
    const String bootConfigFile = options.parameters.value("bootconfig").maybeLast().value(configFileName);
#ifdef STAR_SYSTEM_LINUX
// FezzedOne: If the boot config file does not exist in the working directory, check `$XDG_CONFIG_HOME`.
#define CONFIG_ENV_VAR "XDG_CONFIG_HOME"
#define HOME_ENV_VAR "HOME"
    Json bootConfig = JsonObject{};
    const char* xdgConfigVar = ::getenv(CONFIG_ENV_VAR);
    const char* homePath = ::getenv(HOME_ENV_VAR);

    if (!homePath) throw StarException("$" HOME_ENV_VAR " is somehow not set; set this variable to your home directory");

    const String defaultXdgConfigPath = String(std::string(homePath) + "/.config/");
    const String xdgConfigPath = String(xdgConfigVar ? xdgConfigVar : defaultXdgConfigPath);
    const String linuxConfigPath = xdgConfigPath + "/xStarbound/" + bootConfigFile;
    if (File::exists(bootConfigFile)) {
      bootConfig = Json::parseJson(File::readFileString(bootConfigFile));
    } else if (File::exists(linuxConfigPath)) {
      bootConfig = Json::parseJson(File::readFileString(linuxConfigPath));
    } else {
      throw StarException("Cannot find boot config file; ensure xsbinit.config is present either in working directory"
                          " or in \"$" CONFIG_ENV_VAR "/xStarbound/\" and check permissions, or use -bootconfig");
    }
#else
    const Json bootConfig = Json::parseJson(File::readFileString(bootConfigFile));
#endif

    const Json assetsSettings = jsonMerge(
        BaseAssetsSettings,
        m_defaults.additionalAssetsSettings,
        bootConfig.get("assetsSettings", {}));

    Root::Settings rootSettings;
    rootSettings.assetsSettings.assetTimeToLive = assetsSettings.getInt("assetTimeToLive");
    rootSettings.assetsSettings.audioDecompressLimit = assetsSettings.getFloat("audioDecompressLimit");
    rootSettings.assetsSettings.workerPoolSize = assetsSettings.getUInt("workerPoolSize");
    rootSettings.assetsSettings.missingImage = assetsSettings.optString("missingImage");
    rootSettings.assetsSettings.missingAudio = assetsSettings.optString("missingAudio");
    rootSettings.assetsSettings.pathIgnore = jsonToStringList(assetsSettings.get("pathIgnore"));
    rootSettings.assetsSettings.digestIgnore = jsonToStringList(assetsSettings.get("digestIgnore"));
    rootSettings.assetsSettings.luaGcPause = assetsSettings.getFloat("luaGcPause");
    rootSettings.assetsSettings.luaGcStepMultiplier = assetsSettings.getFloat("luaGcStepMultiplier");

#ifdef STAR_SYSTEM_LINUX
    // FezzedOne: Substitute `${HOME}` and `$HOME` for the user's home directory, but not if the `$` is escaped (i.e., `\$`).
    const std::string homePathStr = std::string(homePath) + "/";
    const List<String> rawAssetDirectories = jsonToStringList(bootConfig.get("assetDirectories"));
    const std::string dollarSign = "$";
#define HOME_SUBST_REGEX_FLAGS (std::regex::ECMAScript)
#define HOME_SUBST_REGEX (R"((?:^|[^\\])\$(\{)" HOME_ENV_VAR R"(\}|)" HOME_ENV_VAR R"())")
#define ESCAPE_SUBST_REGEX (R"(\\\$)")
    try {
      const std::regex envVarRegex(HOME_SUBST_REGEX, HOME_SUBST_REGEX_FLAGS),
          escapeRegex(ESCAPE_SUBST_REGEX, HOME_SUBST_REGEX_FLAGS);

      rootSettings.assetDirectories = {};
      for (const String& assetDir : rawAssetDirectories)
        rootSettings.assetDirectories.emplaceAppend(String(
            std::regex_replace(
                std::regex_replace(assetDir.utf8(), envVarRegex, homePathStr),
                escapeRegex, dollarSign)));
    } catch (std::regex_error const& e) {
      ::printf("[Error] Error during regex substitution for asset directory paths;"
               " not performing $" HOME_ENV_VAR " substitution: %s\nSubstitution regex: '%s'\n",
          e.what(), HOME_SUBST_REGEX);
      rootSettings.assetDirectories = rawAssetDirectories;
    }
#else
    rootSettings.assetDirectories = jsonToStringList(bootConfig.get("assetDirectories"));
#endif

    rootSettings.defaultConfiguration = jsonMerge(
        BaseDefaultConfiguration,
        m_defaults.additionalDefaultConfiguration,
        bootConfig.get("defaultConfiguration", {}));

#ifdef STAR_SYSTEM_LINUX
    const std::string rawStorageDirectory = bootConfig.getString("storageDirectory").utf8();
    try {
      const std::regex envVarRegex(HOME_SUBST_REGEX, HOME_SUBST_REGEX_FLAGS),
          escapeRegex(ESCAPE_SUBST_REGEX, HOME_SUBST_REGEX_FLAGS);
      rootSettings.storageDirectory = String(
          std::regex_replace(
              std::regex_replace(rawStorageDirectory, envVarRegex, homePath),
              escapeRegex, dollarSign));
    } catch (std::regex_error const& e) {
      ::printf("[Error] Error during regex substitution for storage directory path;"
               " not performing $" HOME_ENV_VAR " substitution: %s\nSubstitution regex: '%s'\n",
          e.what(), HOME_SUBST_REGEX);
      rootSettings.storageDirectory = String(rawStorageDirectory);
    }
#else
    rootSettings.storageDirectory = bootConfig.getString("storageDirectory");
#endif

    rootSettings.logFile = options.parameters.value("logfile").maybeFirst().orMaybe(m_defaults.logFile);
    rootSettings.logFileBackups = bootConfig.getUInt("logFileBackups", 5);

    if (auto ll = options.parameters.value("loglevel").maybeFirst())
      rootSettings.logLevel = LogLevelNames.getLeft(*ll);
    else
      rootSettings.logLevel = m_defaults.logLevel;

    if (options.switches.contains("quiet"))
      rootSettings.quiet = true;
    else if (options.switches.contains("verbose"))
      rootSettings.quiet = false;
    else
      rootSettings.quiet = m_defaults.quiet;

    if (auto rc = options.parameters.value("runtimeconfig").maybeFirst())
      rootSettings.runtimeConfigFile = *rc;
    else
      rootSettings.runtimeConfigFile = m_defaults.runtimeConfigFile;

    return rootSettings;

  } catch (std::exception const& e) {
    throw StarException("Could not perform initial Root load", e);
  }
}

} // namespace Star
