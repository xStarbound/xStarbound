#ifndef STAR_ACTOR_MOVEMENT_COMPONENT_HPP
#define STAR_ACTOR_MOVEMENT_COMPONENT_HPP

#include "StarActorMovementController.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

STAR_CLASS(Player);

// Wraps a LuaUpdatableComponent to handle the particularly tricky case of
// maintaining ActorMovementController controls when we do not call the script
// update every tick.
template <typename Base>
class LuaActorMovementComponent : public Base {
public:
  LuaActorMovementComponent();

  void addActorMovementCallbacks(ActorMovementController* actorMovementController);
  void removeActorMovementCallbacks();

  // If true, then the controls are automatically cleared on script update.
  // Defaults to true
  bool autoClearControls() const;
  void setAutoClearControls(bool autoClearControls);

  // Updates the lua script component and applies held controls.  If no script
  // update is scheduled this tick, then the controls from the last update will
  // be held and not cleared.  If a script update is scheduled this tick, then
  // the controls will be cleared only if autoClearControls is set to true.
  template <typename Ret = LuaValue, typename... V>
  Maybe<Ret> update(V&&... args);

  template <typename Parent>
  void addActorMovementCallbacks(ActorMovementController* actorMovementController, Parent* parentPtr) {
    m_movementController = actorMovementController;
    if (m_movementController && parentPtr) {
      LuaCallbacks callbacks;

      auto movementController = GameObjectRegistry::smuggleWrap(m_movementController);
      auto lifetime = GameObjectRegistry::smuggleWrap(parentPtr);

      // FezzedOne: Added missing actor movement controller callbacks.
      callbacks.registerCallback(
          "parameters", [movementController]() { return movementController->parameters().toJson(); });
      callbacks.registerCallbackWithSignature<void, Json>(
          "applyParameters", LUA_BIND(&ActorMovementController::applyParameters, movementController, _1));
      callbacks.registerCallbackWithSignature<void, Json>(
          "resetParameters", LUA_BIND(&ActorMovementController::resetParameters, movementController, _1));

      callbacks.registerCallback("mass", [movementController]() {
        return movementController->mass();
      });

      callbacks.registerCallback("boundBox", [movementController]() {
        return movementController->collisionPoly().boundBox();
      });

      callbacks.registerCallbackWithSignature<RectF>(
          "collisionBoundBox", LUA_BIND(&ActorMovementController::collisionBoundBox, movementController));

      callbacks.registerCallbackWithSignature<RectF>(
          "localBoundBox", LUA_BIND(&ActorMovementController::localBoundBox, movementController));

      callbacks.registerCallback("collisionPoly", [movementController]() {
        return movementController->collisionPoly();
      });

      callbacks.registerCallback("collisionBody", [movementController]() {
        return movementController->collisionBody();
      });

      callbacks.registerCallback("position", [movementController]() {
        return movementController->position();
      });

      callbacks.registerCallback("xPosition", [movementController]() {
        return movementController->xPosition();
      });

      callbacks.registerCallback("yPosition", [movementController]() {
        return movementController->yPosition();
      });

      callbacks.registerCallback("velocity", [movementController]() {
        return movementController->velocity();
      });

      callbacks.registerCallback("xVelocity", [movementController]() {
        return movementController->xVelocity();
      });

      callbacks.registerCallback("yVelocity", [movementController]() {
        return movementController->yVelocity();
      });

      callbacks.registerCallback("rotation", [movementController]() {
        return movementController->rotation();
      });

      callbacks.registerCallback("isColliding", [movementController]() {
        return movementController->isColliding();
      });

      callbacks.registerCallback("isNullColliding", [movementController]() {
        return movementController->isNullColliding();
      });

      callbacks.registerCallback("isCollisionStuck", [movementController]() {
        return movementController->isCollisionStuck();
      });

      callbacks.registerCallback("stickingDirection", [movementController]() {
        return movementController->stickingDirection();
      });

      callbacks.registerCallback("liquidPercentage", [movementController]() {
        return movementController->liquidPercentage();
      });

      callbacks.registerCallback("liquidId", [movementController]() {
        return movementController->liquidId();
      });

      callbacks.registerCallback("onGround", [movementController]() {
        return movementController->onGround();
      });

      callbacks.registerCallback("zeroG", [movementController]() {
        return movementController->zeroG();
      });

      callbacks.registerCallback("atWorldLimit", [movementController](bool bottomOnly) {
        return movementController->atWorldLimit(bottomOnly);
      });

      callbacks.registerCallback("setAnchorState", [movementController](EntityId anchorableEntity, size_t anchorPosition) {
        movementController->setAnchorState({anchorableEntity, anchorPosition});
      });

      callbacks.registerCallback("resetAnchorState", [movementController]() {
        movementController->resetAnchorState();
      });

      callbacks.registerCallback("anchorState", [movementController]() {
        if (auto anchorState = movementController->anchorState())
          return LuaVariadic<LuaValue>{LuaInt(anchorState->entityId), LuaInt(anchorState->positionIndex)};
        return LuaVariadic<LuaValue>();
      });

      callbacks.registerCallback("setPosition", [movementController](Vec2F const& pos) {
        movementController->setPosition(pos);
      });

      callbacks.registerCallback("setXPosition", [movementController](float xPosition) {
        movementController->setXPosition(xPosition);
      });

      callbacks.registerCallback("setYPosition", [movementController](float yPosition) {
        movementController->setYPosition(yPosition);
      });

      callbacks.registerCallback("translate", [movementController](Vec2F const& translate) {
        movementController->translate(translate);
      });

      callbacks.registerCallback("setVelocity", [movementController, lifetime, this](Vec2F const& vel) {
        movementController.checkSmuggle();
        lifetime.checkSmuggle();
        m_resetPathMove = true;
        movementController->setVelocity(vel);
      });

      callbacks.registerCallback("setXVelocity", [movementController, lifetime, this](float xVel) {
        movementController.checkSmuggle();
        lifetime.checkSmuggle();
        m_resetPathMove = true;
        movementController->setXVelocity(xVel);
      });

      callbacks.registerCallback("setYVelocity", [movementController, lifetime, this](float yVel) {
        movementController.checkSmuggle();
        lifetime.checkSmuggle();
        m_resetPathMove = true;
        movementController->setYVelocity(yVel);
      });

      callbacks.registerCallback("addMomentum", [movementController, lifetime, this](Vec2F const& momentum) {
        movementController.checkSmuggle();
        lifetime.checkSmuggle();
        m_resetPathMove = true;
        movementController->addMomentum(momentum);
      });

      callbacks.registerCallback("setRotation", [movementController, lifetime, this](float rotation) {
        movementController.checkSmuggle();
        lifetime.checkSmuggle();
        m_resetPathMove = true;
        movementController->setRotation(rotation);
      });

      callbacks.registerCallback("baseParameters", [movementController]() {
        return movementController->baseParameters();
      });

      callbacks.registerCallback("walking", [movementController]() {
        return movementController->walking();
      });

      callbacks.registerCallback("running", [movementController]() {
        return movementController->running();
      });

      callbacks.registerCallback("movingDirection", [movementController]() {
        return numericalDirection(movementController->movingDirection());
      });

      callbacks.registerCallback("facingDirection", [movementController]() {
        return numericalDirection(movementController->facingDirection());
      });

      callbacks.registerCallback("crouching", [movementController]() {
        return movementController->crouching();
      });

      callbacks.registerCallback("flying", [movementController]() {
        return movementController->flying();
      });

      callbacks.registerCallback("falling", [movementController]() {
        return movementController->falling();
      });

      callbacks.registerCallback("canJump", [movementController]() {
        return movementController->canJump();
      });

      callbacks.registerCallback("jumping", [movementController]() {
        return movementController->jumping();
      });

      callbacks.registerCallback("groundMovement", [movementController]() {
        return movementController->groundMovement();
      });

      callbacks.registerCallback("liquidMovement", [movementController]() {
        return movementController->liquidMovement();
      });

      callbacks.registerCallback("controlRotation", [this, lifetime](float rotation) {
        lifetime.checkSmuggle();
        m_controlRotation += rotation;
      });

      callbacks.registerCallback("controlAcceleration", [this, lifetime](Vec2F const& accel) {
        lifetime.checkSmuggle();
        m_controlAcceleration += accel;
      });

      callbacks.registerCallback("controlForce", [this, lifetime](Vec2F const& force) {
        lifetime.checkSmuggle();
        m_controlForce += force;
      });

      callbacks.registerCallback("controlApproachVelocity", [this, lifetime](Vec2F const& arg1, float arg2) {
        lifetime.checkSmuggle();
        m_controlApproachVelocity.set(make_tuple(arg1, arg2));
      });

      callbacks.registerCallback("controlApproachVelocityAlongAngle", [this, lifetime](float angle, float targetVelocity, float maxControlForce, bool positiveOnly) {
        lifetime.checkSmuggle();
        m_controlApproachVelocityAlongAngle.set(make_tuple(angle, targetVelocity, maxControlForce, positiveOnly));
      });

      callbacks.registerCallback("controlApproachXVelocity", [this, lifetime](float targetXVelocity, float maxControlForce) {
        lifetime.checkSmuggle();
        m_controlApproachVelocityAlongAngle.set(make_tuple(0.0f, targetXVelocity, maxControlForce, false));
      });

      callbacks.registerCallback("controlApproachYVelocity", [this, lifetime](float targetYVelocity, float maxControlForce) {
        lifetime.checkSmuggle();
        m_controlApproachVelocityAlongAngle.set(
            make_tuple(Constants::pi / 2.0f, targetYVelocity, maxControlForce, false));
      });

      callbacks.registerCallback("controlParameters", [this, lifetime](ActorMovementParameters const& arg1) {
        lifetime.checkSmuggle();
        m_controlParameters = m_controlParameters.value().merge(arg1);
      });

      callbacks.registerCallback("controlModifiers", [this, lifetime](ActorMovementModifiers const& arg1) {
        lifetime.checkSmuggle();
        m_controlModifiers = m_controlModifiers.value().combine(arg1);
      });

      callbacks.registerCallback("controlMove", [this, lifetime](Maybe<float> const& arg1, Maybe<bool> const& arg2) {
        lifetime.checkSmuggle();
        if (auto direction = directionOf(arg1.value()))
          m_controlMove.set(make_tuple(*direction, arg2.value(true)));
      });

      callbacks.registerCallback("controlFace", [this, lifetime](Maybe<float> const& arg1) {
        lifetime.checkSmuggle();
        if (auto direction = directionOf(arg1.value()))
          m_controlFace = *direction;
      });

      callbacks.registerCallback("controlDown", [this, lifetime]() {
        lifetime.checkSmuggle();
        m_controlDown = true;
      });

      callbacks.registerCallback("controlCrouch", [this, lifetime]() {
        lifetime.checkSmuggle();
        m_controlCrouch = true;
      });

      callbacks.registerCallback("controlJump", [this, lifetime](bool arg1) {
        lifetime.checkSmuggle();
        m_controlJump = arg1;
      });

      callbacks.registerCallback("controlHoldJump", [this, lifetime]() {
        lifetime.checkSmuggle();
        m_controlHoldJump = true;
      });

      callbacks.registerCallback("controlFly", [this, lifetime](Vec2F const& arg1) {
        lifetime.checkSmuggle();
        m_controlFly = arg1;
      });

      callbacks.registerCallback("controlPathMove", [this, movementController, lifetime](Vec2F const& position, Maybe<bool> run, Maybe<PlatformerAStar::Parameters> parameters) -> Maybe<bool> {
        movementController.checkSmuggle();
        lifetime.checkSmuggle();
        if (m_pathMoveResult && m_pathMoveResult->first == position) {
          return take(m_pathMoveResult).apply([](pair<Vec2F, bool> const& p) { return p.second; });
        } else {
          m_pathMoveResult.reset();
          auto result = movementController->pathMove(position, run.value(false), parameters);
          if (result.isNothing())
            m_controlPathMove = pair<Vec2F, bool>(position, run.value(false));
          return result.apply([](pair<Vec2F, bool> const& p) { return p.second; });
        }
      });

      callbacks.registerCallback("pathfinding", [movementController]() -> bool {
        return movementController->pathfinding();
      });

      callbacks.registerCallbackWithSignature<bool>("autoClearControls", LUA_BIND_PROXY(lifetime, &LuaActorMovementComponent::autoClearControls, this));
      callbacks.registerCallbackWithSignature<void, bool>("setAutoClearControls", LUA_BIND_PROXY(lifetime, &LuaActorMovementComponent::setAutoClearControls, this, _1));
      callbacks.registerCallbackWithSignature<void>("clearControls", LUA_BIND_PROXY(lifetime, &LuaActorMovementComponent::clearControls, this));

      Base::addCallbacks("mcontroller", callbacks);

    } else {
      Base::removeCallbacks("mcontroller");
    }
  }


private:
  void performControls();
  void clearControls();

