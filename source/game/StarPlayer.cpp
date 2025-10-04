#include "StarPlayer.hpp"
#include "StarAiDatabase.hpp"
#include "StarArmors.hpp"
#include "StarAssets.hpp"
#include "StarCelestialLuaBindings.hpp"
#include "StarClientContext.hpp"
#include "StarConfiguration.hpp"
#include "StarDamageManager.hpp"
#include "StarEmoteProcessor.hpp"
#include "StarEncode.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarEntitySplash.hpp"
#include "StarImageProcessing.hpp"
#include "StarInspectionTool.hpp"
#include "StarInteractiveEntity.hpp"
#include "StarItemBag.hpp"
#include "StarItemDatabase.hpp"
#include "StarItemDrop.hpp"
#include "StarJsonExtra.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarPlayerBlueprints.hpp"
#include "StarPlayerCodexes.hpp"
#include "StarPlayerCompanions.hpp"
#include "StarPlayerDeployment.hpp"
#include "StarPlayerFactory.hpp"
#include "StarPlayerInventory.hpp"
#include "StarPlayerLog.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarPlayerTech.hpp"
#include "StarPlayerUniverseMap.hpp"
#include "StarQuestManager.hpp"
#include "StarRoot.hpp"
#include "StarSongbook.hpp"
#include "StarSpeciesDatabase.hpp"
#include "StarStatistics.hpp"
#include "StarStatusController.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarTeamClient.hpp"
#include "StarTechController.hpp"
#include "StarTools.hpp"
#include "StarUniverseClient.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarWorld.hpp"
#include "StarWorldGeometry.hpp"

#if defined TRACY_ENABLE
#include "tracy/Tracy.hpp"
#else
#define ZoneScoped
#endif

namespace Star {

EnumMap<Player::State> const Player::StateNames{
    {Player::State::Idle, "idle"},
    {Player::State::Walk, "walk"},
    {Player::State::Run, "run"},
    {Player::State::Jump, "jump"},
    {Player::State::Fall, "fall"},
    {Player::State::Swim, "swim"},
    {Player::State::SwimIdle, "swimIdle"},
    {Player::State::TeleportIn, "teleportIn"},
    {Player::State::TeleportOut, "teleportOut"},
    {Player::State::Crouch, "crouch"},
    {Player::State::Lounge, "lounge"}};

atomic<bool> Player::s_headRotation = false;

Player::Player(PlayerConfigPtr config, Uuid uuid) {
  auto assets = Root::singleton().assets();
  // FezzedOne: Pre-cache the player config to prevent various lag spikes.
  volatile Json _ = assets->json("/player.config", true);
  (void)_;

  m_config = config;
  m_client = nullptr;

  m_state = State::Idle;
  m_overrideState = {};
  m_emoteState = HumanoidEmote::Idle;

  m_footstepTimer = 0.0f;
  m_teleportTimer = 0.0f;
  m_teleportAnimationType = "default";

  m_shifting = false;

  m_aimPosition = Vec2F();

  setUniqueId(uuid.hex());
  m_identity = m_config->defaultIdentity;
  m_identityUpdated = true;

  m_chatText = "";
  m_chatOpen = false;

  m_cameraOverridePosition = {};

  m_questManager = make_shared<QuestManager>(this);
  m_tools = make_shared<ToolUser>();
  m_armor = make_shared<ArmorWearer>(this);
  m_companions = make_shared<PlayerCompanions>(config->companionsConfig);

  for (auto& p : config->genericScriptContexts) {
    auto scriptComponent = make_shared<GenericScriptComponent>();
    scriptComponent->setScript(p.second);
    m_genericScriptContexts.set(p.first, scriptComponent);
  }

  // all of these are defaults and won't include the correct humanoid config for the species
  m_humanoid = make_shared<Humanoid>(Root::singleton().speciesDatabase()->species(m_identity.species)->humanoidConfig());
  m_humanoid->setIdentity(m_identity);
  auto movementParameters = ActorMovementParameters(jsonMerge(m_humanoid->defaultMovementParameters(), m_config->movementParameters));
  if (!movementParameters.physicsEffectCategories)
    movementParameters.physicsEffectCategories = StringSet({"player"});
  m_movementController = make_shared<ActorMovementController>(movementParameters);
  m_zeroGMovementParameters = ActorMovementParameters(m_config->zeroGMovementParameters);

  m_techController = make_shared<TechController>();
  m_statusController = make_shared<StatusController>(m_config->statusControllerSettings);
  m_deployment = make_shared<PlayerDeployment>(m_config->deploymentConfig);

  m_inventory = make_shared<PlayerInventory>(this);
  m_blueprints = make_shared<PlayerBlueprints>();
  m_universeMap = make_shared<PlayerUniverseMap>();
  m_codexes = make_shared<PlayerCodexes>();
  m_techs = make_shared<PlayerTech>();
  m_log = make_shared<PlayerLog>();

  setModeType(PlayerMode::Casual);

  m_useDown = false;
  m_edgeTriggeredUse = false;
  setTeam(EntityDamageTeam(TeamType::Friendly));

  m_footstepVolumeVariance = assets->json("/sfx.config:footstepVolumeVariance").toFloat();
  m_landingVolume = assets->json("/sfx.config:landingVolume").toFloat();

  m_effectsAnimator = make_shared<NetworkedAnimator>(assets->fetchJson(m_config->effectsAnimator));
  m_effectEmitter = make_shared<EffectEmitter>();

  m_interactRadius = assets->json("/player.config:interactRadius").toFloat();

  m_walkIntoInteractBias = jsonToVec2F(assets->json("/player.config:walkIntoInteractBias"));

  if (m_landingVolume <= 1)
    m_landingVolume = 6;

  m_isAdmin = false;

  m_emoteCooldown = assets->json("/player.config:emoteCooldown").toFloat();
  m_blinkInterval = jsonToVec2F(assets->json("/player.config:blinkInterval"));

  m_emoteCooldownTimer = 0;
  m_blinkCooldownTimer = 0;

  m_chatMessageChanged = false;
  m_chatMessageUpdated = false;

  m_overrideMenuIndicator = false;
  m_overrideChatIndicator = false;

  m_songbook = make_shared<Songbook>(species());

  m_lastDamagedOtherTimer = 0;
  m_lastDamagedTarget = NullEntityId;

  m_ageItemsTimer = GameTimer(assets->json("/player.config:ageItemsEvery").toFloat());

  refreshEquipment();

  m_foodLowThreshold = assets->json("/player.config:foodLowThreshold").toFloat();
  m_foodLowStatusEffects = assets->json("/player.config:foodLowStatusEffects").toArray().transformed(jsonToPersistentStatusEffect);
  m_foodEmptyStatusEffects = assets->json("/player.config:foodEmptyStatusEffects").toArray().transformed(jsonToPersistentStatusEffect);

  m_inCinematicStatusEffects = assets->json("/player.config:inCinematicStatusEffects").toArray().transformed(jsonToPersistentStatusEffect);

  m_statusController->setPersistentEffects("armor", m_armor->statusEffects());
  m_statusController->setPersistentEffects("tools", m_tools->statusEffects());
  m_statusController->resetAllResources();

  m_landingNoisePending = false;
  m_footstepPending = false;

  setKeepAlive(true);

  m_netGroup.addNetElement(&m_stateNetState);
  m_netGroup.addNetElement(&m_shiftingNetState);
  m_netGroup.addNetElement(&m_xAimPositionNetState);
  m_netGroup.addNetElement(&m_yAimPositionNetState);
  m_netGroup.addNetElement(&m_identityNetState);
  m_netGroup.addNetElement(&m_teamNetState);
  m_netGroup.addNetElement(&m_landedNetState);
  m_netGroup.addNetElement(&m_chatMessageNetState);
  m_netGroup.addNetElement(&m_newChatMessageNetState);
  m_netGroup.addNetElement(&m_emoteNetState);

  m_xAimPositionNetState.setFixedPointBase(0.003125);
  m_yAimPositionNetState.setFixedPointBase(0.003125);
  m_yAimPositionNetState.setInterpolator(lerp<float, float>);

  m_netGroup.addNetElement(m_inventory.get());
  m_netGroup.addNetElement(m_tools.get());
  m_netGroup.addNetElement(m_armor.get());
  m_netGroup.addNetElement(m_songbook.get());
  m_netGroup.addNetElement(m_movementController.get());
  m_netGroup.addNetElement(m_effectEmitter.get());
  m_netGroup.addNetElement(m_effectsAnimator.get());
  m_netGroup.addNetElement(m_statusController.get());
  m_netGroup.addNetElement(m_techController.get());

  // FezzedOne: Initialise per-player xClient settings.
  m_ignoreExternalWarps = false;
  m_ignoreExternalRadioMessages = false;
  m_ignoreExternalCinematics = false;
  m_ignoreAllPhysicsEntities = false;
  m_ignoreAllTechOverrides = false;
  m_ignoreForcedNudity = false;
  m_ignoreShipUpdates = false;
  m_alwaysRespawnInWorld = false;
  m_fastRespawn = false;
  m_ignoreItemPickups = false;
  m_canReachAll = false;
  m_damageTeamOverridden = false;
  m_chatBubbleConfig = JsonObject{};

  // FezzedOne: For a `player` callback that suppresses tool usage.
  m_toolUsageSuppressed = false;

  // FezzedOne: Variable to make sure the inventory overflow check doesn't run more than once in `update`.
  m_overflowCheckDone = false;

  m_startedNetworkingCosmetics = false;
  m_pulledCosmeticUpdate = false;
  m_armorSecretNetVersions = Array<uint64_t, 12>::filled(0);

  m_netGroup.setNeedsLoadCallback(bind(&Player::getNetStates, this, _1));
  m_netGroup.setNeedsStoreCallback(bind(&Player::setNetStates, this));
}

Player::Player(PlayerConfigPtr config, ByteArray const& netStore) : Player(config) {
  DataStreamBuffer ds(netStore);

  setUniqueId(ds.read<String>());

  ds.read(m_description);
  ds.read(m_modeType);
  ds.read(m_identity);

  m_humanoid = make_shared<Humanoid>(Root::singleton().speciesDatabase()->species(m_identity.species)->humanoidConfig());
  m_humanoid->setIdentity(m_identity);
  m_movementController->resetBaseParameters(ActorMovementParameters(jsonMerge(m_humanoid->defaultMovementParameters(), m_config->movementParameters)));
}


Player::Player(PlayerConfigPtr config, Json const& diskStore) : Player(config) {
  diskLoad(diskStore);
}

void Player::diskLoad(Json const& diskStore) {
  setUniqueId(diskStore.getString("uuid"));
  m_description = diskStore.getString("description");
  setModeType(PlayerModeNames.getLeft(diskStore.getString("modeType")));
  m_shipUpgrades = ShipUpgrades(diskStore.get("shipUpgrades"));
  m_blueprints = make_shared<PlayerBlueprints>(diskStore.get("blueprints"));
  m_universeMap = make_shared<PlayerUniverseMap>(diskStore.get("universeMap"));
  if (m_clientContext)
    m_universeMap->setServerUuid(m_clientContext->serverUuid());

  m_codexes = make_shared<PlayerCodexes>(diskStore.get("codexes"));
  m_techs = make_shared<PlayerTech>(diskStore.get("techs"));
  m_identity = HumanoidIdentity(diskStore.get("identity"));
  m_identityUpdated = true;

  setTeam(EntityDamageTeam(diskStore.get("team")));

  m_state = State::Idle;

  m_inventory->load(diskStore.get("inventory"));

  m_movementController->loadState(diskStore.get("movementController"));
  m_techController->diskLoad(diskStore.get("techController"));
  m_statusController->diskLoad(diskStore.get("statusController"));

  m_log = make_shared<PlayerLog>(diskStore.get("log"));

  auto speciesDef = Root::singleton().speciesDatabase()->species(m_identity.species);

  m_questManager->diskLoad(diskStore.get("quests", JsonObject{}));
  m_companions->diskLoad(diskStore.get("companions", JsonObject{}));
  m_deployment->diskLoad(diskStore.get("deployment", JsonObject{}));
  m_humanoid = make_shared<Humanoid>(speciesDef->humanoidConfig());
  m_humanoid->setIdentity(m_identity);
  m_movementController->resetBaseParameters(ActorMovementParameters(jsonMerge(m_humanoid->defaultMovementParameters(), m_config->movementParameters)));
  m_effectsAnimator->setGlobalTag("effectDirectives", speciesDef->effectDirectives());

  m_genericProperties = diskStore.getObject("genericProperties");

  /* FezzedOne: Get the player's `"xSbProperties"`, or create one if it doesn't exist. */
  Maybe<JsonObject> xSbProperties = diskStore.optObject("xSbProperties");
  if (xSbProperties) {
    Json xSbPropertyObject = *xSbProperties;
    m_ignoreExternalWarps = xSbPropertyObject.getBool("ignoreExternalWarps", false);
    m_ignoreExternalRadioMessages = xSbPropertyObject.getBool("ignoreExternalRadioMessages", false);
    m_ignoreExternalCinematics = xSbPropertyObject.getBool("ignoreExternalCinematics", false);
    m_ignoreAllPhysicsEntities = xSbPropertyObject.getBool("ignoreAllPhysicsEntities", false);
    m_ignoreAllTechOverrides = xSbPropertyObject.getBool("ignoreTechOverrides", false);
    m_ignoreForcedNudity = xSbPropertyObject.getBool("ignoreNudity", false);
    m_ignoreShipUpdates = xSbPropertyObject.getBool("ignoreShipUpdates", false);
    m_alwaysRespawnInWorld = xSbPropertyObject.getBool("inWorldRespawn", false);
    m_fastRespawn = xSbPropertyObject.getBool("fastWarp", false);
    m_ignoreItemPickups = xSbPropertyObject.getBool("ignoreItemPickups", false);
    m_chatBubbleConfig = xSbPropertyObject.get("chatConfig", JsonObject{});
    m_canReachAll = xSbPropertyObject.getBool("overreach", false);
    m_damageTeamOverridden = xSbPropertyObject.getBool("damageTeamOverridden", false);
  } else {
    /* Support for reading legacy xSB properties. */
    m_ignoreExternalWarps = diskStore.getBool("ignoreExternalWarps", false);
    m_ignoreExternalRadioMessages = diskStore.getBool("ignoreExternalRadioMessages", false);
    m_ignoreExternalCinematics = diskStore.getBool("ignoreExternalCinematics", false);
    m_ignoreAllPhysicsEntities = diskStore.getBool("ignoreAllPhysicsEntities", false);
    m_ignoreAllTechOverrides = diskStore.getBool("ignoreTechOverrides", false);
    m_ignoreForcedNudity = diskStore.getBool("ignoreNudity", false);
    m_ignoreShipUpdates = diskStore.getBool("ignoreShipUpdates", false);
    m_alwaysRespawnInWorld = diskStore.getBool("inWorldRespawn", false);
    m_fastRespawn = diskStore.getBool("fastWarp", false);
    m_ignoreItemPickups = diskStore.getBool("ignoreItemPickups", false);
    m_chatBubbleConfig = diskStore.get("chatConfig", JsonObject{});
    m_canReachAll = diskStore.getBool("overreach", false);
    m_damageTeamOverridden = diskStore.getBool("damageTeamOverridden", false);
  }

  refreshArmor();

  m_codexes->learnInitialCodexes(species());

  m_aiState = AiState(diskStore.get("aiState", JsonObject{}));

  for (auto& script : m_genericScriptContexts)
    script.second->setScriptStorage({});

  for (auto& p : diskStore.get("genericScriptStorage", JsonObject{}).toObject()) {
    if (auto script = m_genericScriptContexts.maybe(p.first).value({})) {
      script->setScriptStorage(p.second.toObject());
    }
  }

  // Make sure to merge the stored player blueprints with what a new player
  // would get as default.
  for (auto const& descriptor : m_config->defaultBlueprints)
    m_blueprints->add(descriptor);
  for (auto const& descriptor : speciesDef->defaultBlueprints())
    m_blueprints->add(descriptor);
}

ClientContextPtr Player::clientContext() const {
  return m_clientContext;
}

void Player::setClientContext(ClientContextPtr clientContext) {
  m_clientContext = std::move(clientContext);
  if (m_clientContext)
    m_universeMap->setServerUuid(m_clientContext->serverUuid());
}

StatisticsPtr Player::statistics() const {
  return m_statistics;
}

void Player::setStatistics(StatisticsPtr statistics) {
  m_statistics = statistics;
}

void Player::setUniverseClient(UniverseClient* client) {
  m_client = client;
  m_questManager->setUniverseClient(client);
}

EntityType Player::entityType() const {
  return EntityType::Player;
}

ClientEntityMode Player::clientEntityMode() const {
  return ClientEntityMode::ClientPresenceMaster;
}

void Player::init(World* world, EntityId entityId, EntityMode mode) {
  Entity::init(world, entityId, mode);

  m_toolUsageSuppressed = false;

  m_overrideMenuIndicator = false;
  m_overrideChatIndicator = false;
  m_overrideState = {};

  m_tools->init(this);
  m_movementController->init(world);
  m_movementController->tickIgnorePhysicsEntities(m_ignoreAllPhysicsEntities);
  m_movementController->setIgnorePhysicsEntities({entityId});
  m_statusController->init(this, m_movementController.get());
  m_techController->init(this, m_movementController.get(), m_statusController.get());

  if (mode == EntityMode::Master) {
    auto speciesDefinition = Root::singleton().speciesDatabase()->species(m_identity.species);
    m_techController->setPlayerToolUsageSuppressed(false);
    m_movementController->setRotation(0);
    m_statusController->setStatusProperty("ouchNoise", speciesDefinition->ouchNoise(m_identity.gender));
    m_emoteState = HumanoidEmote::Idle;
    m_questManager->init(world);
    m_companions->init(this, world);
    m_deployment->init(this, world);
    m_missionRadioMessages.clear();

    m_statusController->setPersistentEffects("species", speciesDefinition->statusEffects());

    for (auto& p : m_genericScriptContexts) {
      p.second->addActorMovementCallbacks(m_movementController.get());
      // FezzedOne: Added missing `entity` callbacks.
      p.second->addCallbacks("entity", LuaBindings::makeEntityCallbacks(as<Entity>(this)));
      p.second->addCallbacks("player", LuaBindings::makePlayerCallbacks(this));
      p.second->addCallbacks("playerAnimator", LuaBindings::makeNetworkedAnimatorCallbacks(m_effectsAnimator.get()));
      p.second->addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_statusController.get()));
      if (m_client)
        p.second->addCallbacks("celestial", LuaBindings::makeCelestialCallbacks(m_client));
      p.second->init(world);
    }
  }

  m_xAimPositionNetState.setInterpolator(world->geometry().xLerpFunction());
  refreshEquipment();
  // Force a cosmetics sync to make sure humanoid overrides get applied on initialisation.
  m_armor->setupHumanoidClothingDrawables(*m_humanoid, forceNude(), true);
}

