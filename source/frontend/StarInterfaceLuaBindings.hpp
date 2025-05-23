#ifndef STAR_INTERFACE_LUA_BINDINGS_HPP
#define STAR_INTERFACE_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(MainInterface);
STAR_CLASS(WorldCamera);
STAR_CLASS(WorldPainter);
STAR_CLASS(WorldClient);

namespace LuaBindings {
  LuaCallbacks makeChatCallbacks(MainInterface* mainInterface, bool removeHoakyCallbacks = false);
  LuaCallbacks makeClipboardCallbacks(MainInterface* mainInterface);
  LuaCallbacks makeInterfaceCallbacks(MainInterface* mainInterface, bool unsafeVersion = false);
  // [OpenStarbound] Kae: Add camera bindings.
  LuaCallbacks makeCameraCallbacks(WorldCamera* camera);
  LuaCallbacks makeRenderingCallbacks(WorldPainter* worldPainter);
} // namespace LuaBindings

} // namespace Star

#endif
