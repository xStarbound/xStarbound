#ifndef STAR_UNIVERSE_CLIENT_HPP
#define STAR_UNIVERSE_CLIENT_HPP

#include "StarAiTypes.hpp"
#include "StarCelestialParameters.hpp"
#include "StarChatTypes.hpp"
#include "StarGameTimers.hpp"
#include "StarHostAddress.hpp"
#include "StarLuaComponents.hpp"
#include "StarMaybe.hpp"
#include "StarSky.hpp"
#include "StarUniverseConnection.hpp"
#include "StarWarping.hpp"

namespace Star {

STAR_CLASS(WorldTemplate);
STAR_CLASS(ClientContext);
STAR_CLASS(Sky);
STAR_STRUCT(Packet);
STAR_CLASS(WorldClient);
STAR_CLASS(SystemWorldClient);
STAR_CLASS(Player);
STAR_CLASS(PlayerStorage);
STAR_CLASS(Statistics);
STAR_CLASS(Clock);
STAR_CLASS(CelestialLog);
STAR_CLASS(CelestialSlaveDatabase);
STAR_CLASS(CelestialDatabase);
STAR_CLASS(JsonRpcInterface);
STAR_CLASS(TeamClient);
STAR_CLASS(QuestManager);
STAR_CLASS(UniverseClient);
STAR_CLASS(LuaRoot);

class UniverseClient {
public:
  UniverseClient(PlayerStoragePtr playerStorage, StatisticsPtr statistics);
  ~UniverseClient();

  void setMainPlayer(PlayerPtr player);
  PlayerPtr mainPlayer() const;

  // Returns error if connection failed
  Maybe<String> connect(UniverseConnection connection, bool allowAssetsMismatch, String const& account = "", String const& password = "", bool forceLegacyConnection = false);
  bool isConnected() const;
  void disconnect();
  Maybe<String> disconnectReason() const;

  // WorldClient may be null if the UniverseClient is not connected.
  WorldClientPtr worldClient() const;
  SystemWorldClientPtr systemWorldClient() const;

  // Updates internal world client in addition to handling universe level
  // commands.
  void update(float dt);

  Maybe<BeamUpRule> beamUpRule() const;
  bool canBeamUp() const;
  bool canBeamDown(bool deploy = false) const;
  bool canBeamToTeamShip() const;
  bool canTeleport() const;

  void warpPlayer(WarpAction const& warpAction, bool animate = true, String const& animationType = "default", bool deploy = false);
  void flyShip(Vec3I const& system, SystemLocation const& destination);

  CelestialDatabasePtr celestialDatabase() const;

  CelestialCoordinate shipCoordinate() const;

  bool playerOnOwnShip() const;
  bool playerIsOriginal() const;

  WorldId playerWorld() const;
  bool isAdmin() const;
  // If the player is in a multi person team returns the team uuid, or if the
  // player is by themselves returns the player uuid.
  Uuid teamUuid() const;

  WorldTemplateConstPtr currentTemplate() const;
  SkyConstPtr currentSky() const;
  bool flying() const;

  void sendChat(String const& text, ChatSendMode sendMode);
  void sendChat(String const& text, String const& sendMode, bool suppressBubble, Maybe<JsonObject> metadata = {});
  List<ChatReceivedMessage> pullChatMessages();

  uint16_t players();
  uint16_t maxPlayers();

  void setLuaCallbacks(String const& groupName, LuaCallbacks const& callbacks, uint8_t safetyLevel = 0);
  void startLua();
  void stopLua();

  bool playerIsLoaded(Uuid const& uuid);
  void reloadAllPlayers(bool resetInterfaces = false, bool showIndicator = false);
  // bool reloadPlayer(Json const& data, Uuid const& uuid, bool resetInterfaces = false, bool showIndicator = false);
  bool swapPlayer(Uuid const& uuid, bool resetInterfaces = false, bool showIndicator = false);
  bool loadPlayer(Uuid const& uuid, bool resetInterfaces = false, bool showIndicator = false);
  bool unloadPlayer(Uuid const& uuid, bool resetInterfaces = false, bool showIndicator = false);

  void doSwitchPlayer(Uuid const& uuid);
  bool switchPlayer(Uuid const& uuid);
  bool switchPlayer(size_t index);
  bool switchPlayer(String const& name);
  bool switchPlayerUuid(String const& uuidStr);

