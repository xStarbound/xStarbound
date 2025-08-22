#ifndef STAR_UNIVERSE_SERVER_HPP
#define STAR_UNIVERSE_SERVER_HPP

#include "StarCelestialCoordinate.hpp"
#include "StarGameTypes.hpp"
#include "StarIdMap.hpp"
#include "StarLockFile.hpp"
#include "StarServerClientContext.hpp"
#include "StarString.hpp"
#include "StarSystemWorldServerThread.hpp"
#include "StarUniverseConnection.hpp"
#include "StarUniverseSettings.hpp"
#include "StarWorkerPool.hpp"
#include "StarWorldServerThread.hpp"

namespace Star {

STAR_CLASS(Clock);
STAR_CLASS(File);
STAR_CLASS(Player);
STAR_CLASS(ChatProcessor);
STAR_CLASS(CommandProcessor);
STAR_CLASS(TeamManager);
STAR_CLASS(UniverseServer);
STAR_CLASS(WorldTemplate);
STAR_CLASS(WorldServer);
STAR_CLASS(UniverseSettings);

STAR_EXCEPTION(UniverseServerException, StarException);

// Manages all running worlds, listens for new client connections and marshalls
// between all the different worlds and all the different client connections
// and routes packets between them.
class UniverseServer : public Thread {
public:
  UniverseServer(String const& storageDir);
  ~UniverseServer();

  // If enabled, will listen on the configured server port for incoming
  // connections.
  void setListeningTcp(bool listenTcp);

  // Connects an arbitrary UniverseConnection to this server
  void addClient(UniverseConnection remoteConnection);
  // Constructs an in-process connection to a UniverseServer for a
  // UniverseClient, and returns the other side of the connection.
  UniverseConnection addLocalClient();

  // Signals the UniverseServer to stop and then joins the thread.
  void stop();

  void setPause(bool pause);

  List<WorldId> activeWorlds() const;
  bool isWorldActive(WorldId const& worldId) const;

  List<ConnectionId> clientIds() const;
  size_t numberOfClients() const;
  uint32_t maxClients() const;
  bool isConnectedClient(ConnectionId clientId) const;

  String clientDescriptor(ConnectionId clientId) const;

  String clientNick(ConnectionId clientId) const;
  Maybe<String> clientAccount(ConnectionId clientId) const;
  Maybe<bool> clientIsGuest(ConnectionId clientId) const;
  Maybe<ConnectionId> findNick(String const& nick) const;

  Maybe<Uuid> uuidForClient(ConnectionId clientId) const;
  Maybe<ConnectionId> clientForUuid(Uuid const& uuid) const;

  void adminBroadcast(String const& text, JsonObject const& metadata = JsonObject{});
  void adminWhisper(ConnectionId clientId, String const& text, JsonObject const& metadata = JsonObject{});
  String adminCommand(String text);

  bool isAdmin(ConnectionId clientId) const;
  bool canBecomeAdmin(ConnectionId clientId) const;
  void setAdmin(ConnectionId clientId, bool admin);

  bool isLocal(ConnectionId clientId) const;

  bool isPvp(ConnectionId clientId) const;
  void setPvp(ConnectionId clientId, bool pvp);

  RpcThreadPromise<Json> sendWorldMessage(WorldId const& worldId, String const& message, JsonArray const& args = {});

  void clientWarpPlayer(ConnectionId clientId, WarpAction action, bool deploy = false);
  void clientFlyShip(ConnectionId clientId, Vec3I const& system, SystemLocation const& location);
  WorldId clientWorld(ConnectionId clientId) const;
  CelestialCoordinate clientShipCoordinate(ConnectionId clientId) const;

  ClockPtr universeClock() const;
  UniverseSettingsPtr universeSettings() const;

  CelestialDatabase& celestialDatabase();

  // If the client exists and is in a valid connection state, executes the
  // given function on the client world and player object in a thread safe way.
  // Returns true if function was called, false if client was not found or in
  // an invalid connection state.
  bool executeForClient(ConnectionId clientId, function<void(WorldServer*, PlayerPtr)> action);
  void disconnectClient(ConnectionId clientId, String const& reason);
  void banUser(ConnectionId clientId, String const& reason, pair<bool, bool> banType, Maybe<int> timeout);
  bool unbanIp(String const& addressString);
  bool unbanUuid(String const& uuidString);

