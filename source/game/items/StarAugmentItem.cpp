#include "StarAugmentItem.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarItemDatabase.hpp"
#include "StarLuaComponents.hpp"
#include "StarItemLuaBindings.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

AugmentItem::AugmentItem(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters), SwingableItem(config) {}

AugmentItem::AugmentItem(AugmentItem const& rhs) : AugmentItem(rhs.config(), rhs.directory(), rhs.parameters()) {}

ItemPtr AugmentItem::clone() const {
  return make_shared<AugmentItem>(*this);
}

List<Drawable> AugmentItem::drawables() const {
  auto drawables = iconDrawables();
  Drawable::scaleAll(drawables, 1.0f / TilePixels);
  Drawable::translateAll(drawables, -handPosition() / TilePixels);
  return drawables;
}

void AugmentItem::fire(FireMode mode, bool shifting, bool edgeTriggered) {}
void AugmentItem::fireTriggered() {}

StringList AugmentItem::augmentScripts() const {
  return jsonToStringList(instanceValue("scripts")).transformed(bind(&AssetPath::relativeTo, directory(), _1));
}

ItemPtr AugmentItem::applyTo(ItemPtr const item) {
  return Root::singleton().itemDatabase()->applyAugment(item, this);
}

}