  void doAddPlayer(Uuid const& uuid);
  bool addPlayer(Uuid const& uuid);
  bool addPlayer(size_t index);
  bool addPlayer(String const& name);
  bool addPlayerUuid(String const& uuidStr);

  void doRemovePlayer(Uuid const& uuid);
  bool removePlayer(Uuid const& uuid);
  bool removePlayer(size_t index);
  bool removePlayer(String const& name);
  bool removePlayerUuid(String const& uuidStr);

  HashMap<Uuid, PlayerPtr> controlledPlayers();
  JsonObject preLoadedPlayers();
  JsonObject preLoadedPlayerNames();
  Json playerSaveData(Uuid const& playerUuid);
  Maybe<bool> playerDead(Uuid const& playerUuid);
  bool playerLoaded(Uuid const& playerUuid);

  typedef std::function<void()> Callback;
  typedef std::function<void(bool)> ReloadPlayerCallback;
  typedef std::function<Maybe<Json>(const String&, bool, const JsonArray&)> InterfaceMessageCallback;
  ReloadPlayerCallback& playerReloadPreCallback();
  ReloadPlayerCallback& playerReloadCallback();
  InterfaceMessageCallback& interfaceMessageCallback();
  Callback& saveCallback();

  ClockConstPtr universeClock() const;
  CelestialLogConstPtr celestialLog() const;
  JsonRpcInterfacePtr rpcInterface() const;
  ClientContextPtr clientContext() const;
  TeamClientPtr teamClient() const;
  QuestManagerPtr questManager() const;
  PlayerStoragePtr playerStorage() const;
  StatisticsPtr statistics() const;

  bool paused() const;

  bool switchingPlayer() const;

  Maybe<Json> receiveMessage(String const& message, bool localMessage, JsonArray const& args);

private:
  struct ServerInfo {
    uint16_t players;
    uint16_t maxPlayers;
  };

  struct LoadedPlayer {
    bool loaded;
    PlayerPtr ptr;

    ~LoadedPlayer() {
      loaded = false;
      ptr.reset();
    }
  };

  void setPause(bool pause);

  void handlePackets(List<PacketPtr> const& packets);
  void reset();

  PlayerStoragePtr m_playerStorage;
  StatisticsPtr m_statistics;
  PlayerPtr m_mainPlayer;
  HashMap<Uuid, LoadedPlayer> m_loadedPlayers;

  bool m_legacyServer;
  bool m_pause;
  ClockPtr m_universeClock;
  WorldClientPtr m_worldClient;
  SystemWorldClientPtr m_systemWorldClient;
  Maybe<UniverseConnection> m_connection;
  Maybe<ServerInfo> m_serverInfo;

  StringMap<LuaCallbacks> m_luaCallbacks;

  CelestialSlaveDatabasePtr m_celestialDatabase;
  ClientContextPtr m_clientContext;
  TeamClientPtr m_teamClient;

  QuestManagerPtr m_questManager;

  WarpAction m_pendingWarp;
  GameTimer m_warpDelay;
  Maybe<GameTimer> m_warpCinemaCancelTimer;

  Maybe<WarpAction> m_warping;
  bool m_respawning;
  GameTimer m_respawnTimer;

  int64_t m_storageTriggerDeadline;

  List<ChatReceivedMessage> m_pendingMessages;
  List<ChatReceivedMessage> m_worldPendingMessages;

  Maybe<String> m_disconnectReason;

  LuaRootPtr m_luaRoot;

  typedef LuaMessageHandlingComponent<LuaUpdatableComponent<LuaBaseComponent>> ScriptComponent;
  typedef shared_ptr<ScriptComponent> ScriptComponentPtr;
  StringMap<ScriptComponentPtr> m_scriptContexts;

  ReloadPlayerCallback m_playerReloadPreCallback;
  ReloadPlayerCallback m_playerReloadCallback;

  bool m_shouldUpdateMainPlayerShip;

  Maybe<Uuid> m_playerToSwitchTo;
  List<Uuid> m_playersToLoad;
  List<Uuid> m_playersToRemove;
  Uuid m_mainPlayerUuid;

  Callback m_saveCallback;
  InterfaceMessageCallback m_interfaceMessageCallback;
};

} // namespace Star

#endif
