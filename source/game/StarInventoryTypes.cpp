#include "StarInventoryTypes.hpp"
#include "StarFormat.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

EnumMap<EquipmentSlot> const EquipmentSlotNames{
    {EquipmentSlot::Head, "head"},
    {EquipmentSlot::Chest, "chest"},
    {EquipmentSlot::Legs, "legs"},
    {EquipmentSlot::Back, "back"},
    {EquipmentSlot::HeadCosmetic, "headCosmetic"},
    {EquipmentSlot::ChestCosmetic, "chestCosmetic"},
    {EquipmentSlot::LegsCosmetic, "legsCosmetic"},
    {EquipmentSlot::BackCosmetic, "backCosmetic"},
    {EquipmentSlot::Overlay1, "cosmetic1"},
    {EquipmentSlot::Overlay2, "cosmetic2"},
    {EquipmentSlot::Overlay3, "cosmetic3"},
    {EquipmentSlot::Overlay4, "cosmetic4"},
    {EquipmentSlot::Overlay5, "cosmetic5"},
    {EquipmentSlot::Overlay6, "cosmetic6"},
    {EquipmentSlot::Overlay7, "cosmetic7"},
    {EquipmentSlot::Overlay8, "cosmetic8"},
    {EquipmentSlot::Overlay9, "cosmetic9"},
    {EquipmentSlot::Overlay10, "cosmetic10"},
    {EquipmentSlot::Overlay11, "cosmetic11"},
    {EquipmentSlot::Overlay12, "cosmetic12"},
    {EquipmentSlot::Overlay13, "cosmetic13"},
    {EquipmentSlot::Overlay14, "cosmetic14"},
    {EquipmentSlot::Overlay15, "cosmetic15"},
    {EquipmentSlot::Overlay16, "cosmetic16"},
};

InventorySlot jsonToInventorySlot(Json const& json) {
  String type = json.getString("type");
  Json location = json.get("location", Json());
  if (type == "equipment")
    return EquipmentSlotNames.getLeft(location.toString());
  else if (type == "swap")
    return SwapSlot();
  else if (type == "trash")
    return TrashSlot();
  else
    return BagSlot(type, location.toUInt());
}

Json jsonFromInventorySlot(InventorySlot const& slot) {
  if (slot.is<EquipmentSlot>()) {
    return JsonObject{
        {"type", "equipment"},
        {"location", EquipmentSlotNames.getRight(slot.get<EquipmentSlot>())}};
  } else if (slot.is<SwapSlot>()) {
    return JsonObject{{"type", "swap"}};
  } else if (slot.is<TrashSlot>()) {
    return JsonObject{{"type", "trash"}};
  } else {
    auto bagSlot = slot.get<BagSlot>();
    return JsonObject{
        {"type", bagSlot.first},
        {"location", bagSlot.second}};
  }
}

std::ostream& operator<<(std::ostream& ostream, InventorySlot const& slot) {
  Json json = jsonFromInventorySlot(slot);

  String type = json.getString("type");
  Json location = json.get("location", {});

  if (location.isNull())
    format(ostream, "InventorySlot{type: {}}", type);
  if (location.isType(Json::Type::String))
    format(ostream, "InventorySlot{type: {}, location: {}}", type, location.toString());
  else
    format(ostream, "InventorySlot{type: {}, location: {}}", type, location.toInt());

  return ostream;
}

EnumMap<EssentialItem> const EssentialItemNames{
    {EssentialItem::BeamAxe, "beamaxe"},
    {EssentialItem::WireTool, "wiretool"},
    {EssentialItem::PaintTool, "painttool"},
    {EssentialItem::InspectionTool, "inspectiontool"}};

SelectedActionBarLocation jsonToSelectedActionBarLocation(Json const& json) {
  if (json.isType(Json::Type::String))
    return EssentialItemNames.getLeft(json.toString());
  else if (json.isNull())
    return SelectedActionBarLocation();
  else
    return (CustomBarIndex)json.toUInt();
}

Json jsonFromSelectedActionBarLocation(SelectedActionBarLocation const& location) {
  if (location.is<CustomBarIndex>())
    return location.get<CustomBarIndex>();
  else if (location.is<EssentialItem>())
    return EssentialItemNames.getRight(location.get<EssentialItem>());
  else
    return Json();
}

} // namespace Star
