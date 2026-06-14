#ifndef STAR_INVENTORY_HPP
#define STAR_INVENTORY_HPP

#include "StarContainerInteractor.hpp"
#include "StarGameTimers.hpp"
#include "StarInventoryTypes.hpp"
#include "StarItemDescriptor.hpp"
#include "StarPane.hpp"
#include "StarPlayerTech.hpp"

namespace Star {

STAR_CLASS(MainInterface);
STAR_CLASS(UniverseClient);
STAR_CLASS(Player);
STAR_CLASS(Item);
STAR_CLASS(ItemSlotWidget);
STAR_CLASS(ItemGridWidget);
STAR_CLASS(ImageWidget);
STAR_CLASS(Widget);
STAR_CLASS(InventoryPane);
STAR_CLASS(WardrobePane);

// FezzedOne: Mapping for the wardrobe slots to the internal cosmetic slots.
const EnumMap<EquipmentSlot> WardrobeSlotNames{
    // The four retail cosmetic slots.
    {EquipmentSlot::HeadCosmetic, "headCosmetic"},
    {EquipmentSlot::ChestCosmetic, "chestCosmetic"},
    {EquipmentSlot::LegsCosmetic, "legsCosmetic"},
    {EquipmentSlot::BackCosmetic, "backCosmetic"},
    // The first twelve extra cosmetic slots are visible to OpenStarbound clients.
    // Chest and legs tems in the first four slots are layered *below* the retail chest item(s).
    {EquipmentSlot::Overlay1, "legsCosmeticLayer1"},
    {EquipmentSlot::Overlay2, "legsCosmeticLayer2"},
    {EquipmentSlot::Overlay3, "legsCosmeticLayer3"},
    {EquipmentSlot::Overlay4, "legsCosmeticLayer4"},
    // Chest and legs items in the next eight slots are layered *above* the retail chest item(s).
    {EquipmentSlot::Overlay5, "chestCosmeticLayer1"},
    {EquipmentSlot::Overlay6, "chestCosmeticLayer2"},
    {EquipmentSlot::Overlay7, "chestCosmeticLayer3"},
    {EquipmentSlot::Overlay8, "chestCosmeticLayer4"},
    {EquipmentSlot::Overlay9, "headCosmeticLayer1"},
    {EquipmentSlot::Overlay10, "headCosmeticLayer2"},
    {EquipmentSlot::Overlay11, "headCosmeticLayer3"},
    {EquipmentSlot::Overlay12, "headCosmeticLayer4"},
    // The last four slots are only visible to xStarbound clients.
    {EquipmentSlot::Overlay13, "backCosmeticLayer1"},
    {EquipmentSlot::Overlay14, "backCosmeticLayer2"},
    {EquipmentSlot::Overlay15, "backCosmeticLayer3"},
    {EquipmentSlot::Overlay16, "backCosmeticLayer4"},
};

class InventoryPane : public Pane {
public:
  InventoryPane(MainInterface* parent, PlayerPtr player, ContainerInteractorPtr containerInteractor);

  void displayed() override;
  PanePtr createTooltip(Vec2I const& screenPosition) override;

  bool giveContainerResult(ContainerResult result);

  // update only item grids, to see if they have had their slots changed
  // this is a little hacky and should probably be checked in the player inventory instead
  void updateItems();
  bool containsNewItems() const;
  void clearChangedSlots();

  bool m_expectingSwap;

protected:
  virtual void update(float dt) override;
  void selectTab(String const& selected);

private:
  MainInterface* m_parent;
  PlayerPtr m_player;
  ContainerInteractorPtr m_containerInteractor;

  InventorySlot m_containerSource;

  GameTimer m_trashBurn;
  ItemSlotWidgetPtr m_trashSlot;

  Map<String, ItemGridWidgetPtr> m_itemGrids;
  Map<String, String> m_tabButtonData;

  Map<String, WidgetPtr> m_newItemMarkers;
  String m_selectedTab;

  StringList m_pickUpSounds;
  StringList m_putDownSounds;
  StringList m_someUpSounds;
  StringList m_someDownSounds;
  Maybe<ItemDescriptor> m_currentSwapSlotItem;

  List<ImageWidgetPtr> m_disabledTechOverlays;

  bool m_anyBagAllowed;
};

class WardrobePane : public Pane {
public:
  WardrobePane(MainInterface* parent, PlayerPtr player, ContainerInteractorPtr containerInteractor);

  PanePtr createTooltip(Vec2I const& screenPosition) override;

protected:
  virtual void update(float dt) override;

private:
  MainInterface* m_parent;
  PlayerPtr m_player;
  ContainerInteractorPtr m_containerInteractor;
};

} // namespace Star

#endif
