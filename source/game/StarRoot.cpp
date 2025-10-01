#include "StarRoot.hpp"
#include "StarAiDatabase.hpp"
#include "StarAssets.hpp"
#include "StarBehaviorDatabase.hpp"
#include "StarBiomeDatabase.hpp"
#include "StarCodexDatabase.hpp"
#include "StarCollectionDatabase.hpp"
#include "StarConfiguration.hpp"
#include "StarDamageDatabase.hpp"
#include "StarDanceDatabase.hpp"
#include "StarDirectoryAssetSource.hpp"
#include "StarDungeonGenerator.hpp"
#include "StarEffectSourceDatabase.hpp"
#include "StarEmoteProcessor.hpp"
#include "StarEncode.hpp"
#include "StarEntityFactory.hpp"
#include "StarFile.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarItemDatabase.hpp"
#include "StarItemDrop.hpp"
#include "StarIterator.hpp"
#include "StarJsonBuilder.hpp"
#include "StarJsonExtra.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarLogging.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarMonster.hpp"
#include "StarNameGenerator.hpp"
#include "StarNpc.hpp"
#include "StarObject.hpp"
#include "StarObjectDatabase.hpp"
#include "StarPackedAssetSource.hpp"
#include "StarParticleDatabase.hpp"
#include "StarPlant.hpp"
#include "StarPlantDrop.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerFactory.hpp"
#include "StarProjectile.hpp"
#include "StarProjectileDatabase.hpp"
#include "StarQuestTemplateDatabase.hpp"
#include "StarRadioMessageDatabase.hpp"
#include "StarSpawnTypeDatabase.hpp"
#include "StarSpeciesDatabase.hpp"
#include "StarStagehandDatabase.hpp"
#include "StarStatisticsDatabase.hpp"
#include "StarStatusEffectDatabase.hpp"
#include "StarStoredFunctions.hpp"
#include "StarTechDatabase.hpp"
#include "StarTenantDatabase.hpp"
#include "StarTerrainDatabase.hpp"
#include "StarTilesetDatabase.hpp"
#include "StarTreasure.hpp"
#include "StarVehicleDatabase.hpp"
#include "StarWorkerPool.hpp"

#if defined TRACY_ENABLE
#include "tracy/Tracy.hpp"
#else
#define ZoneScoped
#define ZoneScopedN(name)
#endif

