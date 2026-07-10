#ifndef STAR_BLUEPRINT_ITEM_HPP
#define STAR_BLUEPRINT_ITEM_HPP

#include "StarItem.hpp"
#include "StarSwingableItem.hpp"
#include "StarWorld.hpp"

namespace Star {

STAR_CLASS(BlueprintItem);

class BlueprintItem : public Item, public SwingableItem {
public:
  BlueprintItem(Json const& config, String const& directory, Json const& data);
  virtual ItemPtr clone() const override;

  virtual List<Drawable> drawables() const override;

  virtual void fireTriggered() override;

  virtual List<Drawable> iconDrawables() const override;
  virtual List<Drawable> dropDrawables() const override;
  virtual float getAngle(float) override;

private:
  ItemDescriptor m_recipe;
  Drawable m_recipeIconUnderlay;
  List<Drawable> m_inHandDrawable;
};

} // namespace Star

#endif
