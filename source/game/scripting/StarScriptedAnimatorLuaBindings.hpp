#ifndef STAR_SCRIPTED_ANIMATOR_LUA_BINDINGS_HPP
#define STAR_SCRIPTED_ANIMATOR_LUA_BINDINGS_HPP

#include "StarJsonExtra.hpp"
#include "StarLua.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarNetworkedAnimator.hpp"

namespace Star {

namespace LuaBindings {
  template <class Parent>
  LuaCallbacks makeScriptedAnimatorCallbacks(const NetworkedAnimator* animator, function<Json(String const&, Json const&)> getParameter, Parent* lifetimePtr);
}

template <class Parent>
LuaCallbacks LuaBindings::makeScriptedAnimatorCallbacks(const NetworkedAnimator* animator, function<Json(String const&, Json const&)> getParameter, Parent* lifetimePtr) {
  LuaCallbacks callbacks;

  auto parent = GameObjectRegistry::smuggleWrap(lifetimePtr);

  callbacks.registerCallback("animationParameter", getParameter);
  callbacks.registerCallback("partPoint", [animator](String const& partName, String const& propertyName) {
    parent.checkSmuggle();
    return animator->partPoint(partName, propertyName);
  });
  callbacks.registerCallback("partPoly", [animator](String const& partName, String const& propertyName) {
    parent.checkSmuggle();
    return animator->partPoly(partName, propertyName);
  });

  callbacks.registerCallback("transformPoint", [animator](Vec2F point, String const& part) -> Vec2F {
    parent.checkSmuggle();
    return animator->partTransformation(part).transformVec2(point);
  });
  callbacks.registerCallback("transformPoly", [animator](PolyF poly, String const& part) -> PolyF {
    parent.checkSmuggle();
    poly.transform(animator->partTransformation(part));
    return poly;
  });

  return callbacks;
}

} // namespace Star

#endif