void Player::uninit() {
  m_techController->uninit();
  m_movementController->uninit();
  m_tools->uninit();
  m_statusController->uninit();

  if (isMaster()) {
    m_questManager->uninit();
    m_companions->uninit();
    m_deployment->uninit();

    for (auto& p : m_genericScriptContexts) {
      p.second->uninit();
      p.second->removeCallbacks("entity");
      p.second->removeCallbacks("player");
      p.second->removeCallbacks("playerAnimator");
      p.second->removeCallbacks("mcontroller");
      p.second->removeCallbacks("status");
      p.second->removeCallbacks("world");
      if (m_client)
        p.second->removeCallbacks("celestial");
    }
  }

  Entity::uninit();
}

List<Drawable> Player::drawables() const {
  List<Drawable> drawables;

  Maybe<float> playerAimAngle = {}; // FezzedOne: Angle used to rotate the head.
  if (inWorld()) {                  // FezzedOne: Only clients need to run this head rotation code.
    if (world()->isClient() && s_headRotation)
      playerAimAngle = getAngleSide(world()->geometry().diff(aimPosition(), position()).angle()).first;
  }

  if (!isTeleporting()) {
    drawables.appendAll(m_techController->backDrawables());
    if (!m_techController->parentHidden()) {
      m_tools->setupHumanoidHandItemDrawables(*m_humanoid);

      // FezzedOne: Detect any `?scalenearest` drawables without a `skip` parameter and handle them separately.
      DirectivesGroup humanoidDirectives;
      Vec2F humanoidScale = Vec2F::filled(1.0f);

      auto extractScaleDirectives = [&](Directives const& directives) -> pair<Vec2F, Directives> {
        Vec2F finalScale = Vec2F::filled(1.0f);

        if (!directives)
          return make_pair(finalScale, Directives());

        for (auto& entry : directives.shared->entries) {
          ScaleImageOperation* op = const_cast<ScaleImageOperation*>(entry.loadOperation(*directives.shared).ptr<ScaleImageOperation>());
          if (op) {
            if (op->mode == ScaleImageOperation::Nearest) /* Not `NearestPixel`. */ {
              finalScale = finalScale.piecewiseMultiply(op->rawScale);
              op->scale = Vec2F::filled(1.0f);
            }
            entry.operation = *op;
          }
        }

        return make_pair(finalScale, directives);
      };

      auto extractScaleFromDirectives = [&](List<Directives> const& directivesList) {
        for (auto& directives : directivesList) {
          auto result = extractScaleDirectives(directives);
          humanoidScale = humanoidScale.piecewiseMultiply(result.first);
          humanoidDirectives.append(result.second);
        }
      };

      extractScaleFromDirectives(m_techController->parentDirectives().list());
      extractScaleFromDirectives(m_statusController->parentDirectives().list());

      for (auto& drawable : m_humanoid->render(true, true, playerAimAngle)) {
        drawable.scale(humanoidScale);
        drawable.translate(position() + m_techController->parentOffset());
        if (drawable.isImage()) {
          drawable.imagePart().addDirectivesGroup(humanoidDirectives, true);
          // drawable.imagePart().addDirectivesGroup(m_techController->parentDirectives(), true);
          // drawable.imagePart().addDirectivesGroup(m_statusController->parentDirectives(), true);

          if (auto anchor = as<LoungeAnchor>(m_movementController->entityAnchor())) {
            if (auto& directives = anchor->directives)
              drawable.imagePart().addDirectives(*directives, true);
          }
        }
        drawables.append(std::move(drawable));
      }
    }
    drawables.appendAll(m_techController->frontDrawables());

    drawables.appendAll(m_statusController->drawables());

    drawables.appendAll(m_tools->renderObjectPreviews(aimPosition(), walkingDirection(), inToolRange(), favoriteColor()));
  }

  drawables.appendAll(m_effectsAnimator->drawables(position()));

  return drawables;
}

List<OverheadBar> Player::bars() const {
  return m_statusController->overheadBars();
}

List<Particle> Player::particles() {
  List<Particle> particles;
  particles.appendAll(m_config->splashConfig.doSplash(position(), m_movementController->velocity(), world()));
  particles.appendAll(take(m_callbackParticles));
  particles.appendAll(m_techController->pullNewParticles());
  particles.appendAll(m_statusController->pullNewParticles());

  return particles;
}

void Player::addParticles(List<Particle> const& particles) {
  m_callbackParticles.appendAll(particles);
}

void Player::addSound(String const& sound, float volume, float pitch) {
  m_callbackSounds.emplaceAppend(sound, volume, pitch);
}

void Player::addEphemeralStatusEffects(List<EphemeralStatusEffect> const& statusEffects) {
  if (isSlave())
    throw PlayerException("Adding status effects to an entity can only be done directly on the master entity.");
  m_statusController->addEphemeralEffects(statusEffects);
}

ActiveUniqueStatusEffectSummary Player::activeUniqueStatusEffectSummary() const {
  return m_statusController->activeUniqueStatusEffectSummary();
}

float Player::powerMultiplier() const {
  return m_statusController->stat("powerMultiplier");
}

bool Player::isDead() const {
  return !m_statusController->resourcePositive("health");
}

void Player::kill() {
  m_statusController->setResource("health", 0);
}

bool Player::wireToolInUse() const {
  return (bool)as<WireTool>(m_tools->primaryHandItem());
}

void Player::setWireConnector(WireConnector* wireConnector) const {
  if (auto wireTool = as<WireTool>(m_tools->primaryHandItem()))
    wireTool->setConnector(wireConnector);
}

List<Drawable> Player::portrait(PortraitMode mode) const {
  if (isPermaDead())
    return m_humanoid->renderSkull();
  if (invisible())
    return {};
  m_armor->setupHumanoidClothingDrawables(*m_humanoid, forceNude());
  return m_humanoid->renderPortrait(mode);
}

List<Drawable> Player::renderHumanoid(bool withItems, bool withRotation) {
  m_armor->setupHumanoidClothingDrawables(*m_humanoid, forceNude());
  return m_humanoid->render(withItems, withRotation);
}

bool Player::underwater() const {
  if (!inWorld())
    return false;
  else
    return world()->liquidLevel(Vec2I(position() + m_config->underwaterSensor)).level >= m_config->underwaterMinWaterLevel;
}

List<LightSource> Player::lightSources() const {
  List<LightSource> lights;
  lights.appendAll(m_tools->lightSources());
  lights.appendAll(m_statusController->lightSources());
  lights.appendAll(m_techController->lightSources());
  return lights;
}

RectF Player::metaBoundBox() const {
  return m_config->metaBoundBox;
}

Maybe<HitType> Player::queryHit(DamageSource const& source) const {
  if (!inWorld() || isDead() || m_isAdmin || isTeleporting() || m_statusController->statPositive("invulnerable"))
    return {};

  if (m_tools->queryShieldHit(source))
    return HitType::ShieldHit;

  if (source.intersectsWithPoly(world()->geometry(), m_movementController->collisionBody()))
    return HitType::Hit;

  return {};
}

Maybe<PolyF> Player::hitPoly() const {
  return m_movementController->collisionBody();
}

List<DamageNotification> Player::applyDamage(DamageRequest const& request) {
  if (!inWorld() || isDead() || m_isAdmin)
    return {};

  return m_statusController->applyDamageRequest(request);
}

List<DamageNotification> Player::selfDamageNotifications() {
  return m_statusController->pullSelfDamageNotifications();
}

void Player::hitOther(EntityId targetEntityId, DamageRequest const& damageRequest) {
  if (!isMaster())
    return;

  m_statusController->hitOther(targetEntityId, damageRequest);
  if (as<DamageBarEntity>(world()->entity(targetEntityId))) {
    m_lastDamagedOtherTimer = 0;
    m_lastDamagedTarget = targetEntityId;
  }
}

void Player::damagedOther(DamageNotification const& damage) {
  if (!isMaster())
    return;

  m_statusController->damagedOther(damage);
}

List<DamageSource> Player::damageSources() const {
  return m_damageSources;
}

bool Player::shouldDestroy() const {
  return isDead();
}

void Player::destroy(RenderCallback* renderCallback) {
  m_state = State::Idle;
  m_emoteState = HumanoidEmote::Idle;
  if (renderCallback) {
    List<Particle> deathParticles = m_humanoid->particles(m_humanoid->defaultDeathParticles());
    renderCallback->addParticles(deathParticles, position());
  }

  if (isMaster()) {
    m_log->addDeathCount(1);

    if (!world()->disableDeathDrops()) {
      if (auto dropString = modeConfig().deathDropItemTypes.maybeLeft()) {
        if (*dropString == "all")
          dropEverything();
      } else {
        List<ItemType> dropList = modeConfig().deathDropItemTypes.right().transformed([](String typeName) {
          return ItemTypeNames.getLeft(typeName);
        });
        Set<ItemType> dropSet = Set<ItemType>::from(dropList);
        auto itemDb = Root::singleton().itemDatabase();
        dropSelectedItems([dropSet, itemDb](ItemPtr item) {
          return dropSet.contains(itemDb->itemType(item->name()));
        });
      }
    }
  }

  m_songbook->stop();
}

Maybe<EntityAnchorState> Player::loungingIn() const {
  if (is<LoungeAnchor>(m_movementController->entityAnchor()))
    return m_movementController->anchorState();
  return {};
}

bool Player::lounge(EntityId loungeableEntityId, size_t anchorIndex) {
  if (!canUseTool())
    return false;

  auto loungeableEntity = world()->get<LoungeableEntity>(loungeableEntityId);
  if (!loungeableEntity || anchorIndex >= loungeableEntity->anchorCount() || !loungeableEntity->entitiesLoungingIn(anchorIndex).empty() || !loungeableEntity->loungeAnchor(anchorIndex))
    return false;

  m_state = State::Lounge;
  m_movementController->setAnchorState({loungeableEntityId, anchorIndex});
  return true;
}

void Player::stopLounging() {
  if (loungingIn()) {
    m_movementController->resetAnchorState();
    m_state = State::Idle;
    m_statusController->setPersistentEffects("lounging", {});
  }
}

Vec2F Player::position() const {
  return m_movementController->position();
}

Vec2F Player::velocity() const {
  return m_movementController->velocity();
}

Vec2F Player::mouthOffset(bool ignoreAdjustments) const {
  return Vec2F(
      m_humanoid->mouthOffset(ignoreAdjustments)[0] * numericalDirection(facingDirection()), m_humanoid->mouthOffset(ignoreAdjustments)[1]);
}

Vec2F Player::feetOffset() const {
  return Vec2F(m_humanoid->feetOffset()[0] * numericalDirection(facingDirection()), m_humanoid->feetOffset()[1]);
}

Vec2F Player::headArmorOffset() const {
  return Vec2F(
      m_humanoid->headArmorOffset()[0] * numericalDirection(facingDirection()), m_humanoid->headArmorOffset()[1]);
}

Vec2F Player::chestArmorOffset() const {
  return Vec2F(
      m_humanoid->chestArmorOffset()[0] * numericalDirection(facingDirection()), m_humanoid->chestArmorOffset()[1]);
}

Vec2F Player::backArmorOffset() const {
  return Vec2F(
      m_humanoid->backArmorOffset()[0] * numericalDirection(facingDirection()), m_humanoid->backArmorOffset()[1]);
}

Vec2F Player::legsArmorOffset() const {
  return Vec2F(
      m_humanoid->legsArmorOffset()[0] * numericalDirection(facingDirection()), m_humanoid->legsArmorOffset()[1]);
}

Vec2F Player::mouthPosition() const {
  return position() + mouthOffset(true);
}

Vec2F Player::mouthPosition(bool ignoreAdjustments) const {
  return position() + mouthOffset(ignoreAdjustments);
}

