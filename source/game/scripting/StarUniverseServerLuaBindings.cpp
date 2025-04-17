#include "StarUniverseServerLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarUniverseServer.hpp"
#include "StarUuid.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeUniverseServerCallbacks(UniverseServer* universe) {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<List<ConnectionId>>("clientIds", bind(UniverseServerCallbacks::clientIds, universe));
  callbacks.registerCallbackWithSignature<size_t>("numberOfClients", bind(UniverseServerCallbacks::numberOfClients, universe));
  callbacks.registerCallbackWithSignature<bool, ConnectionId>("isConnectedClient", bind(UniverseServerCallbacks::isConnectedClient, universe, _1));
  callbacks.registerCallbackWithSignature<Maybe<String>, ConnectionId>("clientNick", bind(UniverseServerCallbacks::clientNick, universe, _1));
  callbacks.registerCallbackWithSignature<Maybe<ConnectionId>, String>("findNick", bind(UniverseServerCallbacks::findNick, universe, _1));
  callbacks.registerCallbackWithSignature<void, String, Maybe<JsonObject>>("adminBroadcast", bind(UniverseServerCallbacks::adminBroadcast, universe, _1, _2));
  callbacks.registerCallbackWithSignature<void, ConnectionId, String, Maybe<JsonObject>>("adminWhisper", bind(UniverseServerCallbacks::adminWhisper, universe, _1, _2, _3));
  callbacks.registerCallbackWithSignature<bool, ConnectionId>("isAdmin", bind(UniverseServerCallbacks::isAdmin, universe, _1));
  callbacks.registerCallbackWithSignature<bool, ConnectionId>("isPvp", bind(UniverseServerCallbacks::isPvp, universe, _1));
  callbacks.registerCallbackWithSignature<void, ConnectionId, bool>("setPvp", bind(UniverseServerCallbacks::setPvp, universe, _1, _2));
  callbacks.registerCallbackWithSignature<bool, ConnectionId>("isLocal", bind(UniverseServerCallbacks::isLocal, universe, _1));
  callbacks.registerCallbackWithSignature<bool, String>("isWorldActive", bind(UniverseServerCallbacks::isWorldActive, universe, _1));
  callbacks.registerCallbackWithSignature<StringList>("activeWorlds", bind(UniverseServerCallbacks::activeWorlds, universe));
  callbacks.registerCallbackWithSignature<RpcThreadPromise<Json>, String, String, LuaVariadic<Json>>("sendWorldMessage", bind(UniverseServerCallbacks::sendWorldMessage, universe, _1, _2, _3));
  callbacks.registerCallbackWithSignature<String, ConnectionId>("clientWorld", bind(UniverseServerCallbacks::clientWorld, universe, _1));
  callbacks.registerCallbackWithSignature<Maybe<String>, ConnectionId>("clientUuid", bind(UniverseServerCallbacks::clientUuid, universe, _1));

  return callbacks;
}

// Gets a list of client ids
//
// @return A list of numerical client IDs.
List<ConnectionId> LuaBindings::UniverseServerCallbacks::clientIds(UniverseServer* universe) {
  return universe->clientIds();
}

// Gets the number of logged in clients
//
// @return An integer containing the number of logged in clients
size_t LuaBindings::UniverseServerCallbacks::numberOfClients(UniverseServer* universe) {
  return universe->numberOfClients();
}

// Returns whether or not the provided client ID is currently connected
//
// @param clientId the client ID in question
// @return A bool that is true if the client is connected and false otherwise
bool LuaBindings::UniverseServerCallbacks::isConnectedClient(UniverseServer* universe, ConnectionId arg1) {
  return universe->isConnectedClient(arg1);
}

// Returns the nickname for the given client ID
//
// @param clientId the client ID in question
// @return A string containing the nickname of the given client
Maybe<String> LuaBindings::UniverseServerCallbacks::clientNick(UniverseServer* universe, ConnectionId arg1) {
  if (universe->isConnectedClient(arg1))
    return universe->clientNick(arg1);
  return {};
}

// Returns the client ID for the given nick
//
// @param nick the nickname of the client to search for
// @return An integer containing the clientID of the nick in question
Maybe<ConnectionId> LuaBindings::UniverseServerCallbacks::findNick(UniverseServer* universe, String const& arg1) {
  return universe->findNick(arg1);
}

