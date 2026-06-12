#ifndef STAR_BEHAVIOR_LUA_BINDINGS_HPP
#define STAR_BEHAVIOR_LUA_BINDINGS_HPP

#include "StarBehaviorState.hpp"
#include "StarLua.hpp"

namespace Star {

STAR_CLASS(Root);
STAR_CLASS(UniverseClient);
STAR_CLASS(Entity);

namespace LuaBindings {
  // FezzedOne: Needs an entity pointer for smuggling checks because of where behaviour lists are stored.
  LuaCallbacks makeBehaviorLuaCallbacks(List<BehaviorStatePtr>* list, Entity* entity);
} // namespace LuaBindings
} // namespace Star

#endif