  bool updatePlanetType(CelestialCoordinate const& coordinate, String const& newType, String const& weatherBiome);

  bool clientHasBuildPermission(ServerClientContextPtr const& client, SystemWorldServerThreadPtr const& currentSystem, Maybe<Vec3I> const& currentLocation) const;
  Maybe<bool> clientHasBuildPermissionCallback(ConnectionId clientId, Maybe<Vec3I> const& systemLocation) const;

  Maybe<WarpAction> clientReturnWarp(ConnectionId clientId) const;
  Maybe<WarpAction> clientReviveWarp(ConnectionId clientId) const;
  void setClientReturnWarp(ConnectionId clientId, WarpAction warp);
  void setClientReviveWarp(ConnectionId clientId, WarpAction warp);

  Maybe<String> clientTeam(ConnectionId clientId) const;

  void addPendingChatMessage(ConnectionId clientId, ChatReceivedMessage const& chatMessage);

  SystemLocation clientShipLocation(ConnectionId clientId) const;

  Json getServerData(String const& key) const;
  Json getServerDataPath(String const& path) const;
  void setServerData(String const& key, Json const& value);
  void setServerDataPath(String const& path, Json const& value);
  void eraseServerData(String const& key);
  void eraseServerDataPath(String const& path);

protected:
  virtual void run();

private:
  struct TimeoutBan {
    int64_t banExpiry;
    String reason;
    Maybe<HostAddress> ip;
    Maybe<Uuid> uuid;
  };

  struct ChatMessage {
    String message;
    ChatSendMode sendMode;
    JsonObject metadata;
  };

  enum class TcpState : uint8_t { No,
    Yes,
    Fuck };

  void processUniverseFlags();
  void sendPendingChat();
  void updateTeams();
  void updateShips();
  void sendClockUpdates();
  void sendClientContextUpdate(ServerClientContextPtr clientContext);
  void sendClientContextUpdates();
  void kickErroredPlayers();
  void reapConnections();
  void processPlanetTypeChanges();
  void warpPlayers();
  void flyShips();
  void arriveShips();
  void respondToCelestialRequests();
  void processChat();
  void clearBrokenWorlds();
  void handleWorldMessages();
  void shutdownInactiveWorlds();
  void doTriggeredStorage();

  void saveSettings();
  void loadSettings();

  // Either returns the default configured starter world, or a new randomized
  // starter world, or if a randomized world is not yet available, starts a job
  // to find a randomized starter world and returns nothing until it is ready.
  Maybe<CelestialCoordinate> nextStarterWorld();

  void loadTempWorldIndex();
  void saveTempWorldIndex();
  String tempWorldFile(InstanceWorldId const& worldId) const;

  Maybe<String> isBannedUser(Maybe<HostAddress> hostAddress, Uuid playerUuid) const;
  void doTempBan(ConnectionId clientId, String const& reason, pair<bool, bool> banType, int timeout);
  void doPermBan(ConnectionId clientId, String const& reason, pair<bool, bool> banType);
  void removeTimedBan();

  void addCelestialRequests(ConnectionId clientId, List<CelestialRequest> requests);

  void worldUpdated(WorldServerThread* worldServer);
  void systemWorldUpdated(SystemWorldServerThread* systemWorldServer);
  void packetsReceived(UniverseConnectionServer* connectionServer, ConnectionId clientId, List<PacketPtr> packets);

  void acceptConnection(UniverseConnection connection, Maybe<HostAddress> remoteAddress);

  // Main lock and clients read lock must be held when calling
  WarpToWorld resolveWarpAction(WarpAction warpAction, ConnectionId clientId, bool deploy) const;

  // Main lock and clients write lock must be held when calling
  void doDisconnection(ConnectionId clientId, String const& reason);

  // Clients read lock must be held when calling
  Maybe<ConnectionId> getClientForUuid(Uuid const& uuid) const;

  // Get the world only if it is already loaded, Main lock must be held when
  // calling.
  WorldServerThreadPtr getWorld(WorldId const& worldId);

  // If the world is not created, block and load it, otherwise just return the
  // loaded world.  Main lock and Clients read lock must be held when calling.
  WorldServerThreadPtr createWorld(WorldId const& worldId);

