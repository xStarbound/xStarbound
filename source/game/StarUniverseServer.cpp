#include "StarUniverseServer.hpp"
#include "StarAiDatabase.hpp"
#include "StarAssets.hpp"
#include "StarBiomeDatabase.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarChatProcessor.hpp"
#include "StarCommandProcessor.hpp"
#include "StarConfiguration.hpp"
#include "StarEncode.hpp"
#include "StarFile.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarSecureRandom.hpp"
#include "StarSha256.hpp"
#include "StarSky.hpp"
#include "StarTcp.hpp"
#include "StarTeamManager.hpp"
#include "StarUniverseServerLuaBindings.hpp"
#include "StarVersioningDatabase.hpp"
#include "StarWorldTemplate.hpp"

#if defined TRACY_ENABLE
#include "tracy/Tracy.hpp"
#else
#define ZoneScoped
#define ZoneScopedN(name)
#endif

namespace Star {

bool UniverseServer::clientHasBuildPermission(ServerClientContextPtr const& client, SystemWorldServerThreadPtr const& currentSystem, Maybe<Vec3I> const& systemLocation) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  if (!client || !(currentSystem || systemLocation))
    return false;

  auto const& configFile = Root::singleton().configuration();
  auto const& jXServerPerms = configFile->get("buildPermissionSettings");
  JsonObject xServerPerms = jXServerPerms.isType(Json::Type::Object) ? jXServerPerms.toObject() : JsonObject{};

  auto getBool = [&](String key) -> bool {
    if (xServerPerms.contains(key)) {
      auto const& value = xServerPerms.get(key);
      return value.isType(Json::Type::Bool) ? value.toBool() : false;
    }
    return false;
  };

  if (!getBool("enabled"))
    return true;

  Json serverConfig;
  if (getBool("storeClaimsInServerData"))
    serverConfig = this->getServerData("claimData");
  else
    serverConfig = configFile->get("claimData");
  serverConfig = serverConfig.isType(Json::Type::Object) ? serverConfig : JsonObject{};

  auto const& jBuildPermissions = serverConfig.get("ownedSystemsByAccount");
  JsonObject const& buildPermissions = jBuildPermissions.isType(Json::Type::Object) ? jBuildPermissions.toObject() : JsonObject{};
  auto const& jBuildPermissionsByUuid = serverConfig.get("ownedSystemsByUuid");
  JsonObject const& buildPermissionsByUuid = jBuildPermissionsByUuid.isType(Json::Type::Object) ? jBuildPermissionsByUuid.toObject() : JsonObject{};

  Vec3I location;
  if (systemLocation)
    location = *systemLocation;
  else if (currentSystem)
    location = currentSystem->location();
  else
    return false;

  String const locationStr = strf("{}:{}:{}", location[0], location[1], location[2]);
  Maybe<String> const& playerAccount = client->playerAccount();
  String const& playerUuid = client->playerUuid().hex(); // FezzedOne: This is the UUID upon connection; also the connected ship world's UUID.

  auto hasBuildPermission = [&locationStr](JsonObject const& permissionList, Maybe<String> const& accountOrUuid) -> Maybe<bool> {
    // FezzedOne: `true` -> has permissions, `false` -> does not have permissions, `{}` -> unclaimed system.
    if (permissionList.contains(locationStr)) {
      auto const& jLocationPermissions = permissionList.get(locationStr);
      JsonObject const& locationPermissions = jLocationPermissions.isType(Json::Type::Object) ? jLocationPermissions.toObject() : JsonObject{};
      if (locationPermissions.contains("owner")) {
        if (!accountOrUuid)
          return false;
        if (locationPermissions.get("owner") == *accountOrUuid)
          return true;
        if (locationPermissions.contains("allowedBuilders")) {
          auto const& jAllowedBuilders = locationPermissions.get("allowedBuilders");
          JsonArray const& allowedBuilders = jAllowedBuilders.isType(Json::Type::Array) ? jAllowedBuilders.toArray() : JsonArray{};
          if (allowedBuilders.contains(true))
            return true;
          if (allowedBuilders.contains(*accountOrUuid))
            return true;
          return false;
        }
        return false;
      }
      return {};
    }
    return {};
  };

  bool guestsAllowed = getBool("guestWorldSpawnsAllowed");

  if (client->canBecomeAdmin())
    return true;

  if (getBool("accountClaimsEnabled")) {
    if (const Maybe<bool> owned = hasBuildPermission(buildPermissions, playerAccount))
      return *owned;
    else if (const Maybe<bool> ownedByUuid = hasBuildPermission(buildPermissionsByUuid, playerUuid))
      return *ownedByUuid;
    return guestsAllowed;
  } else if (const Maybe<bool> owned = hasBuildPermission(buildPermissionsByUuid, playerUuid)) {
    return *owned;
  }

  return guestsAllowed;
}

Maybe<bool> UniverseServer::clientHasBuildPermissionCallback(ConnectionId clientId, Maybe<Vec3I> const& systemLocation) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto client = m_clients.value(clientId))
    return clientHasBuildPermission(client, client->systemWorld(), systemLocation);
  return {};
}

UniverseServer::UniverseServer(String const& storageDir)
    : Thread("UniverseServer"),
      m_workerPool("UniverseServerWorkerPool"),
      m_clients(MinClientConnectionId, MaxClientConnectionId) {
  String const LockFile = "universe.lock";

  m_storageDirectory = storageDir;
  if (!File::isDirectory(m_storageDirectory)) {
    Logger::info("UniverseServer: Creating universe storage directory");
    File::makeDirectory(m_storageDirectory);
  }

  auto& root = Root::singleton();
  auto assets = root.assets();
  auto configuration = root.configuration();

  if (auto assetsDigestOverride = configuration->get("serverOverrideAssetsDigest").optString()) {
    m_assetsDigest = hexDecode(*assetsDigestOverride);
    Logger::info("UniverseServer: Overriding assets digest as '{}'", *assetsDigestOverride);
  } else {
    m_assetsDigest = assets->digest();
  }

  m_commandProcessor = make_shared<CommandProcessor>(this);
  m_chatProcessor = make_shared<ChatProcessor>();
  m_chatProcessor->setCommandHandler(bind(&CommandProcessor::userCommand, m_commandProcessor.get(), _1, _2, _3));

  Logger::info("UniverseServer: Acquiring universe lock file");

  m_storageDirectoryLock = LockFile::acquireLock(File::relativeTo(m_storageDirectory, LockFile));
  if (!m_storageDirectoryLock)
    throw UniverseServerException("Could not acquire lock for the universe directory");

  if (configuration->get("clearUniverseFiles").toBool()) {
    Logger::info("UniverseServer: Clearing all universe files");
    for (auto file : File::dirList(storageDir)) {
      if (!file.second && file.first != LockFile)
        File::remove(File::relativeTo(storageDir, file.first));
    }
  }

  m_celestialDatabase = make_shared<CelestialMasterDatabase>(File::relativeTo(m_storageDirectory, "universe.chunks"));

  Logger::info("UniverseServer: Loading settings");
  loadSettings();
  loadTempWorldIndex();
  m_lastClockUpdateSent = 0.0;
  m_stop = false;
  m_tcpState = TcpState::No;
  m_storageTriggerDeadline = 0;
  m_clearBrokenWorldsDeadline = 0;

  m_rememberReturnWarpsOnDeath = false;

  m_maxPlayers = configuration->get("maxPlayers").toUInt();

  if (auto rememberReturnWarpsOnDeath = configuration->get("rememberReturnWarpsOnDeath").optBool())
    m_rememberReturnWarpsOnDeath = rememberReturnWarpsOnDeath.get();

  for (auto const& pair : assets->json("/universe_server.config:speciesShips").iterateObject())
    m_speciesShips[pair.first] = jsonToStringList(pair.second);

  m_teamManager = make_shared<TeamManager>();
  m_workerPool.start(assets->json("/universe_server.config:workerPoolThreads").toUInt());
  m_connectionServer = make_shared<UniverseConnectionServer>(bind(&UniverseServer::packetsReceived, this, _1, _2, _3));

  m_pause = make_shared<atomic<bool>>(false);
}

UniverseServer::~UniverseServer() {
  stop();
  join();
  m_workerPool.stop();

  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  m_connectionServer->removeAllConnections();
  m_deadConnections.clear();

  // Make sure that all world threads and net sockets (and associated threads)
  // are shutdown before other member destruction.
  m_clients.clear();
  m_worlds.clear();
}

void UniverseServer::setListeningTcp(bool listenTcp) {
  if (!listenTcp || m_tcpState != TcpState::Fuck)
    m_tcpState = listenTcp ? TcpState::Yes : TcpState::No;
}

void UniverseServer::addClient(UniverseConnection remoteConnection) {
  RecursiveMutexLocker locker(m_mainLock);
  // Binding requires us to make the given lambda copy constructible, so the
  // make_shared is required here.
  m_connectionAcceptThreads.append(Thread::invoke("UniverseServer::acceptConnection", [this, conn = make_shared<UniverseConnection>(std::move(remoteConnection))]() {
    acceptConnection(std::move(*conn), {});
  }));
}

UniverseConnection UniverseServer::addLocalClient() {
  auto pair = LocalPacketSocket::openPair();
  addClient(UniverseConnection(std::move(pair.first)));
  return UniverseConnection(std::move(pair.second));
}

void UniverseServer::stop() {
  m_stop = true;
}

void UniverseServer::setPause(bool pause) {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  // Pausing is disabled for multiplayer
  if (m_clients.size() > 1)
    pause = false;

  if (pause == *m_pause)
    return;

  *m_pause = pause;

  if (pause)
    m_universeClock->stop();
  else
    m_universeClock->start();

  for (auto p : m_clients)
    m_connectionServer->sendPackets(p.first, {make_shared<PausePacket>(*m_pause)});
}

List<WorldId> UniverseServer::activeWorlds() const {
  RecursiveMutexLocker locker(m_mainLock);
  return m_worlds.keys();
}

bool UniverseServer::isWorldActive(WorldId const& worldId) const {
  RecursiveMutexLocker locker(m_mainLock);
  return m_worlds.contains(worldId);
}

List<ConnectionId> UniverseServer::clientIds() const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  return m_clients.keys();
}

size_t UniverseServer::numberOfClients() const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  return m_clients.size();
}

uint32_t UniverseServer::maxClients() const {
  return m_maxPlayers;
}

bool UniverseServer::isConnectedClient(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  return m_clients.contains(clientId);
}

String UniverseServer::clientDescriptor(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return clientContext->descriptiveName();
  else
    return "disconnected_client";
}

String UniverseServer::clientNick(ConnectionId clientId) const {
  return m_chatProcessor->connectionNick(clientId);
}

Maybe<String> UniverseServer::clientAccount(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return clientContext->playerAccount();
  return {};
}

Maybe<bool> UniverseServer::clientIsGuest(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return clientContext->isGuest();
  return {};
}

Maybe<ConnectionId> UniverseServer::findNick(String const& nick) const {
  return m_chatProcessor->findNick(nick);
}

Maybe<Uuid> UniverseServer::uuidForClient(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return clientContext->playerUuid();
  return {};
}

Maybe<ConnectionId> UniverseServer::clientForUuid(Uuid const& uuid) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  return getClientForUuid(uuid);
}

void UniverseServer::adminBroadcast(String const& text, JsonObject const& metadata) {
  m_chatProcessor->adminBroadcast(text, metadata);
}

void UniverseServer::adminWhisper(ConnectionId clientId, String const& text, JsonObject const& metadata) {
  m_chatProcessor->adminWhisper(clientId, text, metadata);
}

String UniverseServer::adminCommand(String text) {
  String command = text.extract();
  return m_commandProcessor->adminCommand(command, text);
}

bool UniverseServer::isAdmin(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return clientContext->isAdmin();
  return false;
}

bool UniverseServer::canBecomeAdmin(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return clientContext->canBecomeAdmin();
  return false;
}

void UniverseServer::setAdmin(ConnectionId clientId, bool admin) {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    clientContext->setAdmin(admin);
}

bool UniverseServer::isLocal(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return !clientContext->remoteAddress();
  return false;
}

bool UniverseServer::isPvp(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return clientContext->team().type == TeamType::PVP;
  return false;
}