RectF Player::collisionArea() const {
  return m_movementController->collisionPoly().boundBox();
}

void Player::revive(Vec2F const& footPosition) {
  if (!isDead())
    return;

  m_state = State::Idle;
  m_emoteState = HumanoidEmote::Idle;

  m_statusController->setPersistentEffects("armor", m_armor->statusEffects());
  m_statusController->setPersistentEffects("tools", m_tools->statusEffects());
  m_statusController->resetAllResources();

  m_statusController->clearEphemeralEffects();

  endPrimaryFire();
  endAltFire();
  endTrigger();

  m_effectEmitter->reset();
  m_movementController->setPosition(footPosition - feetOffset());
  m_movementController->setVelocity(Vec2F());

  m_techController->reloadTech();

  if (!(m_alwaysRespawnInWorld || m_fastRespawn)) {
    float moneyCost = m_inventory->currency("money") * modeConfig().reviveCostPercentile;
    m_inventory->consumeCurrency("money", min((uint64_t)round(moneyCost), m_inventory->currency("money")));
  }
}

void Player::setShifting(bool shifting) {
  m_shifting = shifting;
}

void Player::special(int specialKey) {
  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
  if (loungeAnchor && loungeAnchor->controllable) {
    auto anchorState = m_movementController->anchorState();
    if (auto loungeableEntity = world()->get<LoungeableEntity>(anchorState->entityId)) {
      if (specialKey == 1)
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Special1);
      else if (specialKey == 2)
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Special2);
      else if (specialKey == 3)
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Special3);
      return;
    }
  }
  m_techController->special(specialKey);
}

void Player::setMoveVector(Vec2F const& vec) {
  m_moveVector = vec;
}

void Player::moveLeft() {
  m_pendingMoves.add(MoveControlType::Left);
}

void Player::moveRight() {
  m_pendingMoves.add(MoveControlType::Right);
}

void Player::moveUp() {
  m_pendingMoves.add(MoveControlType::Up);
}

void Player::moveDown() {
  m_pendingMoves.add(MoveControlType::Down);
}

void Player::jump() {
  m_pendingMoves.add(MoveControlType::Jump);
}

void Player::dropItem() {
  if (!world())
    return;
  if (!canUseTool())
    return;

  Vec2F throwDirection = world()->geometry().diff(aimPosition(), position());
  for (auto& throwSlot : {m_inventory->primaryHeldSlot(), m_inventory->secondaryHeldSlot()}) {
    if (throwSlot) {
      if (auto drop = m_inventory->takeSlot(*throwSlot)) {
        world()->addEntity(ItemDrop::throwDrop(drop, position(), velocity(), throwDirection));
        break;
      }
    }
  }
}

Maybe<Json> Player::receiveMessage(ConnectionId fromConnection, String const& message, JsonArray const& args) {
  bool localMessage = fromConnection == world()->connection();
  if (message == "queueRadioMessage" && args.size() > 0) {
    if (localMessage || !m_ignoreExternalRadioMessages) {
      float delay = 0;
      if (args.size() > 1 && args.get(1).canConvert(Json::Type::Float))
        delay = args.get(1).toFloat();

      queueRadioMessage(args.get(0), delay);
    } else {
      try {
        Json jsonArgs = Json(args);
        Logger::info("[xSB] Attempted 'queueRadioMessage' message from cID {}. Message arguments: {}", fromConnection, jsonArgs.printJson(0));
      } catch (JsonException const& e) {
        Logger::info("[xSB] Attempted 'queueRadioMessage' message from cID {}. Invalid message arguments.", fromConnection);
      }
    }
  } else if (message == "warp") {
    if (localMessage || !m_ignoreExternalWarps) {
      Maybe<String> animation;
      if (args.size() > 1)
        animation = args.get(1).toString();

      bool deploy = false;
      if (args.size() > 2)
        deploy = args.get(2).toBool();

      setPendingWarp(args.get(0).toString(), animation, deploy);
    } else {
      try {
        Json jsonArgs = Json(args);
        Logger::info("[xSB] Attempted 'warp' message from cID {}. Message arguments: {}", fromConnection, jsonArgs.printJson(0));
      } catch (JsonException const& e) {
        Logger::info("[xSB] Attempted 'warp' message from cID {}. Invalid message arguments.", fromConnection);
      }
    }
  } else if (message == "interruptRadioMessage") {
    if (localMessage || !m_ignoreExternalRadioMessages) {
      m_interruptRadioMessage = true;
    } else {
      try {
        Json jsonArgs = Json(args);
        Logger::info("[xSB] Attempted 'interruptRadioMessage' message from cID {}. Message arguments: {}", fromConnection, jsonArgs.printJson(0));
      } catch (JsonException const& e) {
        Logger::info("[xSB] Attempted 'interruptRadioMessage' message from cID {}. Invalid message arguments.", fromConnection);
      }
    }
  } else if (message == "playCinematic" && args.size() > 0) {
    if (localMessage || !m_ignoreExternalCinematics) {
      bool unique = false;
      if (args.size() > 1)
        unique = args.get(1).toBool();
      setPendingCinematic(args.get(0), unique);
    } else {
      try {
        Json jsonArgs = Json(args);
        Logger::info("[xSB] Attempted 'playCinematic' message from cID {}. Message arguments: {}", fromConnection, jsonArgs.printJson(0));
      } catch (JsonException const& e) {
        Logger::info("[xSB] Attempted 'playCinematic' message from cID {}. Invalid message arguments.", fromConnection);
      }
    }
  } else if (message == "playAltMusic" && args.size() > 0) {
    if (localMessage || !m_ignoreExternalCinematics) {
      float fadeTime = 0;
      if (args.size() > 1)
        fadeTime = args.get(1).toFloat();
      StringList trackList;
      if (args.get(0).canConvert(Json::Type::Array))
        trackList = jsonToStringList(args.get(0).toArray());
      else
        trackList = StringList();
      m_pendingAltMusic = pair<Maybe<StringList>, float>(trackList, fadeTime);
    } else {
      try {
        Json jsonArgs = Json(args);
        Logger::info("[xSB] Attempted 'playAltMusic' message from cID {}. Message arguments: {}", fromConnection, jsonArgs.printJson(0));
      } catch (JsonException const& e) {
        Logger::info("[xSB] Attempted 'playAltMusic' message from cID {}. Invalid message arguments.", fromConnection);
      }
    }
  } else if (message == "stopAltMusic") {
    if (localMessage || !m_ignoreExternalCinematics) {
      float fadeTime = 0;
      if (args.size() > 0)
        fadeTime = args.get(0).toFloat();
      m_pendingAltMusic = pair<Maybe<StringList>, float>({}, fadeTime);
    } else {
      try {
        Json jsonArgs = Json(args);
        Logger::info("[xSB] Attempted 'stopAltMusic' message from cID {}. Message arguments: {}", fromConnection, jsonArgs.printJson(0));
      } catch (JsonException const& e) {
        Logger::info("[xSB] Attempted 'stopAltMusic' message from cID {}. Invalid message arguments.", fromConnection);
      }
    }
  } else if (message == "recordEvent") {
    if (localMessage || !m_ignoreExternalRadioMessages) {
      statistics()->recordEvent(args.at(0).toString(), args.at(1));
    } else {
      try {
        Json jsonArgs = Json(args);
        Logger::info("[xSB] Attempted 'recordEvent' message from cID {}. Message arguments: {}", fromConnection, jsonArgs.printJson(0));
      } catch (JsonException const& e) {
        Logger::info("[xSB] Attempted 'recordEvent' message from cID {}. Invalid message arguments.", fromConnection);
      }
    }
  } else if (message == "addCollectable") {
    if (localMessage || !m_ignoreExternalRadioMessages) {
      auto collection = args.get(0).toString();
      auto collectable = args.get(1).toString();
      if (Root::singleton().collectionDatabase()->hasCollectable(collection, collectable))
        addCollectable(collection, collectable);
    } else {
      try {
        Json jsonArgs = Json(args);
        Logger::info("[xSB] Attempted 'addCollectable' message from cID {}. Message arguments: {}", fromConnection, jsonArgs.printJson(0));
      } catch (JsonException const& e) {
        Logger::info("[xSB] Attempted 'addCollectable' message from cID {}. Invalid message arguments.", fromConnection);
      }
    }
  } else {
    if (localMessage || !m_ignoreExternalRadioMessages) {
      // FezzedOne: If non-local "radio" messages are ignored, prevent receiving *all* non-local messages.
      bool isChatMessage = message == "chatMessage" || message == "newChatMessage";
      if (isChatMessage) {
        // FezzedOne: Allow multiple chat message handlers to be called at once. No need for SE's hack.
        JsonArray results = JsonArray{};
        results.append(m_tools->receiveMessage(message, localMessage, args).value(Json()));
        results.append(m_statusController->receiveMessage(message, localMessage, args).value(Json()));
        results.append(m_companions->receiveMessage(message, localMessage, args).value(Json()));
        results.append(m_deployment->receiveMessage(message, localMessage, args).value(Json()));
        results.append(m_techController->receiveMessage(message, localMessage, args).value(Json()));
        results.append(m_questManager->receiveMessage(message, localMessage, args).value(Json()));
        if (m_client) {
          JsonArray clientAndPaneResults = m_client->receiveMessage(message, localMessage, args).value(JsonArray()).toArray();
          for (auto& p : clientAndPaneResults)
            results.append(p);
        }
        for (auto& p : m_genericScriptContexts)
          results.append(p.second->handleMessage(message, localMessage, args).value(Json()));
        return Json(results);
      } else {
        Maybe<Json> result = m_tools->receiveMessage(message, localMessage, args);
        if (!result)
          result = m_statusController->receiveMessage(message, localMessage, args);
        if (!result)
          result = m_companions->receiveMessage(message, localMessage, args);
        if (!result)
          result = m_deployment->receiveMessage(message, localMessage, args);
        if (!result)
          result = m_techController->receiveMessage(message, localMessage, args);
        if (!result)
          result = m_questManager->receiveMessage(message, localMessage, args);
        if (m_client && !result)
          result = m_client->receiveMessage(message, localMessage, args);
        for (auto& p : m_genericScriptContexts) {
          if (result)
            break;
          result = p.second->handleMessage(message, localMessage, args);
        }
        return result;
      }
    } else {
      try {
        Json jsonArgs = Json(args);
        Logger::info("[xSB] Attempted '{}' message from cID {}. Message arguments: {}", message, fromConnection, jsonArgs.printJson(0));
      } catch (JsonException const& e) {
        Logger::info("[xSB] Attempted '{}' message from cID {}. Invalid message arguments.", message, fromConnection);
      }
    }
  }

  return {};
}

