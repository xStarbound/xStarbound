#include "StarFireableItemLuaBindings.hpp"
#include "StarCasting.hpp"
#include "StarFireableItem.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeFireableItemCallbacks(FireableItem* fireableItemPtr) {
  LuaCallbacks callbacks;

  auto fireableItem = GameObjectRegistry::smuggleWrap(fireableItemPtr);

  callbacks.registerCallbackWithSignature<void, Maybe<String>>("fire", LUA_BIND(FireableItemCallbacks::fire, fireableItem, _1));
  callbacks.registerCallbackWithSignature<void>("triggerCooldown", LUA_BIND(FireableItemCallbacks::triggerCooldown, fireableItem));
  callbacks.registerCallbackWithSignature<void, float>("setCooldown", LUA_BIND(FireableItemCallbacks::setCooldown, fireableItem, _1));
  callbacks.registerCallbackWithSignature<void>("endCooldown", LUA_BIND(FireableItemCallbacks::endCooldown, fireableItem));
  callbacks.registerCallbackWithSignature<float>("cooldownTime", LUA_BIND(FireableItemCallbacks::cooldownTime, fireableItem));
  callbacks.registerCallbackWithSignature<Json, String, Json>("fireableParam", LUA_BIND(FireableItemCallbacks::fireableParam, fireableItem, _1, _2));
  callbacks.registerCallbackWithSignature<String>("fireMode", LUA_BIND(FireableItemCallbacks::fireMode, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("ready", LUA_BIND(FireableItemCallbacks::ready, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("firing", LUA_BIND(FireableItemCallbacks::firing, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("windingUp", LUA_BIND(FireableItemCallbacks::windingUp, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("coolingDown", LUA_BIND(FireableItemCallbacks::coolingDown, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("ownerFullEnergy", LUA_BIND(FireableItemCallbacks::ownerFullEnergy, fireableItem));
  callbacks.registerCallbackWithSignature<float>("ownerEnergy", LUA_BIND(FireableItemCallbacks::ownerEnergy, fireableItem));
  callbacks.registerCallbackWithSignature<bool>("ownerEnergyLocked", LUA_BIND(FireableItemCallbacks::ownerEnergyLocked, fireableItem));
  callbacks.registerCallbackWithSignature<bool, float>("ownerConsumeEnergy", LUA_BIND(FireableItemCallbacks::ownerConsumeEnergy, fireableItem, _1));
  callbacks.registerCallbackWithSignature<Vec2F>("ownerAimPosition", [fireableItem]() {
    return fireableItem->owner()->aimPosition();
  });

  return callbacks;
}

// Triggers the item to fire
//
// @param[opt] fireMode fireMode to trigger; should be 'Primary' or 'Alt'
// (defaults to Primary)
// @return nil
void LuaBindings::FireableItemCallbacks::fire(FireableItem* fireableItem, Maybe<String> const& mode) {
  auto fireMode = FireMode::Primary;
  if (mode) {
    if (mode->equalsIgnoreCase("Alt"))
      fireMode = FireMode::Alt;
    else if (!mode->equalsIgnoreCase("Primary"))
      throw StarException("Invalid fire mode specified! Must be 'Primary' or 'Alt'");
  }

  if (fireableItem->ready())
    fireableItem->fire(fireMode, false, true);
}

// Triggers the item's cooldown
//
// @return nil
void LuaBindings::FireableItemCallbacks::triggerCooldown(FireableItem* fireableItem) {
  fireableItem->triggerCooldown();
}

// Sets the item's current cooldown to the specified time (will not change the
// default cooldown)
//
// @param cooldownTime time in seconds for this cooldown period
// @return nil
void LuaBindings::FireableItemCallbacks::setCooldown(FireableItem* fireableItem, float cooldownTime) {
  fireableItem->setCoolingDown(cooldownTime > 0);
  fireableItem->setFireTimer(cooldownTime);
}

// Ends the item's cooldown, readying it to fire
//
// @return nil
void LuaBindings::FireableItemCallbacks::endCooldown(FireableItem* fireableItem) {
  fireableItem->setCoolingDown(false);
  fireableItem->setFireTimer(0);
}

// Returns the item's current cooldown time
//
// @return the current cooldown time in seconds
float LuaBindings::FireableItemCallbacks::cooldownTime(FireableItem* fireableItem) {
  return fireableItem->cooldownTime();
}

// Gets the value of a configuration option for this item.
//
// @string name the name of the configuration parameter to get, as specified in
// the
// item's configuration
// @param[opt] default a default value that will be returned if the given
// configuration key does not exist in the item's configuration.
// @return[1] nil if the item has no configuration option with the given name,
// and no default value is given.
// @return[2] the value of the configuration option, or the given default value.
Json LuaBindings::FireableItemCallbacks::fireableParam(
    FireableItem* fireableItem, String const& name, Json const& def) {
  return fireableItem->fireableParam(name, def);
}

// Gets the current fire mode of the item
//
// @return string representation of the fire mode: 'Primary', 'Alt' or 'None'
String LuaBindings::FireableItemCallbacks::fireMode(FireableItem* fireableItem) {
  if (fireableItem->fireMode() == FireMode::Primary)
    return "Primary";
  else if (fireableItem->fireMode() == FireMode::Alt)
    return "Alt";
  else
    return "None";
}

// Determine whether the item is currently ready to be fired
//
// @return true if the item is ready
bool LuaBindings::FireableItemCallbacks::ready(FireableItem* fireableItem) {
  return fireableItem->ready();
}

// Determine whether the item is currently firing
//
// @return true if the item is firing
bool LuaBindings::FireableItemCallbacks::firing(FireableItem* fireableItem) {
  return fireableItem->firing();
}

// Determine whether the item is currently winding up to fire
//
// @return true if the item is winding up
bool LuaBindings::FireableItemCallbacks::windingUp(FireableItem* fireableItem) {
  return fireableItem->windup();
}

// Determine whether the item is currently cooling down from firing
//
// @return true if the item is cooling down
bool LuaBindings::FireableItemCallbacks::coolingDown(FireableItem* fireableItem) {
  return fireableItem->coolingDown();
}

// Determine whether the item's owner has full energy
//
// @return true if the owner's energy is full
bool LuaBindings::FireableItemCallbacks::ownerFullEnergy(FireableItem* fireableItem) {
  return fireableItem->owner()->fullEnergy();
}

// Determine the amount of energy that the item's owner currently has
//
// @return the owner's current energy
float LuaBindings::FireableItemCallbacks::ownerEnergy(FireableItem* fireableItem) {
  return fireableItem->owner()->energy();
}

// Determine whether the item's owner's energy pool is currently locked
//
// @return true if the owner's energy pool is currently locked
bool LuaBindings::FireableItemCallbacks::ownerEnergyLocked(FireableItem* fireableItem) {
  return fireableItem->owner()->energyLocked();
}

// Attempt to consume the specified amount of the owner's energy
//
// @param amount the amount of energy to consume
// @return true if the energy was consumed successfully
bool LuaBindings::FireableItemCallbacks::ownerConsumeEnergy(FireableItem* fireableItem, float energy) {
  return fireableItem->owner()->consumeEnergy(energy);
}

} // namespace Star
