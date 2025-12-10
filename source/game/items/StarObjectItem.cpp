#include "StarObjectItem.hpp"
#include "StarDrawable.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarObject.hpp"
#include "StarObjectDatabase.hpp"
#include "StarRoot.hpp"
#include "StarWorld.hpp"

namespace Star {

ObjectItem::ObjectItem(Json const& config, String const& directory, Json const& objectParameters)
    : Item(config, directory, objectParameters), FireableItem(config), BeamItem(config) {
  setTwoHanded(config.getBool("twoHanded", false));

  // Make sure that all script objects that have retainObjectParametersInItem
  // start with a blank scriptStorage entry to help them stack properly.
  if (instanceValue("retainObjectParametersInItem", false).toBool() && instanceValue("scriptStorage").isNull())
    setInstanceValue("scriptStorage", JsonObject());

  m_shifting = false;
}

ItemPtr ObjectItem::clone() const {
  return make_shared<ObjectItem>(*this);
}

void ObjectItem::init(ToolUserEntity* owner, ToolHand hand) {
  FireableItem::init(owner, hand);
  auto drawables = iconDrawables();
  Drawable::scaleAll(drawables, 1.0f / TilePixels);
  setNotBeamaxe(true, drawables);
  BeamItem::init(owner, hand);
}

void ObjectItem::update(float dt, FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) {
  FireableItem::update(dt, fireMode, shifting, moves);
  BeamItem::update(dt, fireMode, shifting, moves);
  setEnd(BeamItem::EndType::Object);
  m_shifting = shifting;
}

List<Drawable> ObjectItem::nonRotatedDrawables() const {
  return beamDrawables(canPlace(m_shifting));
}

float ObjectItem::cooldownTime() const {
  // TODO: Hardcoded
  return 0.25f;
}

void ObjectItem::fire(FireMode mode, bool shifting, bool edgeTriggered) {
  if (!ready())
    return;

  if (placeInWorld(mode, shifting))
    FireableItem::fire(mode, shifting, edgeTriggered);
}

String ObjectItem::objectName() const {
  return instanceValue("objectName", "<objectName missing>").toString();
}

Json ObjectItem::objectParameters() const {
  Json objectParameters = parameters().opt().value(JsonObject{});
  if (!initialized())
    return objectParameters;
  return objectParameters.set("owner", jsonFromMaybe(owner()->uniqueId()));
}

bool ObjectItem::placeInWorld(FireMode, bool shifting) {
  if (!initialized())
    throw ItemException("ObjectItem not init'd properly, or user not recognized as Tool User.");

  if (!ready())
    return false;

  if (!canPlace(shifting))
    return false;

  auto pos = Vec2I(owner()->aimPosition().floor());
  auto objectDatabase = Root::singleton().objectDatabase();
  try {
    const auto jBypassChecks = owner()->inWorld() ? world()->getProperty("bypassBuildChecks", false) : false;
    const bool bypassChecks = jBypassChecks.isType(Json::Type::Bool) ? jBypassChecks.toBool() : false;
    Json params = objectParameters();
    if (bypassChecks && objectName() == "sapling") {
      params = params.set("stages", JsonArray{
                                        JsonObject{{"duration", JsonArray{0.0, 0.0}}},
                                        JsonObject{{"duration", JsonArray{0.0, 0.0}}},
                                        JsonObject{{"tree", true}}});
    }
    if (auto object = objectDatabase->createForPlacement(world(), objectName(), pos, owner()->walkingDirection(), params, bypassChecks)) {
      if (bypassChecks || consume(1)) {
        world()->addEntity(object);
        return true;
      }
    }
  } catch (StarException const& e) {
    Logger::error("Failed to instantiate object for placement. {} {} : {}",
        objectName(),
        objectParameters().repr(0, true),
        outputException(e, true));
    return true;
  }

  return false;
}

bool ObjectItem::canPlace(bool) const {
  if (initialized()) {
    if (owner()->isAdmin() || owner()->inToolRange()) {
      const auto jBypassChecks = owner()->inWorld() ? world()->getProperty("bypassBuildChecks", false) : false;
      const bool bypassChecks = jBypassChecks.isType(Json::Type::Bool) ? jBypassChecks.toBool() : false;
      if (bypassChecks) return true;

      auto pos = Vec2I(owner()->aimPosition().floor());
      auto objectDatabase = Root::singleton().objectDatabase();
      return objectDatabase->canPlaceObject(world(), pos, objectName());
    }
  }
  return false;
}

} // namespace Star
