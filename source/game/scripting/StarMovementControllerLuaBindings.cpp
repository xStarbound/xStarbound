#include "StarMovementControllerLuaBindings.hpp"
#include "StarEntity.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarMovementController.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeMovementControllerCallbacks(MovementController* movementController, Entity* entityPtr) {
  LuaCallbacks callbacks;

  auto entity = GameObjectRegistry::smuggleWrap(entityPtr);

  callbacks.registerCallback(
      "parameters", [movementController, entity]() { 
      entity.checkSmuggle();
      return movementController->parameters().toJson(); });
  callbacks.registerCallbackWithSignature<void, Json>(
      "applyParameters", LUA_BIND_PROXY(entity, &MovementController::applyParameters, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Json>(
      "resetParameters", LUA_BIND_PROXY(entity, &MovementController::resetParameters, movementController, _1));
  callbacks.registerCallbackWithSignature<float>("mass", LUA_BIND_PROXY(entity, &MovementController::mass, movementController));
  callbacks.registerCallback("boundBox", [movementController, entity]() {
    entity.checkSmuggle();
    return movementController->collisionPoly().boundBox();
  });
  callbacks.registerCallbackWithSignature<PolyF>(
      "collisionPoly", LUA_BIND_PROXY(entity, &MovementController::collisionPoly, movementController));
  callbacks.registerCallbackWithSignature<Vec2F>("position", LUA_BIND_PROXY(entity, &MovementController::position, movementController));
  callbacks.registerCallbackWithSignature<float>("xPosition", LUA_BIND_PROXY(entity, &MovementController::xPosition, movementController));
  callbacks.registerCallbackWithSignature<float>("yPosition", LUA_BIND_PROXY(entity, &MovementController::yPosition, movementController));
  callbacks.registerCallbackWithSignature<Vec2F>("velocity", LUA_BIND_PROXY(entity, &MovementController::velocity, movementController));
  callbacks.registerCallbackWithSignature<float>("xVelocity", LUA_BIND_PROXY(entity, &MovementController::xVelocity, movementController));
  callbacks.registerCallbackWithSignature<float>("yVelocity", LUA_BIND_PROXY(entity, &MovementController::yVelocity, movementController));
  callbacks.registerCallbackWithSignature<float>("rotation", LUA_BIND_PROXY(entity, &MovementController::rotation, movementController));
  callbacks.registerCallbackWithSignature<PolyF>(
      "collisionBody", LUA_BIND_PROXY(entity, &MovementController::collisionBody, movementController));
  callbacks.registerCallbackWithSignature<RectF>(
      "collisionBoundBox", LUA_BIND_PROXY(entity, &MovementController::collisionBoundBox, movementController));
  callbacks.registerCallbackWithSignature<RectF>(
      "localBoundBox", LUA_BIND_PROXY(entity, &MovementController::localBoundBox, movementController));
  callbacks.registerCallbackWithSignature<bool>(
      "isColliding", LUA_BIND_PROXY(entity, &MovementController::isColliding, movementController));
  callbacks.registerCallbackWithSignature<bool>(
      "isNullColliding", LUA_BIND_PROXY(entity, &MovementController::isNullColliding, movementController));
  callbacks.registerCallbackWithSignature<bool>(
      "isCollisionStuck", LUA_BIND_PROXY(entity, &MovementController::isCollisionStuck, movementController));
  callbacks.registerCallbackWithSignature<Maybe<float>>(
      "stickingDirection", LUA_BIND_PROXY(entity, &MovementController::stickingDirection, movementController));
  callbacks.registerCallbackWithSignature<float>(
      "liquidPercentage", LUA_BIND_PROXY(entity, &MovementController::liquidPercentage, movementController));
  callbacks.registerCallbackWithSignature<LiquidId>(
      "liquidId", LUA_BIND_PROXY(entity, &MovementController::liquidId, movementController));
  callbacks.registerCallbackWithSignature<bool>("onGround", LUA_BIND_PROXY(entity, &MovementController::onGround, movementController));
  callbacks.registerCallbackWithSignature<bool>("zeroG", LUA_BIND_PROXY(entity, &MovementController::zeroG, movementController));
  callbacks.registerCallbackWithSignature<bool, bool>("atWorldLimit", LUA_BIND_PROXY(entity, &MovementController::atWorldLimit, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "setPosition", LUA_BIND_PROXY(entity, &MovementController::setPosition, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setXPosition", LUA_BIND_PROXY(entity, &MovementController::setXPosition, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setYPosition", LUA_BIND_PROXY(entity, &MovementController::setYPosition, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "translate", LUA_BIND_PROXY(entity, &MovementController::translate, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "setVelocity", LUA_BIND_PROXY(entity, &MovementController::setVelocity, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setXVelocity", LUA_BIND_PROXY(entity, &MovementController::setXVelocity, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setYVelocity", LUA_BIND_PROXY(entity, &MovementController::setYVelocity, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "addMomentum", LUA_BIND_PROXY(entity, &MovementController::addMomentum, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setRotation", LUA_BIND_PROXY(entity, &MovementController::setRotation, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "rotate", LUA_BIND_PROXY(entity, &MovementController::rotate, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "accelerate", LUA_BIND_PROXY(entity, &MovementController::accelerate, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "force", LUA_BIND_PROXY(entity, &MovementController::force, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F, float>(
      "approachVelocity", LUA_BIND_PROXY(entity, &MovementController::approachVelocity, movementController, _1, _2));
  callbacks.registerCallbackWithSignature<void, float, float, float, bool>("approachVelocityAlongAngle",
      LUA_BIND_PROXY(entity, &MovementController::approachVelocityAlongAngle, movementController, _1, _2, _3, _4));
  callbacks.registerCallbackWithSignature<void, float, float>(
      "approachXVelocity", LUA_BIND_PROXY(entity, &MovementController::approachXVelocity, movementController, _1, _2));
  callbacks.registerCallbackWithSignature<void, float, float>(
      "approachYVelocity", LUA_BIND_PROXY(entity, &MovementController::approachYVelocity, movementController, _1, _2));

  return callbacks;
}

} // namespace Star
