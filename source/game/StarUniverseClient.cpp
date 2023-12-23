#include "StarUniverseClient.hpp"
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

namespace Star {

UniverseClient::UniverseClient(PlayerStoragePtr playerStorage, StatisticsPtr statistics) {
  m_storageTriggerDeadline = 0;
  m_playerStorage = move(playerStorage);
  m_statistics = move(statistics);
  m_pause = false;
  m_luaRoot = make_shared<LuaRoot>();
  reset();
}

UniverseClient::~UniverseClient() {
  // FezzedOne: Tell the Lua root to clean up its fucking garbage and properly shut down here.
  if (m_luaRoot) {
    m_luaRoot->collectGarbage();
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
  } else {
    selectedFromTitleScreen = true;
  }

  m_mainPlayer = player;

  if (m_mainPlayer) {
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

  unsigned timeout = assets->json("/client.config:serverConnectTimeout").toUInt();

  {
    auto protocolRequest = make_shared<ProtocolRequestPacket>(StarProtocolVersion);
    protocolRequest->setCompressionMode(PacketCompressionMode::Enabled);
    // Signal that we're xSB-2/OpenSB. Vanilla Starbound only compresses packets above 64 bytes - by forcing it we can communicate this.
    // If you know a less cursed way, please let me know.
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
    Logger::info("UniverseClient: Sending Handshake Response");
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
    m_worldClient = make_shared<WorldClient>(m_mainPlayer);
    for (auto& pair : m_luaCallbacks)
      m_worldClient->setLuaCallbacks(pair.first, pair.second);

    m_connection = move(connection);
    m_celestialDatabase = make_shared<CelestialSlaveDatabase>(move(success->celestialInformation));
    m_systemWorldClient = make_shared<SystemWorldClient>(m_universeClock, m_celestialDatabase, m_mainPlayer->universeMap());

    Logger::info("UniverseClient: Joined {} server as client {}", m_legacyServer ? "Starbound" : "xSB-2/OpenSB", success->clientId);
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
    if (m_mainPlayer) {
      m_playerStorage->savePlayer(m_mainPlayer);
      m_playerStorage->moveToFront(m_mainPlayer->uuid());
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
  if (!text.beginsWith("/"))
    m_mainPlayer->addChatMessageCallback(text);
  m_connection->pushSingle(make_shared<ChatSendPacket>(text, sendMode));
}

void UniverseClient::sendChat(String const &text, String const &sendMode, bool suppressBubble) {
  // Override for `player.sendChat`.
  Maybe<ChatSendMode> sendModeEnumMaybe = ChatSendModeNames.maybeLeft(sendMode);
  ChatSendMode sendModeEnum = sendModeEnumMaybe.value(ChatSendMode::Local);
  if (!text.beginsWith("/") && !suppressBubble)
    m_mainPlayer->addChatMessageCallback(text); // FezzedOne: Allow chat bubbles to inherit custom player settings.
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
      try {
        m_worldClient->sendEntityMessage(m_mainPlayer->entityId(), "chatMessage", JsonArray{messageJson});
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
  setLuaCallbacks("celestial", LuaBindings::makeCelestialCallbacks(this));

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

bool UniverseClient::reloadPlayer(Json const& data, Uuid const& uuid, bool resetInterfaces, bool showIndicator) {
  auto player = mainPlayer();
  bool playerInWorld = player->inWorld();
  auto world = as<WorldClient>(player->world());

  EntityId entityId = (playerInWorld || !world->inWorld())
    ? player->entityId()
    : connectionEntitySpace(world->connection()).first;

  if (m_playerReloadPreCallback)
    m_playerReloadPreCallback(resetInterfaces);

  ProjectilePtr indicator;

  if (playerInWorld) {
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
  } else {
    m_respawning = false;
    m_respawnTimer.reset();
  }

  Json originalData = m_playerStorage->savePlayer(player);
  std::exception_ptr exception;

  try {
    auto newData = data.set("movementController", originalData.get("movementController"));
    player->diskLoad(newData);
  }
  catch (std::exception const& e) {
    player->diskLoad(originalData);
    exception = std::current_exception();
  }

  world->addEntity(player, entityId);

  if (indicator && indicator->inWorld())
    world->removeEntity(indicator->entityId(), false);

  CelestialCoordinate coordinate = m_systemWorldClient->location();
  player->universeMap()->addMappedCoordinate(coordinate);
  player->universeMap()->filterMappedObjects(coordinate, m_systemWorldClient->objectKeys());

  if (m_playerReloadCallback)
    m_playerReloadCallback(resetInterfaces);

  if (exception)
    std::rethrow_exception(exception);

  return true;
}

bool UniverseClient::switchPlayer(Uuid const& uuid) {
  if (uuid == mainPlayer()->uuid())
    return false;
  else if (auto data = m_playerStorage->maybeGetPlayerData(uuid)) {
    if (reloadPlayer(*data, uuid, true, true)) {
      auto dance = Root::singleton().assets()->json("/player.config:swapDance");
      if (dance.isType(Json::Type::String))
        m_mainPlayer->humanoid()->setDance(dance.toString());
      return true;
    }
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

  if (m_mainPlayer)
    m_playerStorage->savePlayer(m_mainPlayer);

  m_connection.reset();
}

}