void Player::update(float dt, uint64_t) {
  ZoneScoped;
  m_movementController->setTimestep(dt);

  if (isMaster()) {
    ZoneScoped;
    m_cameraOverridePosition = {}; // FezzedOne: Reset this every tick.

    // From WasabiRaptor's PR: Spawn any overflowed inventory items. *Has* to be done in `update` to get drops to actually spawn for some reason.
    if (!m_overflowCheckDone) {
      for (auto& p : m_inventory->clearOverflow()) {
        giveItem(p);
        // world()->addEntity(ItemDrop::createRandomizedDrop(p,m_movementController->position(),true));
      }
      m_overflowCheckDone = true;
    }

    if (m_emoteCooldownTimer) {
      m_emoteCooldownTimer -= dt;
      if (m_emoteCooldownTimer <= 0) {
        m_emoteCooldownTimer = 0;
        m_emoteState = HumanoidEmote::Idle;
      }
    }

    if (m_chatMessageUpdated) {
      auto state = Root::singleton().emoteProcessor()->detectEmotes(m_chatMessage);
      if (state != HumanoidEmote::Idle)
        addEmote(state);
      m_chatMessageUpdated = false;
    }

    m_blinkCooldownTimer -= dt;
    if (m_blinkCooldownTimer <= 0) {
      m_blinkCooldownTimer = Random::randf(m_blinkInterval[0], m_blinkInterval[1]);
      auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
      if (m_emoteState == HumanoidEmote::Idle && (!loungeAnchor || !loungeAnchor->emote))
        addEmote(HumanoidEmote::Blink);
    }

    m_lastDamagedOtherTimer += dt;

    if (m_movementController->zeroG())
      m_movementController->controlParameters(m_zeroGMovementParameters);

    if (isTeleporting()) {
      m_teleportTimer -= dt;
      if (m_teleportTimer <= 0 && m_state == State::TeleportIn) {
        m_state = State::Idle;
        if (!m_fastRespawn)
          m_effectsAnimator->burstParticleEmitter(m_teleportAnimationType + "Burst");
      }
    }

    if (!isTeleporting()) {
      processControls();

      m_questManager->update(dt);
      m_companions->update(dt);
      m_deployment->update(dt);

      bool edgeTriggeredUse = take(m_edgeTriggeredUse);

      m_inventory->cleanup();
      refreshArmor(false);
      refreshItems();

      if (inConflictingLoungeAnchor())
        m_movementController->resetAnchorState();

      if (m_state == State::Lounge) {
        if (auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor())) {
          m_statusController->setPersistentEffects("lounging", loungeAnchor->statusEffects);
          addEffectEmitters(loungeAnchor->effectEmitters);
          if (loungeAnchor->emote)
            requestEmote(*loungeAnchor->emote);

          if (!m_ignoreForcedNudity) {
            auto itemDatabase = Root::singleton().itemDatabase();
            if (auto headOverride = loungeAnchor->armorCosmeticOverrides.maybe("head")) {
              auto overrideItem = itemDatabase->item(ItemDescriptor(*headOverride));
              if (m_inventory->itemAllowedAsEquipment(overrideItem, EquipmentSlot::HeadCosmetic))
                m_armor->setHeadCosmeticItem(as<HeadArmor>(overrideItem));
            }
            if (auto chestOverride = loungeAnchor->armorCosmeticOverrides.maybe("chest")) {
              auto overrideItem = itemDatabase->item(ItemDescriptor(*chestOverride));
              if (m_inventory->itemAllowedAsEquipment(overrideItem, EquipmentSlot::ChestCosmetic))
                m_armor->setChestCosmeticItem(as<ChestArmor>(overrideItem));
            }
            if (auto legsOverride = loungeAnchor->armorCosmeticOverrides.maybe("legs")) {
              auto overrideItem = itemDatabase->item(ItemDescriptor(*legsOverride));
              if (m_inventory->itemAllowedAsEquipment(overrideItem, EquipmentSlot::LegsCosmetic))
                m_armor->setLegsCosmeticItem(as<LegsArmor>(overrideItem));
            }
            if (auto backOverride = loungeAnchor->armorCosmeticOverrides.maybe("back")) {
              auto overrideItem = itemDatabase->item(ItemDescriptor(*backOverride));
              if (m_inventory->itemAllowedAsEquipment(overrideItem, EquipmentSlot::BackCosmetic))
                m_armor->setBackCosmeticItem(as<BackArmor>(overrideItem));
            }
          }
        } else {
          m_state = State::Idle;
          m_movementController->resetAnchorState();
        }
      } else {
        m_movementController->resetAnchorState();
        m_statusController->setPersistentEffects("lounging", {});
      }

      if (!forceNude())
        m_armor->effects(*m_effectEmitter);

      m_tools->effects(*m_effectEmitter);

      m_movementController->tickMaster(dt);

      m_techController->tickMaster(dt);

      for (auto& p : m_genericScriptContexts)
        p.second->update(p.second->updateDt(dt));

      if (edgeTriggeredUse) {
        auto anchor = as<LoungeAnchor>(m_movementController->entityAnchor());
        bool useTool = canUseTool();
        if (anchor && (!useTool || anchor->controllable))
          m_movementController->resetAnchorState();
        else if (useTool) {
          if (auto ie = bestInteractionEntity(true))
            interactWithEntity(ie);
        }
      }

      m_statusController->setPersistentEffects("armor", m_armor->statusEffects());
      m_statusController->setPersistentEffects("tools", m_tools->statusEffects());

      if (!m_techController->techOverridden())
        m_techController->setLoadedTech(m_techs->equippedTechs().values());

      if (!isDead())
        m_statusController->tickMaster(dt);

      if (!modeConfig().hunger)
        m_statusController->resetResource("food");

      if (!m_statusController->resourcePositive("food"))
        m_statusController->setPersistentEffects("hunger", m_foodEmptyStatusEffects);
      else if (m_statusController->resourcePercentage("food").value() <= m_foodLowThreshold)
        m_statusController->setPersistentEffects("hunger", m_foodLowStatusEffects);
      else
        m_statusController->setPersistentEffects("hunger", {});

      for (auto& pair : m_delayedRadioMessages) {
        if (pair.first.tick(dt))
          queueRadioMessage(pair.second);
      }
      m_delayedRadioMessages.filter([](pair<GameTimer, RadioMessage>& pair) { return !pair.first.ready(); });
    }

    if (m_isAdmin) {
      m_statusController->resetResource("health");
      m_statusController->resetResource("energy");
      m_statusController->resetResource("food");
      m_statusController->resetResource("breath");
    }

    m_log->addPlayTime(GlobalTimestep);

    if (m_ageItemsTimer.wrapTick(dt)) {
      auto itemDatabase = Root::singleton().itemDatabase();
      m_inventory->forEveryItem([&](InventorySlot const&, ItemPtr& item) {
        itemDatabase->ageItem(item, m_ageItemsTimer.time);
      });
    }

    for (auto tool : {m_tools->primaryHandItem(), m_tools->altHandItem()}) {
      if (auto inspectionTool = as<InspectionTool>(tool)) {
        for (auto ir : inspectionTool->pullInspectionResults()) {
          if (ir.objectName) {
            m_questManager->receiveMessage("objectScanned", true, {*ir.objectName, *ir.entityId});
            m_log->addScannedObject(*ir.objectName);
          }

          addChatMessageCallback(ir.message);

          // FezzedOne: Send inspection messages as entity messages to the player. This allows them to be handled in Lua scripts.
          EntityId scanEntityId = 0;
          if (ir.entityId)
            scanEntityId = *ir.entityId;

          if (world())
            world()->sendEntityMessage(entityId(), "inspectionMessage", JsonArray{scanEntityId, ir.message});
        }
      }
    }

    m_interestingObjects = m_questManager->interestingObjects();

  } else {
    ZoneScoped;
    m_netGroup.tickNetInterpolation(dt);
    m_movementController->tickSlave(dt);
    m_techController->tickSlave(dt);
    m_statusController->tickSlave(dt);
  }

  m_humanoid->setMovingBackwards(false);
  m_humanoid->setRotation(m_movementController->rotation());

  bool suppressedItems = !canUseTool();

  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
  if (loungeAnchor && loungeAnchor->dance)
    m_humanoid->setDance(*loungeAnchor->dance);
  else if ((!suppressedItems && (m_tools->primaryHandItem() || m_tools->altHandItem())) || m_humanoid->danceCyclicOrEnded() || m_movementController->running())
    m_humanoid->setDance({});

  bool isClient = world()->isClient();
  if (isClient)
    m_armor->setupHumanoidClothingDrawables(*m_humanoid, forceNude());

  m_tools->suppressItems(suppressedItems);
  m_tools->tick(dt, m_shifting, m_pendingMoves);

  if (auto overrideFacingDirection = m_tools->setupHumanoidHandItems(*m_humanoid, position(), aimPosition()))
    m_movementController->controlFace(*overrideFacingDirection);

  m_effectsAnimator->resetTransformationGroup("flip");
  if (m_movementController->facingDirection() == Direction::Left)
    m_effectsAnimator->scaleTransformationGroup("flip", Vec2F(-1, 1));

  if (m_state == State::Walk || m_state == State::Run) {
    if ((m_footstepTimer += dt) > m_config->footstepTiming) {
      m_footstepPending = true;
      m_footstepTimer = 0.0;
    }
  }

  if (isClient) {
    m_effectsAnimator->update(dt, &m_effectsAnimatorDynamicTarget);
    m_effectsAnimatorDynamicTarget.updatePosition(position() + m_techController->parentOffset());
  } else {
    m_effectsAnimator->update(dt, nullptr);
  }

  if (!isTeleporting())
    processStateChanges(dt);

  m_damageSources = m_tools->damageSources();
  for (auto& damageSource : m_damageSources) {
    damageSource.sourceEntityId = entityId();
    damageSource.team = getTeam();
  }

  m_songbook->update(*entityMode(), world());

  m_effectEmitter->setSourcePosition("normal", position());
  m_effectEmitter->setSourcePosition("mouth", mouthOffset() + position());
  m_effectEmitter->setSourcePosition("feet", feetOffset() + position());
  m_effectEmitter->setSourcePosition("headArmor", headArmorOffset() + position());
  m_effectEmitter->setSourcePosition("chestArmor", chestArmorOffset() + position());
  m_effectEmitter->setSourcePosition("legsArmor", legsArmorOffset() + position());
  m_effectEmitter->setSourcePosition("backArmor", backArmorOffset() + position());

  m_effectEmitter->setSourcePosition("primary", handPosition(ToolHand::Primary) + position());
  m_effectEmitter->setSourcePosition("alt", handPosition(ToolHand::Alt) + position());

  m_effectEmitter->setDirection(facingDirection());

  m_effectEmitter->tick(dt, *entityMode());

  m_humanoid->setFacingDirection(m_movementController->facingDirection());
  m_humanoid->setMovingBackwards(m_movementController->facingDirection() != m_movementController->movingDirection());

  m_pendingMoves.clear();

  if (isClient)
    SpatialLogger::logPoly("world", m_movementController->collisionBody(), isMaster() ? Color::Orange.toRgba() : Color::Yellow.toRgba());
}

float Player::timeSinceLastGaveDamage() const {
  return m_lastDamagedOtherTimer;
}

EntityId Player::lastDamagedTarget() const {
  return m_lastDamagedTarget;
}

void Player::render(RenderCallback* renderCallback) {
  ZoneScoped;

  if (invisible()) {
    m_techController->pullNewAudios();
    m_techController->pullNewParticles();
    m_statusController->pullNewAudios();
    m_statusController->pullNewParticles();
    return;
  }

  Vec2I footstepSensor = Vec2I((m_config->footstepSensor + m_movementController->position()).floor());
  String footstepSound = getFootstepSound(footstepSensor);

  if (!footstepSound.empty() && !m_techController->parentState() && !m_techController->parentHidden()) {
    auto footstepAudio = Root::singleton().assets()->audio(footstepSound);
    if (m_landingNoisePending) {
      auto landingNoise = make_shared<AudioInstance>(*footstepAudio);
      landingNoise->setPosition(position() + feetOffset());
      landingNoise->setVolume(m_landingVolume);
      renderCallback->addAudio(std::move(landingNoise));
    }

    if (m_footstepPending) {
      auto stepNoise = make_shared<AudioInstance>(*footstepAudio);
      stepNoise->setPosition(position() + feetOffset());
      stepNoise->setVolume(1 - Random::randf(0, m_footstepVolumeVariance));
      renderCallback->addAudio(std::move(stepNoise));
    }
  } else {
    m_footstepTimer = m_config->footstepTiming;
  }
  m_footstepPending = false;
  m_landingNoisePending = false;

  renderCallback->addAudios(m_effectsAnimatorDynamicTarget.pullNewAudios());
  renderCallback->addParticles(m_effectsAnimatorDynamicTarget.pullNewParticles());

  renderCallback->addAudios(m_techController->pullNewAudios());
  renderCallback->addAudios(m_statusController->pullNewAudios());

  for (auto const& p : take(m_callbackSounds)) {
    auto audio = make_shared<AudioInstance>(*Root::singleton().assets()->audio(get<0>(p)));
    audio->setVolume(get<1>(p));
    audio->setPitchMultiplier(get<2>(p));
    audio->setPosition(position());
    renderCallback->addAudio(std::move(audio));
  }

  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
  EntityRenderLayer renderLayer = loungeAnchor ? loungeAnchor->loungeRenderLayer : RenderLayerPlayer;

  renderCallback->addDrawables(drawables(), renderLayer);
  if (!isTeleporting())
    renderCallback->addOverheadBars(bars(), position());
  renderCallback->addParticles(particles());

  m_tools->render(renderCallback, inToolRange(), m_shifting, renderLayer);

  m_effectEmitter->render(renderCallback);
  m_songbook->render(renderCallback);

  if (isMaster())
    m_deployment->render(renderCallback, position());
}

void Player::renderLightSources(RenderCallback* renderCallback) {
  renderCallback->addLightSources(lightSources());
  // Thanks to Kae for fixing an oversight here.
  m_deployment->renderLightSources(renderCallback);
}

Json Player::getGenericProperty(String const& name, Json const& defaultValue) const {
  return m_genericProperties.value(name, defaultValue);
}

void Player::setGenericProperty(String const& name, Json const& value) {
  if (value.isNull())
    m_genericProperties.erase(name);
  else
    m_genericProperties.set(name, value);
}

PlayerInventoryPtr Player::inventory() const {
  return m_inventory;
}

uint64_t Player::itemsCanHold(ItemPtr const& items) const {
  return m_inventory->itemsCanFit(items);
}

ItemPtr Player::pickupItems(ItemPtr const& items) {
  if (isDead() || !items || m_inventory->itemsCanFit(items) == 0)
    return items;

  triggerPickupEvents(items);

  if (items->pickupSound().size()) {
    m_effectsAnimator->setSoundPool("pickup", {items->pickupSound()});
    float pitch = 1.f - ((float)items->count() / (float)items->maxStack()) * 0.5f;
    m_effectsAnimator->setSoundPitchMultiplier("pickup", clamp(pitch * Random::randf(0.8f, 1.2f), 0.f, 2.f));
    m_effectsAnimator->playSound("pickup");
  }
  auto itemDb = Root::singleton().itemDatabase();
  queueItemPickupMessage(itemDb->itemShared(items->descriptor()));
  return m_inventory->addItems(items);
}

void Player::giveItem(ItemPtr const& item) {
  if (auto spill = pickupItems(item))
    world()->addEntity(ItemDrop::createRandomizedDrop(spill->descriptor(), position()));
}

void Player::triggerPickupEvents(ItemPtr const& item) {
  if (item) {
    for (auto b : item->learnBlueprintsOnPickup())
      addBlueprint(b);

    for (auto pair : item->collectablesOnPickup())
      addCollectable(pair.first, pair.second);

    // FezzedOne: Fixed crash to menu whenever an item's `"radioMessagesOnPickup"` is of the wrong type.
    auto radioMessageList = item->instanceValue("radioMessagesOnPickup", JsonArray());
    radioMessageList = radioMessageList.isType(Json::Type::Array) ? radioMessageList : JsonArray();
    for (auto m : radioMessageList.iterateArray()) {
      if (m.isType(Json::Type::Array)) {
        if (m.size() >= 2 && m.get(1).canConvert(Json::Type::Float))
          queueRadioMessage(m.get(0), m.get(1).toFloat());
      } else {
        queueRadioMessage(m);
      }
    }

    if (auto cinematic = item->instanceValue("cinematicOnPickup", Json()))
      setPendingCinematic(cinematic, true);

    for (auto const& quest : item->pickupQuestTemplates()) {
      if (m_questManager->canStart(quest))
        m_questManager->offer(make_shared<Quest>(quest, 0, this));
    }

    if (auto consume = item->instanceValue("consumeOnPickup", Json())) {
      if (consume.isType(Json::Type::Bool) && consume.toBool())
        item->consume(item->count());
    }

    auto itemCategory = item->instanceValue("eventCategory", item->category());
    itemCategory = itemCategory.isType(Json::Type::String) ? itemCategory : String("");

    statistics()->recordEvent("item", JsonObject{
                                          {"itemName", item->name()},
                                          {"count", item->count()},
                                          {"category", itemCategory}});
  }
}

bool Player::hasItem(ItemDescriptor const& descriptor, bool exactMatch) const {
  return m_inventory->hasItem(descriptor, exactMatch);
}

uint64_t Player::hasCountOfItem(ItemDescriptor const& descriptor, bool exactMatch) const {
  return m_inventory->hasCountOfItem(descriptor, exactMatch);
}

ItemDescriptor Player::takeItem(ItemDescriptor const& descriptor, bool consumePartial, bool exactMatch) {
  return m_inventory->takeItems(descriptor, consumePartial, exactMatch);
}

void Player::giveItem(ItemDescriptor const& descriptor) {
  giveItem(Root::singleton().itemDatabase()->item(descriptor));
}

void Player::clearSwap() {
  // If we cannot put the swap slot back into the bag, then just drop it in the
  // world.
  if (!m_inventory->clearSwap()) {
    if (auto world = worldPtr())
      world->addEntity(ItemDrop::createRandomizedDrop(m_inventory->takeSlot(SwapSlot()), position()));
  }

  // Interrupt all firing in case the item being dropped was in use.
  endPrimaryFire();
  endAltFire();
  endTrigger();
}

void Player::refreshItems() {
  if (isSlave())
    return;

  m_tools->setItems(m_inventory->primaryHeldItem(), m_inventory->secondaryHeldItem());
}

void Player::refreshArmor(bool fullRefresh) {
  if (isSlave())
    return;

  // FezzedOne: Force the ArmorWearer to check armour.
  if (fullRefresh)
    m_armor->reset();
  m_armor->setHeadItem(m_inventory->headArmor());
  m_armor->setHeadCosmeticItem(m_inventory->headCosmetic());
  m_armor->setChestItem(m_inventory->chestArmor());
  m_armor->setChestCosmeticItem(m_inventory->chestCosmetic());
  m_armor->setLegsItem(m_inventory->legsArmor());
  m_armor->setLegsCosmeticItem(m_inventory->legsCosmetic());
  m_armor->setBackItem(m_inventory->backArmor());
  m_armor->setBackCosmeticItem(m_inventory->backCosmetic());
  m_movementController->resetBaseParameters(ActorMovementParameters(jsonMerge(m_humanoid->defaultMovementParameters(), m_config->movementParameters)));
  m_identityUpdated = true;
}

void Player::refreshEquipment() {
  refreshArmor();
  refreshItems();
}

PlayerBlueprintsPtr Player::blueprints() const {
  return m_blueprints;
}

bool Player::addBlueprint(ItemDescriptor const& descriptor, bool showFailure) {
  if (descriptor.isNull())
    return false;

  auto itemDb = Root::singleton().itemDatabase();
  auto item = itemDb->item(descriptor);
  auto assets = Root::singleton().assets();
  if (!m_blueprints->isKnown(descriptor)) {
    m_blueprints->add(descriptor);
    queueUIMessage(assets->json("/player.config:blueprintUnlock").toString().replace("<ItemName>", item->friendlyName()));
    return true;
  } else if (showFailure) {
    queueUIMessage(assets->json("/player.config:blueprintAlreadyKnown").toString().replace("<ItemName>", item->friendlyName()));
  }

  return false;
}