// Sends a message to all logged in clients
//
// @param message the message to broadcast
// @param metadata optional chat message metadata
// @return nil
void LuaBindings::UniverseServerCallbacks::adminBroadcast(UniverseServer* universe, String const& arg1, Maybe<JsonObject> const& arg2) {
  universe->adminBroadcast(arg1, arg2.value(JsonObject{}));
}

// Sends a message to a specific client
//
// @param clientId the client id to whisper
// @param message the message to whisper
// @param metadata optional chat message metadata
// @return nil
void LuaBindings::UniverseServerCallbacks::adminWhisper(UniverseServer* universe, ConnectionId arg1, String const& arg2, Maybe<JsonObject> const& arg3) {
  ConnectionId client = arg1;
  String message = arg2;
  universe->adminWhisper(client, message, arg3.value(JsonObject{}));
}

// Returns whether or not a specific client is flagged as an admin
//
// @param clientId the client id to check
// @return a boolean containing true if the client is an admin, false otherwise
bool LuaBindings::UniverseServerCallbacks::isAdmin(UniverseServer* universe, ConnectionId arg1) {
  return universe->isAdmin(arg1);
}

// Returns whether or not a specific client is flagged as pvp
//
// @param clientId the client id to check
// @return a boolean containing true if the client is flagged as pvp, false
// otherwise
bool LuaBindings::UniverseServerCallbacks::isPvp(UniverseServer* universe, ConnectionId arg1) {
  return universe->isPvp(arg1);
}

// Set (or unset) the pvp status of a specific user
//
// @param clientId the client id to check
// @param setPvp set pvp status to this bool, defaults to true
// @return nil
void LuaBindings::UniverseServerCallbacks::setPvp(UniverseServer* universe, ConnectionId arg1, Maybe<bool> arg2) {
  ConnectionId client = arg1;
  bool setPvpTo = arg2.value(true);
  universe->setPvp(client, setPvpTo);
}

// Returns whether the user is local (i.e., the user's client is also the server)
//
// @param clientId The connection ID to check.
// @return A boolean containing true if the client in question is also the server, false otherwise.
bool LuaBindings::UniverseServerCallbacks::isLocal(UniverseServer* universe, ConnectionId arg1) {
  return universe->isLocal(arg1);
}

// Returns whether the user is local (i.e., the user's client is also the server).
//
// @param worldId The world ID to check.
// @return A boolean containing true if the world is active, false otherwise.
bool LuaBindings::UniverseServerCallbacks::isWorldActive(UniverseServer* universe, String const& worldId) {
  return universe->isWorldActive(parseWorldId(worldId));
}

// Returns whether the user is local (i.e., the user's client is also the server)
//
// @return A list of active world IDs.
StringList LuaBindings::UniverseServerCallbacks::activeWorlds(UniverseServer* universe) {
  StringList worlds;
  for (WorldId& world : universe->activeWorlds())
    worlds.append(printWorldId(world));

  return worlds;
}

// Sends a message to a world, returning an RPC promise for that message. Similar to world.sendEntityMessage, but the target is a world's global context and it may be used by universe server scripts.
//
// @param worldId The target world ID to which to send the message.
// @param message A string containing the message name.
// @param args... Optional JSON arguments.
// @return An RPC promise for the sent world message.
RpcThreadPromise<Json> LuaBindings::UniverseServerCallbacks::sendWorldMessage(UniverseServer* universe, String const& worldId, String const& message, LuaVariadic<Json> args) {
  return universe->sendWorldMessage(parseWorldId(worldId), message, JsonArray::from(std::move(args)));
}

// ErodeesFleurs: Returns the world a given player is located on.
//
// @param connectionId The connection ID to check.
// @return A world ID string.
String LuaBindings::UniverseServerCallbacks::clientWorld(UniverseServer* universe, ConnectionId connectionId) {
  return printWorldId(universe->clientWorld(connectionId));
}

// Returns the UUID of a given client.
//
// @param connectionId The connection ID to check.
// @return The UUID, or nil if the connection ID is not connected.
Maybe<String> LuaBindings::UniverseServerCallbacks::clientUuid(UniverseServer* universe, ConnectionId connectionId) {
  if (universe->isConnectedClient(connectionId))
    return universe->uuidForClient(connectionId)->hex();
  else
    return {};
}

} // namespace Star