void UniverseServer::setPvp(ConnectionId clientId, bool pvp) {
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId)) {
    if (pvp) {
      TeamNumber pvpTeam = m_teamManager->getPvpTeam(clientContext->playerUuid());
      if (pvpTeam == 0)
        pvpTeam = soloPvpTeam(clientId);
      clientContext->setTeam(EntityDamageTeam(TeamType::PVP, pvpTeam));
    } else
      clientContext->setTeam(EntityDamageTeam(TeamType::Friendly));
  }
}

RpcThreadPromise<Json> UniverseServer::sendWorldMessage(WorldId const& worldId, String const& message, JsonArray const& args) {
  auto pair = RpcThreadPromise<Json>::createPair();
  RecursiveMutexLocker locker(m_mainLock);
  m_pendingWorldMessages[worldId].push_back({message, args, pair.second});
  return pair.first;
}

void UniverseServer::clientWarpPlayer(ConnectionId clientId, WarpAction action, bool deploy) {
  RecursiveMutexLocker locker(m_mainLock);
  m_pendingPlayerWarps[clientId] = pair<WarpAction, bool>(std::move(action), std::move(deploy));
}

void UniverseServer::clientFlyShip(ConnectionId clientId, Vec3I const& system, SystemLocation const& location) {
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  if (m_pendingFlights.contains(clientId) || m_queuedFlights.contains(clientId))
    return;

  auto clientContext = m_clients.value(clientId);
  if (!clientContext)
    return;

  if (system == Vec3I()) {
    m_pendingFlights.set(clientId, {Vec3I(), {}}); // find starter world
    return;
  }

  auto clientSystem = clientContext->systemWorld();
  bool sameSystem = clientSystem && clientSystem->location() == system;
  bool sameLocation = clientSystem && clientSystem->clientShipLocation(clientId) == location;
  if (m_pendingArrivals.contains(clientId) && sameSystem && location && !sameLocation) {
    // for continuing flight within a system, set the new destination immediately
    clientSystem->setClientDestination(clientId, location);
    return;
  }

  // don't switch systems while already flying
  if (!m_pendingArrivals.contains(clientId) || sameSystem)
    m_pendingFlights.set(clientId, {system, location});
}

WorldId UniverseServer::clientWorld(ConnectionId clientId) const {
  RecursiveMutexLocker locker(m_mainLock);
  // FezzedOne: Fixing non-recursive client mutex lockers!
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return clientContext->playerWorldId();
  return WorldId();
}

CelestialCoordinate UniverseServer::clientShipCoordinate(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId))
    return clientContext->shipCoordinate();
  return CelestialCoordinate();
}

SystemLocation UniverseServer::clientShipLocation(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId)) {
    if (auto clientSystem = clientContext->systemWorld())
      return clientSystem->clientShipLocation(clientId);
  }
  return SystemLocation();
}

ClockPtr UniverseServer::universeClock() const {
  return m_universeClock;
}

UniverseSettingsPtr UniverseServer::universeSettings() const {
  return m_universeSettings;
}

CelestialDatabase& UniverseServer::celestialDatabase() {
  return *m_celestialDatabase;
}

bool UniverseServer::executeForClient(ConnectionId clientId, function<void(WorldServer*, PlayerPtr)> action) {
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  bool success = false;
  if (auto clientContext = m_clients.value(clientId)) {
    if (auto currentWorld = clientContext->playerWorld()) {
      currentWorld->executeAction([clientId, action, &success](WorldServerThread*, WorldServer* worldServer) {
        if (auto player = worldServer->clientPlayer(clientId)) {
          action(worldServer, player);
          success = true;
        }
      });
    }
  }
  return success;
}

void UniverseServer::disconnectClient(ConnectionId clientId, String const& reason) {
  RecursiveMutexLocker locker(m_mainLock);
  m_pendingDisconnections.add(clientId, reason);
}

void UniverseServer::banUser(ConnectionId clientId, String const& reason, pair<bool, bool> banType, Maybe<int> timeout) {
  RecursiveMutexLocker locker(m_mainLock);

  if (timeout)
    doTempBan(clientId, reason, banType, *timeout);
  else
    doPermBan(clientId, reason, banType);

  m_pendingDisconnections.add(clientId, reason);
}

bool UniverseServer::unbanUuid(String const& uuidString) {
  RecursiveMutexLocker locker(m_mainLock);

  bool entryFound = false;

  auto config = Root::singleton().configuration();
  auto bannedUuids = config->get("bannedUuids").toArray();

  eraseWhere(bannedUuids, [&](Json const& entry) {
    if (entry.getString("uuid") == uuidString) {
      entryFound = true;
      return true;
    }
    return false;
  });
  config->set("bannedUuids", bannedUuids);

  eraseWhere(m_tempBans, [&](TimeoutBan const& b) {
    if (b.uuid && b.uuid->hex() == uuidString) {
      entryFound = true;
      return true;
    }
    return false;
  });

  return entryFound;
}

bool UniverseServer::unbanIp(String const& addressString) {
  RecursiveMutexLocker locker(m_mainLock);

  auto addressLookup = HostAddress::lookup(addressString);
  if (addressLookup.isLeft()) {
    return false;
  } else {
    HostAddress address = std::move(addressLookup.right());
    String cleanAddressString = toString(address);

    bool entryFound = false;

    auto config = Root::singleton().configuration();
    auto bannedIPs = config->get("bannedIPs").toArray();
    eraseWhere(bannedIPs, [&](Json const& entry) {
      if (entry.getString("ip") == cleanAddressString) {
        entryFound = true;
        return true;
      }
      return false;
    });
    config->set("bannedIPs", bannedIPs);

    eraseWhere(m_tempBans, [&](TimeoutBan const& b) {
      if (b.ip && b.ip.value() == address) {
        entryFound = true;
        return true;
      }
      return false;
    });

    return entryFound;
  }
}

bool UniverseServer::updatePlanetType(CelestialCoordinate const& coordinate, String const& newType, String const& weatherBiome) {
  RecursiveMutexLocker locker(m_mainLock);

  if (!coordinate.isNull() && m_celestialDatabase->coordinateValid(coordinate)) {
    if (auto celestialParameters = m_celestialDatabase->parameters(coordinate)) {
      if (auto terrestrialParameters = as<TerrestrialWorldParameters>(celestialParameters->visitableParameters())) {
        auto newTerrestrialParameters = make_shared<TerrestrialWorldParameters>(*terrestrialParameters);
        newTerrestrialParameters->typeName = newType;

        auto biomeDatabase = Root::singleton().biomeDatabase();
        auto newWeatherPool = biomeDatabase->biomeWeathers(weatherBiome, celestialParameters->seed(), terrestrialParameters->threatLevel);
        newTerrestrialParameters->weatherPool = newWeatherPool;

        newTerrestrialParameters->terraformed = true;

        celestialParameters->setVisitableParameters(newTerrestrialParameters);

        m_celestialDatabase->updateParameters(coordinate, *celestialParameters);

        RecursiveMutexLocker clientsLocker(m_clientsLock);

        for (auto clientId : m_clients.keys())
          m_connectionServer->sendPackets(clientId, {make_shared<PlanetTypeUpdatePacket>(coordinate)});

        return true;
      }
    }
  }

  return false;
}

void UniverseServer::run() {
  Logger::info("UniverseServer: Starting UniverseServer with UUID: {}", m_universeSettings->uuid().hex());

  int mainWakeupInterval = Root::singleton().assets()->json("/universe_server.config:mainWakeupInterval").toInt();

  TcpServerPtr tcpServer;

  while (!m_stop) {
    ZoneScopedN("UNIVERSE SERVER TICK");
    if (m_tcpState == TcpState::Yes && !tcpServer) {
      ZoneScopedN("Server connection handling");

      auto& root = Root::singleton();
      auto configuration = root.configuration();
      auto assets = root.assets();
      HostAddressWithPort bindAddress(configuration->get("gameServerBind").toString(), configuration->get("gameServerPort").toUInt());
      unsigned maxPendingConnections = assets->json("/universe_server.config:maxPendingConnections").toInt();
      maxPendingConnections = configuration->get("maxPendingServerConnections").optUInt().value(maxPendingConnections);
      unsigned packetTimeout /* In milliseconds. */ = configuration->get("serverPacketTimeout").optUInt().value(60000);
      unsigned connectionAcceptTimeout /* In milliseconds. */ = configuration->get("serverConnectionAcceptTimeout").optUInt().value(20);

      Logger::info("UniverseServer: listening for incoming TCP connections on {}", bindAddress);

      try {
        // FezzedOne: Made the packet and connection acceptance timeouts configurable.
        tcpServer = make_shared<TcpServer>(bindAddress, packetTimeout);
        tcpServer->setAcceptCallback([this, maxPendingConnections](TcpSocketPtr socket) {
          RecursiveMutexLocker locker(m_mainLock);
          if (m_connectionAcceptThreads.size() < maxPendingConnections) {
            Logger::info("UniverseServer: Connection received from: {}", socket->remoteAddress());
            m_connectionAcceptThreads.append(Thread::invoke("UniverseServer::acceptConnection", [this, socket]() {
              acceptConnection(UniverseConnection(TcpPacketSocket::open(socket)), socket->remoteAddress().address());
            }));
          } else {
            Logger::warn("UniverseServer: maximum pending connections, dropping connection from: {}", socket->remoteAddress().address());
          }
        },
            connectionAcceptTimeout);
      } catch (StarException const& e) {
        Logger::error("UniverseServer: Error setting up TCP, cannot accept connections: {}", e.what());
        m_tcpState = TcpState::Fuck;
        tcpServer.reset();
      }
    } else if (m_tcpState == TcpState::No && tcpServer) {
      Logger::info("UniverseServer: Not listening for incoming TCP connections");
      tcpServer.reset();
    }

    LogMap::set("universe_time", m_universeClock->time());

    try {
      ZoneScopedN("Server update tick");
      processUniverseFlags();
      removeTimedBan();
      sendPendingChat();
      updateTeams();
      updateShips();
      sendClockUpdates();
      kickErroredPlayers();
      reapConnections();
      processPlanetTypeChanges();
      warpPlayers();
      flyShips();
      arriveShips();
      processChat();
      sendClientContextUpdates();
      respondToCelestialRequests();
      clearBrokenWorlds();
      handleWorldMessages();
      shutdownInactiveWorlds();
      doTriggeredStorage();
    } catch (std::exception const& e) {
      Logger::error("UniverseServer: exception caught: {}", outputException(e, true));
    }

    Thread::sleep(mainWakeupInterval);
  }

  Logger::info("UniverseServer: Stopping UniverseServer");

  try {
    m_workerPool.stop();

    if (tcpServer) {
      Logger::info("UniverseServer: Stopping TCP Server");
      tcpServer.reset();
    }

    RecursiveMutexLocker locker(m_mainLock);
    RecursiveMutexLocker clientsLocker(m_clientsLock);
    for (auto clientId : m_clients.keys())
      doDisconnection(clientId, "Server is shutting down");

    saveSettings();
    saveTempWorldIndex();
    m_worlds.clear();

  } catch (std::exception const& e) {
    Logger::error("UniverseServer: exception caught cleaning up: {}", outputException(e, true));
  }
}

void UniverseServer::processUniverseFlags() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  if (auto actions = m_universeSettings->pullPendingFlagActions()) {
    for (auto action : *actions) {
      if (action.is<PlaceDungeonFlagAction>()) {
        auto placeDungeonAction = action.get<PlaceDungeonFlagAction>();
        if (instanceWorldStoredOrActive(placeDungeonAction.targetInstance)) {
          auto worldId = InstanceWorldId(placeDungeonAction.targetInstance);
          m_pendingFlagActions.append(pair<WorldId, UniverseFlagAction>{worldId, placeDungeonAction});
        }
      }
    }
  }

  eraseWhere(m_pendingFlagActions, [this](pair<WorldId, UniverseFlagAction> const& p) {
    if (p.first.is<InstanceWorldId>() && instanceWorldStoredOrActive(p.first.get<InstanceWorldId>())) {
      // world is stored or active; perform flag actions once it loads
      if (auto maybeTargetWorld = triggerWorldCreation(p.first)) {
        if (auto targetWorld = maybeTargetWorld.value()) {
          if (p.second.is<PlaceDungeonFlagAction>()) {
            auto placeDungeonAction = p.second.get<PlaceDungeonFlagAction>();
            targetWorld->executeAction([placeDungeonAction](WorldServerThread*, WorldServer* worldServer) {
              worldServer->placeDungeon(placeDungeonAction.dungeonId, placeDungeonAction.targetPosition, 0);
            });
          }
          return true;
        }
      }
      return false;
    } else {
      // world hasn't yet been created; flag actions will be handled by normal creation
      return true;
    }
  });
}