bool Player::blueprintKnown(ItemDescriptor const& descriptor) const {
  if (descriptor.isNull())
    return false;

  return m_blueprints->isKnown(descriptor);
}

bool Player::addCollectable(String const& collectionName, String const& collectableName) {
  if (m_log->addCollectable(collectionName, collectableName)) {
    auto collectionDatabase = Root::singleton().collectionDatabase();

    auto collection = collectionDatabase->collection(collectionName);
    auto collectable = collectionDatabase->collectable(collectionName, collectableName);
    queueUIMessage(Root::singleton().assets()->json("/player.config:collectableUnlock").toString().replace("<collectable>", collectable.title).replace("<collection>", collection.title));
    return true;
  } else {
    return false;
  }
}

PlayerUniverseMapPtr Player::universeMap() const {
  return m_universeMap;
}

PlayerCodexesPtr Player::codexes() const {
  return m_codexes;
}

PlayerTechPtr Player::techs() const {
  return m_techs;
}

void Player::overrideTech(Maybe<StringList> const& techModules) {
  if (techModules)
    m_techController->setOverrideTech(*techModules);
  else
    m_techController->clearOverrideTech();
}

bool Player::techOverridden() const {
  return m_techController->techOverridden();
}

PlayerCompanionsPtr Player::companions() const {
  return m_companions;
}

PlayerLogPtr Player::log() const {
  return m_log;
}

InteractiveEntityPtr Player::bestInteractionEntity(bool includeNearby) {
  if (!inWorld())
    return {};

  InteractiveEntityPtr interactiveEntity;
  if (auto entity = world()->getInteractiveInRange(m_aimPosition, (isAdmin() || m_canReachAll) ? m_aimPosition : position(), m_interactRadius, (isAdmin() || m_canReachAll))) {
    interactiveEntity = entity;
  } else if (includeNearby) {
    Vec2F interactBias = m_walkIntoInteractBias;
    if (facingDirection() == Direction::Left)
      interactBias[0] *= -1;
    Vec2F pos = position() + interactBias;

    if (auto entity = world()->getInteractiveInRange(pos, position(), m_interactRadius, (isAdmin() || m_canReachAll)))
      interactiveEntity = entity;
  }

  if (interactiveEntity && ((isAdmin() || m_canReachAll) || world()->canReachEntity(position(), interactRadius(), interactiveEntity->entityId()))) {
    if (interactiveEntity->isInteractive() || isAdmin() || m_canReachAll)
      return interactiveEntity;
  }
  return {};
}

void Player::interactWithEntity(InteractiveEntityPtr entity) {
  bool questIntercepted = false;
  for (auto quest : m_questManager->listActiveQuests()) {
    if (quest->interactWithEntity(entity->entityId()))
      questIntercepted = true;
  }
  if (questIntercepted)
    return;

  bool anyTurnedIn = false;

  for (auto questId : entity->turnInQuests()) {
    if (m_questManager->canTurnIn(questId)) {
      auto const& quest = m_questManager->getQuest(questId);
      quest->setEntityParameter("questReceiver", entity);
      m_questManager->getQuest(questId)->complete();
      anyTurnedIn = true;
    }
  }

  if (anyTurnedIn)
    return;

  for (auto questArc : entity->offeredQuests()) {
    if (m_questManager->canStart(questArc)) {
      auto quest = make_shared<Quest>(questArc, 0, this);
      quest->setWorldId(clientContext()->playerWorldId());
      quest->setServerUuid(clientContext()->serverUuid());
      quest->setEntityParameter("questGiver", entity);
      m_questManager->offer(quest);
      return;
    }
  }

  m_pendingInteractActions.append(world()->interact(InteractRequest{
      entityId(), position(), entity->entityId(), aimPosition()}));
}

void Player::aim(Vec2F const& position) {
  m_techController->setAimPosition(position);
  m_aimPosition = position;
}

Vec2F Player::aimPosition() const {
  return m_aimPosition;
}

Vec2F Player::armPosition(ToolHand hand, Direction facingDirection, float armAngle, Vec2F offset) const {
  return m_tools->armPosition(*m_humanoid, hand, facingDirection, armAngle, offset);
}

Vec2F Player::handOffset(ToolHand hand, Direction facingDirection) const {
  return m_tools->handOffset(*m_humanoid, hand, facingDirection);
}

Vec2F Player::handPosition(ToolHand hand, Vec2F const& handOffset) const {
  return m_tools->handPosition(hand, *m_humanoid, handOffset);
}

ItemPtr Player::handItem(ToolHand hand) const {
  if (hand == ToolHand::Primary)
    return m_tools->primaryHandItem();
  else
    return m_tools->altHandItem();
}

Vec2F Player::armAdjustment() const {
  return m_humanoid->armAdjustment();
}

void Player::setCameraFocusEntity(Maybe<EntityId> const& cameraFocusEntity) {
  m_cameraFocusEntity = cameraFocusEntity;
}

void Player::playEmote(HumanoidEmote emote) {
  addEmote(emote);
}

bool Player::toolUsageSuppressed() const {
  return m_toolUsageSuppressed;
}

bool Player::canUseTool() const {
  // FezzedOne: `TechController::toolUsageSuppressed` "reciprocally" calls `Player::toolUsageSuppressed` above.
  bool canUse = !isDead() && !isTeleporting() && ((!m_techController->toolUsageSuppressed()) || m_ignoreAllTechOverrides);
  if (canUse) {
    if (auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor()))
      if (loungeAnchor->suppressTools.value(loungeAnchor->controllable) && !m_ignoreAllTechOverrides)
        return false;
  }
  return canUse;
}

void Player::beginPrimaryFire() {
  m_techController->beginPrimaryFire();
  m_tools->beginPrimaryFire();
}

void Player::beginAltFire() {
  m_techController->beginAltFire();
  m_tools->beginAltFire();
}

void Player::endPrimaryFire() {
  m_techController->endPrimaryFire();
  m_tools->endPrimaryFire();
}

void Player::endAltFire() {
  m_techController->endAltFire();
  m_tools->endAltFire();
}

void Player::beginTrigger() {
  if (!m_useDown)
    m_edgeTriggeredUse = true;
  m_useDown = true;
}

void Player::endTrigger() {
  m_useDown = false;
}

float Player::toolRadius() const {
  auto radius = m_tools->toolRadius();
  if (radius)
    return *radius;
  return interactRadius();
}

float Player::interactRadius() const {
  return m_interactRadius;
}

void Player::setInteractRadius(float interactRadius) {
  m_interactRadius = interactRadius;
}

List<InteractAction> Player::pullInteractActions() {
  List<InteractAction> results;
  eraseWhere(m_pendingInteractActions, [&results](auto& promise) {
    if (auto res = promise.result())
      results.append(res.take());
    return promise.finished();
  });
  return results;
}

uint64_t Player::currency(String const& currencyType) const {
  return m_inventory->currency(currencyType);
}

float Player::health() const {
  return m_statusController->resource("health");
}

float Player::maxHealth() const {
  return *m_statusController->resourceMax("health");
}

DamageBarType Player::damageBar() const {
  return DamageBarType::Default;
}

float Player::healthPercentage() const {
  return *m_statusController->resourcePercentage("health");
}

float Player::energy() const {
  return m_statusController->resource("energy");
}

float Player::maxEnergy() const {
  return *m_statusController->resourceMax("energy");
}

float Player::energyPercentage() const {
  return *m_statusController->resourcePercentage("energy");
}

float Player::energyRegenBlockPercent() const {
  return *m_statusController->resourcePercentage("energyRegenBlock");
}

bool Player::fullEnergy() const {
  return energy() >= maxEnergy();
}

bool Player::energyLocked() const {
  return m_statusController->resourceLocked("energy");
}

bool Player::consumeEnergy(float energy) {
  if (m_isAdmin)
    return true;
  return m_statusController->overConsumeResource("energy", energy);
}

float Player::foodPercentage() const {
  return *m_statusController->resourcePercentage("food");
}

float Player::breath() const {
  return m_statusController->resource("breath");
}

float Player::maxBreath() const {
  return *m_statusController->resourceMax("breath");
}

float Player::protection() const {
  return m_statusController->stat("protection");
}

bool Player::forceNude() const {
  return m_statusController->statPositive("nude") && !m_ignoreForcedNudity;
}

String Player::description() const {
  return m_description;
}

void Player::setDescription(String const& description) {
  m_description = description;
}

Direction Player::walkingDirection() const {
  return m_movementController->movingDirection();
}

Direction Player::facingDirection() const {
  return m_movementController->facingDirection();
}

pair<ByteArray, uint64_t> Player::writeNetState(uint64_t fromVersion) {
  return m_netGroup.writeNetState(fromVersion);
}

void Player::readNetState(ByteArray data, float interpolationTime) {
  m_netGroup.readNetState(std::move(data), interpolationTime);
}

void Player::enableInterpolation(float) {
  m_netGroup.enableNetInterpolation();
}

void Player::disableInterpolation() {
  m_netGroup.disableNetInterpolation();
}

UniverseClient* Player::getUniverseClient() const {
  return m_client;
}

Json Player::teamMembers() const {
  if (m_client) {
    TeamClientPtr team = m_client->teamClient();
    if (team) {
      return team->teamMembers();
    } else {
      return Json();
    }
  } else {
    return Json();
  }
}

void Player::passChatText(String const& chatText) {
  m_chatText = chatText;
}

void Player::passChatOpen(bool chatOpen) {
  m_chatOpen = chatOpen;
}

bool Player::chatOpen() const {
  return m_chatOpen;
}

String Player::chatText() const {
  return m_chatText;
}

void Player::overrideChatIndicator(bool overridden) {
  m_overrideChatIndicator = overridden;
}

bool Player::chatIndicatorOverridden() const {
  return m_overrideChatIndicator;
}

void Player::overrideMenuIndicator(bool overridden) {
  m_overrideMenuIndicator = overridden;
}

bool Player::menuIndicatorOverridden() const {
  return m_overrideMenuIndicator;
}

void Player::processControls() {
  bool run = !m_shifting && !m_statusController->statPositive("encumberance");

  bool useMoveVector = m_moveVector.x() != 0.0f;
  if (useMoveVector) {
    for (auto move : m_pendingMoves) {
      if (move == MoveControlType::Left || move == MoveControlType::Right) {
        useMoveVector = false;
        break;
      }
    }
  }

  if (useMoveVector) {
    m_pendingMoves.insert(m_moveVector.x() < 0.0f ? MoveControlType::Left : MoveControlType::Right);
    m_movementController->setMoveSpeedMultiplier(clamp(abs(m_moveVector.x()), 0.0f, 1.0f));
  } else
    m_movementController->setMoveSpeedMultiplier(1.0f);

  if (auto fireableMain = as<FireableItem>(m_tools->primaryHandItem())) {
    if (fireableMain->inUse() && fireableMain->walkWhileFiring())
      run = false;
  }

  if (auto fireableAlt = as<FireableItem>(m_tools->altHandItem())) {
    if (fireableAlt->inUse() && fireableAlt->walkWhileFiring())
      run = false;
  }

  bool move = true;

  if (auto fireableMain = as<FireableItem>(m_tools->primaryHandItem())) {
    if (fireableMain->inUse() && fireableMain->stopWhileFiring())
      move = false;
  }

  if (auto fireableAlt = as<FireableItem>(m_tools->altHandItem())) {
    if (fireableAlt->inUse() && fireableAlt->stopWhileFiring())
      move = false;
  }

  auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
  if (loungeAnchor && loungeAnchor->controllable) {
    auto anchorState = m_movementController->anchorState();
    if (auto loungeableEntity = world()->get<LoungeableEntity>(anchorState->entityId)) {
      for (auto move : m_pendingMoves) {
        if (move == MoveControlType::Up)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Up);
        else if (move == MoveControlType::Down)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Down);
        else if (move == MoveControlType::Left)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Left);
        else if (move == MoveControlType::Right)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Right);
        else if (move == MoveControlType::Jump)
          loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::Jump);
      }
      if (m_tools->firingPrimary())
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::PrimaryFire);
      if (m_tools->firingAlt())
        loungeableEntity->loungeControl(anchorState->positionIndex, LoungeControl::AltFire);

      loungeableEntity->loungeAim(anchorState->positionIndex, m_aimPosition);
    }
    move = false;
  }

  m_techController->setShouldRun(run);

  if (move) {
    for (auto move : m_pendingMoves) {
      switch (move) {
        case MoveControlType::Right:
          m_techController->moveRight();
          break;
        case MoveControlType::Left:
          m_techController->moveLeft();
          break;
        case MoveControlType::Up:
          m_techController->moveUp();
          break;
        case MoveControlType::Down:
          m_techController->moveDown();
          break;
        case MoveControlType::Jump:
          m_techController->jump();
          break;
      }
    }
  }

  if (m_state == State::Lounge && !m_pendingMoves.empty() && move)
    stopLounging();
}