namespace Star {

namespace {
  unsigned const RootMaintenanceSleep = 5000;
  unsigned const ImageMaintenancePeriods = 30;
  unsigned const RootLoadThreads = 4;
} // namespace

atomic<Root*> Root::s_singleton;

Root* Root::singletonPtr() {
  return s_singleton.load();
}

Root& Root::singleton() {
  auto ptr = s_singleton.load();
  if (!ptr)
    throw RootException("Root::singleton() called with no Root instance available");
  else
    return *ptr;
}

Root::Root(Settings settings) {
  Root* oldRoot = nullptr;
  if (!s_singleton.compare_exchange_strong(oldRoot, this))
    throw RootException("Singleton Root has been constructed twice");

  m_settings = std::move(settings);
  if (m_settings.runtimeConfigFile)
    m_runtimeConfigFile = toStoragePath(*m_settings.runtimeConfigFile);

  if (!File::isDirectory(m_settings.storageDirectory))
    File::makeDirectory(m_settings.storageDirectory);

  if (m_settings.logFile) {
    String logFile = toStoragePath(*m_settings.logFile);
    File::backupFileInSequence(logFile, m_settings.logFileBackups);
    Logger::addSink(make_shared<FileLogSink>(logFile, m_settings.logLevel, true));
  }
  Logger::stdoutSink()->setLevel(m_settings.logLevel);

  if (m_settings.quiet)
    Logger::removeStdoutSink();

  Logger::info("Root: Preparing Root...");

  m_stopImageMaintenanceThread = false;
  m_imageMaintenanceThread = Thread::invoke("Root::imageMaintenanceMain", [this]() {
    MutexLocker locker(m_imageMaintenanceStopMutex);
    unsigned maintenancePeriods = 0;
    while (!m_stopImageMaintenanceThread) {
      maintenancePeriods++;
      if (maintenancePeriods == ImageMaintenancePeriods) {
        maintenancePeriods = 0;
        MutexLocker locker(m_imageMetadataDatabaseMutex);
        if (ImageMetadataDatabasePtr imgDb = m_imageMetadataDatabase) {
          locker.unlock();
          imgDb->cleanup();
        }
      }

      m_imageMaintenanceStopCondition.wait(m_imageMaintenanceStopMutex, RootMaintenanceSleep);
    }
  });

  m_stopMaintenanceThread = false;
  m_maintenanceThread = Thread::invoke("Root::maintenanceMain", [this]() {
    MutexLocker locker(m_maintenanceStopMutex);
    while (!m_stopMaintenanceThread) {
      m_reloadListeners.clearExpiredListeners();

      {
        MutexLocker locker(m_objectDatabaseMutex);
        if (ObjectDatabasePtr objectDb = m_objectDatabase) {
          locker.unlock();
          objectDb->cleanup();
        }
      }
      {
        MutexLocker locker(m_itemDatabaseMutex);
        if (ItemDatabasePtr itemDb = m_itemDatabase) {
          locker.unlock();
          itemDb->cleanup();
        }
      }
      {
        MutexLocker locker(m_monsterDatabaseMutex);
        if (MonsterDatabasePtr monsterDb = m_monsterDatabase) {
          locker.unlock();
          monsterDb->cleanup();
        }
      }
      {
        MutexLocker locker(m_assetsMutex);
        if (AssetsPtr assets = m_assets) {
          locker.unlock();
          assets->cleanup();
        }
      }
      {
        MutexLocker locker(m_tenantDatabaseMutex);
        if (TenantDatabasePtr tenantDb = m_tenantDatabase) {
          locker.unlock();
          tenantDb->cleanup();
        }
      }

      Random::addEntropy();

      {
        MutexLocker locker(m_configurationMutex);
        writeConfig();
      }

      m_maintenanceStopCondition.wait(m_maintenanceStopMutex, RootMaintenanceSleep);
    }
  });

  Logger::info("Root: Done preparing Root.");
}

Root::~Root() {
  Logger::info("Root: Shutting down Root");

  {
    MutexLocker locker(m_maintenanceStopMutex);
    m_stopMaintenanceThread = true;
    m_maintenanceStopCondition.signal();
  }
  m_maintenanceThread.finish();

  {
    MutexLocker locker(m_imageMaintenanceStopMutex);
    m_stopImageMaintenanceThread = true;
    m_imageMaintenanceStopCondition.signal();
  }
  m_imageMaintenanceThread.finish();

  m_reloadListeners.clearAllListeners();

  writeConfig();

  s_singleton.store(nullptr);
}

void Root::reload() {
  ZoneScoped;
  Logger::info("Root: Reloading from disk");

  {
    // We need to lock all the mutexes to reset everything to cause it to be
    // reloaded, but whenever we lock individual members we should always do it
    // in the same order (well, the same order*ing* not necessarily the same
    // order) to avoid deadlocks.  This means that we need to enumerate the
    // finicky, implicit dependency order that we have due to each member's
    // constructor referencing root recursively.  We could avoid doing this
    // explicitly with C++11's std::lock (if c++11 threading primitives were
    // finally reliable on all targets), or some other equivalent deadlock
    // avoidance algorithm.

    // Entity factory depends on all the entity databases and the versioning
    // database.
    MutexLocker entityFactoryLock(m_entityFactoryMutex);

    // Species database depends on the item database.
    MutexLocker speciesDatabaseLock(m_speciesDatabaseMutex);

    // Item database depends on object database, codex database and (FezzedOne)
    // configuration.
    MutexLocker itemDatabaseLock(m_itemDatabaseMutex);

    // These databases depend on various things below, but not the item database
    MutexLocker objectDatabaseLock(m_objectDatabaseMutex);
    MutexLocker playerFactoryLock(m_playerFactoryMutex);
    MutexLocker npcDatabaseLock(m_npcDatabaseMutex);
    MutexLocker stagehandDatabaseLock(m_stagehandDatabaseMutex);
    MutexLocker vehicleDatabaseLock(m_vehicleDatabaseMutex);
    MutexLocker monsterDatabaseLock(m_monsterDatabaseMutex);
    MutexLocker plantDatabaseLock(m_plantDatabaseMutex);
    MutexLocker projectileDatabaseLock(m_projectileDatabaseMutex);

    // Biome database depends on liquids, materials, and stored function
    // databases.
    MutexLocker biomeDatabaseLock(m_biomeDatabaseMutex);

    // Dungeon definitions database depends on the material and liquids database
    MutexLocker dungeonDefinitionsLock(m_dungeonDefinitionsMutex);
    MutexLocker tilesetDatabaseLock(m_tilesetDatabaseMutex);

    MutexLocker statisticsDatabaseLock(m_statisticsDatabaseMutex);

    // Liquids database depends on the materials database
    MutexLocker liquidsDatabaseLock(m_liquidsDatabaseMutex);

    // Material database depends on particle database
    MutexLocker materialDatabaseLock(m_materialDatabaseMutex);

    // Databases that depend on functions database.
    MutexLocker damageDatabaseLock(m_damageDatabaseMutex);
    MutexLocker effectSourceDatabaseLock(m_effectSourceDatabaseMutex);
    MutexLocker statusEffectDatabaseLock(m_statusEffectDatabaseMutex);
    MutexLocker treasureDatabaseLock(m_treasureDatabaseMutex);

    // Databases that don't depend on anything other than assets
    MutexLocker codexDatabaseLock(m_codexDatabaseMutex);
    MutexLocker behaviorDatabaseMutex(m_behaviorDatabaseMutex);
    MutexLocker techDatabaseLock(m_techDatabaseMutex);
    MutexLocker aiDatabaseLock(m_aiDatabaseMutex);
    MutexLocker questTemplateDatabaseLock(m_questTemplateDatabaseMutex);
    MutexLocker emoteProcessorLock(m_emoteProcessorMutex);
    MutexLocker terrainDatabaseLock(m_terrainDatabaseMutex);
    MutexLocker particleDatabaseLock(m_particleDatabaseMutex);
    MutexLocker versioningDatabaseLock(m_versioningDatabaseMutex); // FezzedOne: Also depends on configuration now.
    MutexLocker functionDatabaseLock(m_functionDatabaseMutex);
    MutexLocker imageMetadataDatabaseLock(m_imageMetadataDatabaseMutex);
    MutexLocker tenantDatabaseLock(m_tenantDatabaseMutex);
    MutexLocker nameGeneratorLock(m_nameGeneratorMutex);
    MutexLocker danceDatabaseLock(m_danceDatabaseMutex);
    MutexLocker spawnTypeDatabaseLock(m_spawnTypeDatabaseMutex);
    MutexLocker radioMessageDatabaseLock(m_radioMessageDatabaseMutex);
    MutexLocker collectionDatabaseLock(m_collectionDatabaseMutex);

    // Configuration and Assets are at the very bottom of the hierarchy.
    MutexLocker configurationLock(m_configurationMutex);
    MutexLocker assetsLock(m_assetsMutex);

    writeConfig();

    m_entityFactory.reset();
    m_speciesDatabase.reset();
    m_itemDatabase.reset();
    m_objectDatabase.reset();
    m_playerFactory.reset();
    m_stagehandDatabase.reset();
    m_vehicleDatabase.reset();
    m_npcDatabase.reset();
    m_monsterDatabase.reset();
    m_plantDatabase.reset();
    m_projectileDatabase.reset();
    m_biomeDatabase.reset();
    m_dungeonDefinitions.reset();
    m_tilesetDatabase.reset();
    m_statisticsDatabase.reset();
    m_liquidsDatabase.reset();
    m_materialDatabase.reset();
    m_damageDatabase.reset();
    m_effectSourceDatabase.reset();
    m_statusEffectDatabase.reset();
    m_treasureDatabase.reset();
    m_codexDatabase.reset();
    m_behaviorDatabase.reset();
    m_techDatabase.reset();
    m_aiDatabase.reset();
    m_questTemplateDatabase.reset();
    m_emoteProcessor.reset();
    m_terrainDatabase.reset();
    m_particleDatabase.reset();
    m_versioningDatabase.reset();
    m_functionDatabase.reset();
    // FezzedOne: Clean up the image metadata database when reloading.
    if (m_imageMetadataDatabase) m_imageMetadataDatabase->cleanup(true);
    m_imageMetadataDatabase.reset();
    m_tenantDatabase.reset();
    m_nameGenerator.reset();
    m_danceDatabase.reset();
    m_spawnTypeDatabase.reset();
    m_radioMessageDatabase.reset();
    m_collectionDatabase.reset();
    m_assets.reset();
    m_configuration.reset();
  }

  m_reloadListeners.trigger();
}

void Root::hotReload() {
  MutexLocker assetsLock(m_assetsMutex);
  m_assets->hotReload();
}

void Root::reloadWithMods(StringList modDirectories) {
  MutexLocker locker(m_modsMutex);
  m_modDirectories = std::move(modDirectories);
  reload();
}

void Root::fullyLoad() {
  auto workerPool = WorkerPool("Root::fullyLoad", RootLoadThreads);
  List<WorkerPoolHandle> loaders;

  loaders.reserve(40);

  loaders.append(workerPool.addWork(swallow(bind(&Root::assets, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::configuration, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::codexDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::behaviorDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::techDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::aiDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::questTemplateDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::emoteProcessor, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::terrainDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::particleDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::versioningDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::functionDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::imageMetadataDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::tenantDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::nameGenerator, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::danceDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::spawnTypeDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::radioMessageDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::collectionDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::statisticsDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::speciesDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::projectileDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::stagehandDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::damageDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::effectSourceDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::statusEffectDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::treasureDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::materialDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::objectDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::npcDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::plantDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::itemDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::monsterDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::vehicleDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::playerFactory, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::entityFactory, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::biomeDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::liquidsDatabase, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::dungeonDefinitions, this))));
  loaders.append(workerPool.addWork(swallow(bind(&Root::tilesetDatabase, this))));

  auto startSeconds = Time::monotonicTime();
  for (auto& loader : loaders)
    loader.finish();
  Logger::info("Root: Loaded everything in {} seconds", Time::monotonicTime() - startSeconds);

  {
    MutexLocker locker(m_assetsMutex);
    if (m_assets)
      m_assets->clearCache();
  }
}

void Root::cleanUpImageMetadata() {
  MutexLocker locker(m_imageMetadataDatabaseMutex);
  if (ImageMetadataDatabasePtr imgMetadataDb = m_imageMetadataDatabase) {
    locker.unlock();
    imgMetadataDb->cleanup(true);
  }
}

void Root::registerReloadListener(ListenerWeakPtr reloadListener) {
  m_reloadListeners.addListener(std::move(reloadListener));
}

String Root::toStoragePath(String const& path) const {
  return File::relativeTo(m_settings.storageDirectory, File::convertDirSeparators(path));
}

AssetsConstPtr Root::assets() {
  return loadMemberFunction<Assets>(m_assets, m_assetsMutex, "Assets", [this]() {
    StringList assetDirectories = m_settings.assetDirectories;
    assetDirectories.appendAll(m_modDirectories);

    auto assets = make_shared<Assets>(m_settings.assetsSettings, scanForAssetSources(assetDirectories));
    Logger::info("Assets digest is {}", hexEncode(assets->digest()));
    return assets;
  });
}

ConfigurationPtr Root::configuration() {
  return loadMemberFunction<Configuration>(m_configuration, m_configurationMutex, "Configuration", [this]() {
    Json currentConfig;

    if (m_runtimeConfigFile) {
      if (!File::isFile(*m_runtimeConfigFile)) {
        Logger::info("Root: no runtime config file, creating new default runtime config");
        currentConfig = m_settings.defaultConfiguration;
      } else {
        try {
          Json config = Json::parseJson(File::readFileString(*m_runtimeConfigFile));
          if (!config.isType(Json::Type::Object))
            throw ConfigurationException("User config is not of JSON type Object");

          if (config.get("configurationVersion", {}) != m_settings.defaultConfiguration.get("configurationVersion", {}))
            throw ConfigurationException("User config version does not match default config version");

          currentConfig = config;
        } catch (std::exception const& e) {
          Logger::warn("Root: Failed to load user configuration file {}, resetting user config: {}", *m_runtimeConfigFile, outputException(e, false));
          currentConfig = m_settings.defaultConfiguration;
          File::rename(*m_runtimeConfigFile, *m_runtimeConfigFile + ".old");
        }
      }
    } else {
      currentConfig = m_settings.defaultConfiguration;
    }

    return make_shared<Configuration>(m_settings.defaultConfiguration, currentConfig);
  });
}

ObjectDatabaseConstPtr Root::objectDatabase() {
  return loadMember(m_objectDatabase, m_objectDatabaseMutex, "ObjectDatabase");
}

PlantDatabaseConstPtr Root::plantDatabase() {
  return loadMember(m_plantDatabase, m_plantDatabaseMutex, "PlantDatabase");
}

ProjectileDatabaseConstPtr Root::projectileDatabase() {
  return loadMember(m_projectileDatabase, m_projectileDatabaseMutex, "ProjectileDatabase");
}

MonsterDatabaseConstPtr Root::monsterDatabase() {
  return loadMember(m_monsterDatabase, m_monsterDatabaseMutex, "MonsterDatabase");
}

NpcDatabaseConstPtr Root::npcDatabase() {
  return loadMember(m_npcDatabase, m_npcDatabaseMutex, "NpcDatabase");
}

StagehandDatabaseConstPtr Root::stagehandDatabase() {
  return loadMember(m_stagehandDatabase, m_stagehandDatabaseMutex, "StagehandDatabase");
}

VehicleDatabaseConstPtr Root::vehicleDatabase() {
  return loadMember(m_vehicleDatabase, m_vehicleDatabaseMutex, "VehicleDatabase");
}

PlayerFactoryConstPtr Root::playerFactory() {
  return loadMember(m_playerFactory, m_playerFactoryMutex, "PlayerFactory");
}

EntityFactoryConstPtr Root::entityFactory() {
  return loadMember(m_entityFactory, m_entityFactoryMutex, "EntityFactory");
}

PatternedNameGeneratorConstPtr Root::nameGenerator() {
  return loadMember(m_nameGenerator, m_nameGeneratorMutex, "NameGenerator");
}

ItemDatabaseConstPtr Root::itemDatabase() {
  return loadMember(m_itemDatabase, m_itemDatabaseMutex, "ItemDatabase");
}

MaterialDatabaseConstPtr Root::materialDatabase() {
  return loadMember(m_materialDatabase, m_materialDatabaseMutex, "MaterialDatabase");
}

TerrainDatabaseConstPtr Root::terrainDatabase() {
  return loadMember(m_terrainDatabase, m_terrainDatabaseMutex, "TerrainDatabase");
}

BiomeDatabaseConstPtr Root::biomeDatabase() {
  return loadMember(m_biomeDatabase, m_biomeDatabaseMutex, "BiomeDatabase");
}

LiquidsDatabaseConstPtr Root::liquidsDatabase() {
  return loadMember(m_liquidsDatabase, m_liquidsDatabaseMutex, "LiquidsDatabase");
}

StatusEffectDatabaseConstPtr Root::statusEffectDatabase() {
  return loadMember(m_statusEffectDatabase, m_statusEffectDatabaseMutex, "StatusEffectDatabase");
}

DamageDatabaseConstPtr Root::damageDatabase() {
  return loadMember(m_damageDatabase, m_damageDatabaseMutex, "DamageDatabase");
}

ParticleDatabaseConstPtr Root::particleDatabase() {
  return loadMember(m_particleDatabase, m_particleDatabaseMutex, "ParticleDatabase");
}

EffectSourceDatabaseConstPtr Root::effectSourceDatabase() {
  return loadMember(m_effectSourceDatabase, m_effectSourceDatabaseMutex, "EffectSourceDatabase");
}

FunctionDatabaseConstPtr Root::functionDatabase() {
  return loadMember(m_functionDatabase, m_functionDatabaseMutex, "FunctionDatabase");
}

TreasureDatabaseConstPtr Root::treasureDatabase() {
  return loadMember(m_treasureDatabase, m_treasureDatabaseMutex, "TreasureDatabase");
}

DungeonDefinitionsConstPtr Root::dungeonDefinitions() {
  return loadMember(m_dungeonDefinitions, m_dungeonDefinitionsMutex, "DungeonDefinitions");
}

TilesetDatabaseConstPtr Root::tilesetDatabase() {
  return loadMember(m_tilesetDatabase, m_tilesetDatabaseMutex, "TilesetDatabase");
}

StatisticsDatabaseConstPtr Root::statisticsDatabase() {
  return loadMember(m_statisticsDatabase, m_statisticsDatabaseMutex, "StatisticsDatabase");
}

EmoteProcessorConstPtr Root::emoteProcessor() {
  return loadMember(m_emoteProcessor, m_emoteProcessorMutex, "EmoteProcessor");
}

SpeciesDatabaseConstPtr Root::speciesDatabase() {
  return loadMember(m_speciesDatabase, m_speciesDatabaseMutex, "SpeciesDatabase");
}

ImageMetadataDatabaseConstPtr Root::imageMetadataDatabase() {
  return loadMember(m_imageMetadataDatabase, m_imageMetadataDatabaseMutex, "ImageMetadataDatabase");
}

VersioningDatabaseConstPtr Root::versioningDatabase() {
  return loadMember(m_versioningDatabase, m_versioningDatabaseMutex, "VersioningDatabase");
}

QuestTemplateDatabaseConstPtr Root::questTemplateDatabase() {
  return loadMember(m_questTemplateDatabase, m_questTemplateDatabaseMutex, "QuestTemplateDatabase");
}

AiDatabaseConstPtr Root::aiDatabase() {
  return loadMember(m_aiDatabase, m_aiDatabaseMutex, "AiDatabase");
}

TechDatabaseConstPtr Root::techDatabase() {
  return loadMember(m_techDatabase, m_techDatabaseMutex, "TechDatabase");
}

CodexDatabaseConstPtr Root::codexDatabase() {
  return loadMember(m_codexDatabase, m_codexDatabaseMutex, "CodexDatabase");
}

BehaviorDatabaseConstPtr Root::behaviorDatabase() {
  return loadMember(m_behaviorDatabase, m_behaviorDatabaseMutex, "BehaviorDatabase");
}

TenantDatabaseConstPtr Root::tenantDatabase() {
  return loadMember(m_tenantDatabase, m_tenantDatabaseMutex, "TenantDatabase");
}

DanceDatabaseConstPtr Root::danceDatabase() {
  return loadMember(m_danceDatabase, m_danceDatabaseMutex, "DanceDatabase");
}

SpawnTypeDatabaseConstPtr Root::spawnTypeDatabase() {
  return loadMember(m_spawnTypeDatabase, m_spawnTypeDatabaseMutex, "SpawnTypeDatabase");
}

RadioMessageDatabaseConstPtr Root::radioMessageDatabase() {
  return loadMember(m_radioMessageDatabase, m_radioMessageDatabaseMutex, "RadioMessageDatabase");
}

CollectionDatabaseConstPtr Root::collectionDatabase() {
  return loadMember(m_collectionDatabase, m_collectionDatabaseMutex, "CollectionDatabase");
}

StringList Root::scanForAssetSources(StringList const& directories) {
  struct AssetSource {
    String path;
    Maybe<String> name;
    float priority;
    StringList requires_;
    StringList includes;
    StringList substitutes;
    Maybe<String> version;
  };
  List<shared_ptr<AssetSource>> assetSources;
  StringMap<shared_ptr<AssetSource>> namedSources;

  // Scan for assets in each given directory, the first-level ordering of asset
  // sources comes from the scanning order here, and then alphabetically by the
  // file / directory name

  for (auto const& directory : directories) {
    if (!File::isDirectory(directory)) {
      Logger::info("Root: Skipping asset directory '{}', directory not found", directory);
      continue;
    }

    Logger::info("Root: Scanning for asset sources in directory '{}'", directory);

    for (auto entry : File::dirList(directory, true).sorted()) {
      AssetSourcePtr source;
      auto fileName = File::relativeTo(directory, entry.first);
      if (entry.first.beginsWith(".") || entry.first.beginsWith("_"))
        Logger::info("Root: Skipping hidden '{}' in asset directory", entry.first);
      else if (entry.second)
        source = make_shared<DirectoryAssetSource>(fileName);
      else if (entry.first.endsWith(".pak"))
        source = make_shared<PackedAssetSource>(fileName);
      else
        Logger::warn("Root: Unrecognised file in asset directory '{}', skipping", entry.first);

      if (!source)
        continue;

      auto metadata = source->metadata();

      auto assetSource = make_shared<AssetSource>();
      // FezzedOne: Get rid of crashes from invalid or bad metadata.
      assetSource->path = fileName;
      if (auto nameJson = metadata.ptr("name"))
        assetSource->name = nameJson->isType(Json::Type::String) ? nameJson->toString() : nameJson->repr();
      else
        assetSource->name = {};
      if (auto priorityJson = metadata.ptr("priority")) {
        if (priorityJson->isType(Json::Type::Float) || priorityJson->isType(Json::Type::Int)) {
          assetSource->priority = priorityJson->toFloat();
        } else {
          assetSource->priority = 0.0f;
          Logger::warn("Root: Asset source '{}' (file name '{}') has non-numerical priority value; assuming priority of 0",
              assetSource->name, fileName);
        }
      } else {
        assetSource->priority = 0.0f;
      }
      if (auto requiresJson = metadata.ptr("requires")) {
        StringList requiresList{};
        if (requiresJson->isType(Json::Type::Array)) {
          for (auto val : requiresJson->iterateArray()) {
            requiresList.append(val.isType(Json::Type::String) ? val.toString() : val.repr());
          }
        } else {
          Logger::warn("Root: Asset source '{}' (file name '{}') has non-array \"requires\" value; skipping requires",
              assetSource->name, fileName);
        }
        assetSource->requires_ = requiresList;
      }
      if (auto includesJson = metadata.ptr("includes")) {
        StringList includesList{};
        if (includesJson->isType(Json::Type::Array)) {
          for (auto val : includesJson->iterateArray()) {
            includesList.append(val.isType(Json::Type::String) ? val.toString() : val.repr());
          }
        } else {
          Logger::warn("Root: Asset source '{}' (file name '{}') has non-array \"includes\" value; skipping includes",
              assetSource->name, fileName);
        }
        assetSource->includes = includesList;
      }
      if (auto substitutesJson = metadata.ptr("substitutes")) {
        StringList substitutesList{};
        if (substitutesJson->isType(Json::Type::Array)) {
          for (auto val : substitutesJson->iterateArray()) {
            substitutesList.append(val.isType(Json::Type::String) ? val.toString() : val.repr());
          }
        } else {
          Logger::warn("Root: Asset source '{}' (file name '{}') has non-array \"substitutes\" value; skipping substitutions",
              assetSource->name, fileName);
        }
        assetSource->substitutes = substitutesList;
      }
      if (auto versionJson = metadata.ptr("version"))
        assetSource->version = versionJson->isType(Json::Type::String) ? versionJson->toString() : versionJson->repr();
      else
        assetSource->version = {};

      bool needsSubstitutionCheck = false;

      if (assetSource->name) {
        if (auto oldAssetSource = namedSources.value(*assetSource->name)) {
          if (oldAssetSource->priority <= assetSource->priority) {
            Logger::warn("Root: Overriding duplicate asset source '{}' named '{}' with higher or equal priority source '{}'",
                oldAssetSource->path, *assetSource->name, assetSource->path);
            *oldAssetSource = *assetSource;
            assetSources.append(assetSource);
          } else {
            Logger::warn("Root: Skipping duplicate asset source '{}' named '{}', previous source '{}' has higher priority",
                assetSource->path, *assetSource->name, oldAssetSource->path);
            needsSubstitutionCheck = true;
          }
        } else {
          namedSources[*assetSource->name] = assetSource;
          assetSources.append(assetSource);
        }
      } else {
        assetSources.append(assetSource);
      }

      // FezzedOne: Added requirement/include substitution.
      if (!assetSource->substitutes.empty()) {
        for (String const& substitute : assetSource->substitutes) {
          if (auto oldAssetSource = namedSources.value(substitute)) {
            if (oldAssetSource->priority <= assetSource->priority) {
              Logger::warn("Root: Overriding duplicate asset source '{}' named '{}' with higher or equal priority source '{}' named '{}' substituting as '{}'",
                  oldAssetSource->path, oldAssetSource->name.value("<unnamed>"), assetSource->path, assetSource->name.value("<unnamed>"), substitute);
              namedSources[substitute] = assetSource;
              if (needsSubstitutionCheck) {
                assetSources.append(assetSource);
                needsSubstitutionCheck = false;
              }
            } else {
              Logger::warn("Root: Skipping duplicate asset source '{}' named '{}', previous source '{}' named '{}' substituting as '{}' has higher priority",
                  assetSource->path, assetSource->name.value("<unnamed>"), oldAssetSource->path, oldAssetSource->name.value("<unnamed>"), substitute);
            }
          } else {
            Logger::info("Root: Asset source '{}' named '{}' substituting as '{}'",
                assetSource->path, assetSource->name.value("<unnamed>"), substitute);
            namedSources[substitute] = assetSource;
            if (needsSubstitutionCheck) {
              assetSources.append(assetSource);
              needsSubstitutionCheck = false;
            }
          }
        }
      }
    }
  }

  // Then, order asset sources so that lower priority assets come before higher
  // priority ones

  assetSources.sort([](auto const& a, auto const& b) {
    return a->priority < b->priority;
  });

  // Finally, sort asset sources so that sources that have dependencies come
  // after their dependencies.

  HashSet<shared_ptr<AssetSource>> workingSet;
  OrderedHashSet<shared_ptr<AssetSource>> dependencySortedSources;

  function<void(shared_ptr<AssetSource>)> dependencySortVisit;
  dependencySortVisit = [&](shared_ptr<AssetSource> source) {
    if (workingSet.contains(source))
      throw StarException("Asset dependencies form a cycle");

    if (dependencySortedSources.contains(source))
      return;

    workingSet.add(source);

    for (auto const& includeName : source->includes) {
      if (auto include = namedSources.ptr(includeName))
        dependencySortVisit(*include);
    }

    for (auto const& requirementName : source->requires_) {
      if (auto requirement = namedSources.ptr(requirementName))
        dependencySortVisit(*requirement);
      else
        throw StarException(strf("Asset source '{}' is missing dependency '{}'", source->name ? *source->name : "<unnamed>", requirementName));
    }

    workingSet.remove(source);

    dependencySortedSources.add(std::move(source));
  };

  for (auto source : assetSources)
    dependencySortVisit(std::move(source));

  StringList sourcePaths;
  // Throw exceptions and close the game if there's an xStarbound asset version mismatch or the xStarbound or base game assets aren't found.
  bool xSbAssetsFound = false;
  bool baseAssetsFound = false;
  for (auto const& source : dependencySortedSources) {
    if (sourcePaths.contains(source->path)) continue;
    if (source->name) {
      if (*source->name == "base") {
        baseAssetsFound = true;
        Logger::info("Root: Detected base game assets at '{}'", source->path);
      } else if (*source->name == "xSBassets") {
        xSbAssetsFound = true;
        if (source->version) {
          if (*source->version == xSbAssetVersionString) {
            Logger::info("Root: Detected xStarbound assets, version '{}', at '{}'.", *source->version, source->path);
          } else {
            throw StarException(strf("Root: Detected mismatched version '{}' of xStarbound assets at '{}', expected version '{}'! "
                                     "Make sure xStarbound is correctly installed and up to date.",
                *source->version, source->path, xSbAssetVersionString));
          }
        } else {
          throw StarException(strf("Root: Detected non-versioned xStarbound assets at '{}', expected version '{}'! "
                                   "Make sure xStarbound is correctly installed and up to date.",
              source->path, xSbAssetVersionString));
        }
      } else {
        Logger::info("Root: Detected asset source named '{}'{} at '{}'.",
            *source->name,
            (source->version ? strf(", version '{}',", *source->version) : ", unversioned,"),
            source->path);
      }
    } else {
      Logger::info("Root: Detected unnamed asset source at '{}'.", source->path);
    }
    sourcePaths.append(source->path);
  }

  if (!baseAssetsFound)
    throw StarException("Root: Base game assets not found! Make sure xStarbound is correctly installed.");

  if (!xSbAssetsFound)
    throw StarException("Root: xStarbound assets not found! Make sure xStarbound is correctly installed.");

  return sourcePaths;
}

void Root::writeConfig() {
  if (m_configuration) {
    auto currentConfig = m_configuration->currentConfiguration();
    if (m_lastRuntimeConfig != currentConfig) {
      if (m_runtimeConfigFile) {
        Logger::info("Root: Writing runtime configuration to '{}'", *m_runtimeConfigFile);
        File::overwriteFileWithRename(m_configuration->printConfiguration(), *m_runtimeConfigFile);
      }
      m_lastRuntimeConfig = currentConfig;
    }
  }
}

template <typename T, typename... Params>
shared_ptr<T> Root::loadMember(shared_ptr<T>& ptr, Mutex& mutex, char const* name, Params&&... params) {
  return loadMemberFunction<T>(ptr, mutex, name, [&]() {
    return make_shared<T>(std::forward<Params>(params)...);
  });
}

template <typename T>
shared_ptr<T> Root::loadMemberFunction(shared_ptr<T>& ptr, Mutex& mutex, char const* name, function<shared_ptr<T>()> loadFunction) {
  MutexLocker locker(mutex);
  if (!ptr) {
    ZoneScopedN("Root::loadMemberFunction");
#ifdef TRACY_ENABLE
    ZoneTextF("Loading %s", name);
#endif
    auto startSeconds = Time::monotonicTime();
    ptr = loadFunction();
    Logger::info("Root: Loaded {} in {} seconds", name, Time::monotonicTime() - startSeconds);
  }
  return ptr;
}

} // namespace Star