void UniverseServer::sendPendingChat() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  for (auto const& p : m_clients) {
    for (auto const& message : m_chatProcessor->pullPendingMessages(p.first)) {
      if (auto result = m_commandProcessor->handleChatMessage(p.first, message)) {
        ConnectionId clientId = get<0>(*result);
        ChatReceivedMessage handledMessage = get<1>(*result);
        if (clientId != ServerConnectionId)
          m_connectionServer->sendPackets(get<0>(*result), {make_shared<ChatReceivePacket>(handledMessage)});
      }
    }
  }
}

void UniverseServer::updateTeams() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  StringMap<List<Uuid>> connectedPlayers;
  auto teams = m_teamManager->getPvpTeams();
  for (auto const& p : m_clients) {
    connectedPlayers[p.second->playerName().toLower()].append(p.second->playerUuid());

    if (p.second->team().type == TeamType::PVP)
      p.second->setTeam(EntityDamageTeam(TeamType::PVP, teams.value(p.second->playerUuid(), soloPvpTeam(p.second->clientId()))));
    else
      p.second->setTeam(EntityDamageTeam(TeamType::Friendly));

    auto channels = m_chatProcessor->clientChannels(p.first);
    auto team = m_teamManager->getTeam(p.second->playerUuid());
    for (auto const& channel : channels) {
      if (channel != printWorldId(p.second->playerWorldId()) && (!team || channel != team.value().hex()))
        m_chatProcessor->leaveChannel(p.first, channel);
    }
    if (team && !channels.contains(team.value().hex()))
      m_chatProcessor->joinChannel(p.first, team.value().hex());
  }

  m_teamManager->setConnectedPlayers(connectedPlayers);
}

void UniverseServer::updateShips() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  for (auto const& p : m_clients) {
    auto newShipUpgrades = p.second->shipUpgrades();
    if (auto shipWorld = getWorld(ClientShipWorldId(p.second->playerUuid()))) {
      shipWorld->executeAction([&](WorldServerThread*, WorldServer* shipWorld) {
        auto const& speciesShips = m_speciesShips.get(p.second->playerSpecies());
        Json jOldShipLevel = shipWorld->getProperty("ship.level");
        unsigned newShipLevel = min<unsigned>(speciesShips.size() - 1, newShipUpgrades.shipLevel);


        if (jOldShipLevel.isType(Json::Type::Int)) {
          auto oldShipLevel = jOldShipLevel.toUInt();
          if (oldShipLevel < newShipLevel) {
            for (unsigned i = oldShipLevel + 1; i <= newShipLevel; ++i) {
              auto shipStructure = WorldStructure(speciesShips[i]);
              shipWorld->setCentralStructure(shipStructure);
              newShipUpgrades.apply(shipStructure.configValue("shipUpgrades"));
            }

            p.second->setShipUpgrades(newShipUpgrades);
            p.second->updateShipChunks(shipWorld->readChunks());
          }
        }
        shipWorld->setProperty("ship.level", newShipUpgrades.shipLevel);
        shipWorld->setProperty("ship.maxFuel", newShipUpgrades.maxFuel);
        shipWorld->setProperty("ship.crewSize", newShipUpgrades.crewSize);
        shipWorld->setProperty("ship.fuelEfficiency", newShipUpgrades.fuelEfficiency);
      });
    }

    if (auto systemWorld = p.second->systemWorld()) {
      float speed = newShipUpgrades.shipSpeed;
      systemWorld->executeClientShipAction(p.first, [speed](SystemClientShip* ship) {
        if (ship)
          ship->setSpeed(speed);
      });
    }
  }
}

void UniverseServer::sendClockUpdates() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  int64_t currentTime = Time::monotonicMilliseconds();
  if (currentTime > m_lastClockUpdateSent + Root::singleton().assets()->json("/universe_server.config:clockUpdatePacketInterval").toInt()) {
    for (auto clientId : m_clients.keys())
      m_connectionServer->sendPackets(clientId, {make_shared<UniverseTimeUpdatePacket>(m_universeClock->time())});
    m_lastClockUpdateSent = currentTime;
  }
}

void UniverseServer::sendClientContextUpdate(ServerClientContextPtr clientContext) {
  auto clientContextData = clientContext->writeUpdate();
  if (!clientContextData.empty())
    m_connectionServer->sendPackets(clientContext->clientId(), {make_shared<ClientContextUpdatePacket>(clientContextData)});
}

void UniverseServer::sendClientContextUpdates() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  for (auto const& p : m_clients)
    sendClientContextUpdate(p.second);
}

void UniverseServer::kickErroredPlayers() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  for (auto const& worldId : m_worlds.keys()) {
    if (auto world = getWorld(worldId)) {
      for (auto clientId : world->erroredClients())
        m_pendingDisconnections[clientId] = "A packet from your client has caused a server error";
    }
  }
}

void UniverseServer::reapConnections() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);

  int64_t startTime = Time::monotonicMilliseconds();
  int64_t timeout = Root::singleton().assets()->json("/universe_server.config:connectionTimeout").toInt();

  eraseWhere(m_connectionAcceptThreads, [](ThreadFunction<void>& function) {
    if (!function.isRunning()) {
      try {
        function.finish();
      } catch (std::exception const& e) {
        Logger::error("UniverseServer: Exception caught accepting new connection: {}", outputException(e, true));
      }
    }
    return function.isFinished();
  });

  RecursiveMutexLocker clientsLocker(m_clientsLock);
  for (auto p : take(m_pendingDisconnections))
    doDisconnection(p.first, p.second);

  for (auto clientId : m_clients.keys()) {
    auto clientContext = m_clients.value(clientId);
    if (!m_connectionServer->connectionIsOpen(clientId)) {
      Logger::info("UniverseServer: Client {} connection lost", clientContext->descriptiveName());
      doDisconnection(clientId, String("You were disconnected due to a bad connection"));
    } else {
      if (clientContext->remoteAddress() && startTime - m_connectionServer->lastActivityTime(clientId) > timeout) {
        Logger::info("UniverseServer: Kicking client {} due to inactivity", clientContext->descriptiveName());
        doDisconnection(clientId, String("You were disconnected due to inactivity"));
      }
    }
  }

  // Once connections are waiting to close, send any pending data and wait up
  // to the connection timeout for the client to do the closing to ensure the
  // client has all the data.
  size_t previousDeadConnections = m_deadConnections.size();
  m_deadConnections.filter([startTime, timeout](auto& pair) {
    if (pair.first.send())
      pair.second = startTime;
    return pair.first.isOpen() && startTime - pair.second < timeout;
  });
  if (previousDeadConnections > m_deadConnections.size())
    Logger::info(previousDeadConnections == 1 ? "UniverseServer: Reaped {} dead connection" : "UniverseServer: Reaped {} dead connections", previousDeadConnections);
}

void UniverseServer::processPlanetTypeChanges() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);

  for (auto const& worldId : m_worlds.keys()) {
    if (auto celestialWorldId = worldId.ptr<CelestialWorldId>()) {
      if (auto world = getWorld(worldId)) {
        if (auto newPlanetType = world->pullNewPlanetType())
          updatePlanetType(*celestialWorldId, newPlanetType->first, newPlanetType->second);
      }
    }
  }
}

Maybe<WarpAction> UniverseServer::clientReturnWarp(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId)) {
    auto returnWarp = clientContext->playerReturnWarp();
    return returnWarp ? WarpAction(returnWarp) : Maybe<WarpAction>{};
  }
  return {};
}

Maybe<WarpAction> UniverseServer::clientReviveWarp(ConnectionId clientId) const {
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId)) {
    auto reviveWarp = clientContext->playerReviveWarp();
    return reviveWarp ? WarpAction(reviveWarp) : Maybe<WarpAction>{};
  }
  return {};
}

void UniverseServer::setClientReturnWarp(ConnectionId clientId, WarpAction warp) {
  if (auto warpToWorld = warp.maybe<WarpToWorld>()) {
    if (!(*warpToWorld)) return;

    RecursiveMutexLocker clientsLocker(m_clientsLock);
    if (auto clientContext = m_clients.value(clientId))
      clientContext->setPlayerReturnWarp(*warpToWorld);
  }
}

void UniverseServer::setClientReviveWarp(ConnectionId clientId, WarpAction warp) {
  if (auto warpToWorld = warp.maybe<WarpToWorld>()) {
    if (!(*warpToWorld)) return;

    RecursiveMutexLocker clientsLocker(m_clientsLock);
    if (auto clientContext = m_clients.value(clientId))
      clientContext->setPlayerReviveWarp(*warpToWorld);
  }
}

void UniverseServer::addPendingChatMessage(ConnectionId clientId, ChatReceivedMessage const& chatMessage) {
  // RecursiveMutexLocker locker(m_mainLock);
  m_chatProcessor->addPendingMessage(clientId, chatMessage);
}

Maybe<String> UniverseServer::clientTeam(ConnectionId clientId) const {
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId)) {
    auto team = m_teamManager->getTeam(clientContext->playerUuid());
    if (team.isValid())
      return team.value().hex();
    return {};
  }
  return {};
}

void UniverseServer::warpPlayers() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  for (auto const& clientId : m_pendingPlayerWarps.keys()) {
    auto& warp = m_pendingPlayerWarps.get(clientId);
    WarpAction warpAction = warp.first;
    bool deploy = warp.second;

    auto clientContext = m_clients.value(clientId);
    if (!clientContext)
      continue;

    WarpToWorld warpToWorld = resolveWarpAction(warpAction, clientId, deploy);

    if (auto maybeToWorld = triggerWorldCreation(warpToWorld.world)) {
      Logger::info("UniverseServer: Warping player {} to {}", clientId, printWarpAction(warpToWorld));
      if (auto toWorld = maybeToWorld.value()) {
        if (toWorld->spawnTargetValid(warpToWorld.target)) {
          if (auto currentWorld = clientContext->playerWorld()) {
            if (auto playerRevivePosition = currentWorld->playerRevivePosition(clientId)) {
              if (!m_rememberReturnWarpsOnDeath)
                clientContext->setPlayerReturnWarp(WarpToWorld{currentWorld->worldId(), SpawnTargetPosition(*playerRevivePosition)});
            }
            clientContext->clearPlayerWorld();
            m_connectionServer->sendPackets(clientId, currentWorld->removeClient(clientId));
            m_chatProcessor->leaveChannel(clientId, printWorldId(currentWorld->worldId()));
          }
          clientContext->setOrbitWarpAction({});

          // having stale world ids in the client context is bad,
          // make sure it's at least null until the next client context update
          sendClientContextUpdate(clientContext);

          // Checking the spawn target validity then adding the client is not
          // perfect, it can still become invalid in between, if we fail at
          // adding the client we need to warp them back.
          if (toWorld && toWorld->addClient(clientId, warpToWorld.target, !clientContext->remoteAddress(), clientContext->canBecomeAdmin(), clientContext->playerUuid(), clientContext->playerAccount(), clientContext->isGuest())) {
            clientContext->setPlayerWorld(toWorld);
            m_chatProcessor->joinChannel(clientId, printWorldId(warpToWorld.world));

            if (warpToWorld.world.is<ClientShipWorldId>()) {
              if (auto clientId = getClientForUuid(warpToWorld.world.get<ClientShipWorldId>())) {
                if (auto systemWorld = m_clients.get(*clientId)->systemWorld())
                  clientContext->setOrbitWarpAction(systemWorld->clientWarpAction(*clientId));
              }
            }
          } else if (auto returnWarp = clientContext->playerReturnWarp()) {
            Logger::info("UniverseServer: Warping player {} failed, returning to '{}'", clientId, printWarpAction(returnWarp));
            m_pendingPlayerWarps[clientId] = {returnWarp, false};
          } else {
            Logger::info("UniverseServer: Warping player {} failed, returning to ship", clientId);
            m_pendingPlayerWarps[clientId] = {WarpAlias::OwnShip, false};
          }
          m_connectionServer->sendPackets(clientId, {make_shared<PlayerWarpResultPacket>(true, warpAction, false)});
          m_pendingPlayerWarps.remove(clientId);
        } else {
          Logger::info("UniverseServer: Warping player {} failed, invalid spawn target '{}'", clientId, printSpawnTarget(warpToWorld.target));
          m_connectionServer->sendPackets(clientId, {make_shared<PlayerWarpResultPacket>(false, warpAction, true)});
          m_pendingPlayerWarps.remove(clientId);
        }
      } else {
        Logger::info("UniverseServer: Warping player {} failed, invalid world '{}' or world failed to load", clientId, printWorldId(warpToWorld.world));
        m_connectionServer->sendPackets(clientId, {make_shared<PlayerWarpResultPacket>(false, warpAction, false)});
        m_pendingPlayerWarps.remove(clientId);
      }
    } else {
      // If the world is not created yet, just set a new warp again to wait for
      // it to create.
      m_pendingPlayerWarps[clientId] = {warpAction, deploy};
    }
  }
}