void Player::processStateChanges(float dt) {
  if (isMaster()) {

    // Set the current player state based on what movement controller tells us
    // we're doing and do some state transition logic
    State oldState = m_state;

    if (m_movementController->zeroG()) {
      if (m_movementController->flying())
        m_state = State::Swim;
      else if (m_state != State::Lounge)
        m_state = State::SwimIdle;
    } else if (m_movementController->groundMovement()) {
      if (m_movementController->running()) {
        m_state = State::Run;
      } else if (m_movementController->walking()) {
        m_state = State::Walk;
      } else if (m_movementController->crouching()) {
        m_state = State::Crouch;
      } else {
        if (m_state != State::Lounge)
          m_state = State::Idle;
      }
    } else if (m_movementController->liquidMovement()) {
      if (m_movementController->jumping()) {
        m_state = State::Swim;
      } else {
        if (m_state != State::Lounge)
          m_state = State::SwimIdle;
      }
    } else {
      if (m_movementController->jumping()) {
        m_state = State::Jump;
      } else {
        if (m_movementController->falling()) {
          m_state = State::Fall;
        }
        if (m_movementController->velocity()[1] > 0) {
          if (m_state != State::Lounge)
            m_state = State::Jump;
        }
      }
    }

    if (m_moveVector.x() != 0.0f && (m_state == State::Run))
      m_state = abs(m_moveVector.x()) > 0.5f ? State::Run : State::Walk;

    if (m_state == State::Jump && (oldState == State::Idle || oldState == State::Run || oldState == State::Walk || oldState == State::Crouch))
      m_effectsAnimator->burstParticleEmitter("jump");

    if (!m_movementController->isNullColliding()) {
      if (oldState == State::Fall && oldState != m_state && m_state != State::Swim && m_state != State::SwimIdle && m_state != State::Jump) {
        m_effectsAnimator->burstParticleEmitter("landing");
        m_landedNetState.trigger();
        m_landingNoisePending = true;
      }
    }
  }

  m_humanoid->animate(dt);

  if (m_overrideState) {
    // FezzedOne: If a player humanoid state override is present, use it. Player state overrides take precedence over tech parent states.
    m_humanoid->setState(*m_overrideState);
  } else if (auto techState = m_techController->parentState()) {
    if (techState == TechController::ParentState::Stand) {
      m_humanoid->setState(Humanoid::Idle);
    } else if (techState == TechController::ParentState::Fly) {
      m_humanoid->setState(Humanoid::Jump);
    } else if (techState == TechController::ParentState::Fall) {
      m_humanoid->setState(Humanoid::Fall);
    } else if (techState == TechController::ParentState::Sit) {
      m_humanoid->setState(Humanoid::Sit);
    } else if (techState == TechController::ParentState::Lay) {
      m_humanoid->setState(Humanoid::Lay);
    } else if (techState == TechController::ParentState::Duck) {
      m_humanoid->setState(Humanoid::Duck);
    } else if (techState == TechController::ParentState::Walk) {
      m_humanoid->setState(Humanoid::Walk);
    } else if (techState == TechController::ParentState::Run) {
      m_humanoid->setState(Humanoid::Run);
    } else if (techState == TechController::ParentState::Swim) {
      m_humanoid->setState(Humanoid::Swim);
    } else if (techState == TechController::ParentState::SwimIdle) {
      m_humanoid->setState(Humanoid::SwimIdle);
    }
  } else {
    auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor());
    if (m_state == State::Idle) {
      m_humanoid->setState(Humanoid::Idle);
    } else if (m_state == State::Walk) {
      m_humanoid->setState(Humanoid::Walk);
    } else if (m_state == State::Run) {
      m_humanoid->setState(Humanoid::Run);
    } else if (m_state == State::Jump) {
      m_humanoid->setState(Humanoid::Jump);
    } else if (m_state == State::Fall) {
      m_humanoid->setState(Humanoid::Fall);
    } else if (m_state == State::Swim) {
      m_humanoid->setState(Humanoid::Swim);
    } else if (m_state == State::SwimIdle) {
      m_humanoid->setState(Humanoid::SwimIdle);
    } else if (m_state == State::Crouch) {
      m_humanoid->setState(Humanoid::Duck);
    } else if (m_state == State::Lounge && loungeAnchor && loungeAnchor->orientation == LoungeOrientation::Sit) {
      m_humanoid->setState(Humanoid::Sit);
    } else if (m_state == State::Lounge && loungeAnchor && loungeAnchor->orientation == LoungeOrientation::Lay) {
      m_humanoid->setState(Humanoid::Lay);
    } else if (m_state == State::Lounge && loungeAnchor && loungeAnchor->orientation == LoungeOrientation::Stand) {
      m_humanoid->setState(Humanoid::Idle);
    }
  }

  m_humanoid->setEmoteState(m_emoteState);
}

String Player::getFootstepSound(Vec2I const& sensor) const {
  auto materialDatabase = Root::singleton().materialDatabase();

  String fallback = materialDatabase->defaultFootstepSound();
  List<Vec2I> scanOrder{{0, 0}, {0, -1}, {-1, 0}, {1, 0}, {-1, -1}, {1, -1}};
  for (auto subSensor : scanOrder) {
    String footstepSound = materialDatabase->footstepSound(world()->material(sensor + subSensor, TileLayer::Foreground),
        world()->mod(sensor + subSensor, TileLayer::Foreground));
    if (!footstepSound.empty()) {
      if (footstepSound != fallback) {
        return footstepSound;
      }
    }
  }
  return fallback;
}

bool Player::inInteractionRange() const {
  return inInteractionRange(centerOfTile(aimPosition()));
}

bool Player::inInteractionRange(Vec2F aimPos) const {
  return canReachAll() || isAdmin() || world()->geometry().diff(aimPos, position()).magnitude() < interactRadius();
}

bool Player::inToolRange() const {
  return inToolRange(centerOfTile(aimPosition()));
}

bool Player::inToolRange(Vec2F const& aimPos) const {
  return canReachAll() || isAdmin() || world()->geometry().diff(aimPos, position()).magnitude() < toolRadius();
}

void Player::getNetStates(bool initial) {
  m_state = (State)m_stateNetState.get();
  m_shifting = m_shiftingNetState.get();
  m_aimPosition[0] = m_xAimPositionNetState.get();
  m_aimPosition[1] = m_yAimPositionNetState.get();

  if (m_identityNetState.pullUpdated()) {
    m_identity = m_identityNetState.get();
    m_humanoid->setIdentity(m_identity);
  }

  if (!m_damageTeamOverridden)
    setTeam(m_teamNetState.get());

  if (m_landedNetState.pullOccurred() && !initial)
    m_landingNoisePending = true;

  if (m_newChatMessageNetState.pullOccurred() && !initial) {
    m_chatMessage = m_chatMessageNetState.get();
    m_chatMessageUpdated = true;
    m_pendingChatActions.append(SayChatAction{entityId(), m_chatMessage, m_movementController->position()});
  }

  m_emoteState = HumanoidEmoteNames.getLeft(m_emoteNetState.get());

  (void)getNetArmorSecrets();
}

void Player::setNetStates() {
  m_stateNetState.set((unsigned)m_state);
  m_shiftingNetState.set(m_shifting);
  m_xAimPositionNetState.set(m_aimPosition[0]);
  m_yAimPositionNetState.set(m_aimPosition[1]);

  if (m_identityUpdated) {
    m_identityNetState.push(m_humanoid ? m_humanoid->netIdentity() : m_identity);
    m_identityUpdated = false;
  }

  m_teamNetState.set(getTeam());

  if (m_chatMessageChanged) {
    m_chatMessageChanged = false;
    m_chatMessageNetState.push(m_chatMessage);
    m_newChatMessageNetState.trigger();
  }

  m_emoteNetState.set(HumanoidEmoteNames.getRight(m_emoteState));
}

void Player::setAdmin(bool isAdmin) {
  m_isAdmin = isAdmin;
}

bool Player::isAdmin() const {
  return m_isAdmin;
}

void Player::setFavoriteColor(Vec4B color) {
  m_identity.color = color;
  updateIdentity();
}

Vec4B Player::favoriteColor() const {
  return m_identity.color;
}

bool Player::isTeleporting() const {
  return (m_state == State::TeleportIn) || (m_state == State::TeleportOut);
}

bool Player::isTeleportingOut() const {
  return inWorld() && (m_state == State::TeleportOut) && m_teleportTimer >= 0.0f;
}

bool Player::canDeploy() {
  return m_deployment->canDeploy();
}

void Player::deployAbort(String const& animationType) {
  m_teleportAnimationType = animationType;
  m_deployment->setDeploying(false);
}

bool Player::isDeploying() const {
  return m_deployment->isDeploying();
}

bool Player::isDeployed() const {
  return m_deployment->isDeployed();
}

void Player::setBusyState(PlayerBusyState busyState) {
  m_effectsAnimator->setState("busy", PlayerBusyStateNames.getRight(busyState));
}

void Player::teleportOut(String const& animationType, bool deploy) {
  m_state = State::TeleportOut;
  m_teleportAnimationType = animationType;
  if (!m_fastRespawn)
    m_effectsAnimator->setState("teleport", m_teleportAnimationType + "Out");
  m_deployment->setDeploying(deploy);
  m_deployment->teleportOut();
  if (m_fastRespawn)
    m_teleportTimer = 0.0f;
  else
    m_teleportTimer = deploy ? m_config->deployOutTime : m_config->teleportOutTime;
}

void Player::teleportIn() {
  m_state = State::TeleportIn;
  if (!m_fastRespawn)
    m_effectsAnimator->setState("teleport", m_teleportAnimationType + "In");
  if (!m_fastRespawn)
    m_teleportTimer = m_deployment->isDeployed() ? m_config->deployInTime : m_config->teleportInTime;
  else
    m_teleportTimer = 0.0f;

  auto statusEffects = Root::singleton().assets()->json("/player.config:teleportInStatusEffects").toArray().transformed(jsonToEphemeralStatusEffect);
  m_statusController->addEphemeralEffects(statusEffects);
}

void Player::teleportAbort() {
  m_state = State::TeleportIn;
  if (!m_fastRespawn)
    m_effectsAnimator->setState("teleport", "abort");
  m_deployment->setDeploying(m_deployment->isDeployed());
  m_teleportTimer = m_fastRespawn ? 0.0f : m_config->teleportInTime;
}

void Player::moveTo(Vec2F const& footPosition) {
  m_movementController->setPosition(footPosition - feetOffset());
  m_movementController->setVelocity(Vec2F());
}

ItemPtr Player::primaryHandItem() const {
  return m_tools->primaryHandItem();
}

ItemPtr Player::altHandItem() const {
  return m_tools->altHandItem();
}

Uuid Player::uuid() const {
  return Uuid(*uniqueId());
}

PlayerMode Player::modeType() const {
  return m_modeType;
}

void Player::setModeType(PlayerMode mode) {
  m_modeType = mode;

  auto assets = Root::singleton().assets();
  m_modeConfig = PlayerModeConfig(assets->json("/playermodes.config").get(PlayerModeNames.getRight(mode)));
}

PlayerModeConfig Player::modeConfig() const {
  return m_modeConfig;
}

ShipUpgrades Player::shipUpgrades() {
  return m_shipUpgrades;
}

void Player::setShipUpgrades(ShipUpgrades shipUpgrades) {
  m_shipUpgrades = std::move(shipUpgrades);
}

void Player::applyShipUpgrades(Json const& upgrades) {
  if (m_clientContext->playerUuid() == uuid())
    m_clientContext->rpcInterface()->invokeRemote("ship.applyShipUpgrades", upgrades);
  else
    m_shipUpgrades.apply(upgrades);
}

String Player::name() const {
  return m_identity.name;
}

void Player::setName(String const& name) {
  m_identity.name = name;
  updateIdentity();
}

Maybe<String> Player::statusText() const {
  return {};
}

bool Player::displayNametag() const {
  return true;
}

Vec3B Player::nametagColor() const {
  auto assets = Root::singleton().assets();
  return jsonToVec3B(assets->json("/player.config:nametagColor"));
}

Vec2F Player::nametagOrigin() const {
  return mouthPosition(false);
}

void Player::updateIdentity() {
  m_identityUpdated = true;
  m_humanoid->setIdentity(m_identity);
  m_armor->setupHumanoidClothingDrawables(*m_humanoid, forceNude(), true);
}

void Player::setBodyDirectives(String const& directives) {
  m_identity.bodyDirectives = directives;
  updateIdentity();
}

void Player::setEmoteDirectives(String const& directives) {
  m_identity.emoteDirectives = directives;
  updateIdentity();
}

void Player::setHairGroup(String const& group) {
  m_identity.hairGroup = group;
  updateIdentity();
}

void Player::setHairType(String const& type) {
  m_identity.hairType = type;
  updateIdentity();
}

void Player::setHairDirectives(String const& directives) {
  m_identity.hairDirectives = directives;
  updateIdentity();
}

void Player::setFacialHairGroup(String const& group) {
  m_identity.facialHairGroup = group;
  updateIdentity();
}

void Player::setFacialHairType(String const& type) {
  m_identity.facialHairType = type;
  updateIdentity();
}

void Player::setFacialHairDirectives(String const& directives) {
  m_identity.facialHairDirectives = directives;
  updateIdentity();
}

void Player::setFacialMaskGroup(String const& group) {
  m_identity.facialMaskGroup = group;
  updateIdentity();
}

void Player::setFacialMaskType(String const& type) {
  m_identity.facialMaskType = type;
  updateIdentity();
}

void Player::setFacialMaskDirectives(String const& directives) {
  m_identity.facialMaskDirectives = directives;
  updateIdentity();
}

void Player::setHair(String const& group, String const& type, String const& directives) {
  m_identity.hairGroup = group;
  m_identity.hairType = type;
  m_identity.hairDirectives = directives;
  updateIdentity();
}

void Player::setFacialHair(String const& group, String const& type, String const& directives) {
  m_identity.facialHairGroup = group;
  m_identity.facialHairType = type;
  m_identity.facialHairDirectives = directives;
  updateIdentity();
}

void Player::setFacialMask(String const& group, String const& type, String const& directives) {
  m_identity.facialMaskGroup = group;
  m_identity.facialMaskType = type;
  m_identity.facialMaskDirectives = directives;
  updateIdentity();
}

bool Player::checkSpecies(String const& species, Maybe<String> const& maybeCallbackName) { // FezzedOne: Check whether a species exists in the loaded assets.
  Star::Root& root = Root::singleton();
  String callbackName = "setSpecies";
  if (maybeCallbackName)
    callbackName = maybeCallbackName.get();
  bool speciesFound = false;

  for (auto const& nameDefPair : root.speciesDatabase()->allSpecies()) {
    String speciesName = nameDefPair.second->options().species;

    if (species == speciesName) {
      speciesFound = true;
      break;
    }
  }

  if (!speciesFound && maybeCallbackName)
    Logger::warn("{}: Nonexistent species '{}'; species was not changed or base species used instead of \"imagePath\" species for humanoid config.", callbackName, species);
  return speciesFound;
}

void Player::setSpecies(String const& species) {
  if (checkSpecies(species)) { // FezzedOne: Only sets the species if it actually exists. Prevents crashes.
    m_identity.species = species;
    auto speciesToUse = m_identity.imagePath ? *m_identity.imagePath : species;
    auto speciesDef = Root::singleton().speciesDatabase()->species(checkSpecies(speciesToUse, String("setSpecies")) ? speciesToUse : species);
    m_humanoid = make_shared<Humanoid>(speciesDef->humanoidConfig());
    updateIdentity();
  }
}

Gender Player::gender() const {
  return m_identity.gender;
}

void Player::setGender(Gender const& gender) {
  m_identity.gender = gender;
  updateIdentity();
  // FezzedOne: Fixed bug where changing gender does not immediately swap armour sprites.
  refreshArmor();
}

String Player::species() const {
  return m_identity.species;
}

void Player::setPersonality(Personality const& personality) {
  m_identity.personality = personality;
  updateIdentity();
}

void Player::setImagePath(Maybe<String> const& imagePath) {
  String speciesName = imagePath ? *imagePath : m_identity.species;
  m_identity.imagePath = imagePath;
  auto speciesToUse = m_identity.imagePath ? *m_identity.imagePath : m_identity.species;
  auto speciesDef = Root::singleton().speciesDatabase()->species(checkSpecies(speciesToUse) ? speciesToUse : m_identity.species);
  m_humanoid = make_shared<Humanoid>(speciesDef->humanoidConfig());
  updateIdentity();
}

HumanoidPtr Player::humanoid() {
  return m_humanoid;
}

HumanoidIdentity const& Player::identity() const {
  return m_identity;
}

// FezzedOne: Different function name to avoid issues with `auto` in the future.
Json Player::getIdentity() const {
  return m_identity.toJson();
}

void Player::setIdentity(HumanoidIdentity identity) {
  String speciesName = identity.imagePath ? *identity.imagePath : identity.species;
  String oldSpeciesName = m_identity.imagePath ? *m_identity.imagePath : m_identity.species;
  bool speciesChanged = speciesName != oldSpeciesName;
  m_identity = std::move(identity);
  if (speciesChanged) {
    auto speciesDef = Root::singleton().speciesDatabase()->species(speciesName);
    m_humanoid = make_shared<Humanoid>(speciesDef->humanoidConfig());
  }
  updateIdentity();
}

