#ifndef STAR_NETWORKED_ANIMATOR_LUA_BINDINGS_HPP
#define STAR_NETWORKED_ANIMATOR_LUA_BINDINGS_HPP

#include "StarColor.hpp"
#include "StarJsonExtra.hpp"
#include "StarLua.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarNetworkedAnimator.hpp"
#include "StarPoly.hpp"

namespace Star {

STAR_CLASS(NetworkedAnimator);

namespace LuaBindings {
  // FezzedOne: Needed to turn this into a generic and put it in the header because of the mess that is `NetworkedAnimator` lifetimes and parent objects.
  template <class Parent>
  LuaCallbacks makeNetworkedAnimatorCallbacks(NetworkedAnimator* networkedAnimator, Parent* lifetimePtr);
} // namespace LuaBindings

template <class Parent>
LuaCallbacks LuaBindings::makeNetworkedAnimatorCallbacks(NetworkedAnimator* networkedAnimator, Parent* lifetimePtr) {
  LuaCallbacks callbacks;

  SmugglePtr<Parent> parent = GameObjectRegistry::smuggleWrap(lifetimePtr);

  callbacks.registerCallbackWithSignature<bool, String, String, bool>(
      "setAnimationState", LUA_BIND_PROXY(parent, &NetworkedAnimator::setState, networkedAnimator, _1, _2, _3));
  callbacks.registerCallbackWithSignature<String, String>(
      "animationState", LUA_BIND_PROXY(parent, &NetworkedAnimator::state, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<Json, String, String>(
      "animationStateProperty", LUA_BIND_PROXY(parent, &NetworkedAnimator::stateProperty, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, String>(
      "setGlobalTag", LUA_BIND_PROXY(parent, &NetworkedAnimator::setGlobalTag, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, String, String>(
      "setPartTag", LUA_BIND_PROXY(parent, &NetworkedAnimator::setPartTag, networkedAnimator, _1, _2, _3));
  callbacks.registerCallback("setFlipped",
      [networkedAnimator, parent](bool flipped, Maybe<float> relativeCenterLine) {
        parent.checkSmuggle();
        networkedAnimator->setFlipped(flipped, relativeCenterLine.value());
      });
  callbacks.registerCallbackWithSignature<void, float>(
      "setAnimationRate", LUA_BIND_PROXY(parent, &NetworkedAnimator::setAnimationRate, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<void, String, float, bool>(
      "rotateGroup", LUA_BIND_PROXY(parent, &NetworkedAnimator::rotateGroup, networkedAnimator, _1, _2, _3));
  callbacks.registerCallbackWithSignature<float, String>(
      "currentRotationAngle", LUA_BIND_PROXY(parent, &NetworkedAnimator::currentRotationAngle, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<bool, String>(
      "hasTransformationGroup", LUA_BIND_PROXY(parent, &NetworkedAnimator::hasTransformationGroup, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<void, String, Vec2F>("translateTransformationGroup",
      LUA_BIND_PROXY(parent, &NetworkedAnimator::translateTransformationGroup, networkedAnimator, _1, _2));
  callbacks.registerCallback("rotateTransformationGroup",
      [networkedAnimator, parent](String const& transformationGroup, float rotation, Maybe<Vec2F> const& rotationCenter) {
        parent.checkSmuggle();
        networkedAnimator->rotateTransformationGroup(transformationGroup, rotation, rotationCenter.value());
      });
  callbacks.registerCallback("scaleTransformationGroup",
      [networkedAnimator, parent](LuaEngine& engine, String const& transformationGroup, LuaValue scale, Maybe<Vec2F> const& scaleCenter) {
        parent.checkSmuggle();
        if (auto cs = engine.luaMaybeTo<Vec2F>(scale))
          networkedAnimator->scaleTransformationGroup(transformationGroup, *cs, scaleCenter.value());
        else
          networkedAnimator->scaleTransformationGroup(transformationGroup, engine.luaTo<float>(scale), scaleCenter.value());
      });
  callbacks.registerCallbackWithSignature<void, String, float, float, float, float, float, float>(
      "transformTransformationGroup",
      LUA_BIND_PROXY(parent, &NetworkedAnimator::transformTransformationGroup, networkedAnimator, _1, _2, _3, _4, _5, _6, _7));
  callbacks.registerCallbackWithSignature<void, String, float, float, float, float, float, float, float, float, float>(
      "fullTransformTransformationGroup",
      LUA_BIND_PROXY(parent, &NetworkedAnimator::fullTransformTransformationGroup, networkedAnimator, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10));
  callbacks.registerCallbackWithSignature<void, String, float, float, float, float, float, float, float, float, float>(
      "setTransformationGroupTransform",
      LUA_BIND_PROXY(parent, &NetworkedAnimator::setTransformationGroupTransform, networkedAnimator, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10));
  callbacks.registerCallbackWithSignature<void, String>(
      "resetTransformationGroup", LUA_BIND_PROXY(parent, &NetworkedAnimator::resetTransformationGroup, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<void, String, bool>(
      "setParticleEmitterActive", LUA_BIND_PROXY(parent, &NetworkedAnimator::setParticleEmitterActive, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, float>("setParticleEmitterEmissionRate",
      LUA_BIND_PROXY(parent, &NetworkedAnimator::setParticleEmitterEmissionRate, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, unsigned>("setParticleEmitterBurstCount",
      LUA_BIND_PROXY(parent, &NetworkedAnimator::setParticleEmitterBurstCount, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, RectF>("setParticleEmitterOffsetRegion",
      LUA_BIND_PROXY(parent, &NetworkedAnimator::setParticleEmitterOffsetRegion, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String>(
      "burstParticleEmitter", LUA_BIND_PROXY(parent, &NetworkedAnimator::burstParticleEmitter, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<void, String, bool>(
      "setLightActive", LUA_BIND_PROXY(parent, &NetworkedAnimator::setLightActive, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, Vec2F>(
      "setLightPosition", LUA_BIND_PROXY(parent, &NetworkedAnimator::setLightPosition, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, Color>(
      "setLightColor", LUA_BIND_PROXY(parent, &NetworkedAnimator::setLightColor, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, float>(
      "setLightPointAngle", LUA_BIND_PROXY(parent, &NetworkedAnimator::setLightPointAngle, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String>(
      "hasSound", LUA_BIND_PROXY(parent, &NetworkedAnimator::hasSound, networkedAnimator, _1));
  callbacks.registerCallbackWithSignature<void, String, StringList>(
      "setSoundPool", LUA_BIND_PROXY(parent, &NetworkedAnimator::setSoundPool, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, Vec2F>(
      "setSoundPosition", LUA_BIND_PROXY(parent, &NetworkedAnimator::setSoundPosition, networkedAnimator, _1, _2));
  callbacks.registerCallback("playSound",
      [networkedAnimator, parent](String const& sound, Maybe<int> loops) {
        parent.checkSmuggle();
        networkedAnimator->playSound(sound, loops.value());
      });

  callbacks.registerCallback("setSoundVolume",
      [networkedAnimator, parent](String const& sound, float targetVolume, Maybe<float> rampTime) {
        parent.checkSmuggle();
        networkedAnimator->setSoundVolume(sound, targetVolume, rampTime.value(0));
      });
  callbacks.registerCallback("setSoundPitch",
      [networkedAnimator, parent](String const& sound, float targetPitch, Maybe<float> rampTime) {
        parent.checkSmuggle();
        networkedAnimator->setSoundPitchMultiplier(sound, targetPitch, rampTime.value(0));
      });

  callbacks.registerCallback("stopAllSounds",
      [networkedAnimator, parent](String const& sound, Maybe<float> rampTime) {
        parent.checkSmuggle();
        networkedAnimator->stopAllSounds(sound, rampTime.value());
      });

  callbacks.registerCallbackWithSignature<void, String, bool>(
      "setEffectActive", LUA_BIND_PROXY(parent, &NetworkedAnimator::setEffectEnabled, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<Maybe<Vec2F>, String, String>("partPoint", LUA_BIND_PROXY(parent, &NetworkedAnimator::partPoint, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<Maybe<PolyF>, String, String>("partPoly", LUA_BIND_PROXY(parent, &NetworkedAnimator::partPoly, networkedAnimator, _1, _2));
  callbacks.registerCallbackWithSignature<Json, String, String>("partProperty", LUA_BIND_PROXY(parent, &NetworkedAnimator::partProperty, networkedAnimator, _1, _2));

  callbacks.registerCallback("transformPoint", [networkedAnimator, parent](Vec2F point, String const& part) -> Vec2F {
    parent.checkSmuggle();
    return networkedAnimator->partTransformation(part).transformVec2(point);
  });
  callbacks.registerCallback("transformPoly", [networkedAnimator, parent](PolyF poly, String const& part) -> PolyF {
    parent.checkSmuggle();
    poly.transform(networkedAnimator->partTransformation(part));
    return poly;
  });


  return callbacks;
}

} // namespace Star

#endif