void UniverseServer::flyShips() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  double queuedFlightWaitTime = Root::singleton().assets()->json("/universe_server.config:queuedFlightWaitTime").toDouble();
  for (auto clientId : m_queuedFlights.keys()) {
    if (!m_pendingFlights.contains(clientId) && !m_pendingArrivals.contains(clientId)) {
      auto& flight = m_queuedFlights.get(clientId);
      if (flight.second.isNothing())
        flight.second = m_universeClock->time() + queuedFlightWaitTime;
      else if (m_universeClock->time() > *flight.second)
        m_pendingFlights.set(clientId, flight.first);

      if (m_pendingFlights.contains(clientId))
        m_queuedFlights.remove(clientId);
    }
  }

  eraseWhere(m_pendingFlights, [this](pair<ConnectionId const, pair<Vec3I, SystemLocation>> const& p) {
    ConnectionId clientId = p.first;
    Vec3I system = p.second.first;
    SystemLocation location = p.second.second;

    auto clientContext = m_clients.value(clientId);
    if (!clientContext)
      return true;

    auto clientSystem = clientContext->systemWorld();
    if (!clientSystem)
      system = Vec3I();

    if (system != Vec3I() && clientContext->shipCoordinate().location() == system && clientContext->shipLocation() == location)
      return true;

    // if the ship is flying to another system do nothing
    // if the ship is flying within the target system, just update the ship destination
    if (m_pendingArrivals.contains(clientId)) {
      return true;
    }

    auto maybeClientShip = triggerWorldCreation(ClientShipWorldId(clientContext->playerUuid()));
    if (!maybeClientShip)
      return false; // ship is not loaded yet
    auto clientShip = *maybeClientShip;
    if (!clientShip)
      return true; // ship is broken

    CelestialCoordinate destination = location.maybe<CelestialCoordinate>().value(CelestialCoordinate(system));
    bool interstellar = clientSystem ? clientContext->shipCoordinate().location() != system : true;
    if (!interstellar) {
      // don't fly to null locations in the same system
      if (!location)
        return true;

      clientSystem->setClientDestination(clientId, location);
    } else if (system != Vec3I()) {
      // changing systems
      clientSystem->removeClient(clientId);
      clientContext->setSystemWorld({});

      if (location)
        m_queuedFlights.set(clientId, {{system, location}, {}});

      destination = CelestialCoordinate(system);
    }

    if (destination.isNull())
      Logger::info("Flying ship for player {} to new starter world", clientId);
    else
      Logger::info("Flying ship for player {} to {}", clientId, destination);

    bool startInWarp = system == Vec3I();
    clientShip->executeAction([interstellar, startInWarp](WorldServerThread*, WorldServer* worldServer) {
      worldServer->startFlyingSky(interstellar, startInWarp);
    });

    clientContext->setShipCoordinate(CelestialCoordinate(system));
    clientContext->setOrbitWarpAction({});
    for (auto clientId : clientShip->clients()) {
      if (auto clientContext = m_clients.get(clientId))
        clientContext->setOrbitWarpAction({});
    }

    m_pendingArrivals.set(clientId, destination);

    return true;
  });
}

void UniverseServer::arriveShips() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  eraseWhere(m_pendingArrivals, [this](pair<ConnectionId const, CelestialCoordinate>& p) {
    auto& clientId = p.first;
    auto& coordinate = p.second;

    if (!coordinate)
      coordinate = nextStarterWorld().value();

    if (!coordinate)
      return false;

    auto clientContext = m_clients.value(clientId);
    if (!clientContext)
      return true;

    auto clientSystem = clientContext->systemWorld();
    if (!clientSystem) {
      clientSystem = createSystemWorld(coordinate.location());
      if (coordinate.isSystem())
        clientSystem->addClient(clientId, clientContext->playerUuid(), clientContext->shipUpgrades().shipSpeed, {});
      else
        clientSystem->addClient(clientId, clientContext->playerUuid(), clientContext->shipUpgrades().shipSpeed, coordinate);

      clientContext->setSystemWorld(clientSystem);
    }

    auto location = clientSystem->clientShipLocation(clientId);
    if (!location)
      return false;

    if (!coordinate.isSystem() && !triggerWorldCreation(CelestialWorldId(coordinate)))
      return false;

    Logger::info("UniverseServer: Arriving ship for player {} at {}", clientId, coordinate);

    // world is loaded, ship has arrived
    clientContext->setShipCoordinate(coordinate);
    clientContext->setShipLocation(location);

    if (auto clientShip = createWorld(ClientShipWorldId(clientContext->playerUuid()))) {
      auto skyParameters = clientSystem->clientSkyParameters(clientId);
      clientShip->executeAction([skyParameters](WorldServerThread*, WorldServer* worldServer) {
        worldServer->stopFlyingSkyAt(skyParameters);
      });

      for (auto shipClientId : clientShip->clients()) {
        if (auto clientContext = m_clients.get(shipClientId))
          clientContext->setOrbitWarpAction(clientSystem->clientWarpAction(clientId));
      }
    }
    return true;
  });
}

void UniverseServer::respondToCelestialRequests() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  for (auto& p : m_pendingCelestialRequests) {
    List<CelestialResponse> responses;
    eraseWhere(p.second, [&responses](WorkerPoolPromise<CelestialResponse> const& request) {
      if (request.poll()) {
        responses.append(request.get());
        return true;
      }
      return false;
    });
    if (m_clients.contains(p.first))
      m_connectionServer->sendPackets(p.first, {make_shared<CelestialResponsePacket>(std::move(responses))});
  }
  eraseWhere(m_pendingCelestialRequests, [](auto const& p) {
    return p.second.empty();
  });
}

void UniverseServer::processChat() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  for (auto const& p : take(m_pendingChat)) {
    if (auto clientContext = m_clients.value(p.first)) {
      for (auto const& chat : p.second) {
        if (clientContext->remoteAddress())
          Logger::info("Chat: <{}> {}", clientContext->playerName(), chat.message);

        auto team = m_teamManager->getTeam(clientContext->playerUuid());
        if (chat.sendMode == ChatSendMode::Broadcast)
          m_chatProcessor->broadcast(p.first, chat.message, chat.metadata);
        else if (chat.sendMode == ChatSendMode::Party && team.isValid())
          m_chatProcessor->message(p.first, MessageContext::Mode::Party, team.value().hex(), chat.message, chat.metadata);
        else
          m_chatProcessor->message(p.first, MessageContext::Mode::Local, printWorldId(clientContext->playerWorldId()), chat.message, chat.metadata);
      }
    }
  }
}

void UniverseServer::clearBrokenWorlds() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);

  if (Time::monotonicMilliseconds() >= m_clearBrokenWorldsDeadline) {
    // Clear out all broken worlds
    eraseWhere(m_worlds, [](auto const& p) {
      if (!p.second.isValid()) {
        Logger::info("UniverseServer: Clearing broken world {}", p.first);
        return true;
      } else {
        return false;
      }
    });

    int clearBrokenWorldsInterval = Root::singleton().assets()->json("/universe_server.config:clearBrokenWorldsInterval").toInt();
    m_clearBrokenWorldsDeadline = Time::monotonicMilliseconds() + clearBrokenWorldsInterval;
  }
}

void UniverseServer::handleWorldMessages() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  auto it = m_pendingWorldMessages.begin();
  while (it != m_pendingWorldMessages.end()) {
    auto& worldId = it->first;
    if (auto worldResult = triggerWorldCreation(worldId)) {
      auto& world = *worldResult;

      if (world)
        world->passMessages(std::move(it->second));
      else
        for (auto& message : it->second)
          message.promise.fail("Error creating world");

      it = m_pendingWorldMessages.erase(it);
    } else
      ++it;
  }
}

void UniverseServer::shutdownInactiveWorlds() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  // Shutdown idle and errored worlds.
  for (auto const& worldId : m_worlds.keys()) {
    if (auto world = getWorld(worldId)) {
      if (world->serverErrorOccurred()) {
        world->stop();
        Logger::error("UniverseServer: World {} has stopped due to an error", worldId);
        worldDiedWithError(world->worldId());
      } else if (world->noClients()) {
        bool anyPendingWarps = false;
        for (auto const& p : m_pendingPlayerWarps) {
          if (resolveWarpAction(p.second.first, p.first, p.second.second).world == world->worldId()) {
            anyPendingWarps = true;
            break;
          }
        }

        if (!anyPendingWarps && world->shouldExpire()) {
          Logger::info("UniverseServer: Stopping idle world {}", worldId);
          world->stop();
        }
      }

      if (world->isJoined()) {
        auto kickClients = world->clients();
        if (!kickClients.empty()) {
          Logger::info("UniverseServer: World {} shutdown, kicking {} players to their own ships", worldId, world->clients().size());
          for (auto clientId : world->clients())
            clientWarpPlayer(clientId, WarpAlias::OwnShip);
        }

        if (worldId.is<ClientShipWorldId>()) {
          if (auto clientId = getClientForUuid(worldId.get<ClientShipWorldId>()))
            m_clients.get(*clientId)->updateShipChunks(world->readChunks());
        }

        m_worlds.remove(worldId);
        // Once a world is shutdown, mark its shutdown time in m_tempWorldIndex
        if (auto instanceWorldId = worldId.maybe<InstanceWorldId>()) {
          if (m_tempWorldIndex.contains(*instanceWorldId))
            m_tempWorldIndex[*instanceWorldId].first = m_universeClock->milliseconds();
        }
      }
    }
  }

  // Clear out all temporary worlds shut down more than tempWorldDeleteTime time ago.
  // Keep around worlds that are currently running or are active in system worlds
  Set<InstanceWorldId> systemLocationWorlds;
  for (auto p : m_systemWorlds) {
    for (auto instanceWorldId : p.second->activeInstanceWorlds()) {
      if (m_tempWorldIndex.contains(instanceWorldId))
        systemLocationWorlds.add(instanceWorldId);
    }
  }
  eraseWhere(m_tempWorldIndex, [this, systemLocationWorlds](pair<InstanceWorldId, pair<uint64_t, uint64_t>> const& p) {
    String storageFile = tempWorldFile(p.first);
    if (!m_worlds.contains(WorldId(p.first)) && !systemLocationWorlds.contains(p.first) && m_universeClock->milliseconds() > int64_t(p.second.first + p.second.second)) {
      Logger::info("UniverseServer: Expiring temporary world {}", printWorldId(p.first));
      if (File::isFile(storageFile))
        File::remove(storageFile);
      return true;
    }
    return false;
  });

  // Clear out empty system worlds
  eraseWhere(m_systemWorlds, [](pair<Vec3I, SystemWorldServerThreadPtr> w) {
    return w.second->clients().empty();
  });
}

void UniverseServer::doTriggeredStorage() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  if (Time::monotonicMilliseconds() >= m_storageTriggerDeadline) {
    Logger::debug("UniverseServer: periodic sync to disk");
    saveSettings();
    saveTempWorldIndex();

    for (auto const& p : m_clients) {
      if (auto shipWorld = getWorld(ClientShipWorldId(p.second->playerUuid())))
        p.second->updateShipChunks(shipWorld->readChunks());

      auto versioningDatabase = Root::singleton().versioningDatabase();
      String clientContextFile = File::relativeTo(m_storageDirectory, strf("{}.clientcontext", p.second->playerUuid().hex()));
      VersionedJson::writeFile(versioningDatabase->makeCurrentVersionedJson("ClientContext", p.second->storeServerData()), clientContextFile);
    }

    int storageTriggerInterval = Root::singleton().assets()->json("/universe_server.config:universeStorageInterval").toInt();
    m_storageTriggerDeadline = Time::monotonicMilliseconds() + storageTriggerInterval;

    m_celestialDatabase->cleanupAndCommit();
  }
}