void Player::setIdentity(Json const& newIdentity) {
  Maybe<String> oldImagePath = m_identity.imagePath;
  String oldSpecies = oldImagePath ? *oldImagePath : m_identity.species;
  Json oldIdentity = m_identity.toJson();
  Json mergedIdentity = oldIdentity;
  if (newIdentity.type() == Json::Type::Object) {
    mergedIdentity = jsonMerge(oldIdentity, newIdentity);
    if (newIdentity.contains("imagePath")) {
      // FezzedOne: Check if the "imagePath" is an explicit `null` (i.e., it's in the internal Lua "nils"
      // table and thus `Json::contains` returns `true`). If so, set the new imagePath to `null`.
      if (newIdentity.get("imagePath").type() == Json::Type::Null) {
        mergedIdentity = mergedIdentity.set("imagePath", Json());
      }
    }
    String speciesName = mergedIdentity.getString("species");
    String imagePath = mergedIdentity.optString("imagePath").value(speciesName);
    if (!checkSpecies(speciesName, String("setIdentity"))) { // FezzedOne: If the new species doesn't exist, retain the old species.
      mergedIdentity = mergedIdentity.set("species", oldIdentity.getString("species"));
      speciesName = oldSpecies;
    }
    m_identity = HumanoidIdentity(mergedIdentity);
    if (imagePath != oldSpecies) {
      auto speciesDef = Root::singleton().speciesDatabase()->species(checkSpecies(imagePath) ? imagePath : speciesName);
      m_humanoid = make_shared<Humanoid>(speciesDef->humanoidConfig());
    }
    updateIdentity();
    // FezzedOne: Fixed bug where changing gender does not immediately swap armour sprites.
    refreshArmor();
  }
}

List<String> Player::pullQueuedMessages() {
  return take(m_queuedMessages);
}

List<ItemPtr> Player::pullQueuedItemDrops() {
  return take(m_queuedItemPickups);
}

void Player::queueUIMessage(String const& message) {
  if (!isSlave())
    m_queuedMessages.append(message);
}

void Player::queueItemPickupMessage(ItemPtr const& item) {
  if (!isSlave())
    m_queuedItemPickups.append(item);
}

void Player::addChatMessage(String const& message) {
  starAssert(!isSlave());
  m_chatMessage = message;
  m_chatMessageUpdated = true;
  m_chatMessageChanged = true;
  m_pendingChatActions.append(SayChatAction{entityId(), message, mouthPosition()});
}

// FezzedOne: Four-argument overload for use with a Lua callback. This actually deals with *chat bubbles*, by the way.
void Player::addChatMessage(String const& message, Maybe<String> const& portrait, Maybe<EntityId> const& sourceEntityId, Maybe<Json> const& bubbleConfig) {
  starAssert(!isSlave());
  m_chatMessage = message;
  m_chatMessageUpdated = true;
  m_chatMessageChanged = true;
  if (portrait) {
    if (bubbleConfig) {
      String chatPrefix = "";
      try {
        chatPrefix = bubbleConfig.get().getString("chatPrefix", "");
      } catch (JsonException) { chatPrefix = ""; }
      String chatSuffix = "";
      try {
        chatSuffix = bubbleConfig.get().getString("chatSuffix", "");
      } catch (JsonException) { chatSuffix = ""; }
      String modifiedMessage = chatPrefix + message + chatSuffix;
      m_pendingChatActions.append(PortraitChatAction{sourceEntityId.value(entityId()), portrait.get(), modifiedMessage, mouthPosition(), *bubbleConfig});
    } else {
      m_pendingChatActions.append(PortraitChatAction{sourceEntityId.value(entityId()), portrait.get(), message, mouthPosition()});
    }
  } else {
    if (bubbleConfig) {
      String chatPrefix = "";
      try {
        chatPrefix = bubbleConfig.get().getString("chatPrefix", "");
      } catch (JsonException) { chatPrefix = ""; }
      String chatSuffix = "";
      try {
        chatSuffix = bubbleConfig.get().getString("chatSuffix", "");
      } catch (JsonException) { chatSuffix = ""; }
      String modifiedMessage = chatPrefix + message + chatSuffix;
      m_pendingChatActions.append(SayChatAction{sourceEntityId.value(entityId()), modifiedMessage, mouthPosition(), *bubbleConfig});
    } else {
      m_pendingChatActions.append(SayChatAction{sourceEntityId.value(entityId()), message, mouthPosition()});
    }
  }
}

void Player::addChatMessageCallback(String const& message) {
  starAssert(!isSlave());
  m_chatMessage = message;
  m_chatMessageUpdated = true;
  m_chatMessageChanged = true;

  Maybe<String> chatPortrait;
  try {
    chatPortrait = m_chatBubbleConfig.getString("chatPortrait");
  } catch (JsonException) {
    chatPortrait = {};
  }
  bool skipChatBubble = false;
  try {
    skipChatBubble = m_chatBubbleConfig.getBool("skipChatBubble");
  } catch (JsonException) {
    skipChatBubble = false;
  }

  String chatPrefix = "";
  try {
    chatPrefix = m_chatBubbleConfig.getString("chatPrefix", "");
  } catch (JsonException) { chatPrefix = ""; }
  String chatSuffix = "";
  try {
    chatSuffix = m_chatBubbleConfig.getString("chatSuffix", "");
  } catch (JsonException) { chatSuffix = ""; }
  String modifiedMessage = chatPrefix + message + chatSuffix;

  if (!skipChatBubble) {
    if (chatPortrait) {
      m_pendingChatActions.append(PortraitChatAction{entityId(), chatPortrait.get(), modifiedMessage, mouthPosition(), m_chatBubbleConfig});
    } else {
      m_pendingChatActions.append(SayChatAction{entityId(), modifiedMessage, mouthPosition(), m_chatBubbleConfig});
    }
  }
}

// FezzedOne: Sets the chat config.
void Player::setChatBubbleConfig(Maybe<Json> const& bubbleConfig) {
  if (bubbleConfig) {
    JsonObject newBubbleConfig;
    bool isValid = true;
    try {
      newBubbleConfig = bubbleConfig.get().toObject();
    } catch (JsonException const& e) {
      Logger::error("setChatBubbleConfig: Invalid JSON: {}", e.what());
      isValid = false;
    }
    if (isValid)
      m_chatBubbleConfig = newBubbleConfig;
  }
}

Json Player::getChatBubbleConfig() {
  return m_chatBubbleConfig;
}

void Player::addEmote(HumanoidEmote const& emote, Maybe<float> emoteCooldown) {
  starAssert(!isSlave());
  m_emoteState = emote;
  m_emoteCooldownTimer = emoteCooldown.value(m_emoteCooldown);
}

pair<HumanoidEmote, float> Player::currentEmote() const {
  return make_pair(m_emoteState, m_emoteCooldownTimer);
}

List<ChatAction> Player::pullPendingChatActions() {
  return take(m_pendingChatActions);
}

Maybe<String> Player::inspectionLogName() const {
  auto identifier = uniqueId();
  if (String* str = identifier.ptr()) {
    auto hash = XXH3_128bits(str->utf8Ptr(), str->utf8Size());
    return String("Player #") + hexEncode((const char*)&hash, sizeof(hash));
  }
  return identifier;
}

Maybe<String> Player::inspectionDescription(String const& species) const {
  return m_description;
}

float Player::beamGunRadius() const {
  return m_tools->beamGunRadius();
}

bool Player::instrumentPlaying() {
  return m_songbook->instrumentPlaying();
}

void Player::instrumentEquipped(String const& instrumentKind) {
  if (canUseTool())
    m_songbook->keepalive(instrumentKind, mouthPosition());
}

void Player::interact(InteractAction const& action) {
  starAssert(!isSlave());
  m_pendingInteractActions.append(RpcPromise<InteractAction>::createFulfilled(action));
}

void Player::addEffectEmitters(StringSet const& emitters) {
  starAssert(!isSlave());
  m_effectEmitter->addEffectSources("normal", emitters);
}

void Player::requestEmote(String const& emote) {
  auto state = HumanoidEmoteNames.getLeft(emote);
  if (state != HumanoidEmote::Idle && (m_emoteState == state || m_emoteState == HumanoidEmote::Idle || m_emoteState == HumanoidEmote::Blink))
    addEmote(state);
}

void Player::requestEmote(String const& emote, Maybe<float> cooldownTime) {
  auto state = HumanoidEmoteNames.getLeft(emote);
  if (state != HumanoidEmote::Idle && (m_emoteState == state || m_emoteState == HumanoidEmote::Idle || m_emoteState == HumanoidEmote::Blink))
    addEmote(state, cooldownTime);
}

ActorMovementController* Player::movementController() {
  return m_movementController.get();
}

StatusController* Player::statusController() {
  return m_statusController.get();
}

List<PhysicsForceRegion> Player::forceRegions() const {
  return m_tools->forceRegions();
}


StatusControllerPtr Player::statusControllerPtr() {
  return m_statusController;
}

ActorMovementControllerPtr Player::movementControllerPtr() {
  return m_movementController;
}

PlayerConfigPtr Player::config() {
  return m_config;
}

SongbookPtr Player::songbook() const {
  return m_songbook;
}

QuestManagerPtr Player::questManager() const {
  return m_questManager;
}

Json Player::diskStore() {
  JsonObject genericScriptStorage;
  for (auto& p : m_genericScriptContexts) {
    auto scriptStorage = p.second->getScriptStorage();
    if (!scriptStorage.empty())
      genericScriptStorage[p.first] = std::move(scriptStorage);
  }

  /* FezzedOne: Save the player's `"xSbProperties"` to the file. */
  Json xSbProperties = JsonObject{
      {"ignoreExternalWarps", m_ignoreExternalWarps},
      {"ignoreExternalRadioMessages", m_ignoreExternalRadioMessages},
      {"ignoreExternalCinematics", m_ignoreExternalCinematics},
      {"ignoreAllPhysicsEntities", m_ignoreAllPhysicsEntities},
      {"ignoreTechOverrides", m_ignoreAllTechOverrides},
      {"ignoreNudity", m_ignoreForcedNudity},
      {"damageTeamOverridden", m_damageTeamOverridden},
      {"overreach", m_canReachAll},
      {"inWorldRespawn", m_alwaysRespawnInWorld},
      {"fastWarp", m_fastRespawn},
      {"ignoreShipUpdates", m_ignoreShipUpdates},
      {"ignoreItemPickups", m_ignoreItemPickups},
      {"chatConfig", m_chatBubbleConfig}};

  return JsonObject{
      {"uuid", *uniqueId()},
      {"xSbProperties", xSbProperties},
      {"description", m_description},
      {"modeType", PlayerModeNames.getRight(m_modeType)},
      {"shipUpgrades", m_shipUpgrades.toJson()},
      {"blueprints", m_blueprints->toJson()},
      {"universeMap", m_universeMap->toJson()},
      {"codexes", m_codexes->toJson()},
      {"techs", m_techs->toJson()},
      {"identity", m_identity.toJson()},
      {"team", getTeam().toJson()},
      {"inventory", m_inventory->store()},
      {"movementController", m_movementController->storeState()},
      {"techController", m_techController->diskStore()},
      {"statusController", m_statusController->diskStore()},
      {"log", m_log->toJson()},
      {"aiState", m_aiState.toJson()},
      {"quests", m_questManager->diskStore()},
      {"companions", m_companions->diskStore()},
      {"deployment", m_deployment->diskStore()},
      {"genericProperties", m_genericProperties},
      {"genericScriptStorage", genericScriptStorage},
  };
}

ByteArray Player::netStore() {
  DataStreamBuffer ds;

  ds.write(*uniqueId());
  ds.write(m_description);
  ds.write(m_modeType);
  ds.write(m_humanoid ? m_humanoid->netIdentity() : m_identity);

  return ds.data();
}

void Player::finalizeCreation() {
  m_blueprints = make_shared<PlayerBlueprints>();
  m_techs = make_shared<PlayerTech>();

  auto itemDatabase = Root::singleton().itemDatabase();
  for (auto const& descriptor : m_config->defaultItems)
    m_inventory->addItems(itemDatabase->item(descriptor));

  for (auto const& descriptor : Root::singleton().speciesDatabase()->species(m_identity.species)->defaultItems())
    m_inventory->addItems(itemDatabase->item(descriptor));

  for (auto const& descriptor : m_config->defaultBlueprints)
    m_blueprints->add(descriptor);

  for (auto const& descriptor : Root::singleton().speciesDatabase()->species(m_identity.species)->defaultBlueprints())
    m_blueprints->add(descriptor);

  refreshEquipment();

  m_state = State::Idle;
  m_emoteState = HumanoidEmote::Idle;

  m_statusController->setPersistentEffects("armor", m_armor->statusEffects());
  m_statusController->setPersistentEffects("tools", m_tools->statusEffects());
  m_statusController->resetAllResources();

  m_effectEmitter->reset();

  m_description = strf("This {} seems to have nothing to say for {}self.",
      m_identity.gender == Gender::Male ? "guy" : "gal",
      m_identity.gender == Gender::Male ? "him" : "her");
}

bool Player::invisible() const {
  return m_statusController->statPositive("invisible");
}

void Player::animatePortrait(float dt) {
  m_humanoid->animate(dt);
  if (m_emoteCooldownTimer) {
    m_emoteCooldownTimer -= dt;
    if (m_emoteCooldownTimer <= 0) {
      m_emoteCooldownTimer = 0;
      m_emoteState = HumanoidEmote::Idle;
    }
  }
  m_humanoid->setEmoteState(m_emoteState);
}

bool Player::isOutside() {
  if (!inWorld())
    return false;
  return !world()->isUnderground(position()) && !world()->tileIsOccupied(Vec2I::floor(mouthPosition()), TileLayer::Background);
}

void Player::dropSelectedItems(function<bool(ItemPtr)> filter) {
  if (!world())
    return;

  m_inventory->forEveryItem([&](InventorySlot const&, ItemPtr& item) {
    if (item && (!filter || filter(item)))
      world()->addEntity(ItemDrop::throwDrop(take(item), position(), velocity(), Vec2F::withAngle(Random::randf(-Constants::pi, Constants::pi)), true));
  });
}

void Player::dropEverything() {
  dropSelectedItems({});
}

bool Player::isPermaDead() const {
  if (!isDead())
    return false;
  return modeConfig().permadeath;
}

bool Player::interruptRadioMessage() {
  if (m_interruptRadioMessage) {
    m_interruptRadioMessage = false;
    return true;
  }
  return false;
}

Maybe<RadioMessage> Player::pullPendingRadioMessage() {
  if (m_pendingRadioMessages.count()) {
    if (m_pendingRadioMessages.at(0).unique)
      m_log->addRadioMessage(m_pendingRadioMessages.at(0).messageId);
    return m_pendingRadioMessages.takeFirst();
  }
  return {};
}

