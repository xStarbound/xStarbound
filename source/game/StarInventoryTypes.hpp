#ifndef STAR_INVENTORY_TYPES_HPP
#define STAR_INVENTORY_TYPES_HPP

#include "StarBiMap.hpp"
#include "StarJson.hpp"
#include "StarStrongTypedef.hpp"

namespace Star {

// FezzedOne: Twelve extra overlay slots to take after OpenStarbound's.
enum class EquipmentSlot : uint8_t {
  Head = 0,
  Chest = 1,
  Legs = 2,
  Back = 3,
  HeadCosmetic = 4,
  ChestCosmetic = 5,
  LegsCosmetic = 6,
  BackCosmetic = 7,
  Overlay1 = 8,
  Overlay2 = 9,
  Overlay3 = 10,
  Overlay4 = 11,
  Overlay5 = 12,
  Overlay6 = 13,
  Overlay7 = 14,
  Overlay8 = 15,
  Overlay9 = 16,
  Overlay10 = 17,
  Overlay11 = 18,
  Overlay12 = 19,
  Overlay13 = 20,
  Overlay14 = 21,
  Overlay15 = 22,
  Overlay16 = 23
};

extern EnumMap<EquipmentSlot> const EquipmentSlotNames;

typedef pair<String, size_t> BagSlot;
typedef pair<String, uint8_t> BagSlotCompat;

strong_typedef(Empty, SwapSlot);
strong_typedef(Empty, TrashSlot);

// Any manageable location in the player inventory can be pointed to by an
// InventorySlot
typedef Variant<EquipmentSlot, BagSlot, SwapSlot, TrashSlot> InventorySlot;
// FezzedOne: Network compatibility shim.
typedef Variant<EquipmentSlot, BagSlotCompat, SwapSlot, TrashSlot> InventorySlotCompat;

InventorySlot jsonToInventorySlot(Json const& json);
Json jsonFromInventorySlot(InventorySlot const& slot);

std::ostream& operator<<(std::ostream& ostream, InventorySlot const& slot);

// Special items in the player inventory that are not generally manageable
enum class EssentialItem : uint8_t {
  BeamAxe = 0,
  WireTool = 1,
  PaintTool = 2,
  InspectionTool = 3
};
extern EnumMap<EssentialItem> const EssentialItemNames;

// A player's action bar is a collection of custom item shortcuts, and special
// hard coded shortcuts to the essential items.  There is one location selected
// at a time, which is either an entry on the custom bar, or one of the
// essential items, or nothing.
typedef uint8_t CustomBarIndex;
typedef MVariant<CustomBarIndex, EssentialItem> SelectedActionBarLocation;

SelectedActionBarLocation jsonToSelectedActionBarLocation(Json const& json);
Json jsonFromSelectedActionBarLocation(SelectedActionBarLocation const& location);

static uint8_t const EquipmentSize = 8;
static uint8_t const EssentialItemCount = 4;

} // namespace Star

template <>
struct fmt::formatter<Star::InventorySlot> : ostream_formatter {};

#endif