void UniverseServer::saveSettings() {
  RecursiveMutexLocker locker(m_mainLock);
  auto versioningDatabase = Root::singleton().versioningDatabase();
  auto versionedSettings = versioningDatabase->makeCurrentVersionedJson("UniverseSettings",
      m_universeSettings->toJson().set("time", m_universeClock->time()));
  VersionedJson::writeFile(versionedSettings, File::relativeTo(m_storageDirectory, "universe.dat"));
  if (m_serverData) {
    auto versionedServerData = versioningDatabase->makeCurrentVersionedJson("ServerData", m_serverData);
    VersionedJson::writeFile(versionedServerData, File::relativeTo(m_storageDirectory, "server.dat"));
  } else {
    Logger::warn("[xSB] UniverseServer: Server data not yet loaded");
  }
}

void UniverseServer::loadSettings() {
  RecursiveMutexLocker locker(m_mainLock);

  auto loadDefaultSettings = [this]() {
    m_universeClock = make_shared<Clock>();
    m_universeSettings = make_shared<UniverseSettings>();
  };

  auto versioningDatabase = Root::singleton().versioningDatabase();
  auto storageFile = File::relativeTo(m_storageDirectory, "universe.dat");
  if (File::isFile(storageFile)) {
    try {
      auto settings = versioningDatabase->loadVersionedJson(VersionedJson::readFile(storageFile), "UniverseSettings");
      m_universeSettings = make_shared<UniverseSettings>(settings);
      m_universeClock = make_shared<Clock>();
      m_universeClock->setTime(settings.getDouble("time"));
    } catch (std::exception const& e) {
      Logger::error("UniverseServer: Could not load universe settings file, loading defaults: {}", outputException(e, false));
      File::rename(storageFile, strf("{}.{}.fail", storageFile, Time::millisecondsSinceEpoch()));
      loadDefaultSettings();
    }
  } else {
    loadDefaultSettings();
  }

  m_universeClock->start();

  auto loadBlankServerData = [this]() {
    m_serverData = JsonObject{};
  };

  auto serverDataFile = File::relativeTo(m_storageDirectory, "server.dat");
  if (File::isFile(serverDataFile)) {
    try {
      m_serverData = versioningDatabase->loadVersionedJson(VersionedJson::readFile(serverDataFile), "ServerData");
    } catch (std::exception const& e) {
      Logger::error("[xSB] UniverseServer: Could not load server data file, loading blank file: {}", outputException(e, false));
      File::rename(serverDataFile, strf("{}.{}.fail", serverDataFile, Time::millisecondsSinceEpoch()));
      loadBlankServerData();
    }
  } else {
    loadBlankServerData();
  }
}

Json UniverseServer::getServerData(String const& key) const {
  RecursiveMutexLocker locker(m_mainLock);
  if (m_serverData)
    return m_serverData.get(key, Json());
  return Json();
}

Json UniverseServer::getServerDataPath(String const& path) const {
  RecursiveMutexLocker locker(m_mainLock);
  if (m_serverData)
    return m_serverData.query(path, Json());
  return Json();
}

void UniverseServer::setServerData(String const& key, Json const& value) {
  RecursiveMutexLocker locker(m_mainLock);
  if (m_serverData)
    m_serverData = m_serverData.set(key, value);
}

void UniverseServer::setServerDataPath(String const& path, Json const& value) {
  RecursiveMutexLocker locker(m_mainLock);
  if (m_serverData) {
    if (path.empty() && !value.isType(Json::Type::Object)) return; // FezzedOne: Keeps the root object an object.
    m_serverData = m_serverData.setPath(path, value);
  }
}

void UniverseServer::eraseServerData(String const& key) {
  RecursiveMutexLocker locker(m_mainLock);
  if (m_serverData)
    m_serverData = m_serverData.eraseKey(key);
}

void UniverseServer::eraseServerDataPath(String const& path) {
  RecursiveMutexLocker locker(m_mainLock);
  if (m_serverData) {
    if (path.empty()) return; // FezzedOne: Keeps the root object an object.
    m_serverData = m_serverData.erasePath(path);
  }
}

Maybe<CelestialCoordinate> UniverseServer::nextStarterWorld() {
  RecursiveMutexLocker locker(m_mainLock);

  auto assets = Root::singleton().assets();
  String defaultWorldCoordinate = assets->json("/universe_server.config:defaultWorldCoordinate").toString();
  if (!defaultWorldCoordinate.empty())
    return CelestialCoordinate(defaultWorldCoordinate);

  if (m_nextRandomizedStarterWorld && m_nextRandomizedStarterWorld->done()) {
    CelestialCoordinate nextWorld = m_nextRandomizedStarterWorld->get();
    m_nextRandomizedStarterWorld.reset();
    return nextWorld;
  }

  if (!m_nextRandomizedStarterWorld) {
    m_nextRandomizedStarterWorld = m_workerPool.addProducer<CelestialCoordinate>([assets, celestialDatabase = m_celestialDatabase]() {
      Logger::info("Searching for new randomized starter world");
      auto filterWorld = [celestialDatabase](CelestialCoordinate const& coordinate, Json const& filter) {
        auto parameters = celestialDatabase->parameters(coordinate);
        auto visitableParameters = parameters->visitableParameters();
        if (!visitableParameters)
          return false;

        if (auto biome = filter.optString("terrestrialBiome")) {
          auto terrestrialParameters = as<TerrestrialWorldParameters>(visitableParameters);
          if (!terrestrialParameters || *biome != terrestrialParameters->primaryBiome)
            return false;
        }

        if (auto size = filter.optString("terrestrialSize")) {
          auto terrestrialParameters = as<TerrestrialWorldParameters>(visitableParameters);
          if (!terrestrialParameters || *size != terrestrialParameters->sizeName)
            return false;
        }

        if (auto dungeon = filter.optString("floatingDungeon")) {
          auto dungeonParameters = as<FloatingDungeonWorldParameters>(visitableParameters);
          if (!dungeonParameters || *dungeon != dungeonParameters->primaryDungeon)
            return false;
        }

        return true;
      };

      auto findParameters = assets->json("/universe_server.config:findStarterWorldParameters");
      auto randomWorld = celestialDatabase->findRandomWorld(findParameters.getUInt("tries"), findParameters.getUInt("range"), [&](CelestialCoordinate const& coordinate) {
        if (!filterWorld(coordinate, findParameters.get("starterWorld")))
          return false;

        List<CelestialCoordinate> allChildren;
        for (auto const& planet : celestialDatabase->children(coordinate.system())) {
          allChildren.append(planet);
          for (auto const& satellite : celestialDatabase->children(planet))
            allChildren.append(satellite);
        }

        for (auto const& requiredSystemWorld : findParameters.getArray("requiredSystemWorlds", {})) {
          bool worldFound = false;
          for (auto const& world : allChildren) {
            if (filterWorld(world, requiredSystemWorld)) {
              worldFound = true;
              break;
            }
          }

          if (!worldFound)
            return false;
        }

        return true;
      });

      if (randomWorld)
        Logger::info("UniverseServer: Found randomized starter world at {}", *randomWorld);
      else
        Logger::error("UniverseServer: Could not find randomized starter world!");

      return randomWorld.value();
    });
  }

  return {};
}

void UniverseServer::loadTempWorldIndex() {
  auto versioningDatabase = Root::singleton().versioningDatabase();
  auto storageFile = File::relativeTo(m_storageDirectory, "tempworlds.index");
  if (File::isFile(storageFile)) {
    try {
      m_tempWorldIndex.clear();
      auto settings = versioningDatabase->loadVersionedJson(VersionedJson::readFile(storageFile), "TempWorldIndex");
      for (auto p : settings.iterateObject()) {
        WorldId worldId = parseWorldId(p.first);
        pair<uint64_t, uint64_t> deleteTime = {p.second.get(0).toUInt(), p.second.get(1).toUInt()};
        m_tempWorldIndex.insert(worldId.get<InstanceWorldId>(), deleteTime);
      }
    } catch (std::exception const& e) {
      Logger::error("UniverseServer: Could not load temp world index file", outputException(e, false));
      File::rename(storageFile, strf("{}.{}.fail", storageFile, Time::millisecondsSinceEpoch()));
    }
  }

  // delete temporary instance worlds not found in the index on load
  auto tempWorldFiles = m_tempWorldIndex.keys().transformed([this](InstanceWorldId const& worldId) { return tempWorldFile(worldId); });
  for (auto p : File::dirList(m_storageDirectory)) {
    if (p.second == false && p.first.endsWith(".tempworld")) {
      String storageFile = File::relativeTo(m_storageDirectory, p.first);
      if (!tempWorldFiles.contains(storageFile)) {
        Logger::info("UniverseServer: Removing unindexed temporary world {}", p.first);
        File::remove(storageFile);
      }
    }
  }
}

void UniverseServer::saveTempWorldIndex() {
  JsonObject worldIndex = JsonObject();
  for (auto p : m_tempWorldIndex) {
    worldIndex.set(printWorldId(p.first), JsonArray{p.second.first, p.second.second});
  }

  auto versioningDatabase = Root::singleton().versioningDatabase();
  auto versionedJson = versioningDatabase->makeCurrentVersionedJson("TempWorldIndex", worldIndex);
  VersionedJson::writeFile(versionedJson, File::relativeTo(m_storageDirectory, "tempworlds.index"));
}

String UniverseServer::tempWorldFile(InstanceWorldId const& worldId) const {
  String identifier = worldId.instance;
  if (worldId.uuid)
    identifier = strf("{}-{}", identifier, worldId.uuid->hex());
  if (worldId.level)
    identifier = strf("{}-{}", identifier, worldId.level.value());
  return File::relativeTo(m_storageDirectory, strf("{}.tempworld", identifier));
}

Maybe<String> UniverseServer::isBannedUser(Maybe<HostAddress> hostAddress, Uuid playerUuid) const {
  RecursiveMutexLocker locker(m_mainLock);
  auto config = Root::singleton().configuration();

  if (hostAddress) {
    for (auto const& ban : m_tempBans) {
      if (ban.ip) {
        if (*ban.ip == *hostAddress) {
          return ban.reason;
        }
      }
    }

    for (auto banEntry : config->get("bannedIPs").iterateArray()) {
      if (HostAddress(banEntry.getString("ip")) == *hostAddress) {
        return banEntry.getString("reason");
      }
    }
  }

  for (auto const& ban : m_tempBans) {
    if (ban.uuid) {
      if (*ban.uuid == playerUuid)
        return ban.reason;
    }
  }

  for (auto banEntry : config->get("bannedUuids").iterateArray()) {
    if (Uuid(banEntry.getString("uuid")) == playerUuid)
      return banEntry.getString("reason");
  }

  return {};
}

void UniverseServer::doTempBan(ConnectionId clientId, String const& reason, pair<bool, bool> banType, int timeout) {
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  if (auto clientContext = m_clients.value(clientId)) {
    if (!clientContext->remoteAddress())
      return;

    auto banExpiry = Time::monotonicMilliseconds() + timeout * 1000; // current time is in millis, conversion factor
    Maybe<HostAddress> ip;
    if (banType.first)
      ip = clientContext->remoteAddress();

    Maybe<Uuid> uuid;

    if (banType.second)
      uuid = clientContext->playerUuid();

    m_tempBans.append({banExpiry, reason, ip, uuid});
  }
}

void UniverseServer::doPermBan(ConnectionId clientId, String const& reason, pair<bool, bool> banType) {
  RecursiveMutexLocker locker(m_mainLock);
  RecursiveMutexLocker clientsLocker(m_clientsLock);

  if (auto clientContext = m_clients.value(clientId)) {
    if (!clientContext->remoteAddress())
      return;

    auto config = Root::singleton().configuration();
    if (banType.first) {
      auto bannedIPs = config->get("bannedIPs").toArray();

      bannedIPs.append(JsonObject{
          {"ip", toString(*clientContext->remoteAddress())},
          {"reason", reason},
      });

      config->set("bannedIPs", bannedIPs);
    }

    if (banType.second) {
      auto bannedUuids = config->get("bannedUuids").toArray();

      bannedUuids.append(JsonObject{
          {"uuid", clientContext->playerUuid().hex()},
          {"reason", reason},
      });

      config->set("bannedUuids", bannedUuids);
    }
  }
}

void UniverseServer::removeTimedBan() {
  ZoneScoped;
  RecursiveMutexLocker locker(m_mainLock);
  auto currentTime = Time::monotonicMilliseconds();
  eraseWhere(m_tempBans, [currentTime](TimeoutBan const& b) {
    return b.banExpiry <= currentTime;
  });
}

