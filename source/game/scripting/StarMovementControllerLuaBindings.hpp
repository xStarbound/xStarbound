#ifndef STAR_MOVEMENT_CONTROLLER_LUA_BINDINGS_HPP
#define STAR_MOVEMENT_CONTROLLER_LUA_BINDINGS_HPP

#include "StarLua.hpp"

namespace Star {

STAR_CLASS(MovementController);
STAR_CLASS(Entity);

namespace LuaBindings {
  // FezzedOne: The entity pointer is needed for smuggling checks because of the way movement controllers are handled on some entity types.
  LuaCallbacks makeMovementControllerCallbacks(MovementController* movementController, Entity* entity);
} // namespace LuaBindings
} // namespace Star

#endif
