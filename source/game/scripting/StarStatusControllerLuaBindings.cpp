#include "StarStatusControllerLuaBindings.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarStatusController.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeStatusControllerCallbacks(StatusController* statControllerPtr) {
  LuaCallbacks callbacks;

  auto statController = GameObjectRegistry::smuggleWrap(statControllerPtr);

  callbacks.registerCallbackWithSignature<Json, String, Json>(
      "statusProperty", LUA_BIND(StatusControllerCallbacks::statusProperty, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, Json>(
      "setStatusProperty", LUA_BIND(StatusControllerCallbacks::setStatusProperty, statController, _1, _2));
  callbacks.registerCallbackWithSignature<float, String>(
      "stat", LUA_BIND(StatusControllerCallbacks::stat, statController, _1));
  callbacks.registerCallbackWithSignature<bool, String>(
      "statPositive", LUA_BIND(StatusControllerCallbacks::statPositive, statController, _1));
  callbacks.registerCallbackWithSignature<StringList>(
      "resourceNames", LUA_BIND(StatusControllerCallbacks::resourceNames, statController));
  callbacks.registerCallbackWithSignature<bool, String>(
      "isResource", LUA_BIND(StatusControllerCallbacks::isResource, statController, _1));
  callbacks.registerCallbackWithSignature<float, String>(
      "resource", LUA_BIND(StatusControllerCallbacks::resource, statController, _1));
  callbacks.registerCallbackWithSignature<bool, String>(
      "resourcePositive", LUA_BIND(StatusControllerCallbacks::resourcePositive, statController, _1));
  callbacks.registerCallbackWithSignature<void, String, float>(
      "setResource", LUA_BIND(StatusControllerCallbacks::setResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, float>(
      "modifyResource", LUA_BIND(StatusControllerCallbacks::modifyResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<float, String, float>(
      "giveResource", LUA_BIND(StatusControllerCallbacks::giveResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String, float>(
      "consumeResource", LUA_BIND(StatusControllerCallbacks::consumeResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String, float>(
      "overConsumeResource", LUA_BIND(StatusControllerCallbacks::overConsumeResource, statController, _1, _2));
  callbacks.registerCallbackWithSignature<bool, String>(
      "resourceLocked", LUA_BIND(StatusControllerCallbacks::resourceLocked, statController, _1));
  callbacks.registerCallbackWithSignature<void, String, bool>(
      "setResourceLocked", LUA_BIND(StatusControllerCallbacks::setResourceLocked, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String>(
      "resetResource", LUA_BIND(StatusControllerCallbacks::resetResource, statController, _1));
  callbacks.registerCallbackWithSignature<void>(
      "resetAllResources", LUA_BIND(StatusControllerCallbacks::resetAllResources, statController));
  callbacks.registerCallbackWithSignature<Maybe<float>, String>(
      "resourceMax", LUA_BIND(StatusControllerCallbacks::resourceMax, statController, _1));
  callbacks.registerCallbackWithSignature<Maybe<float>, String>(
      "resourcePercentage", LUA_BIND(StatusControllerCallbacks::resourcePercentage, statController, _1));
  callbacks.registerCallbackWithSignature<float, String, float>(
      "setResourcePercentage", LUA_BIND(StatusControllerCallbacks::setResourcePercentage, statController, _1, _2));
  callbacks.registerCallbackWithSignature<float, String, float>(
      "modifyResourcePercentage", LUA_BIND(StatusControllerCallbacks::modifyResourcePercentage, statController, _1, _2));
  callbacks.registerCallbackWithSignature<JsonArray, String>(
      "getPersistentEffects", LUA_BIND(StatusControllerCallbacks::getPersistentEffects, statController, _1));
  callbacks.registerCallbackWithSignature<void, String, Json>(
      "addPersistentEffect", LUA_BIND(StatusControllerCallbacks::addPersistentEffect, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, JsonArray>(
      "addPersistentEffects", LUA_BIND(StatusControllerCallbacks::addPersistentEffects, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String, JsonArray>(
      "setPersistentEffects", LUA_BIND(StatusControllerCallbacks::setPersistentEffects, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String>(
      "clearPersistentEffects", LUA_BIND(StatusControllerCallbacks::clearPersistentEffects, statController, _1));
  callbacks.registerCallbackWithSignature<void>(
      "clearAllPersistentEffects", LUA_BIND(StatusControllerCallbacks::clearAllPersistentEffects, statController));
  callbacks.registerCallbackWithSignature<void, String, Maybe<float>, Maybe<EntityId>>(
      "addEphemeralEffect", LUA_BIND(StatusControllerCallbacks::addEphemeralEffect, statController, _1, _2, _3));
  callbacks.registerCallbackWithSignature<void, JsonArray, Maybe<EntityId>>(
      "addEphemeralEffects", LUA_BIND(StatusControllerCallbacks::addEphemeralEffects, statController, _1, _2));
  callbacks.registerCallbackWithSignature<void, String>(
      "removeEphemeralEffect", LUA_BIND(StatusControllerCallbacks::removeEphemeralEffect, statController, _1));
  callbacks.registerCallbackWithSignature<void>(
      "clearEphemeralEffects", LUA_BIND(StatusControllerCallbacks::clearEphemeralEffects, statController));
  callbacks.registerCallbackWithSignature<LuaTupleReturn<List<Json>, uint64_t>, Maybe<uint64_t>>(
      "damageTakenSince", LUA_BIND(StatusControllerCallbacks::damageTakenSince, statController, _1));
  callbacks.registerCallbackWithSignature<LuaTupleReturn<List<Json>, uint64_t>, Maybe<uint64_t>>(
      "inflictedHitsSince", LUA_BIND(StatusControllerCallbacks::inflictedHitsSince, statController, _1));
  callbacks.registerCallbackWithSignature<LuaTupleReturn<List<Json>, uint64_t>, Maybe<uint64_t>>(
      "inflictedDamageSince", LUA_BIND(StatusControllerCallbacks::inflictedDamageSince, statController, _1));
  callbacks.registerCallbackWithSignature<List<JsonArray>>("activeUniqueStatusEffectSummary",
      LUA_BIND(&StatusControllerCallbacks::activeUniqueStatusEffectSummary, statController));
  callbacks.registerCallbackWithSignature<bool, String>("uniqueStatusEffectActive",
      LUA_BIND(&StatusControllerCallbacks::uniqueStatusEffectActive, statController, _1));

  // FezzedOne: Downstreamed OpenStarbound bugfix for segfault caused by invocation of this callback.
  callbacks.registerCallbackWithSignature<Directives>("primaryDirectives", LUA_BIND(&StatusController::primaryDirectives, statController));
  callbacks.registerCallback("setPrimaryDirectives", [statController](Maybe<String> const& directives) {
    // FezzedOne: Added this workaround because some mods forget to prepend a `?` to the first directive for some reason.
    if (auto p = directives.ptr()) {
      String const& directiveString = *p;
      if (!directiveString.empty() && !directiveString.beginsWith('?'))
        statController->setPrimaryDirectives(Directives('?' + directiveString));
      else
        statController->setPrimaryDirectives(Directives(std::move(directiveString)));
    } else {
      statController->setPrimaryDirectives(Directives());
    }
  });

  callbacks.registerCallbackWithSignature<void, DamageRequest>("applySelfDamageRequest", LUA_BIND(&StatusController::applySelfDamageRequest, statController, _1));

  return callbacks;
}

Json LuaBindings::StatusControllerCallbacks::statusProperty(
    StatusController* statController, String const& arg1, Json const& arg2) {
  return statController->statusProperty(arg1, arg2);
}

void LuaBindings::StatusControllerCallbacks::setStatusProperty(
    StatusController* statController, String const& arg1, Json const& arg2) {
  statController->setStatusProperty(arg1, arg2);
}

float LuaBindings::StatusControllerCallbacks::stat(StatusController* statController, String const& arg1) {
  return statController->stat(arg1);
}

bool LuaBindings::StatusControllerCallbacks::statPositive(StatusController* statController, String const& arg1) {
  return statController->statPositive(arg1);
}

StringList LuaBindings::StatusControllerCallbacks::resourceNames(StatusController* statController) {
  return statController->resourceNames();
}

bool LuaBindings::StatusControllerCallbacks::isResource(StatusController* statController, String const& arg1) {
  return statController->isResource(arg1);
}

float LuaBindings::StatusControllerCallbacks::resource(StatusController* statController, String const& arg1) {
  return statController->resource(arg1);
}

bool LuaBindings::StatusControllerCallbacks::resourcePositive(StatusController* statController, String const& arg1) {
  return statController->resourcePositive(arg1);
}

void LuaBindings::StatusControllerCallbacks::setResource(
    StatusController* statController, String const& arg1, float arg2) {
  statController->setResource(arg1, arg2);
}

void LuaBindings::StatusControllerCallbacks::modifyResource(
    StatusController* statController, String const& arg1, float arg2) {
  statController->modifyResource(arg1, arg2);
}

float LuaBindings::StatusControllerCallbacks::giveResource(
    StatusController* statController, String const& resourceName, float amount) {
  return statController->giveResource(resourceName, amount);
}

bool LuaBindings::StatusControllerCallbacks::consumeResource(
    StatusController* statController, String const& arg1, float arg2) {
  return statController->consumeResource(arg1, arg2);
}

bool LuaBindings::StatusControllerCallbacks::overConsumeResource(
    StatusController* statController, String const& arg1, float arg2) {
  return statController->overConsumeResource(arg1, arg2);
}

bool LuaBindings::StatusControllerCallbacks::resourceLocked(StatusController* statController, String const& arg1) {
  return statController->resourceLocked(arg1);
}

void LuaBindings::StatusControllerCallbacks::setResourceLocked(
    StatusController* statController, String const& arg1, bool arg2) {
  statController->setResourceLocked(arg1, arg2);
}

void LuaBindings::StatusControllerCallbacks::resetResource(StatusController* statController, String const& arg1) {
  statController->resetResource(arg1);
}

void LuaBindings::StatusControllerCallbacks::resetAllResources(StatusController* statController) {
  statController->resetAllResources();
}

Maybe<float> LuaBindings::StatusControllerCallbacks::resourceMax(StatusController* statController, String const& arg1) {
  return statController->resourceMax(arg1);
}

Maybe<float> LuaBindings::StatusControllerCallbacks::resourcePercentage(
    StatusController* statController, String const& arg1) {
  return statController->resourcePercentage(arg1);
}

float LuaBindings::StatusControllerCallbacks::setResourcePercentage(
    StatusController* statController, String const& arg1, float arg2) {
  return statController->setResourcePercentage(arg1, arg2);
}

float LuaBindings::StatusControllerCallbacks::modifyResourcePercentage(
    StatusController* statController, String const& arg1, float arg2) {
  return statController->modifyResourcePercentage(arg1, arg2);
}

JsonArray LuaBindings::StatusControllerCallbacks::getPersistentEffects(
    StatusController* statController, String const& arg1) {
  return statController->getPersistentEffects(arg1).transformed(jsonFromPersistentStatusEffect);
}

void LuaBindings::StatusControllerCallbacks::addPersistentEffect(
    StatusController* statController, String const& arg1, Json const& arg2) {
  addPersistentEffects(statController, arg1, JsonArray{arg2});
}

void LuaBindings::StatusControllerCallbacks::addPersistentEffects(
    StatusController* statController, String const& arg1, JsonArray const& arg2) {
  statController->addPersistentEffects(arg1, arg2.transformed(jsonToPersistentStatusEffect));
}

void LuaBindings::StatusControllerCallbacks::setPersistentEffects(
    StatusController* statController, String const& arg1, JsonArray const& arg2) {
  statController->setPersistentEffects(arg1, arg2.transformed(jsonToPersistentStatusEffect));
}

void LuaBindings::StatusControllerCallbacks::clearPersistentEffects(
    StatusController* statController, String const& arg1) {
  statController->clearPersistentEffects(arg1);
}

void LuaBindings::StatusControllerCallbacks::clearAllPersistentEffects(StatusController* statController) {
  statController->clearAllPersistentEffects();
}

void LuaBindings::StatusControllerCallbacks::addEphemeralEffect(
    StatusController* statController, String const& arg1, Maybe<float> arg2, Maybe<EntityId> sourceEntityId) {
  statController->addEphemeralEffect(EphemeralStatusEffect{UniqueStatusEffect{arg1}, arg2}, sourceEntityId);
}

void LuaBindings::StatusControllerCallbacks::addEphemeralEffects(
    StatusController* statController, JsonArray const& arg1, Maybe<EntityId> sourceEntityId) {
  statController->addEphemeralEffects(arg1.transformed(jsonToEphemeralStatusEffect), sourceEntityId);
}

void LuaBindings::StatusControllerCallbacks::removeEphemeralEffect(
    StatusController* statController, String const& arg1) {
  statController->removeEphemeralEffect(UniqueStatusEffect{arg1});
}

void LuaBindings::StatusControllerCallbacks::clearEphemeralEffects(StatusController* statController) {
  statController->clearEphemeralEffects();
}

LuaTupleReturn<List<Json>, uint64_t> LuaBindings::StatusControllerCallbacks::damageTakenSince(
    StatusController* statController, Maybe<uint64_t> timestep) {
  auto pair = statController->damageTakenSince(timestep.value());
  return luaTupleReturn(pair.first.transformed(mem_fn(&DamageNotification::toJson)), pair.second);
}

LuaTupleReturn<List<Json>, uint64_t> LuaBindings::StatusControllerCallbacks::inflictedHitsSince(
    StatusController* statController, Maybe<uint64_t> timestep) {
  auto pair = statController->inflictedHitsSince(timestep.value());
  return luaTupleReturn(
      pair.first.transformed([](auto const& p) { return p.second.toJson().set("targetEntityId", p.first); }),
      pair.second);
}

LuaTupleReturn<List<Json>, uint64_t> LuaBindings::StatusControllerCallbacks::inflictedDamageSince(
    StatusController* statController, Maybe<uint64_t> timestep) {
  auto pair = statController->inflictedDamageSince(timestep.value());
  return luaTupleReturn(pair.first.transformed(mem_fn(&DamageNotification::toJson)), pair.second);
}

List<JsonArray> LuaBindings::StatusControllerCallbacks::activeUniqueStatusEffectSummary(
    StatusController* statController) {
  auto summary = statController->activeUniqueStatusEffectSummary();
  return summary.transformed([](pair<UniqueStatusEffect, Maybe<float>> effect) {
    JsonArray effectJson = {effect.first};
    if (effect.second)
      effectJson.append(*effect.second);
    return effectJson;
  });
}

bool LuaBindings::StatusControllerCallbacks::uniqueStatusEffectActive(
    StatusController* statController, String const& effectName) {
  return statController->uniqueStatusEffectActive(effectName);
}

} // namespace Star
