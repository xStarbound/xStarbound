#ifndef STAR_UNIVERSE_SERVER_LUA_BINDINGS_HPP
#define STAR_UNIVERSE_SERVER_LUA_BINDINGS_HPP

#include "StarGameTypes.hpp"
#include "StarLua.hpp"
#include "StarRpcThreadPromise.hpp"

namespace Star {

STAR_CLASS(UniverseServer);

namespace LuaBindings {
  LuaCallbacks makeUniverseServerCallbacks(UniverseServer* universe);

  namespace UniverseServerCallbacks {
    List<ConnectionId> clientIds(UniverseServer* universe);
    size_t numberOfClients(UniverseServer* universe);
    bool isConnectedClient(UniverseServer* universe, ConnectionId arg1);
    Maybe<String> clientNick(UniverseServer* universe, ConnectionId arg1);
    Maybe<ConnectionId> findNick(UniverseServer* universe, String const& arg1);
    void adminBroadcast(UniverseServer* universe, String const& arg1, Maybe<JsonObject> const& arg2);
    void adminWhisper(UniverseServer* universe, ConnectionId arg1, String const& arg2, Maybe<JsonObject> const& arg3);
    bool isAdmin(UniverseServer* universe, ConnectionId arg1);
    bool isPvp(UniverseServer* universe, ConnectionId arg1);
    void setPvp(UniverseServer* universe, ConnectionId arg1, Maybe<bool> arg2);
    bool isLocal(UniverseServer* universe, ConnectionId arg1);
    bool isWorldActive(UniverseServer* universe, String const& worldId);
    StringList activeWorlds(UniverseServer* universe);
    RpcThreadPromise<Json> sendWorldMessage(UniverseServer* universe, String const& worldId, String const& message, LuaVariadic<Json> args);
    String clientWorld(UniverseServer* universe, ConnectionId connectionId);
    Maybe<String> clientUuid(UniverseServer* universe, ConnectionId connectionId);
    void disconnectClient(UniverseServer* universe, ConnectionId clientId, Maybe<String> const& reason);
    void banClient(UniverseServer* universe, ConnectionId clientId, Maybe<String> const& reason, Maybe<bool> banIp, Maybe<bool> banUuid, Maybe<int> timeout);
  } // namespace UniverseServerCallbacks
} // namespace LuaBindings
} // namespace Star

#endif