  ActorMovementController* m_movementController;
  bool m_autoClearControls;

  float m_controlRotation;
  Vec2F m_controlAcceleration;
  Vec2F m_controlForce;
  Maybe<tuple<Vec2F, float>> m_controlApproachVelocity;
  Maybe<tuple<float, float, float, bool>> m_controlApproachVelocityAlongAngle;
  Maybe<ActorMovementParameters> m_controlParameters;
  Maybe<ActorMovementModifiers> m_controlModifiers;
  Maybe<tuple<Direction, bool>> m_controlMove;
  Maybe<Direction> m_controlFace;
  bool m_controlDown;
  bool m_controlCrouch;
  Maybe<bool> m_controlJump;
  bool m_controlHoldJump;
  Maybe<Vec2F> m_controlFly;

  bool m_resetPathMove;
  Maybe<pair<Vec2F, bool>> m_controlPathMove;
  Maybe<pair<Vec2F, bool>> m_pathMoveResult;
};

template <typename Base>
LuaActorMovementComponent<Base>::LuaActorMovementComponent()
    : m_autoClearControls(true),
      m_controlRotation(0.0f),
      m_controlDown(false),
      m_controlCrouch(false),
      m_controlHoldJump(false) {}

template <typename Base>
void LuaActorMovementComponent<Base>::removeActorMovementCallbacks() {
  addActorMovementCallbacks<Player>(nullptr, nullptr);
}

