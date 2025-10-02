#ifndef STAR_ARMORS_HPP
#define STAR_ARMORS_HPP

#include "StarEffectSourceItem.hpp"
#include "StarGameTypes.hpp"
#include "StarItem.hpp"
#include "StarPreviewableItem.hpp"
#include "StarStatusEffectItem.hpp"
#include "StarSwingableItem.hpp"
namespace Star {

STAR_CLASS(ArmorItem);
STAR_CLASS(HeadArmor);
STAR_CLASS(ChestArmor);
STAR_CLASS(LegsArmor);
STAR_CLASS(BackArmor);

class ArmorItem : public Item, public EffectSourceItem, public SwingableItem {
public:
  ArmorItem(Json const& config, String const& directory, Json const& data);

  // FezzedOne: For oSB's `"armorTypesToHide"`. On xSB, these cause items in the cosmetic slots to hide armour items even if underlay status is active.
  // oSB's behaviour where this also applies to overlays is emulated when an oSB client is detected.
  struct HiddenArmorTypes {
    bool head = false;
    bool chest = false;
    bool legs = false;
    bool back = false;
  };

  virtual ~ArmorItem() {}

  virtual List<PersistentStatusEffect> statusEffects() const override;
  virtual StringSet effectSources() const override;

  virtual List<Drawable> drawables() const override;

  virtual float getAngle(float aimAngle) override;

  virtual void fire(FireMode mode, bool shifting, bool edgeTriggered) override;
  virtual void fireTriggered() override;

  List<String> const& colorOptions();

  Directives const& directives() const;
  Directives const& flippedDirectives() const;

  bool hideBody() const;
  bool isUnderlaid() const;

  bool hideInStockSlots() const;
  bool bypassNudity() const;
  HiddenArmorTypes armorTypesToHide() const;
  void setHideInStockSlots(bool hide);

  void setUnderlaid(bool underlaid);

  List<ItemPtr> getStackedCosmetics() const;
  void setStackedCosmetics(List<ItemPtr> const& newStack);
  ItemPtr popCosmetic();
  void pushCosmetic(ItemPtr const& newItem);
  size_t cosmeticStackCount();

  Maybe<String> const& techModule() const;

protected:
  void refreshIconDrawables();
  void refreshStatusEffects();

  List<String> m_colorOptions;
  List<PersistentStatusEffect> m_statusEffects;
  StringSet m_effectSources;
  Directives m_directives;
  Maybe<Directives> m_flipDirectives;
  bool m_hideBody;
  bool m_underlaid;
  // FezzedOne: For oSB's `"hideInVanillaSlots"`. On xSB, this makes the item visible only when used as an overlay.
  bool m_hideInStockSlots;
  bool m_bypassNudity;
  bool m_genderOverride;
  bool m_femaleOverride;
  // FezzedOne: For oSB's `"armorTypesToHide"`.
  HiddenArmorTypes m_armorTypesToHide;
  Maybe<String> m_techModule;
  List<ItemPtr> m_stackedCosmetics;
};

class HeadArmor : public ArmorItem, public PreviewableItem {
public:
  HeadArmor(Json const& config, String const& directory, Json const& data);
  virtual ~HeadArmor() {}

  virtual ItemPtr clone() const;

  String const& frameset(Gender gender) const;
  Directives const& maskDirectives() const;

  virtual List<Drawable> preview(PlayerPtr const& viewer = {}) const;

private:
  String m_maleImage;
  String m_femaleImage;
  Directives m_maskDirectives;
};

class ChestArmor : public ArmorItem, public PreviewableItem {
public:
  ChestArmor(Json const& config, String const& directory, Json const& data);
  virtual ~ChestArmor() {}

  virtual ItemPtr clone() const;

  // Will have :run, :normal, :duck, and :portrait
  String const& bodyFrameset(Gender gender) const;
  // Will have :idle[1-5], :duck, :rotation, :walk[1-5], :run[1-5], :jump[1-4],
  // :fall[1-4]
  String const& frontSleeveFrameset(Gender gender) const;
  // Same as FSleeve
  String const& backSleeveFrameset(Gender gender) const;

  virtual List<Drawable> preview(PlayerPtr const& viewer = {}) const;

private:
  String m_maleBodyImage;
  String m_maleFrontSleeveImage;
  String m_maleBackSleeveImage;

  String m_femaleBodyImage;
  String m_femaleFrontSleeveImage;
  String m_femaleBackSleeveImage;
};

class LegsArmor : public ArmorItem, public PreviewableItem {
public:
  LegsArmor(Json const& config, String const& directory, Json const& data);
  virtual ~LegsArmor() {}

  virtual ItemPtr clone() const;

  // Will have :idle, :duck, :walk[1-8], :run[1-8], :jump[1-4], :fall[1-4]
  String const& frameset(Gender gender) const;

  virtual List<Drawable> preview(PlayerPtr const& viewer = {}) const;

private:
  String m_maleImage;
  String m_femaleImage;
};

class BackArmor : public ArmorItem, public PreviewableItem {
public:
  BackArmor(Json const& config, String const& directory, Json const& data);
  virtual ~BackArmor() {}

  virtual ItemPtr clone() const;

  // Will have :idle, :duck, :walk[1-8], :run[1-8], :jump[1-4], :fall[1-4]
  String const& frameset(Gender gender) const;

  virtual List<Drawable> preview(PlayerPtr const& viewer = {}) const;
  bool rotateWithHead() const;

private:
  String m_maleImage;
  String m_femaleImage;
  bool m_rotateWithHead;
};

} // namespace Star

#endif