void UniverseServer::addCelestialRequests(ConnectionId clientId, List<CelestialRequest> requests) {
  RecursiveMutexLocker locker(m_mainLock);
  for (auto request : requests) {
    m_pendingCelestialRequests[clientId].append(m_workerPool.addProducer<CelestialResponse>([this, request]() {
      return m_celestialDatabase->respondToRequest(request);
    }));
  }
}

void UniverseServer::worldUpdated(WorldServerThread* server) {
  for (auto clientId : server->clients()) {
    auto packets = server->pullOutgoingPackets(clientId);
    m_connectionServer->sendPackets(clientId, std::move(packets));
  }
}

void UniverseServer::systemWorldUpdated(SystemWorldServerThread* systemWorldServer) {
  for (auto clientId : systemWorldServer->clients()) {
    auto packets = systemWorldServer->pullOutgoingPackets(clientId);
    m_connectionServer->sendPackets(clientId, std::move(packets));
  }
}

void UniverseServer::packetsReceived(UniverseConnectionServer*, ConnectionId clientId, List<PacketPtr> packets) {
  ZoneScoped;
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clientContext = m_clients.value(clientId)) {
    clientsLocker.unlock();

    for (auto& packet : packets) {
      if (auto warpAction = as<PlayerWarpPacket>(packet)) {
        clientWarpPlayer(clientId, warpAction->action, warpAction->deploy);

      } else if (auto flyShip = as<FlyShipPacket>(packet)) {
        clientFlyShip(clientId, flyShip->system, flyShip->location);

      } else if (auto chatSend = as<ChatSendPacket>(packet)) {
        RecursiveMutexLocker locker(m_mainLock);
        m_pendingChat[clientId].append({std::move(chatSend->text), chatSend->sendMode, chatSend->data});

      } else if (auto clientContextUpdatePacket = as<ClientContextUpdatePacket>(packet)) {
        clientContext->readUpdate(std::move(clientContextUpdatePacket->updateData));

      } else if (auto clientDisconnectPacket = as<ClientDisconnectRequestPacket>(packet)) {
        disconnectClient(clientId, String());

      } else if (auto celestialRequest = as<CelestialRequestPacket>(packet)) {
        addCelestialRequests(clientId, std::move(celestialRequest->requests));

      } else if (is<SystemObjectSpawnPacket>(packet)) {
        if (auto currentSystem = clientContext->systemWorld()) {
          // FezzedOne: Build permission check on world spawns in systems. If the client doesn't have permission, no system object will be spawned.
          if (clientHasBuildPermission(clientContext, currentSystem, {}))
            currentSystem->pushIncomingPacket(clientId, std::move(packet));
        }
      } else {
        if (auto currentWorld = clientContext->playerWorld())
          currentWorld->pushIncomingPackets(clientId, {std::move(packet)});
      }
    }
  } else {
    Logger::warn("UniverseServer: Packet sent from invalid cID {}", clientId);
  }
}

void UniverseServer::acceptConnection(UniverseConnection connection, Maybe<HostAddress> remoteAddress) {
  auto& root = Root::singleton();
  auto assets = root.assets();
  auto configuration = root.configuration();
  auto versioningDatabase = root.versioningDatabase();

  int clientWaitLimit = assets->json("/universe_server.config:clientWaitLimit").toInt();
  String serverAssetsMismatchMessage = assets->json("/universe_server.config:serverAssetsMismatchMessage").toString();
  String clientAssetsMismatchMessage = assets->json("/universe_server.config:clientAssetsMismatchMessage").toString();

  // FezzedOne: Due to networking code changes in OpenStarbound that cause compatibility issues, added a
  // `"forceLegacyConnection"` setting to the server's `xserver.config` or host's `xclient.config`.
  bool forceLegacyConnection = root.configuration()->get("forceLegacyConnection").optBool().value(false);

  RecursiveMutexLocker mainLocker(m_mainLock, false);

  connection.receiveAny(clientWaitLimit);
  auto protocolRequest = as<ProtocolRequestPacket>(connection.pullSingle());
  if (!protocolRequest) {
    Logger::warn("UniverseServer: Client connection aborted, expected ProtocolRequestPacket");
    return;
  }

  bool legacyClient = protocolRequest->compressionMode() != PacketCompressionMode::Enabled;
  bool legacyConnection = forceLegacyConnection || legacyClient;
  connection.setLegacy(legacyConnection);
  if (forceLegacyConnection && !legacyClient)
    Logger::info("UniverseServer: Detected connection from custom client, but forcing legacy protocol");

  auto protocolResponse = make_shared<ProtocolResponsePacket>();
  // FezzedOne: Signal that we're a custom server unless legacy connections are forced.
  // Since xStarbound now changes the networking protocol for xStarbound clients, check the xSB protocol version number if necessary.
  // This kicks OpenStarbound clients!
  protocolResponse->setCompressionMode(forceLegacyConnection ? PacketCompressionMode::Disabled : PacketCompressionMode::Enabled);
  if (protocolRequest->requestProtocolVersion != (legacyConnection ? StarProtocolVersion : xSbProtocolVersion)) {
    Logger::warn("UniverseServer: Client connection aborted, unsupported {} protocol version {}, supported version {}",
        legacyConnection ? "legacy" : "xStarbound", protocolRequest->requestProtocolVersion, StarProtocolVersion);
    protocolResponse->allowed = false;
    connection.pushSingle(protocolResponse);
    connection.sendAll(clientWaitLimit);
    mainLocker.lock();
    m_deadConnections.append({std::move(connection), Time::monotonicMilliseconds()});
    return;
  }

  protocolResponse->allowed = true;
  connection.pushSingle(protocolResponse);
  connection.sendAll(clientWaitLimit);

  String remoteAddressString = remoteAddress ? toString(*remoteAddress) : "local";
  Logger::info("UniverseServer: Awaiting connection info from {}, {} client", remoteAddressString, legacyClient ? "stock" : "custom");

  connection.receiveAny(clientWaitLimit);
  auto clientConnect = as<ClientConnectPacket>(connection.pullSingle());
  if (!clientConnect) {
    Logger::warn("UniverseServer: Invalid client connection aborted");
    connection.pushSingle(make_shared<ConnectFailurePacket>("connect timeout"));
    mainLocker.lock();
    m_deadConnections.append({std::move(connection), Time::monotonicMilliseconds()});
    return;
  }

  bool administrator = false;
  bool isGuest = false;

  Maybe<String> accountName = !clientConnect->account.empty() ? clientConnect->account : Maybe<String>{};
  // String accountString = accountName ? strf("'{}'", *accountName) : "<anonymous>";

  auto connectionFail = [&](String message) {
    if (accountName)
      Logger::warn("UniverseServer: Login attempt failed with account '{}' as player '{}' from address {}, error: {}",
          *accountName, clientConnect->playerName, remoteAddressString, message);
    else
      Logger::warn("UniverseServer: Anonymous connection attempt failed as player '{}' from address {}, error: {}",
          clientConnect->playerName, remoteAddressString, message);
    connection.pushSingle(make_shared<ConnectFailurePacket>(std::move(message)));
    mainLocker.lock();
    m_deadConnections.append({std::move(connection), Time::monotonicMilliseconds()});
  };

  if (!remoteAddress) {
    administrator = true;
    isGuest = false;
    Logger::info("UniverseServer: Logged in player '{}' locally", clientConnect->playerName);
  } else {
    if (clientConnect->assetsDigest != m_assetsDigest) {
      if (!configuration->get("allowAssetsMismatch").toBool()) {
        connectionFail(serverAssetsMismatchMessage);
        return;
      } else if (!clientConnect->allowAssetsMismatch) {
        connectionFail(clientAssetsMismatchMessage);
        return;
      }
    }

    if (!m_speciesShips.contains(clientConnect->playerSpecies)) {
      connectionFail("Unknown player species. Ensure your mods are compatible with this server.");
      Logger::warn("UniverseServer: Unknown species name is '{}'", clientConnect->playerSpecies);
      return;
    }

    if (accountName) {
      auto passwordSalt = secureRandomBytes(assets->json("/universe_server.config:passwordSaltLength").toUInt());
      Logger::info("UniverseServer: Sending handshake challenge");
      connection.pushSingle(make_shared<HandshakeChallengePacket>(passwordSalt));
      connection.sendAll(clientWaitLimit);
      connection.receiveAny(clientWaitLimit);
      shared_ptr<HandshakeResponsePacket> handshakeResponsePacket = as<HandshakeResponsePacket>(connection.pullSingle());
      if (!handshakeResponsePacket) {
        connectionFail("Expected HandshakeResponsePacket. Check your client and network configuration.");
        return;
      }

      bool success = false;
      if (Json account = configuration->get("serverUsers").get(*accountName, {})) {
        administrator = account.getBool("admin", false);
        isGuest = account.getBool("guest", false);
        ByteArray passAccountSalt = (account.getString("password") + (*accountName)).utf8Bytes();
        passAccountSalt.append(passwordSalt);
        ByteArray passHash = sha256(passAccountSalt);
        if (passHash == handshakeResponsePacket->passHash)
          success = true;
      }
      // Give the same message for missing account vs wrong password to
      // prevent account detection, overkill given the overall level of
      // security but hey, why not.
      if (!success) {
        connectionFail(strf("No such account '{}' or incorrect password.", clientConnect->account));
        return;
      }
    } else {
      if (!configuration->get("allowAnonymousConnections").toBool()) {
        connectionFail("Anonymous connections disallowed.");
        return;
      }
      administrator = configuration->get("anonymousConnectionsAreAdmin").toBool();
      isGuest = true;
    }

    if (auto reason = isBannedUser(remoteAddress, clientConnect->playerUuid)) {
      connectionFail("You have been banned: " + *reason);
      return;
    }
  }

  if (accountName)
    Logger::info("UniverseServer: Logged in account '{}' as player '{}' from address {}",
        *accountName, clientConnect->playerName, remoteAddressString);
  else
    Logger::info("UniverseServer: Logged in anonymously as player '{}' from address {}",
        clientConnect->playerName, remoteAddressString);

  mainLocker.lock();
  RecursiveMutexLocker clientsLocker(m_clientsLock);
  if (auto clashId = getClientForUuid(clientConnect->playerUuid)) {
    if (administrator) {
      doDisconnection(*clashId, String("Duplicate player UUID; prioritised newly joined admin player"));
    } else {
      connectionFail("Duplicate player UUID");
      return;
    }
  }

  if (m_clients.size() + 1 > m_maxPlayers && !administrator) {
    connectionFail("Server is at its maximum player limit. Try reconnecting later.");
    return;
  }

  ConnectionId clientId = m_clients.nextId();
  auto clientContext = make_shared<ServerClientContext>(clientId, remoteAddress, clientConnect->playerUuid,
      clientConnect->playerName, clientConnect->playerSpecies, administrator, clientConnect->shipChunks, isGuest, accountName);
  m_clients.add(clientId, clientContext);
  clientsLocker.unlock();

  clientContext->registerRpcHandlers(m_teamManager->rpcHandlers());

  String clientContextFile = File::relativeTo(m_storageDirectory, strf("{}.clientcontext", clientConnect->playerUuid.hex()));
  if (File::isFile(clientContextFile)) {
    try {
      auto contextStore = versioningDatabase->loadVersionedJson(VersionedJson::readFile(clientContextFile), "ClientContext");
      clientContext->loadServerData(contextStore);
    } catch (std::exception const& e) {
      Logger::error("UniverseServer: Could not load client context file for <User: {}>, ignoring! {}",
          clientConnect->playerName, outputException(e, false));
      File::rename(clientContextFile, strf("{}.{}.fail", clientContextFile, Time::millisecondsSinceEpoch()));
    }
  }

  // Need to do this after loadServerData because it sets the admin flag
  if (!administrator)
    clientContext->setAdmin(false);

  clientContext->setShipUpgrades(clientConnect->shipUpgrades);

  m_connectionServer->addConnection(clientId, std::move(connection));
  m_chatProcessor->connectClient(clientId, clientConnect->playerName);

  m_connectionServer->sendPackets(clientId, {make_shared<ConnectSuccessPacket>(clientId, m_universeSettings->uuid(), m_celestialDatabase->baseInformation()),
                                                make_shared<UniverseTimeUpdatePacket>(m_universeClock->time())});

  setPvp(clientId, false);

  Vec3I location = clientContext->shipCoordinate().location();
  if (location != Vec3I()) {
    auto clientSystem = createSystemWorld(location);
    clientSystem->addClient(clientId, clientContext->playerUuid(), clientContext->shipUpgrades().shipSpeed, clientContext->shipLocation());
    // Kae's fix for an issue where the client's celestial chunk isn't immediately loaded.
    addCelestialRequests(clientId, {makeLeft(location.vec2()), makeRight(location)});
    clientContext->setSystemWorld(clientSystem);
  }

  Json introInstance = assets->json("/universe_server.config:introInstance");
  String speciesIntroInstance = introInstance.getString(clientConnect->playerSpecies, introInstance.getString("default", ""));
  // FezzedOne: If the specified intro instance is an empty string, the intro is skipped.
  if (!speciesIntroInstance.empty() && !clientConnect->introComplete) {
    Logger::info("UniverseServer: Spawning player in intro instance {}", speciesIntroInstance);
    WarpAction introWarp = WarpToWorld{InstanceWorldId(speciesIntroInstance, clientContext->playerUuid()), {}};
    clientWarpPlayer(clientId, introWarp);
  } else if (auto reviveWarp = clientContext->playerReviveWarp()) {
    // Do not revive players at non-persistent instance worlds or on ship worlds that
    // are not their own ship.
    bool useReviveWarp = true;
    if (reviveWarp.world.is<InstanceWorldId>()) {
      String instance = reviveWarp.world.get<InstanceWorldId>().instance;
      Json worldConfig = Root::singleton().assets()->json("/instance_worlds.config").get(instance);
      if (!worldConfig.getBool("persistent", false))
        useReviveWarp = false;
    }

    if (reviveWarp.world.is<ClientShipWorldId>() && reviveWarp.world.get<ClientShipWorldId>() != clientConnect->playerUuid)
      useReviveWarp = false;

    if (useReviveWarp) {
      Logger::info("UniverseServer: Reviving player at {}", reviveWarp.world);
      clientWarpPlayer(clientId, reviveWarp);
    } else {
      Logger::info("UniverseServer: Player revive position is expired, spawning back at own ship");
      clientWarpPlayer(clientId, WarpAlias::OwnShip);
    }
  } else {
    Maybe<String> defaultReviveWarp = assets->json("/universe_server.config").optString("defaultReviveWarp");
    if (defaultReviveWarp) {
      Logger::info("UniverseServer: Spawning player at default warp");
      clientWarpPlayer(clientId, parseWarpAction(*defaultReviveWarp));
    } else {
      Logger::info("UniverseServer: Spawning player at ship");
      clientWarpPlayer(clientId, WarpAlias::OwnShip);
    }
  }

  clientFlyShip(clientId, clientContext->shipCoordinate().location(), clientContext->shipLocation());
  Logger::info("UniverseServer: Client {} connected", clientContext->descriptiveName());

  RecursiveMutexLocker clientsReadLocker(m_clientsLock);
  auto players = static_cast<uint16_t>(m_clients.size());
  auto clients = m_clients.keys();
  clientsReadLocker.unlock();

  for (auto clientId : clients) {
    m_connectionServer->sendPackets(clientId, {make_shared<ServerInfoPacket>(players, static_cast<uint16_t>(m_maxPlayers))});
  }
}

