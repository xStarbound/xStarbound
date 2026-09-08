#ifndef STAR_PLAYER_LUA_BINDINGS_HPP
#define STAR_PLAYER_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Player);
STAR_CLASS(Songbook);

namespace LuaBindings {
  LuaCallbacks makePlayerCallbacks(Player* player, bool removeChatCallbacks = false);
  LuaCallbacks makeSongbookCallbacks(Songbook* songbook);
} // namespace LuaBindings
} // namespace Star

#endif