  // Trigger off-thread world creation, returns a value when the creation is
  // finished, either successfully or with an error.  Main lock and Clients
  // read lock must be held when calling.
  Maybe<WorldServerThreadPtr> triggerWorldCreation(WorldId const& worldId);

  // Main lock and clients read lock must be held when calling world promise
  // generators
  Maybe<WorkerPoolPromise<WorldServerThreadPtr>> makeWorldPromise(WorldId const& worldId);
  Maybe<WorkerPoolPromise<WorldServerThreadPtr>> shipWorldPromise(ClientShipWorldId const& uuid);
  Maybe<WorkerPoolPromise<WorldServerThreadPtr>> celestialWorldPromise(CelestialWorldId const& coordinate);
  Maybe<WorkerPoolPromise<WorldServerThreadPtr>> instanceWorldPromise(InstanceWorldId const& instanceWorld);

  // If the system world is not created, initialize it, otherwise return the
  // already initialized one
  SystemWorldServerThreadPtr createSystemWorld(Vec3I const& location);

  bool instanceWorldStoredOrActive(InstanceWorldId const& worldId) const;

  // Signal that a world either failed to load, or died due to an exception,
  // kicks clients if that world is a ship world.  Main lock and clients read
  // lock must be held when calling.
  void worldDiedWithError(WorldId world);

  // Get SkyParameters if the coordinate is a valid world, and empty
  // SkyParameters otherwise.
  SkyParameters celestialSkyParameters(CelestialCoordinate const& coordinate) const;

  mutable RecursiveMutex m_mainLock;

  String m_storageDirectory;
  ByteArray m_assetsDigest;
  Maybe<LockFile> m_storageDirectoryLock;
  StringMap<StringList> m_speciesShips;
  CelestialMasterDatabasePtr m_celestialDatabase;
  ClockPtr m_universeClock;
  UniverseSettingsPtr m_universeSettings;
  WorkerPool m_workerPool;

  int64_t m_storageTriggerDeadline;
  int64_t m_clearBrokenWorldsDeadline;
  int64_t m_lastClockUpdateSent;
  atomic<bool> m_stop;
  atomic<TcpState> m_tcpState;

  // Fezzedone: Needs to be recursive because world scripts have access to universe server callbacks.
  mutable RecursiveMutex m_clientsLock;
  unsigned m_maxPlayers;
  IdMap<ConnectionId, ServerClientContextPtr> m_clients;

  shared_ptr<atomic<bool>> m_pause;
  Map<WorldId, Maybe<WorkerPoolPromise<WorldServerThreadPtr>>> m_worlds;
  Map<InstanceWorldId, pair<int64_t, int64_t>> m_tempWorldIndex;
  Map<Vec3I, SystemWorldServerThreadPtr> m_systemWorlds;
  UniverseConnectionServerPtr m_connectionServer;

  List<ThreadFunction<void>> m_connectionAcceptThreads;
  LinkedList<pair<UniverseConnection, int64_t>> m_deadConnections;

  ChatProcessorPtr m_chatProcessor;
  CommandProcessorPtr m_commandProcessor;
  TeamManagerPtr m_teamManager;

  HashMap<ConnectionId, pair<WarpAction, bool>> m_pendingPlayerWarps;
  HashMap<ConnectionId, pair<pair<Vec3I, SystemLocation>, Maybe<double>>> m_queuedFlights;
  HashMap<ConnectionId, pair<Vec3I, SystemLocation>> m_pendingFlights;
  HashMap<ConnectionId, CelestialCoordinate> m_pendingArrivals;
  HashMap<ConnectionId, String> m_pendingDisconnections;
  HashMap<ConnectionId, List<WorkerPoolPromise<CelestialResponse>>> m_pendingCelestialRequests;
  List<pair<WorldId, UniverseFlagAction>> m_pendingFlagActions;
  HashMap<ConnectionId, List<ChatMessage>> m_pendingChat;
  Maybe<WorkerPoolPromise<CelestialCoordinate>> m_nextRandomizedStarterWorld;
  Map<WorldId, List<WorldServerThread::Message>> m_pendingWorldMessages;

  List<TimeoutBan> m_tempBans;

  shared_ptr<Json> m_serverData;

  bool m_rememberReturnWarpsOnDeath;
};

} // namespace Star

#endif