WarpToWorld UniverseServer::resolveWarpAction(WarpAction warpAction, ConnectionId clientId, bool deploy) const {
  auto clientContext = m_clients.value(clientId);
  if (!clientContext)
    return {};

  WorldId toWorldId;
  SpawnTarget spawnTarget;
  if (auto toWorld = warpAction.ptr<WarpToWorld>()) {
    if (!toWorld->world)
      toWorldId = clientContext->playerWorldId();
    else
      toWorldId = toWorld->world;
    spawnTarget = toWorld->target;
  } else if (auto toPlayerUuid = warpAction.ptr<WarpToPlayer>()) {
    if (auto toClientId = getClientForUuid(*toPlayerUuid)) {
      if (auto toClientWorld = m_clients.get(*toClientId)->playerWorld()) {
        if (auto toClientPosition = toClientWorld->playerRevivePosition(*toClientId)) {
          toWorldId = toClientWorld->worldId();
          if (deploy)
            spawnTarget.reset();
          else
            spawnTarget = SpawnTargetPosition(*toClientPosition);
        }
      }
    }
  } else if (auto shortcut = warpAction.ptr<WarpAlias>()) {
    if (*shortcut == WarpAlias::Return) {
      if (auto returnWarp = clientContext->playerReturnWarp()) {
        toWorldId = returnWarp.world;
        spawnTarget = returnWarp.target;
      }
    } else if (*shortcut == WarpAlias::OrbitedWorld) {
      if (auto warpAction = clientContext->orbitWarpAction()) {
        if (auto warpToWorld = warpAction->first.maybe<WarpToWorld>()) {
          toWorldId = warpToWorld->world;
          spawnTarget = warpToWorld->target;
        }
      }
    } else if (*shortcut == WarpAlias::OwnShip) {
      toWorldId = ClientShipWorldId(clientContext->playerUuid());
    }
  }

  return WarpToWorld(toWorldId, spawnTarget);
}

void UniverseServer::doDisconnection(ConnectionId clientId, String const& reason) {
  if (auto clientContext = m_clients.value(clientId)) {
    m_teamManager->playerDisconnected(clientContext->playerUuid());

    // The client should revive at their ship if they are in an un-revivable
    // state
    WarpToWorld reviveWarp = WarpToWorld(ClientShipWorldId(clientContext->playerUuid()));
    if (auto currentWorld = clientContext->playerWorld()) {
      auto currentWorldId = currentWorld->worldId();
      if (auto playerRevivePosition = currentWorld->playerRevivePosition(clientId))
        reviveWarp = WarpToWorld(currentWorldId, SpawnTargetPosition(*playerRevivePosition));
      m_connectionServer->sendPackets(clientId, currentWorld->removeClient(clientId));
      m_chatProcessor->leaveChannel(clientId, printWorldId(currentWorld->worldId()));
    }

    clientContext->clearPlayerWorld();
    clientContext->setPlayerReviveWarp(reviveWarp);

    if (auto systemWorld = clientContext->systemWorld())
      systemWorld->removeClient(clientId);

    clientContext->clearSystemWorld();

    if (m_chatProcessor->hasClient(clientId))
      m_chatProcessor->disconnectClient(clientId);

    if (m_connectionServer->connectionIsOpen(clientId)) {
      // Send the client the last ship update.
      if (auto shipWorld = getWorld(ClientShipWorldId(clientContext->playerUuid()))) {
        clientContext->updateShipChunks(shipWorld->readChunks());
        shipWorld->stop();
      }
      sendClientContextUpdate(clientContext);

      // Then send the disconnect packet.
      m_connectionServer->sendPackets(clientId, {make_shared<ServerDisconnectPacket>(reason)});
    }

    // Write the final client context.
    auto versioningDatabase = Root::singleton().versioningDatabase();
    String clientContextFile = File::relativeTo(m_storageDirectory, strf("{}.clientcontext", clientContext->playerUuid().hex()));
    VersionedJson::writeFile(versioningDatabase->makeCurrentVersionedJson("ClientContext", clientContext->storeServerData()), clientContextFile);

    m_clients.remove(clientId);
    m_deadConnections.append({m_connectionServer->removeConnection(clientId), Time::monotonicMilliseconds()});
    Logger::info("UniverseServer: Client {} disconnected for reason: {}", clientContext->descriptiveName(), reason);

    auto players = static_cast<uint16_t>(m_clients.size());
    for (auto clientId : m_clients.keys()) {
      m_connectionServer->sendPackets(clientId, {make_shared<ServerInfoPacket>(players, static_cast<uint16_t>(m_maxPlayers))});
    }
  }
}

Maybe<ConnectionId> UniverseServer::getClientForUuid(Uuid const& uuid) const {
  for (auto const& p : m_clients) {
    if (p.second->playerUuid() == uuid)
      return p.second->clientId();
  }

  return {};
}

WorldServerThreadPtr UniverseServer::getWorld(WorldId const& worldId) {
  if (m_worlds.contains(worldId)) {
    auto& maybeWorldPromise = m_worlds.get(worldId);
    try {
      if (!maybeWorldPromise || !maybeWorldPromise->poll())
        return {};

      return maybeWorldPromise->get();
    } catch (std::exception const& e) {
      maybeWorldPromise.reset();
      Logger::error("UniverseServer: error during world create: {}", outputException(e, true));
      worldDiedWithError(worldId);
    }
  }

  return {};
}

WorldServerThreadPtr UniverseServer::createWorld(WorldId const& worldId) {
  if (!m_worlds.contains(worldId)) {
    if (auto promise = makeWorldPromise(worldId))
      m_worlds.add(worldId, promise.take());
    else
      return {};
  }

  auto& maybeWorldPromise = m_worlds.get(worldId);
  if (!maybeWorldPromise)
    return {};
  try {
    return maybeWorldPromise->get();
  } catch (std::exception const& e) {
    maybeWorldPromise.reset();
    Logger::error("UniverseServer: error during world create: {}", outputException(e, true));
    worldDiedWithError(worldId);
    return {};
  }
}

Maybe<WorldServerThreadPtr> UniverseServer::triggerWorldCreation(WorldId const& worldId) {
  if (!m_worlds.contains(worldId)) {
    if (auto promise = makeWorldPromise(worldId)) {
      m_worlds.add(worldId, promise.take());
      return {};
    } else {
      return WorldServerThreadPtr();
    }
  } else {
    auto& maybeWorldPromise = m_worlds.get(worldId);
    try {
      // If the promise is reset, this means that the promise threw an
      // exception, return nullptr to signify error.
      if (!maybeWorldPromise)
        return WorldServerThreadPtr();

      if (!maybeWorldPromise->poll())
        return {};

      return maybeWorldPromise->get();
    } catch (std::exception const& e) {
      maybeWorldPromise.reset();
      Logger::error("UniverseServer: error during world create: {}", outputException(e, true));
      worldDiedWithError(worldId);
      return WorldServerThreadPtr();
    }
  }
}

Maybe<WorkerPoolPromise<WorldServerThreadPtr>> UniverseServer::makeWorldPromise(WorldId const& worldId) {
  if (auto celestialWorld = worldId.ptr<CelestialWorldId>())
    return celestialWorldPromise(*celestialWorld);
  else if (auto shipWorld = worldId.ptr<ClientShipWorldId>())
    return shipWorldPromise(*shipWorld);
  else if (auto instanceWorld = worldId.ptr<InstanceWorldId>())
    return instanceWorldPromise(*instanceWorld);
  else
    return {};
}

