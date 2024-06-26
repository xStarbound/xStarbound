#include "StarUniverseClient.hpp"
#include "StarEntityMap.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarVersion.hpp"
#include "StarRoot.hpp"
#include "StarConfiguration.hpp"
#include "StarProjectileDatabase.hpp"
#include "StarPlayerStorage.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerLog.hpp"
#include "StarAssets.hpp"
#include "StarTime.hpp"
#include "StarNetPackets.hpp"
#include "StarTcp.hpp"
#include "StarWorldClient.hpp"
#include "StarSystemWorldClient.hpp"
#include "StarClientContext.hpp"
#include "StarTeamClient.hpp"
#include "StarSha256.hpp"
#include "StarEncode.hpp"
#include "StarPlayerCodexes.hpp"
#include "StarQuestManager.hpp"
#include "StarPlayerUniverseMap.hpp"
#include "StarWorldTemplate.hpp"
#include "StarCelestialLuaBindings.hpp"
#include "StarStatusController.hpp"
#include "scripting/StarWorldLuaBindings.hpp"

namespace Star {

UniverseClient::UniverseClient(PlayerStoragePtr playerStorage, StatisticsPtr statistics) {
  m_storageTriggerDeadline = 0;
  m_playerStorage = move(playerStorage);
  m_statistics = move(statistics);
  m_pause = false;
  m_playerToSwitchTo = {};
  m_playersToLoad = {};
  m_loadedPlayers = {};
  reset();
}

UniverseClient::~UniverseClient() {
  // FezzedOne: Tell the Lua root to shut down properly.
  if (m_luaRoot) {
    m_luaRoot->shutdown();
  }
  disconnect();
}

void UniverseClient::setMainPlayer(PlayerPtr player) {
  if (isConnected())
    throw StarException("Cannot call UniverseClient::setMainPlayer while connected");

  bool selectedFromTitleScreen = false;

  if (m_mainPlayer) {
    m_playerStorage->savePlayer(m_mainPlayer);
    m_mainPlayer->setClientContext({});
    m_mainPlayer->setStatistics({});
    m_mainPlayerUuid = Uuid();

    for (auto loadedPlayer : m_loadedPlayers) {
      if (auto& player = loadedPlayer.second.ptr) {
        m_playerStorage->savePlayer(player);
        player->setClientContext({});
        player->setStatistics({});
      }
    }

    m_loadedPlayers = {};
  } else {
    selectedFromTitleScreen = true;
  }

  m_mainPlayer = player;

  if (m_mainPlayer) {
    m_mainPlayerUuid = m_mainPlayer->uuid();
    m_mainPlayer->setClientContext(m_clientContext);
    m_mainPlayer->setStatistics(m_statistics);
    m_mainPlayer->setUniverseClient(this);
    m_playerStorage->backupCycle(m_mainPlayer->uuid());
    m_playerStorage->savePlayer(m_mainPlayer);
    m_playerStorage->moveToFront(m_mainPlayer->uuid());
    if (selectedFromTitleScreen)
      m_shouldUpdateMainPlayerShip = m_mainPlayer->shipUpdatesIgnored();
  }    
}

PlayerPtr UniverseClient::mainPlayer() const {
  return m_mainPlayer;
}

Maybe<String> UniverseClient::connect(UniverseConnection connection, bool allowAssetsMismatch, String const& account, String const& password) {
  auto& root = Root::singleton();
  auto assets = root.assets();

  reset();
  m_disconnectReason = {};

  if (!m_mainPlayer)
    throw StarException("Cannot call UniverseClient::connect with no main player");

  for (auto uuid : m_playerStorage->playerUuids()) {
    if (uuid == m_mainPlayerUuid) {
      m_loadedPlayers[uuid] = {true, m_mainPlayer};
    } else {
      m_playerStorage->backupCycle(uuid);
      auto playerToLoad = m_playerStorage->loadPlayer(uuid);
      m_playerStorage->savePlayer(playerToLoad);
      m_loadedPlayers[uuid] = {false, playerToLoad};
    }
  }

  size_t playerCount = m_playerStorage->playerCount();
  Logger::info("UniverseClient: Pre-loaded {} saved {}",
    playerCount,
    playerCount == 1 ? "player" : "players");

  unsigned timeout = assets->json("/client.config:serverConnectTimeout").toUInt();

  {
    auto protocolRequest = make_shared<ProtocolRequestPacket>(StarProtocolVersion);
    protocolRequest->setCompressionMode(PacketCompressionMode::Enabled);
    // Signal that we're xStarbound/OpenSB.
    connection.pushSingle(protocolRequest);
  }
  connection.sendAll(timeout);
  connection.receiveAny(timeout);

  auto protocolResponsePacket = as<ProtocolResponsePacket>(connection.pullSingle());
  if (!protocolResponsePacket)
    return String("Join failed! Timeout while establishing connection.");
  else if (!protocolResponsePacket->allowed)
    return String(strf("Join failed! Server does not support connections with protocol version {}", StarProtocolVersion));

  m_legacyServer = protocolResponsePacket->compressionMode() != PacketCompressionMode::Enabled; // True if server is vanilla
  connection.setLegacy(m_legacyServer);
  connection.pushSingle(make_shared<ClientConnectPacket>(Root::singleton().assets()->digest(), allowAssetsMismatch, m_mainPlayer->uuid(), m_mainPlayer->name(),
      m_mainPlayer->species(), m_playerStorage->loadShipData(m_mainPlayer->uuid()), m_mainPlayer->shipUpgrades(),
      m_mainPlayer->log()->introComplete(), account));
  connection.sendAll(timeout);

  connection.receiveAny(timeout);
  auto packet = connection.pullSingle();
  if (auto challenge = as<HandshakeChallengePacket>(packet)) {
    Logger::info("UniverseClient: Sending handshake response");
    ByteArray passAccountSalt = (password + account).utf8Bytes();
    passAccountSalt.append(challenge->passwordSalt);
    ByteArray passHash = Star::sha256(passAccountSalt);

    connection.pushSingle(make_shared<HandshakeResponsePacket>(passHash));
    connection.sendAll(timeout);

    connection.receiveAny(timeout);
    packet = connection.pullSingle();
  }

  if (auto success = as<ConnectSuccessPacket>(packet)) {
    m_universeClock = make_shared<Clock>();
    m_clientContext = make_shared<ClientContext>(success->serverUuid, m_mainPlayer->uuid());
    m_teamClient = make_shared<TeamClient>(m_mainPlayer, m_clientContext);

    m_mainPlayer->setClientContext(m_clientContext);
    m_mainPlayer->setStatistics(m_statistics);
    m_mainPlayer->setUniverseClient(this);

    size_t loadedCount = m_loadedPlayers.size();
    Logger::info("UniverseClient: Setting up {} pre-loaded {}",
      loadedCount,
      loadedCount == 1 ? "player" : "players");

    for (auto& loadedPlayer : m_loadedPlayers) {
      auto& player = loadedPlayer.second.ptr;
      player->setClientContext(m_clientContext);
      player->setStatistics(m_statistics);
      player->setUniverseClient(this);
    }

    // FezzedOne: I'll never know why the gods-damned world client couldn't access the universe client to begin with.
    m_worldClient = make_shared<WorldClient>(m_mainPlayer, this); 
    for (auto& pair : m_luaCallbacks)
      m_worldClient->setLuaCallbacks(pair.first, pair.second);

    m_connection = move(connection);
    m_celestialDatabase = make_shared<CelestialSlaveDatabase>(move(success->celestialInformation));
    m_systemWorldClient = make_shared<SystemWorldClient>(m_universeClock, m_celestialDatabase, m_mainPlayer->universeMap());

    size_t playerCount = m_playerStorage->playerCount();

    Logger::info("UniverseClient: Joined {} server as client {}",
      m_legacyServer ? "Starbound" : "xStarbound/OpenSB",
      success->clientId);
    return {};
  } else if (auto failure = as<ConnectFailurePacket>(packet)) {
    Logger::error("UniverseClient: Join failed: {}", failure->reason);
    return failure->reason;
  } else {
    Logger::error("UniverseClient: Join failed! No server response received");
    return String("Join failed! No server response received");
  }
}

bool UniverseClient::isConnected() const {
  return m_connection && m_connection->isOpen();
}

void UniverseClient::disconnect() {
  auto assets = Root::singleton().assets();
  int timeout = assets->json("/client.config:serverDisconnectTimeout").toInt();

  if (isConnected()) {
    Logger::info("UniverseClient: Client disconnecting...");
    m_connection->pushSingle(make_shared<ClientDisconnectRequestPacket>());
  }

  // Try to handle all the shutdown packets before returning.
  while (m_connection) {
    m_connection->sendAll(timeout);
    if (m_connection->receiveAny(timeout))
      handlePackets(std::move(m_connection->pull()));
    else
      break;
  }

  reset();
  m_mainPlayer = {};
  m_loadedPlayers = {};
  size_t playerCount = m_playerStorage->playerCount();
  Logger::info("UniverseClient: Unloaded {} {}",
    playerCount,
    playerCount == 1 ? "player" : "players");
}

Maybe<String> UniverseClient::disconnectReason() const {
  return m_disconnectReason;
}

WorldClientPtr UniverseClient::worldClient() const {
  return m_worldClient;
}

SystemWorldClientPtr UniverseClient::systemWorldClient() const {
  return m_systemWorldClient;
}

void UniverseClient::update(float dt) {
  auto assets = Root::singleton().assets();

  if (!isConnected())
    return;

  if (m_mainPlayer && playerIsOriginal())
    m_shouldUpdateMainPlayerShip = !m_mainPlayer->shipUpdatesIgnored();

  if (!m_warping && !m_pendingWarp) {
    if (auto playerWarp = m_mainPlayer->pullPendingWarp())
      warpPlayer(parseWarpAction(playerWarp->action), (bool)playerWarp->animation, playerWarp->animation.value("default"), playerWarp->deploy);
  }

  if (m_pendingWarp) {
    if (m_mainPlayer->fastRespawn())
      m_warpDelay.setDone();
    if ((m_warping && !m_mainPlayer->isTeleportingOut()) || (!m_warping && m_warpDelay.tick(dt))) {
      m_connection->pushSingle(make_shared<PlayerWarpPacket>(take(m_pendingWarp), m_mainPlayer->isDeploying()));
      m_warpDelay.reset();
      if (m_warping) {
        m_warpCinemaCancelTimer = GameTimer(assets->json("/client.config:playerWarpCinemaMinimumTime").toFloat());

        bool isDeploying = m_mainPlayer->isDeploying();
        String cinematicJsonPath = isDeploying ? "/client.config:deployCinematic" : "/client.config:warpCinematic";
        String cinematicAssetPath = assets->json(cinematicJsonPath).toString()
        .replaceTags(StringMap<String>{{"species", m_mainPlayer->species()}});

        if (assets->assetExists(cinematicAssetPath)) {
          Json cinematic = jsonMerge(assets->json(cinematicJsonPath + "Base"), assets->json(cinematicAssetPath));
          if (!(m_mainPlayer->alwaysRespawnInWorld() || m_mainPlayer->fastRespawn()))
            m_mainPlayer->setPendingCinematic(cinematic);
        }
      }
    }
  }

  // Don't cancel the warp cinema until at LEAST the
  // playerWarpCinemaMinimumTime has passed, even if warping is faster than
  // that. Unless the player has fast respawn enabled, of course.
  if (m_warpCinemaCancelTimer) {
    m_warpCinemaCancelTimer->tick();
    if (m_mainPlayer->fastRespawn())
      m_warpCinemaCancelTimer->setDone();
    if (m_warpCinemaCancelTimer->ready() && !m_warping) {
      m_warpCinemaCancelTimer = {};
      m_mainPlayer->setPendingCinematic(Json());
      m_mainPlayer->teleportIn();
    }
  }

  m_connection->receive();
  handlePackets(std::move(m_connection->pull()));

  if (!isConnected())
    return;

  LogMap::set("universe_time_client", m_universeClock->time());

  m_statistics->update();

  if (!m_pause) {
    m_worldClient->update(dt);
    for (auto& p : m_scriptContexts)
      p.second->update();
  }
  m_connection->push(m_worldClient->getOutgoingPackets());

  if (!m_pause)
    m_systemWorldClient->update(dt);
  m_connection->push(m_systemWorldClient->pullOutgoingPackets());

  m_teamClient->update();

  auto contextUpdate = m_clientContext->writeUpdate();
  if (!contextUpdate.empty())
    m_connection->pushSingle(make_shared<ClientContextUpdatePacket>(move(contextUpdate)));

  auto celestialRequests = m_celestialDatabase->pullRequests();
  if (!celestialRequests.empty())
    m_connection->pushSingle(make_shared<CelestialRequestPacket>(move(celestialRequests)));

  m_connection->send();

  if (Time::monotonicMilliseconds() >= m_storageTriggerDeadline) {
    // if (m_mainPlayer) {
    //   m_playerStorage->savePlayer(m_mainPlayer);
    //   m_playerStorage->moveToFront(m_mainPlayer->uuid());
    // }

    for (auto& player : m_loadedPlayers) {
      if (player.second.ptr && player.second.loaded) {
        m_playerStorage->savePlayer(player.second.ptr);
        Uuid playerUuid = player.second.ptr->uuid();
        if (playerUuid == m_mainPlayerUuid)
          m_playerStorage->moveToFront(playerUuid);
      }
    }

    m_storageTriggerDeadline = Time::monotonicMilliseconds() + assets->json("/client.config:storageTriggerInterval").toUInt();
  }

  if (m_respawning) {
    if (m_respawnTimer.ready()) {
      if ((playerOnOwnShip() || m_worldClient->respawnInWorld() || m_mainPlayer->alwaysRespawnInWorld()) && m_worldClient->inWorld()) {
        m_worldClient->reviveMainPlayer();
        m_respawning = false;
      }
    } else {
      if (m_respawnTimer.tick(dt)) {
        String cinematic = assets->json("/client.config:respawnCinematic").toString();
        cinematic = cinematic.replaceTags(StringMap<String>{
            {"species", m_mainPlayer->species()},
            {"mode", PlayerModeNames.getRight(m_mainPlayer->modeType())}
          });
        if (assets->assetExists(cinematic) && !(m_mainPlayer->fastRespawn() || m_mainPlayer->externalCinematicsIgnored()))
          // FezzedOne: If a respawn cinematic doesn't exist for the species, don't try to play a nonexistent cinematic.
          // Also respect xSB player settings, of course.
          m_mainPlayer->setPendingCinematic(Json(move(cinematic)));
        if (!(m_worldClient->respawnInWorld() || m_mainPlayer->alwaysRespawnInWorld()))
          m_pendingWarp = WarpAlias::OwnShip;
        if (m_mainPlayer->fastRespawn())
          m_respawnTimer.setDone();
        m_warpDelay.reset();
      }
    }
  } else {
    if (m_worldClient->mainPlayerDead()) {
      if (m_mainPlayer->modeConfig().permadeath) {
        // tooo bad....
      } else {
        m_respawning = true;
        m_respawnTimer.reset();
      }
    }
  }

  if (m_playersToRemove.size()) {
    for (auto& uuid : m_playersToRemove)
      doRemovePlayer(uuid);
    m_playersToRemove = {};
  }

  if (m_playersToLoad.size()) {
    for (auto& uuid : m_playersToLoad)
      doAddPlayer(uuid);
    m_playersToLoad = {};
  }

  if (m_playerToSwitchTo) {
    doSwitchPlayer(*m_playerToSwitchTo);
    m_playerToSwitchTo = Maybe<Uuid>{};
  }

  m_celestialDatabase->cleanup();

  if (auto netStats = m_connection->incomingStats()) {
    LogMap::set("net_incoming_bps", netStats->bytesPerSecond);
    LogMap::set("net_worst_incoming", strf("{}:{}", PacketTypeNames.getRight(netStats->worstPacketType), netStats->worstPacketSize));
  }
  if (auto netStats = m_connection->outgoingStats()) {
    LogMap::set("net_outgoing_bps", netStats->bytesPerSecond);
    LogMap::set("net_worst_outgoing",
        strf("{}:{}", PacketTypeNames.getRight(netStats->worstPacketType), netStats->worstPacketSize));
  }
}

Maybe<BeamUpRule> UniverseClient::beamUpRule() const {
  if (auto worldTemplate = currentTemplate())
    if (auto parameters = worldTemplate->worldParameters()) {
      BeamUpRule beamUpRule = parameters->beamUpRule;
      if (m_mainPlayer->externalWarpsIgnored()) {
        // FezzedOne: If the player is ignoring external warps, ignore all limitations on beaming up too.
        return BeamUpRule::Anywhere;
      } else if (beamUpRule == BeamUpRule::AnywhereWithWarning && m_mainPlayer->fastRespawn()) {
        // FezzedOne: If the player has `"fastRespawn"`, skip that pesky "are you sure you want to beam out?" dialogue.
        return BeamUpRule::Anywhere;
      } else {
        return beamUpRule;
      }
    }

  return {};
}

bool UniverseClient::canBeamUp() const {
  auto playerWorldId = m_clientContext->playerWorldId();

  if (playerWorldId.empty() || playerWorldId.is<ClientShipWorldId>())
    return false;
  if (m_mainPlayer->isAdmin() || m_mainPlayer->externalWarpsIgnored())
    return true;
  if (m_mainPlayer->isDead() || m_mainPlayer->isTeleporting())
    return false;

  auto beamUp = beamUpRule();
  if (beamUp == BeamUpRule::Anywhere || beamUp == BeamUpRule::AnywhereWithWarning)
    return true;
  else if (beamUp == BeamUpRule::Surface)
    return mainPlayer()->modeConfig().allowBeamUpUnderground || mainPlayer()->isOutside();

  return false;
}

bool UniverseClient::canBeamDown(bool deploy) const {
  if (!m_clientContext->orbitWarpAction() || flying())
    return false;
  if (auto warpAction = m_clientContext->orbitWarpAction()) {
    if (!deploy && warpAction->second == WarpMode::DeployOnly && !m_mainPlayer->externalWarpsIgnored())
      return false;
    else if (deploy && ((warpAction->second == WarpMode::BeamOnly && !m_mainPlayer->externalWarpsIgnored()) || !m_mainPlayer->canDeploy()))
      return false;
  }
  if (m_mainPlayer->isAdmin() || m_mainPlayer->fastRespawn()) // FezzedOne: Players with `"fastRespawn"` have fewer restrictions on beaming down.
    return true;
  if (m_mainPlayer->isDead() || m_mainPlayer->isTeleporting() || (!m_clientContext->shipUpgrades().capabilities.contains("teleport") && !m_mainPlayer->externalWarpsIgnored()))
    return false;
  return true;
}

bool UniverseClient::canBeamToTeamShip() const {
  auto playerWorldId = m_clientContext->playerWorldId();
  if (playerWorldId.empty())
    return false;

  if (m_mainPlayer->isAdmin() || m_mainPlayer->externalWarpsIgnored())
    return true;

  if (canBeamUp())
    return true;

  if (playerWorldId.is<ClientShipWorldId>() && (m_clientContext->shipUpgrades().capabilities.contains("teleport") || m_mainPlayer->externalWarpsIgnored()))
    return true;

  return false;
}

bool UniverseClient::canTeleport() const {
  if (m_mainPlayer->isAdmin() || m_mainPlayer->externalWarpsIgnored())
    return true;

  if (m_clientContext->playerWorldId().is<ClientShipWorldId>())
    return (!flying() && m_clientContext->shipUpgrades().capabilities.contains("teleport")) || m_mainPlayer->externalWarpsIgnored();

  return m_mainPlayer->canUseTool();
}

void UniverseClient::warpPlayer(WarpAction const& warpAction, bool animate, String const& animationType, bool deploy) {
  // don't interrupt teleportation in progress
  if (m_warping || m_respawning)
    return;

  m_mainPlayer->stopLounging();
  if (animate) {
    m_mainPlayer->teleportOut(animationType, deploy);
    m_warping = warpAction;
    m_warpDelay.reset();
  }

  m_pendingWarp = warpAction;
}

void UniverseClient::flyShip(Vec3I const& system, SystemLocation const& destination) {
  m_connection->pushSingle(make_shared<FlyShipPacket>(system, destination));
}

CelestialDatabasePtr UniverseClient::celestialDatabase() const {
  return m_celestialDatabase;
}

CelestialCoordinate UniverseClient::shipCoordinate() const {
  return m_clientContext->shipCoordinate();
}

bool UniverseClient::playerOnOwnShip() const {
  return playerWorld().is<ClientShipWorldId>() && playerWorld().get<ClientShipWorldId>() == m_clientContext->playerUuid();
}

bool UniverseClient::playerIsOriginal() const {
  return m_clientContext->playerUuid() == mainPlayer()->uuid();
}

WorldId UniverseClient::playerWorld() const {
  return m_clientContext->playerWorldId();
}

bool UniverseClient::isAdmin() const {
  return m_mainPlayer->isAdmin();
}

Uuid UniverseClient::teamUuid() const {
  if (auto team = m_teamClient->currentTeam())
    return *team;
  return m_clientContext->playerUuid();
}

WorldTemplateConstPtr UniverseClient::currentTemplate() const {
  return m_worldClient->currentTemplate();
}

SkyConstPtr UniverseClient::currentSky() const {
  return m_worldClient->currentSky();
}

bool UniverseClient::flying() const {
  if (auto sky = currentSky())
    return sky->flying();
  return false;
}

void UniverseClient::sendChat(String const& text, ChatSendMode sendMode) {
  if (!text.beginsWith("/")) {
    if (m_mainPlayer)
      m_mainPlayer->addChatMessageCallback(text);
  }
  m_connection->pushSingle(make_shared<ChatSendPacket>(text, sendMode));
}

void UniverseClient::sendChat(String const &text, String const &sendMode, bool suppressBubble) {
  // Override for `player.sendChat`.
  Maybe<ChatSendMode> sendModeEnumMaybe = ChatSendModeNames.maybeLeft(sendMode);
  ChatSendMode sendModeEnum = sendModeEnumMaybe.value(ChatSendMode::Local);
  if (!text.beginsWith("/") && !suppressBubble) {
    if (m_mainPlayer)
      m_mainPlayer->addChatMessageCallback(text); // FezzedOne: Allow chat bubbles to inherit custom player settings.
  }
  m_connection->pushSingle(make_shared<ChatSendPacket>(text, sendModeEnum));
}

List<ChatReceivedMessage> UniverseClient::pullChatMessages() {
  // FezzedOne: Also send chat messages to any existing main player. We ensure messages aren't dropped while
  // swapping players or warping to worlds.
  if (m_worldClient && m_mainPlayer) {
    bool worldInitialised = true;
    for (auto pm : m_worldPendingMessages) {
      Json messageJson = JsonObject{
        {"context", JsonObject{
          {"mode", MessageContextModeNames.getRight(pm.context.mode)},
          {"channel", pm.context.channelName}
        }},
        {"connection", pm.fromConnection},
        {"nick", pm.fromNick},
        {"portrait", pm.portrait},
        {"message", pm.text}
      };
      // FezzedOne: For StarExtensions compatibility.
      Json seMessageJson = JsonObject{
        {"mode", MessageContextModeNames.getRight(pm.context.mode)},
        {"channel", pm.context.channelName},
        {"connection", pm.fromConnection},
        {"nickname", pm.fromNick},
        {"portrait", pm.portrait},
        {"text", pm.text}
        // FezzedOne: Note that `"scripted"` should always be assumed `null` or `false` on xStarbound.
      };
      try {
        m_worldClient->sendEntityMessage(m_mainPlayer->entityId(), "chatMessage", JsonArray{messageJson});
        m_worldClient->sendEntityMessage(m_mainPlayer->entityId(), "newChatMessage", JsonArray{seMessageJson});
      } catch (WorldClientException const& e) {
        worldInitialised = false;
        break;
      }
    }
    if (worldInitialised)
      m_worldPendingMessages = {};
  }
  return take(m_pendingMessages);
}

uint16_t UniverseClient::players() {
  return m_serverInfo.apply([](auto const& info) { return info.players; }).value(1);
}

uint16_t UniverseClient::maxPlayers() {
  return m_serverInfo.apply([](auto const& info) { return info.maxPlayers; }).value(1);
}

void UniverseClient::setLuaCallbacks(String const& groupName, LuaCallbacks const& callbacks) {
  m_luaCallbacks[groupName] = callbacks;
  if (m_worldClient)
    m_worldClient->setLuaCallbacks(groupName, callbacks);
}

void UniverseClient::startLua() {
  if (!m_luaRoot)
    m_luaRoot = make_shared<LuaRoot>();
  setLuaCallbacks("celestial", LuaBindings::makeCelestialCallbacks(this));
  if (m_worldClient)
    setLuaCallbacks("world", LuaBindings::makeWorldCallbacks(as<World>(m_worldClient.get())));

  auto assets = Root::singleton().assets();
  for (auto& p : assets->json("/client.config:universeScriptContexts").toObject()) {
    auto scriptComponent = make_shared<ScriptComponent>();
    scriptComponent->setLuaRoot(m_luaRoot);
    scriptComponent->setScripts(jsonToStringList(p.second.toArray()));

    for (auto& pair : m_luaCallbacks)
      scriptComponent->addCallbacks(pair.first, pair.second);

    m_scriptContexts.set(p.first, scriptComponent);
    scriptComponent->init();
  }
}

void UniverseClient::stopLua() {
  for (auto& p : m_scriptContexts)
    p.second->uninit();

  m_scriptContexts.clear();
}

bool UniverseClient::playerIsLoaded(Uuid const& uuid) {
  if (m_loadedPlayers.contains(uuid)) {
    return m_loadedPlayers[uuid].loaded;
  }
  return false;
}

void UniverseClient::reloadAllPlayers(bool resetInterfaces, bool showIndicator) {
  bool safeScriptsEnabled = false;
  if (m_playerReloadPreCallback && resetInterfaces)
    safeScriptsEnabled = Root::singleton().configuration()->get("safeScripts").toBool();


  for (auto& loadedPlayer : m_loadedPlayers) {
    if (!loadedPlayer.second.loaded) return;

    auto& player = loadedPlayer.second.ptr;
    if (!player) throw PlayerException("Attempted to reload an unloaded player!");

    bool playerInWorld = player->inWorld();
    if (!playerInWorld) continue;

    bool alreadyLoaded = false;
    auto worldPtr = m_mainPlayer->world();
    auto world = as<WorldClient>(worldPtr);
    auto uuid = player->uuid();
    auto data = m_playerStorage->getPlayerData(uuid);

    auto entitySpace = connectionEntitySpace(world->connection());
    bool alreadyHasId = playerInWorld || !world->inWorld();
    EntityId entityId = alreadyHasId ? player->entityId() : entitySpace.first; // entitySpace.first;

    bool safeScriptsEnabled = false;

    if (m_playerReloadPreCallback) {
      if (resetInterfaces && safeScriptsEnabled)
        // FezzedOne: Needed because of `interface.bindRegisteredPane`.
        stopLua();
      m_playerReloadPreCallback(resetInterfaces);
    }

    ProjectilePtr indicator;

    if (showIndicator) {
      // EntityCreatePacket for player entities can be pretty big.
      // We can show a loading projectile to other players while the create packet uploads.
      auto projectileDb = Root::singleton().projectileDatabase();
      auto config = projectileDb->projectileConfig("xsb:playerloading");
      indicator = projectileDb->createProjectile("stationpartsound", config);
      indicator->setInitialPosition(player->position());
      indicator->setInitialDirection({ 1.0f, 0.0f });
      world->addEntity(indicator);
    }

    world->removeEntity(player->entityId(), false);

    world->addEntity(player, entityId);

    if (indicator && indicator->inWorld())
      world->removeEntity(indicator->entityId(), false);

    if (m_playerReloadCallback) {
      if (resetInterfaces && safeScriptsEnabled) {
        // FezzedOne: Needed because of `interface.bindRegisteredPane`.
        m_luaRoot = make_shared<LuaRoot>();
        startLua();
      }
      m_playerReloadCallback(resetInterfaces);
    }
  }
}

HashMap<Uuid, PlayerPtr> UniverseClient::controlledPlayers() {
  HashMap<Uuid, PlayerPtr> returnValue = {};
  for (auto& player : m_loadedPlayers) {
    if (player.second.ptr && player.second.loaded)
      returnValue[player.first] = player.second.ptr;
  }
  return returnValue;
}

bool UniverseClient::swapPlayer(Uuid const& uuid, bool resetInterfaces, bool showIndicator) {
  if (!m_mainPlayer) throw PlayerException("Attempted to swap from an unloaded primary player!");

  if (m_respawning || !m_loadedPlayers.contains(uuid)) return false; // Don't allow swapping players while respawning!
  auto swapPlayer = m_loadedPlayers[uuid].ptr;
  if (!swapPlayer) throw PlayerException("Attempted to swap to a player that isn't pre-loaded!");

  if (m_mainPlayer->uuid() == uuid || (swapPlayer->isPermaDead() && !m_mainPlayer->isAdmin()))
    return false;

  bool playerInWorld = m_mainPlayer->inWorld();
  if (!playerInWorld) return false;

  bool swapPlayerLoaded = m_loadedPlayers[uuid].loaded;
  bool swapPlayerInWorld = swapPlayer->inWorld();
  if (swapPlayerLoaded && (swapPlayer->isDead() || !swapPlayerInWorld)) return false; // Don't allow swapping to a dead player before he's revived!

  auto worldPtr = m_mainPlayer->world();
  auto world = as<WorldClient>(worldPtr);
  auto oldEntityId = m_mainPlayer->entityId();
  auto oldUuid = m_mainPlayer->uuid();

  EntityId entityId = (!swapPlayerInWorld) ? m_mainPlayer->entityId() : NullEntityId; // entitySpace.first;

  m_mainPlayer->setBusyState(PlayerBusyState::None);
  m_mainPlayer->passChatOpen(false);

  bool safeScriptsEnabled = (bool)m_playerReloadPreCallback && 
                            resetInterfaces &&
                            Root::singleton().configuration()->get("safeScripts").toBool();

  if (m_playerReloadPreCallback) {
    if (resetInterfaces && safeScriptsEnabled)
      // FezzedOne: Needed because of `interface.bindRegisteredPane`.
      stopLua();
    m_playerReloadPreCallback(resetInterfaces);
  }

  ProjectilePtr indicator(nullptr);

  if (!swapPlayerInWorld) {
    if (showIndicator) {
      // The EntityCreatePacket for player entities can be pretty big.
      // We can show a loading projectile to other players while the create packet uploads.
      auto projectileDb = Root::singleton().projectileDatabase();
      auto config = projectileDb->projectileConfig("xsb:playerloading");
      indicator = projectileDb->createProjectile("stationpartsound", config);
      indicator->setInitialPosition(m_mainPlayer->position());
      indicator->setInitialDirection({ 1.0f, 0.0f });
      world->addEntity(indicator);
    }
  }

  m_playerStorage->savePlayer(m_mainPlayer);
  m_playerStorage->savePlayer(swapPlayer);
  m_playerStorage->moveToFront(uuid);

  Logger::info("[xSB] UniverseClient: {} player '{}' [{}].",
    swapPlayerInWorld ? "Swapping to loaded secondary" : "Loading primary",
    swapPlayer->name(),
    uuid.hex());

  bool dupeError = false;
  if (!swapPlayerInWorld) {
    try {
      world->addEntity(swapPlayer, entityId);
      swapPlayer->moveTo(m_mainPlayer->position() + swapPlayer->feetOffset());
    } catch (EntityMapException const& e) {
      Logger::warn("Player with UUID {} is already in world; not swapping!", uuid.hex());
      swapPlayer->uninit();
      dupeError = true;
    }
  }

  if (!dupeError) {
    m_mainPlayer = swapPlayer;
    m_mainPlayerUuid = uuid;
    m_worldClient->setMainPlayer(m_mainPlayer);
    m_teamClient->setMainPlayer(m_mainPlayer);
    m_systemWorldClient->setUniverseMap(m_mainPlayer->universeMap());

    if (!swapPlayerInWorld) {
      CelestialCoordinate coordinate = m_systemWorldClient->location();
      m_mainPlayer->universeMap()->addMappedCoordinate(coordinate);
      m_mainPlayer->universeMap()->filterMappedObjects(coordinate, m_systemWorldClient->objectKeys());
      m_loadedPlayers[uuid].loaded = true;

      world->removeEntity(oldEntityId, false);
      m_loadedPlayers[oldUuid].loaded = false;
    }
  }

  if (indicator && indicator->inWorld())
    world->removeEntity(indicator->entityId(), false);

  if (m_playerReloadCallback) {
    if (resetInterfaces && safeScriptsEnabled) {
      // FezzedOne: Needed because of `interface.bindRegisteredPane`.
      m_luaRoot = make_shared<LuaRoot>();
      startLua();
    }
    m_playerReloadCallback(resetInterfaces);
  }

  return !dupeError;
}

bool UniverseClient::loadPlayer(Uuid const& uuid, bool resetInterfaces, bool showIndicator) {
  if (!m_mainPlayer) throw PlayerException("Attempted to load a secondary player from an unloaded primary player!");

  if (m_respawning || !m_loadedPlayers.contains(uuid)) return false; // Don't allow loading players while respawning!
  auto playerToLoad = m_loadedPlayers[uuid].ptr;
  if (!playerToLoad) throw PlayerException("Attempted to load a secondary player that isn't pre-loaded!");

  if (m_mainPlayer->uuid() == uuid || (playerToLoad->isPermaDead() && !m_mainPlayer->isAdmin()))
    return false;

  bool alreadyLoaded = m_loadedPlayers[uuid].loaded;
  bool playerInWorld = m_mainPlayer->inWorld();
  if (alreadyLoaded || !playerInWorld || playerToLoad->inWorld()) return false;

  auto worldPtr = m_mainPlayer->world();
  auto world = as<WorldClient>(worldPtr);

  if (m_playerReloadPreCallback)
    m_playerReloadPreCallback(resetInterfaces);

  ProjectilePtr indicator(nullptr);

  if (showIndicator) {
    // The EntityCreatePacket for player entities can be pretty big.
    // We can show a loading projectile to other players while the create packet uploads.
    auto projectileDb = Root::singleton().projectileDatabase();
    auto config = projectileDb->projectileConfig("xsb:playerloading");
    indicator = projectileDb->createProjectile("stationpartsound", config);
    indicator->setInitialPosition(m_mainPlayer->position());
    indicator->setInitialDirection({ 1.0f, 0.0f });
    world->addEntity(indicator);
  }

  m_playerStorage->savePlayer(m_mainPlayer);
  m_playerStorage->savePlayer(playerToLoad);

  Logger::info("[xSB] UniverseClient: Loading secondary player '{}' [{}].",
    playerToLoad->name(),
    uuid.hex());

  bool dupeError = false;
  if (!playerToLoad->isDead()) { // If loading a dead player, don't revive him immediately. Wait until a warp or primary player death.
    try {
      world->addEntity(playerToLoad);
      playerToLoad->moveTo(m_mainPlayer->position() + playerToLoad->feetOffset());
    } catch (EntityMapException const& e) {
      Logger::warn("Player with UUID {} is already in world; not adding!", uuid.hex());
      playerToLoad->uninit();
      dupeError = true;
    }
  }

  if (!dupeError) {
    CelestialCoordinate coordinate = m_systemWorldClient->location();
    playerToLoad->universeMap()->addMappedCoordinate(coordinate);
    playerToLoad->universeMap()->filterMappedObjects(coordinate, m_systemWorldClient->objectKeys());
    m_loadedPlayers[uuid].loaded = true;
  }

  if (indicator && indicator->inWorld())
    world->removeEntity(indicator->entityId(), false);

  if (m_playerReloadCallback)
    m_playerReloadCallback(resetInterfaces);

  return !dupeError;
}

bool UniverseClient::unloadPlayer(Uuid const& uuid, bool resetInterfaces, bool showIndicator) {
  if (!m_mainPlayer) throw PlayerException("Attempted to unload a secondary player from an unloaded primary player!");

  if (m_respawning || !m_loadedPlayers.contains(uuid)) return false; // Don't allow loading players while respawning!
  auto playerToUnload = m_loadedPlayers[uuid].ptr;

  if (m_mainPlayer->uuid() == uuid) return false;

  bool playerInWorld = m_mainPlayer->inWorld();
  if (!playerInWorld) return false;

  auto worldPtr = m_mainPlayer->world();
  auto world = as<WorldClient>(worldPtr);

  if (m_playerReloadPreCallback)
    m_playerReloadPreCallback(resetInterfaces);

  ProjectilePtr indicator(nullptr);

  if (showIndicator) {
    auto projectileDb = Root::singleton().projectileDatabase();
    auto config = projectileDb->projectileConfig("xsb:playerloading");
    indicator = projectileDb->createProjectile("stationpartsound", config);
    indicator->setInitialPosition(m_mainPlayer->position());
    indicator->setInitialDirection({ 1.0f, 0.0f });
    world->addEntity(indicator);
  }

  m_playerStorage->savePlayer(m_mainPlayer);
  m_playerStorage->savePlayer(playerToUnload);

  Logger::info("[xSB] UniverseClient: Unloading secondary player '{}' [{}].",
    playerToUnload->name(),
    uuid.hex());

  if (playerToUnload->inWorld())
    world->removeEntity(playerToUnload->entityId(), false);
  m_loadedPlayers[uuid].loaded = false;

  if (indicator && indicator->inWorld())
    world->removeEntity(indicator->entityId(), false);

  if (m_playerReloadCallback)
    m_playerReloadCallback(resetInterfaces);

  return true;
}

void UniverseClient::doSwitchPlayer(Uuid const& uuid) {
  // reloadAllPlayers(true, true);
  bool isAlreadyLoaded = playerIsLoaded(uuid);
  if (uuid == mainPlayer()->uuid())
    return;
  else {
    auto oldUuid = m_mainPlayer->uuid();
    try {
      if (swapPlayer(uuid, true, true)) {
        auto dance = Root::singleton().assets()->json("/player.config:swapDance");
        if (dance.isType(Json::Type::String))
          m_loadedPlayers[uuid].ptr->humanoid()->setDance(dance.toString());
      }
    } catch (std::exception const& e) {
      Logger::error("UniverseClient: Exception while attempting to swap player: {}", e.what());
      swapPlayer(oldUuid, true, true);
    }
  }
}

void UniverseClient::doAddPlayer(Uuid const& uuid) {
  bool isAlreadyLoaded = playerIsLoaded(uuid);
  if (uuid == mainPlayer()->uuid())
    return;
  else if (isAlreadyLoaded) {
    return;
  } else {
    // reloadAllPlayers(true, true);
    try {
      if (loadPlayer(uuid, false, true)) {
        auto dance = Root::singleton().assets()->json("/player.config:swapDance");
        if (dance.isType(Json::Type::String))
          m_loadedPlayers[uuid].ptr->humanoid()->setDance(dance.toString());
      }
    } catch (std::exception const& e) {
      Logger::error("UniverseClient: Exception while attempting to load player: {}", e.what());
    }
  }
}

void UniverseClient::doRemovePlayer(Uuid const& uuid) {
  if (uuid == mainPlayer()->uuid())
    return;
  else {
    unloadPlayer(uuid, false, true);
    // reloadAllPlayers(true, true);
  }

  return;
}

bool UniverseClient::switchPlayer(Uuid const& uuid) {
  if (uuid == mainPlayer()->uuid())
    return false;
  else {
    m_playerToSwitchTo = uuid;
    return true;
  }

  return false;
}

bool UniverseClient::switchPlayer(size_t index) {
  if (auto uuid = m_playerStorage->playerUuidAt(index))
    return switchPlayer(*uuid);
  else
    return false;
}

bool UniverseClient::switchPlayer(String const& name) {
  if (auto uuid = m_playerStorage->playerUuidByName(name, mainPlayer()->uuid()))
    return switchPlayer(*uuid);
  else
    return false;
}

bool UniverseClient::switchPlayerUuid(String const& uuidStr) {
  // FezzedOne: Attempts to convert the passed string to a UUID, and if
  // successful, attempts to load the player file with that UUID.
  Uuid uuid = Uuid();
  try {
    uuid = Uuid(uuidStr);
  } catch (std::exception const& e) {
    return false;
  }
  return switchPlayer(uuid);
}

bool UniverseClient::addPlayer(Uuid const& uuid) {
  if (uuid == mainPlayer()->uuid())
    return false;
  else {
    m_playersToLoad.append(uuid);
    return true;
  }

  return false;
}

bool UniverseClient::addPlayer(size_t index) {
  if (auto uuid = m_playerStorage->playerUuidAt(index))
    return addPlayer(*uuid);
  else
    return false;
}

bool UniverseClient::addPlayer(String const& name) {
  if (auto uuid = m_playerStorage->playerUuidByName(name, mainPlayer()->uuid()))
    return addPlayer(*uuid);
  else
    return false;
}

bool UniverseClient::addPlayerUuid(String const& uuidStr) {
  // FezzedOne: Attempts to convert the passed string to a UUID, and if
  // successful, attempts to add the player with that UUID.
  Uuid uuid = Uuid();
  try {
    uuid = Uuid(uuidStr);
  } catch (std::exception const& e) {
    return false;
  }
  return addPlayer(uuid);
}

bool UniverseClient::removePlayer(Uuid const& uuid) {
  if (uuid == mainPlayer()->uuid())
    return false;
  else {
    m_playersToRemove.append(uuid);
    return true;
  }

  return false;
}

bool UniverseClient::removePlayer(size_t index) {
  if (auto uuid = m_playerStorage->playerUuidAt(index))
    return removePlayer(*uuid);
  else
    return false;
}

bool UniverseClient::removePlayer(String const& name) {
  if (auto uuid = m_playerStorage->playerUuidByName(name, mainPlayer()->uuid()))
    return removePlayer(*uuid);
  else
    return false;
}

bool UniverseClient::removePlayerUuid(String const& uuidStr) {
  // FezzedOne: Attempts to convert the passed string to a UUID, and if
  // successful, attempts to remove the player with that UUID.
  Uuid uuid = Uuid();
  try {
    uuid = Uuid(uuidStr);
  } catch (std::exception const& e) {
    return false;
  }
  return removePlayer(uuid);
}

UniverseClient::ReloadPlayerCallback& UniverseClient::playerReloadPreCallback() {
  return m_playerReloadPreCallback;
}

UniverseClient::ReloadPlayerCallback& UniverseClient::playerReloadCallback() {
  return m_playerReloadCallback;
}

ClockConstPtr UniverseClient::universeClock() const {
  return m_universeClock;
}

JsonRpcInterfacePtr UniverseClient::rpcInterface() const {
  return m_clientContext->rpcInterface();
}

ClientContextPtr UniverseClient::clientContext() const {
  return m_clientContext;
}

TeamClientPtr UniverseClient::teamClient() const {
  return m_teamClient;
}

QuestManagerPtr UniverseClient::questManager() const {
  return m_mainPlayer->questManager();
}

PlayerStoragePtr UniverseClient::playerStorage() const {
  return m_playerStorage;
}

StatisticsPtr UniverseClient::statistics() const {
  return m_statistics;
}

bool UniverseClient::paused() const {
  return m_pause;
}

bool UniverseClient::switchingPlayer() const {
  if (m_playerToSwitchTo) return true;
  else return false;
}

void UniverseClient::setPause(bool pause) {
  m_pause = pause;

  if (pause)
    m_universeClock->stop();
  else
    m_universeClock->start();
}

void UniverseClient::handlePackets(List<PacketPtr> const& packets) {
  for (auto const& packet : packets) {
    if (auto clientContextUpdate = as<ClientContextUpdatePacket>(packet)) {
      m_clientContext->readUpdate(clientContextUpdate->updateData);
      if (m_shouldUpdateMainPlayerShip)
        m_playerStorage->applyShipUpdates(m_clientContext->playerUuid(), m_clientContext->newShipUpdates());

      // FezzedOne: Don't apply ship upgrades on protected ships.
      if (playerIsOriginal() && m_shouldUpdateMainPlayerShip)
        m_mainPlayer->setShipUpgrades(m_clientContext->shipUpgrades());

      m_mainPlayer->setAdmin(m_clientContext->isAdmin());
      if (!m_mainPlayer->damageTeamOverridden())
        m_mainPlayer->setTeam(m_clientContext->team());

    } else if (auto chatReceivePacket = as<ChatReceivePacket>(packet)) {
      m_pendingMessages.append(chatReceivePacket->receivedMessage);
      m_worldPendingMessages.append(chatReceivePacket->receivedMessage);

    } else if (auto universeTimeUpdatePacket = as<UniverseTimeUpdatePacket>(packet)) {
      m_universeClock->setTime(universeTimeUpdatePacket->universeTime);

    } else if (auto serverDisconnectPacket = as<ServerDisconnectPacket>(packet)) {
      reset();
      m_disconnectReason = serverDisconnectPacket->reason;
      break; // Stop handling other packets

    } else if (auto celestialResponse = as<CelestialResponsePacket>(packet)) {
      m_celestialDatabase->pushResponses(move(celestialResponse->responses));

    } else if (auto warpResult = as<PlayerWarpResultPacket>(packet)) {
      if (m_mainPlayer->isDeploying() && m_warping && m_warping->is<WarpToPlayer>()) {
        Uuid target = m_warping->get<WarpToPlayer>();
        for (auto member : m_teamClient->members()) {
          if (member.uuid == target) {
            if (member.warpMode != WarpMode::DeployOnly && member.warpMode != WarpMode::BeamOrDeploy)
              m_mainPlayer->deployAbort();
            break;
          }
        }
      }

      m_warping.reset();
      if (!warpResult->success) {
        m_mainPlayer->teleportAbort();
        if (warpResult->warpActionInvalid)
          m_mainPlayer->universeMap()->invalidateWarpAction(warpResult->warpAction);
      }
    } else if (auto planetTypeUpdate = as<PlanetTypeUpdatePacket>(packet)) {
      m_celestialDatabase->invalidateCacheFor(planetTypeUpdate->coordinate);
    } else if (auto pausePacket = as<PausePacket>(packet)) {
      setPause(pausePacket->pause);
    } else if (auto serverInfoPacket = as<ServerInfoPacket>(packet)) {
      m_serverInfo = ServerInfo{serverInfoPacket->players, serverInfoPacket->maxPlayers};
    } else if (!m_systemWorldClient->handleIncomingPacket(packet)) {
      // see if the system world will handle it, otherwise pass it along to the world client
      m_worldClient->handleIncomingPackets({packet});
    }
  }
}

void UniverseClient::reset() {
  stopLua();

  m_universeClock.reset();
  m_worldClient.reset();
  m_celestialDatabase.reset();
  m_clientContext.reset();
  m_teamClient.reset();
  m_warping.reset();
  m_respawning = false;

  auto assets = Root::singleton().assets();
  m_warpDelay = GameTimer(assets->json("/client.config:playerWarpDelay").toFloat());
  m_respawnTimer = GameTimer(assets->json("/client.config:playerReviveTime").toFloat());

  // if (m_mainPlayer)
  //   m_playerStorage->savePlayer(m_mainPlayer);

  for (auto& player : m_loadedPlayers) {
    if (player.second.ptr) {
      m_playerStorage->savePlayer(player.second.ptr);
    }
  }

  m_loadedPlayers = {};
  m_connection.reset();

  // FezzedOne: In order to safely inject `world` callbacks into universe client scripts, must reset the Lua root when the world client is reset.
  auto clientConfig = Root::singleton().assets()->json("/client.config");
  if (m_luaRoot) m_luaRoot->shutdown();
  m_luaRoot = make_shared<LuaRoot>();
  m_luaRoot->tuneAutoGarbageCollection(clientConfig.getFloat("luaGcPause"), clientConfig.getFloat("luaGcStepMultiplier"));
}

}