template <typename Base>
bool LuaActorMovementComponent<Base>::autoClearControls() const {
  return m_autoClearControls;
}

template <typename Base>
void LuaActorMovementComponent<Base>::setAutoClearControls(bool autoClearControls) {
  m_autoClearControls = autoClearControls;
}

template <typename Base>
template <typename Ret, typename... V>
Maybe<Ret> LuaActorMovementComponent<Base>::update(V&&... args) {
  if (Base::updateReady()) {
    if (m_autoClearControls)
      clearControls();
  }
  Maybe<Ret> ret = Base::template update<Ret>(std::forward<V>(args)...);
  performControls();
  return ret;
}

template <typename Base>
void LuaActorMovementComponent<Base>::performControls() {
  if (m_movementController) {
    m_movementController->controlRotation(m_controlRotation);
    m_movementController->controlAcceleration(m_controlAcceleration);
    m_movementController->controlForce(m_controlForce);
    if (m_controlApproachVelocity)
      tupleUnpackFunction(bind(&ActorMovementController::controlApproachVelocity, m_movementController, _1, _2), *m_controlApproachVelocity);
    if (m_controlApproachVelocityAlongAngle)
      tupleUnpackFunction(bind(&ActorMovementController::controlApproachVelocityAlongAngle, m_movementController, _1, _2, _3, _4), *m_controlApproachVelocityAlongAngle);
    if (m_controlParameters)
      m_movementController->controlParameters(*m_controlParameters);
    if (m_controlModifiers)
      m_movementController->controlModifiers(*m_controlModifiers);
    if (m_controlMove)
      tupleUnpackFunction(bind(&ActorMovementController::controlMove, m_movementController, _1, _2), *m_controlMove);
    if (m_controlFace)
      m_movementController->controlFace(*m_controlFace);
    if (m_controlDown)
      m_movementController->controlDown();
    if (m_controlCrouch)
      m_movementController->controlCrouch();
    if (m_controlJump)
      m_movementController->controlJump(*m_controlJump);
    if (m_controlHoldJump && !m_movementController->onGround())
      m_movementController->controlJump();
    if (m_controlFly)
      m_movementController->controlFly(*m_controlFly);

    // some action was taken that has priority over pathing, setting position or velocity
    if (m_resetPathMove)
      m_controlPathMove = {};
    if (m_controlPathMove && m_pathMoveResult.isNothing())
      m_pathMoveResult = m_movementController->controlPathMove(m_controlPathMove->first, m_controlPathMove->second);
  }
}

template <typename Base>
void LuaActorMovementComponent<Base>::clearControls() {
  m_controlRotation = {};
  m_controlAcceleration = {};
  m_controlForce = {};
  m_controlApproachVelocity = {};
  m_controlApproachVelocityAlongAngle = {};
  m_controlParameters = {};
  m_controlModifiers = {};
  m_controlMove = {};
  m_controlFace = {};
  m_controlDown = {};
  m_controlCrouch = {};
  m_controlJump = {};
  m_controlHoldJump = {};
  m_controlFly = {};

  m_resetPathMove = false;
  // Clear path move result one clear after controlPathMove is no longer called
  // to keep the result available for the following update
  if (m_controlPathMove.isNothing())
    m_pathMoveResult = {};
  m_controlPathMove = {};
}
} // namespace Star

#endif