Maybe<WorkerPoolPromise<WorldServerThreadPtr>> UniverseServer::shipWorldPromise(
    ClientShipWorldId const& clientShipWorldId) {
  auto clientId = clientForUuid(clientShipWorldId);
  if (!clientId)
    return {};

  auto clientContext = m_clients.get(*clientId);
  auto speciesShips = m_speciesShips;
  auto celestialDatabase = m_celestialDatabase;
  auto universeClock = m_universeClock;

  return m_workerPool.addProducer<WorldServerThreadPtr>([this, clientShipWorldId, clientContext, speciesShips, celestialDatabase, universeClock]() {
    WorldServerPtr shipWorld;

    auto shipChunks = clientContext->shipChunks();
    if (!shipChunks.empty()) {
      try {
        Logger::info("UniverseServer: Loading client ship world {}", clientShipWorldId);
        shipWorld = make_shared<WorldServer>(shipChunks);
      } catch (std::exception const& e) {
        Logger::error("UniverseServer: Could not load client ship {}, resetting ship to default state! {}",
            clientShipWorldId, outputException(e, false));
      }
    }

    if (!shipWorld) {
      Logger::info("UniverseServer: Creating new client ship world {}", clientShipWorldId);
      shipWorld = make_shared<WorldServer>(Vec2U(2048, 2048), File::ephemeralFile());
      auto shipStructure = WorldStructure(speciesShips.get(clientContext->playerSpecies()).first());
      shipStructure = shipWorld->setCentralStructure(shipStructure);

      ShipUpgrades currentUpgrades = clientContext->shipUpgrades();
      currentUpgrades.apply(Root::singleton().assets()->json("/ships/shipupgrades.config"));
      currentUpgrades.apply(shipStructure.configValue("shipUpgrades"));
      clientContext->setShipUpgrades(currentUpgrades);

      shipWorld->setSpawningEnabled(false);
      shipWorld->setProperty("invinciblePlayers", true);
      shipWorld->setProperty("ship.level", 0);
      shipWorld->setProperty("ship.fuel", 0);
      shipWorld->setProperty("ship.maxFuel", currentUpgrades.maxFuel);
      shipWorld->setProperty("ship.crewSize", currentUpgrades.crewSize);
      shipWorld->setProperty("ship.fuelEfficiency", currentUpgrades.fuelEfficiency);
      // Downstreamed from OpenStarbound: Fixed inconsistent plant growth on shipworlds by giving ships their own clock. Server-side fix. /* { */
      shipWorld->setProperty("ship.epoch", Time::timeSinceEpoch());
    }
    auto shipClock = make_shared<Clock>();
    auto shipTime = shipWorld->getProperty("ship.epoch");
    if (!shipTime.canConvert(Json::Type::Float)) {
      auto now = Time::timeSinceEpoch();
      shipWorld->setProperty("ship.epoch", now);
    } else {
      shipClock->setTime(Time::timeSinceEpoch() - shipTime.toDouble());
    }

    shipWorld->setUniverseSettings(m_universeSettings);
    shipWorld->setReferenceClock(shipClock);
    shipClock->start();
    /* } */

    if (auto systemWorld = clientContext->systemWorld())
      shipWorld->setOrbitalSky(systemWorld->clientSkyParameters(clientContext->clientId()));
    else
      shipWorld->setOrbitalSky(celestialSkyParameters(clientContext->shipCoordinate()));

    shipWorld->initLua(this);

    auto shipWorldThread = make_shared<WorldServerThread>(shipWorld, ClientShipWorldId(clientShipWorldId));
    shipWorldThread->setPause(m_pause);
    clientContext->updateShipChunks(shipWorldThread->readChunks());
    shipWorldThread->start();
    shipWorldThread->setUpdateAction(bind(&UniverseServer::worldUpdated, this, _1));

    return shipWorldThread;
  });
}

Maybe<WorkerPoolPromise<WorldServerThreadPtr>> UniverseServer::celestialWorldPromise(CelestialWorldId const& celestialWorldId) {
  if (!celestialWorldId)
    return {};

  auto storageDirectory = m_storageDirectory;
  auto celestialDatabase = m_celestialDatabase;
  auto universeClock = m_universeClock;

  return m_workerPool.addProducer<WorldServerThreadPtr>([this, celestialWorldId, storageDirectory, celestialDatabase, universeClock]() {
    WorldServerPtr worldServer;
    String storageFile = File::relativeTo(storageDirectory, strf("{}.world", celestialWorldId.filename()));
    if (File::isFile(storageFile)) {
      WorldStorage::repackWorldFile(storageFile, strf("celestial world at '{}'", storageFile));

      try {
        Logger::info("UniverseServer: Loading celestial world {}", celestialWorldId);
        worldServer = make_shared<WorldServer>(File::open(storageFile, IOMode::ReadWrite));
      } catch (std::exception const& e) {
        Logger::error("UniverseServer: Could not load celestial world {}, removing! Cause: {}",
            celestialWorldId, outputException(e, false));
        File::rename(storageFile, strf("{}.{}.fail", storageFile, Time::millisecondsSinceEpoch()));
      }
    }

    if (!worldServer) {
      Logger::info("UniverseServer: Creating celestial world {}", celestialWorldId);
      auto worldTemplate = make_shared<WorldTemplate>(celestialWorldId, celestialDatabase);
      worldServer = make_shared<WorldServer>(worldTemplate, File::open(storageFile, IOMode::ReadWrite | IOMode::Truncate));
    }

    worldServer->setUniverseSettings(m_universeSettings);
    worldServer->setReferenceClock(universeClock);
    worldServer->initLua(this);

    auto worldThread = make_shared<WorldServerThread>(worldServer, celestialWorldId);
    worldThread->setPause(m_pause);
    worldThread->start();
    worldThread->setUpdateAction(bind(&UniverseServer::worldUpdated, this, _1));

    return worldThread;
  });
}

Maybe<WorkerPoolPromise<WorldServerThreadPtr>> UniverseServer::instanceWorldPromise(InstanceWorldId const& instanceWorldId) {
  auto storageDirectory = m_storageDirectory;
  auto universeClock = m_universeClock;
  return m_workerPool.addProducer<WorldServerThreadPtr>([this, storageDirectory, instanceWorldId, universeClock]() {
    Json worldConfig = Root::singleton().assets()->json("/instance_worlds.config").get(instanceWorldId.instance);
    uint64_t worldSeed;
    if (worldConfig.contains("seed"))
      worldSeed = worldConfig.getUInt("seed");
    else
      worldSeed = Random::randu64();

    String worldType = worldConfig.getString("type");

    VisitableWorldParametersPtr worldParameters;
    if (worldType.equalsIgnoreCase("Terrestrial"))
      worldParameters = generateTerrestrialWorldParameters(worldConfig.getString("planetType"), worldConfig.getString("planetSize"), worldSeed);
    else if (worldType.equalsIgnoreCase("Asteroids"))
      worldParameters = generateAsteroidsWorldParameters(worldSeed);
    else if (worldType.equalsIgnoreCase("FloatingDungeon"))
      worldParameters = generateFloatingDungeonWorldParameters(worldConfig.getString("dungeonWorld"));
    else
      throw UniverseServerException(strf("Unknown world type: '{}'\n", worldType));

    if (instanceWorldId.level)
      worldParameters->threatLevel = *instanceWorldId.level;

    if (worldConfig.contains("beamUpRule"))
      worldParameters->beamUpRule = BeamUpRuleNames.getLeft(worldConfig.getString("beamUpRule"));
    worldParameters->disableDeathDrops = worldConfig.getBool("disableDeathDrops", false);

    SkyParameters skyParameters = SkyParameters(worldConfig.get("skyParameters", Json()));
    auto worldTemplate = make_shared<WorldTemplate>(worldParameters, skyParameters, worldSeed);
    Json worldProperties = worldConfig.get("worldProperties", JsonObject{});
    bool spawningEnabled = worldConfig.getBool("spawningEnabled", true);
    bool persistent = worldConfig.getBool("persistent", false);
    bool useUniverseClock = worldConfig.getBool("useUniverseClock", false);

    WorldServerPtr worldServer;

    bool worldExisted = false;

    if (persistent) {
      String identifier = instanceWorldId.instance;
      if (instanceWorldId.uuid)
        identifier = strf("{}-{}", identifier, instanceWorldId.uuid->hex());
      if (instanceWorldId.level)
        identifier = strf("{}-{}", identifier, instanceWorldId.level.value());
      String storageFile = File::relativeTo(storageDirectory, strf("unique-{}.world", identifier));
      if (File::isFile(storageFile)) {
        try {
          Logger::info("UniverseServer: Loading persistent unique instance world {}", instanceWorldId.instance);
          worldServer = make_shared<WorldServer>(File::open(storageFile, IOMode::ReadWrite));
          worldExisted = true;
        } catch (std::exception const& e) {
          Logger::error("UniverseServer: Could not load persistent unique instance world {}, removing! Cause: {}",
              instanceWorldId.instance, outputException(e, false));
          File::rename(storageFile, strf("{}.{}.fail", storageFile, Time::millisecondsSinceEpoch()));
        }
      }

      if (!worldServer) {
        Logger::info("UniverseServer: Creating persistent unique instance world {}", instanceWorldId.instance);
        worldServer = make_shared<WorldServer>(worldTemplate, File::open(storageFile, IOMode::ReadWrite | IOMode::Truncate));
      }
    } else {
      String storageFile = tempWorldFile(instanceWorldId);
      uint64_t deleteTime = worldConfig.optInt("tempWorldDeleteTime").value(0);
      if (File::isFile(storageFile)) {
        if (m_tempWorldIndex.contains(instanceWorldId)) {
          auto file = File::open(storageFile, IOMode::ReadWrite);
          if (file->size() > 0) {
            Logger::info("UniverseServer: Loading temporary instance world {} from storage", instanceWorldId);
            try {
              worldServer = make_shared<WorldServer>(file);
              worldExisted = true;
            } catch (std::exception const& e) {
              Logger::error("UniverseServer: Could not load temporary instance world '{}', re-creating cause: {}",
                  instanceWorldId, outputException(e, false));
            }
          }
        } else {
          File::remove(storageFile);
        }
      }

      if (!worldServer) {
        Logger::info("UniverseServer: Creating temporary instance world '{}' with expiry time {}", instanceWorldId, deleteTime);

        worldServer = make_shared<WorldServer>(worldTemplate, File::open(storageFile, IOMode::ReadWrite));
        m_tempWorldIndex.set(instanceWorldId, pair<uint64_t, uint64_t>(m_universeClock->milliseconds(), deleteTime));
      }
    }

    worldServer->setUniverseSettings(m_universeSettings);
    for (auto const& p : worldProperties.iterateObject())
      worldServer->setProperty(p.first, p.second);
    worldServer->setProperty("ephemeral", !persistent);
    worldServer->setSpawningEnabled(spawningEnabled);
    if (useUniverseClock)
      worldServer->setReferenceClock(universeClock);

    if (!worldExisted) {
      for (auto flagAction : m_universeSettings->currentFlagActionsForInstanceWorld(instanceWorldId.instance)) {
        if (flagAction.is<PlaceDungeonFlagAction>()) {
          auto placeDungeonAction = flagAction.get<PlaceDungeonFlagAction>();
          worldServer->placeDungeon(placeDungeonAction.dungeonId, placeDungeonAction.targetPosition, 0);
        }
      }
    }

    worldServer->initLua(this);

    auto worldThread = make_shared<WorldServerThread>(worldServer, instanceWorldId);
    worldThread->setPause(m_pause);
    worldThread->start();
    worldThread->setUpdateAction(bind(&UniverseServer::worldUpdated, this, _1));

    return worldThread;
  });
}

SystemWorldServerThreadPtr UniverseServer::createSystemWorld(Vec3I const& location) {
  if (!m_systemWorlds.contains(location)) {
    SystemWorldServerPtr systemWorld;

    String storageFile = File::relativeTo(m_storageDirectory, strf("{}_{}_{}.system", location[0], location[1], location[2]));
    bool loadedFromStorage = false;
    if (File::isFile(storageFile)) {
      Logger::info("UniverseServer: Loading system world {} from disk storage", location);
      try {
        auto versioningDatabase = Root::singleton().versioningDatabase();
        VersionedJson versionedStore = VersionedJson::readFile(storageFile);
        Json store = versioningDatabase->loadVersionedJson(versionedStore, "System");

        systemWorld = make_shared<SystemWorldServer>(store, m_universeClock, m_celestialDatabase);
        loadedFromStorage = true;
      } catch (std::exception const& e) {
        Logger::error("UniverseServer: Failed to load system {} from disk storage, re-creating. Cause: {}", location, outputException(e, false));
        File::rename(storageFile, strf("{}.{}.fail", storageFile, Time::millisecondsSinceEpoch()));
        loadedFromStorage = false;
      }
    }

    if (!loadedFromStorage) {
      Logger::info("UniverseServer: Creating new system world at location {}", location);
      systemWorld = make_shared<SystemWorldServer>(location, m_universeClock, m_celestialDatabase);
    }

    auto systemThread = make_shared<SystemWorldServerThread>(location, systemWorld, storageFile);
    systemThread->setUpdateAction(bind(&UniverseServer::systemWorldUpdated, this, _1));
    systemThread->start();
    m_systemWorlds.set(location, systemThread);
  }

  return m_systemWorlds.get(location);
}

bool UniverseServer::instanceWorldStoredOrActive(InstanceWorldId const& worldId) const {
  String storageFile = File::relativeTo(m_storageDirectory, strf("unique-{}.world", worldId.instance));
  return m_worlds.value(worldId).isValid() || m_tempWorldIndex.contains(worldId) || File::isFile(storageFile);
}

void UniverseServer::worldDiedWithError(WorldId world) {
  if (world.is<ClientShipWorldId>()) {
    if (auto clientId = getClientForUuid(world.get<ClientShipWorldId>()))
      m_pendingDisconnections.add(*clientId, "Your shipworld has caused a server error");
  }
}

SkyParameters UniverseServer::celestialSkyParameters(CelestialCoordinate const& coordinate) const {
  if (m_celestialDatabase->coordinateValid(coordinate))
    return SkyParameters(coordinate, m_celestialDatabase);
  return SkyParameters();
}

} // namespace Star