void Player::queueRadioMessage(Json const& messageConfig, float delay) {
  RadioMessage message;
  try {
    message = Root::singleton().radioMessageDatabase()->createRadioMessage(messageConfig);

    if (message.type == RadioMessageType::Tutorial && !Root::singleton().configuration()->get("tutorialMessages").toBool())
      return;

    // non-absolute portrait image paths are assumed to be a frame name within the player's species-specific AI
    if (!message.portraitImage.empty() && message.portraitImage[0] != '/')
      message.portraitImage = Root::singleton().aiDatabase()->portraitImage(species(), message.portraitImage);
  } catch (RadioMessageDatabaseException const& e) {
    Logger::error("Couldn't queue radio message '{}': {}", messageConfig, e.what());
    return;
  }

  if (m_log->radioMessages().contains(message.messageId)) {
    return;
  } else {
    if (message.type == RadioMessageType::Mission) {
      if (m_missionRadioMessages.contains(message.messageId))
        return;
      else
        m_missionRadioMessages.add(message.messageId);
    }

    for (RadioMessage const& pendingMessage : m_pendingRadioMessages) {
      if (pendingMessage.messageId == message.messageId)
        return;
    }
    for (auto& delayedMessagePair : m_delayedRadioMessages) {
      if (delayedMessagePair.second.messageId == message.messageId) {
        if (delay == 0)
          delayedMessagePair.first.setDone();
        return;
      }
    }
  }

  if (delay > 0) {
    m_delayedRadioMessages.append(pair<GameTimer, RadioMessage>{GameTimer(delay), message});
  } else {
    queueRadioMessage(message);
  }
}

void Player::queueRadioMessage(RadioMessage message) {
  if (message.important) {
    m_interruptRadioMessage = true;
    m_pendingRadioMessages.prepend(message);
  } else {
    m_pendingRadioMessages.append(message);
  }
}

Maybe<Json> Player::pullPendingCinematic() {
  if (m_pendingCinematic && m_pendingCinematic->isType(Json::Type::String))
    m_log->addCinematic(m_pendingCinematic->toString());
  return take(m_pendingCinematic);
}

void Player::setPendingCinematic(Json const& cinematic, bool unique) {
  if (unique && cinematic.isType(Json::Type::String) && m_log->cinematics().contains(cinematic.toString()))
    return;
  m_pendingCinematic = cinematic;
}

void Player::setInCinematic(bool inCinematic) {
  if (inCinematic)
    m_statusController->setPersistentEffects("cinematic", m_inCinematicStatusEffects);
  else
    m_statusController->setPersistentEffects("cinematic", {});
}

Maybe<pair<Maybe<StringList>, float>> Player::pullPendingAltMusic() {
  if (m_pendingAltMusic)
    return m_pendingAltMusic.take();
  return {};
}

Maybe<PlayerWarpRequest> Player::pullPendingWarp() {
  if (m_pendingWarp)
    return m_pendingWarp.take();
  return {};
}

void Player::setPendingWarp(String const& action, Maybe<String> const& animation, bool deploy) {
  m_pendingWarp = PlayerWarpRequest{action, animation, deploy};
}

// FezzedOne: Function to toggle ignoring external warp requests.
void Player::setExternalWarpsIgnored(bool ignored) {
  m_ignoreExternalWarps = ignored;
}

// FezzedOne: Function to toggle ignoring external radio messages.
void Player::setExternalRadioMessagesIgnored(bool ignored) {
  m_ignoreExternalRadioMessages = ignored;
}

// FezzedOne: Function to toggle ignoring external cinematics.
void Player::setExternalCinematicsIgnored(bool ignored) {
  m_ignoreExternalCinematics = ignored;
}

// FezzedOne: Function to toggle ignoring all physics entities.
void Player::setPhysicsEntitiesIgnored(bool ignored) {
  m_ignoreAllPhysicsEntities = ignored;
  m_movementController->tickIgnorePhysicsEntities(m_ignoreAllPhysicsEntities);
}

// FezzedOne: Function to toggle ignoring `"nude"` stats.
void Player::setForcedNudityIgnored(bool ignored) {
  m_ignoreForcedNudity = ignored;
}

// FezzedOne: Function to toggle ignoring tech overrides.
void Player::setTechOverridesIgnored(bool ignored) {
  m_ignoreAllTechOverrides = ignored;
}

// FezzedOne: Function to toggle universal entity interaction.
void Player::setCanReachAll(bool newSetting) {
  m_canReachAll = newSetting;
}

// FezzedOne: Function to toggle fast respawning and warping.
void Player::setFastRespawn(bool newSetting) {
  m_fastRespawn = newSetting;
}

// FezzedOne: Function to toggle fast respawning and warping.
void Player::setAlwaysRespawnInWorld(bool newSetting) {
  m_alwaysRespawnInWorld = newSetting;
}

// FezzedOne: Function to toggle ship protection.
void Player::setIgnoreShipUpdates(bool ignore) {
  m_ignoreShipUpdates = ignore;
}

// FezzedOne: Function to toggle ship protection.
void Player::setIgnoreItemPickups(bool ignore) {
  m_ignoreItemPickups = ignore;
}

// FezzedOne: Function to get whether external warp requests are ignored.
bool Player::externalWarpsIgnored() const {
  return m_ignoreExternalWarps;
}

// FezzedOne: Function to get whether external radio messages are ignored.
bool Player::externalRadioMessagesIgnored() const {
  return m_ignoreExternalRadioMessages;
}

// FezzedOne: Function to get whether external cinematics are ignored.
bool Player::externalCinematicsIgnored() const {
  return m_ignoreExternalCinematics;
}

// FezzedOne: Function to get whether all physics entities are ignored.
bool Player::physicsEntitiesIgnored() const {
  return m_ignoreAllPhysicsEntities;
}

// FezzedOne: Function to get whether all physics entities are ignored.
bool Player::forcedNudityIgnored() const {
  return m_ignoreForcedNudity;
}

// FezzedOne: Function to get whether the player is set to respawn quickly and with no animation.
bool Player::fastRespawn() const {
  return m_fastRespawn;
}

// FezzedOne: Function to get whether the player is set to always respawn in the world he or she died in.
bool Player::alwaysRespawnInWorld() const {
  return m_alwaysRespawnInWorld;
}

// FezzedOne: Function to get whether the player's shipworld is ignoring updates.
bool Player::shipUpdatesIgnored() const {
  return m_ignoreShipUpdates;
}

// FezzedOne: Function to get whether the player is ignoring item pickups (like Terraria's Encumbering Stone).
bool Player::ignoreItemPickups() const {
  return m_ignoreItemPickups;
}

// FezzedOne: Function to get whether the player is ignoring world tech overrides.
bool Player::ignoreAllTechOverrides() const {
  return m_ignoreAllTechOverrides;
}

// FezzedOne: Function to get whether the player is ignoring world tech overrides.
bool Player::canReachAll() const {
  return m_canReachAll;
}

// FezzedOne: Interface to check whether the player's damage team was overridden.
bool Player::damageTeamOverridden() const {
  return m_damageTeamOverridden;
}

void Player::setDamageTeam(EntityDamageTeam newTeam) {
  m_damageTeamOverridden = true;
  setTeam(newTeam);
}

void Player::setDamageTeam() {
  m_damageTeamOverridden = false;
}

// FezzedOne: Sets whether player tool usage is suppressed. Resets on `Player::init`.
void Player::setToolUsageSuppressed(Maybe<bool> suppressed) {
  if (suppressed)
    m_toolUsageSuppressed = *suppressed;
  else
    m_toolUsageSuppressed = false;
  // Make sure the tech controller's net state is properly synced.
  m_techController->setPlayerToolUsageSuppressed(m_toolUsageSuppressed);
}

// FezzedOne: Sets the player's humanoid override state. Resets every tick.
void Player::setOverrideState(Maybe<Humanoid::State> overrideState) {
  m_overrideState = overrideState;
}

Maybe<pair<Json, RpcPromiseKeeper<Json>>> Player::pullPendingConfirmation() {
  if (m_pendingConfirmations.count() > 0)
    return m_pendingConfirmations.takeFirst();
  return {};
}

void Player::queueConfirmation(Json const& dialogConfig, RpcPromiseKeeper<Json> const& resultPromise) {
  m_pendingConfirmations.append(make_pair(dialogConfig, resultPromise));
}

AiState const& Player::aiState() const {
  return m_aiState;
}

AiState& Player::aiState() {
  return m_aiState;
}

bool Player::inspecting() const {
  return is<InspectionTool>(m_tools->primaryHandItem()) || is<InspectionTool>(m_tools->altHandItem());
}

EntityHighlightEffect Player::inspectionHighlight(InspectableEntityPtr const& inspectableEntity) const {
  auto inspectionTool = as<InspectionTool>(m_tools->primaryHandItem());
  if (!inspectionTool)
    inspectionTool = as<InspectionTool>(m_tools->altHandItem());

  if (!inspectionTool)
    return EntityHighlightEffect();

  if (auto name = inspectableEntity->inspectionLogName()) {
    auto ehe = EntityHighlightEffect();
    ehe.level = inspectionTool->inspectionHighlightLevel(inspectableEntity);
    if (ehe.level > 0) {
      if (m_interestingObjects.contains(*name))
        ehe.type = EntityHighlightEffectType::Interesting;
      else if (m_log->scannedObjects().contains(*name))
        ehe.type = EntityHighlightEffectType::Inspected;
      else
        ehe.type = EntityHighlightEffectType::Inspectable;
    }
    return ehe;
  }

  return EntityHighlightEffect();
}

void Player::overrideCameraPosition(Maybe<Vec2F> newPosition) {
  m_cameraOverridePosition = newPosition;
}

Vec2F Player::cameraPosition() {
  if (inWorld()) {
    if (m_cameraOverridePosition) { // FezzedOne: If the camera position is overridden, return the overridden position.
      return *m_cameraOverridePosition;
    }

    if (auto loungeAnchor = as<LoungeAnchor>(m_movementController->entityAnchor())) {
      if (loungeAnchor->cameraFocus) {
        if (auto anchoredEntity = world()->entity(m_movementController->anchorState()->entityId))
          return anchoredEntity->position();
      }
    }

    if (m_cameraFocusEntity) {
      if (auto focusedEntity = world()->entity(*m_cameraFocusEntity))
        return focusedEntity->position();
      else
        m_cameraFocusEntity = {};
    }
  }
  return position();
}

NetworkedAnimatorPtr Player::effectsAnimator() {
  return m_effectsAnimator;
}

const String secretProprefix = "\0JsonProperty\0"s;

Maybe<StringView> Player::getSecretPropertyView(String const& name) const {
  if (auto tag = m_effectsAnimator->globalTagPtr(secretProprefix + name)) {
    auto& view = tag->utf8();
    DataStreamExternalBuffer buffer(view.data(), view.size());
    try {
      uint8_t typeIndex = buffer.read<uint8_t>() - 1;
      if ((Json::Type)typeIndex == Json::Type::String) {
        size_t len = buffer.readVlqU();
        size_t pos = buffer.pos();
        if (pos + len == buffer.size())
          return StringView(buffer.ptr() + pos, len);
      }
    } catch (StarException const& e) {}
  }

  return {};
}

Json Player::getSecretProperty(String const& name, Json defaultValue) const {
  if (auto tag = m_effectsAnimator->globalTagPtr(secretProprefix + name)) {
    DataStreamExternalBuffer buffer(tag->utf8Ptr(), tag->utf8Size());
    try {
      return buffer.read<Json>();
    } catch (StarException const& e) { Logger::error("Exception reading secret player property '{}': {}", name, e.what()); }
  }

  return std::move(defaultValue);
}

String const* Player::getSecretPropertyPtr(String const& name) const {
  return m_effectsAnimator->globalTagPtr(secretProprefix + name);
}

void Player::setSecretProperty(String const& name, Json const& value) {
  if (value) {
    DataStreamBuffer ds;
    ds.write(value);
    auto& data = ds.data();
    m_effectsAnimator->setGlobalTag(secretProprefix + name, String(data.ptr(), data.size()));
  } else
    m_effectsAnimator->removeGlobalTag(secretProprefix + name);
}

void Player::setNetArmorSecret(uint8_t cosmeticSlot, ArmorItemPtr const& armor) { // FezzedOne: Called in the ArmorWearer whenever armour is updated.
  String const& slotName = strf("cosmetic{}", ((cosmeticSlot + 4) % 12) + 1);     // FezzedOne: Hacky workaround for OpenStarbound's weird legs/chest layering
  // that should make it less likely for overlays to be covered up by an item in the stock chest slot on OpenStarbound.

  auto const& playerIdentity = m_identity;

  const StringMap<String> identityTags{
      {"species", playerIdentity.species},
      {"imagePath", playerIdentity.imagePath ? *playerIdentity.imagePath : playerIdentity.species},
      {"gender", playerIdentity.gender == Gender::Male ? "male" : "female"},
      {"bodyDirectives", playerIdentity.bodyDirectives.string()},
      {"emoteDirectives", playerIdentity.bodyDirectives.string()},
      {"hairGroup", playerIdentity.hairGroup},
      {"hairType", playerIdentity.hairType},
      {"hairDirectives", playerIdentity.hairDirectives.string()},
      {"facialHairGroup", playerIdentity.facialHairGroup},
      {"facialHairType", playerIdentity.facialHairType},
      {"facialHairDirectives", playerIdentity.facialHairDirectives.string()},
      {"facialMaskGroup", playerIdentity.facialMaskGroup},
      {"facialMaskType", playerIdentity.facialMaskType},
      {"facialMaskDirectives", playerIdentity.facialMaskDirectives.string()},
      {"personalityIdle", playerIdentity.personality.idle},
      {"personalityArmIdle", playerIdentity.personality.armIdle},
      {"headOffsetX", strf("{}", playerIdentity.personality.headOffset[0])},
      {"headOffsetY", strf("{}", playerIdentity.personality.headOffset[1])},
      {"armOffsetX", strf("{}", playerIdentity.personality.armOffset[0])},
      {"armOffsetY", strf("{}", playerIdentity.personality.armOffset[1])},
      {"color", Color::rgba(playerIdentity.color).toHex()}};

  if (!m_startedNetworkingCosmetics) {
    setSecretProperty("armorWearer.isXStarbound", true);
  }

  if (armor) {
    auto armourItem = ArmorWearer::setUpArmourItemNetworking(identityTags, armor);
    setSecretProperty(strf("armorWearer.{}.data", slotName), armourItem.diskStore());
  }
  if (!m_startedNetworkingCosmetics) {
    setSecretProperty("armorWearer.replicating", true);
    m_startedNetworkingCosmetics = true;
  }
  setSecretProperty(strf("armorWearer.{}.version", slotName), ++m_armorSecretNetVersions[cosmeticSlot]);
}

Array<ArmorItemPtr, 12> const& Player::getNetArmorSecrets() {
  if (isSlave() && getSecretPropertyPtr("armorWearer.replicating") && !getSecretPropertyPtr("armorWearer.isXStarbound")) {
    auto itemDatabase = Root::singleton().itemDatabase();

    for (uint8_t i = 0; i != 12; ++i) {
      String const& slotName = strf("cosmetic{}", i + 1);
      uint64_t& curVersion = m_armorSecretNetVersions[i];
      uint64_t newVersion = 0;
      if (auto jVersion = getSecretProperty(strf("armorWearer.{}.version", slotName), 0); jVersion.isType(Json::Type::Int))
        newVersion = jVersion.toUInt();
      if (newVersion > curVersion) {
        m_pulledCosmeticUpdate = true;
        curVersion = newVersion;
        ArmorItemPtr item = nullptr;
        if (auto jData = getSecretProperty(strf("armorWearer.{}.data", slotName)))
          itemDatabase->diskLoad(jData, item);
        m_openSbCosmetics[i] = item;
      }
    }
  }

  return m_openSbCosmetics;
}

bool Player::pulledCosmeticUpdate() {
  bool pulledUpdate = m_pulledCosmeticUpdate;
  m_pulledCosmeticUpdate = false;
  return pulledUpdate;
}

} // namespace Star
