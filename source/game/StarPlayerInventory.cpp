#include "StarPlayerInventory.hpp"
#include "StarArmors.hpp"
#include "StarAssets.hpp"
#include "StarCurrency.hpp"
#include "StarItem.hpp"
#include "StarItemBag.hpp"
#include "StarItemDatabase.hpp"
#include "StarJsonExtra.hpp"
#include "StarLiquidItem.hpp"
#include "StarMaterialItem.hpp"
#include "StarObjectItem.hpp"
#include "StarPlayer.hpp"
#include "StarPointableItem.hpp"
#include "StarRoot.hpp"

namespace Star {

#define InventorySetting(setting) ((bool)(m_inventorySettings & InventorySettings::setting))

bool PlayerInventory::itemAllowedInBag(ItemPtr const& items, String const& bagType) {
  // any inventory type can have empty slots
  if (!items)
    return true;

  return checkInventoryFilter(items, bagType);
}

bool PlayerInventory::itemAllowedAsEquipment(ItemPtr const& item, EquipmentSlot equipmentSlot) {
  // any equipment slot can be empty
  if (!item)
    return true;

  if (equipmentSlot == EquipmentSlot::Head || equipmentSlot == EquipmentSlot::HeadCosmetic)
    return is<HeadArmor>(item);
  else if (equipmentSlot == EquipmentSlot::Chest || equipmentSlot == EquipmentSlot::ChestCosmetic)
    return is<ChestArmor>(item);
  else if (equipmentSlot == EquipmentSlot::Legs || equipmentSlot == EquipmentSlot::LegsCosmetic)
    return is<LegsArmor>(item);
  else
    return is<BackArmor>(item);
}

PlayerInventory::PlayerInventory(Player* player) : m_player(player) {
  // To avoid client-side lag spikes on very large modpacks, persistently preload these infrequently accessed configs.
  auto config = Root::singleton().assets()->json("/player.config:inventory", true);
  auto currenciesConfig = Root::singleton().assets()->json("/currencies.config", true);
  volatile Json _ = Root::singleton().assets()->json("/player.config:inventoryFilters", true);
  (void)_;

  m_inventorySettings = InventorySettings::Default;
  if (config.optBool("allowAnyBagItem").value(false))
    m_inventorySettings |= InventorySettings::AllowAnyItemInBags;
  if (config.optBool("disableBagPreferences").value(false))
    m_inventorySettings |= InventorySettings::NoBagPreferences;
  if (config.optBool("autoAddToCosmetics").value(false))
    m_inventorySettings |= InventorySettings::AddToCosmetics;

  auto bags = config.get("itemBags");
  auto bagOrder = bags.toObject().keys().sorted([&bags](String const& a, String const& b) {
    return bags.get(a).getInt("priority", 0) < bags.get(b).getInt("priority", 0);
  });
  for (auto name : bagOrder) {
    size_t size = bags.get(name).getUInt("size");
    m_bags[name] = make_shared<ItemBag>(size);
  }
  auto networkedBags = config.get("networkedItemBags");
  auto networkedBagOrder = networkedBags.toObject().keys().sorted([&networkedBags](String const& a, String const& b) {
    return networkedBags.get(a).getInt("priority", 0) < networkedBags.get(b).getInt("priority", 0);
  });
  for (auto name : networkedBagOrder) {
    size_t size = networkedBags.get(name).getUInt("size");
    m_bagsNetState[name].resize(size);
  }

  for (auto p : currenciesConfig.iterateObject())
    m_currencies[p.first] = 0;

  size_t customBarGroups = config.getUInt("customBarGroups");
  size_t customBarIndexes = config.getUInt("customBarIndexes");
  m_customBarGroup = 0;
  m_customBar.resize(customBarGroups, customBarIndexes);

  addNetElement(&m_equipmentNetState[EquipmentSlot::Head]);
  addNetElement(&m_equipmentNetState[EquipmentSlot::Chest]);
  addNetElement(&m_equipmentNetState[EquipmentSlot::Legs]);
  addNetElement(&m_equipmentNetState[EquipmentSlot::Back]);
  addNetElement(&m_equipmentNetState[EquipmentSlot::HeadCosmetic]);
  addNetElement(&m_equipmentNetState[EquipmentSlot::ChestCosmetic]);
  addNetElement(&m_equipmentNetState[EquipmentSlot::LegsCosmetic]);
  addNetElement(&m_equipmentNetState[EquipmentSlot::BackCosmetic]);

  for (auto& p : m_bagsNetState) {
    for (auto& e : p.second)
      addNetElement(&e);
  }

  addNetElement(&m_swapSlotNetState);
  addNetElement(&m_trashSlotNetState);

  addNetElement(&m_currenciesNetState);

  m_networkedCustomBarGroups = config.getUInt("networkedCustomBarGroups");
  m_networkedCustomBarIndexes = config.getUInt("networkedCustomBarIndexes");
  addNetElement(&m_customBarGroupNetState);
  m_customBarNetState.resize(m_networkedCustomBarGroups, m_networkedCustomBarIndexes);
  m_customBarNetState.forEach([this](Array2S const&, NetElementData<CustomBarLinkCompat>& e) {
    addNetElement(&e);
  });

  addNetElement(&m_selectedActionBarNetState);

  addNetElement(&m_essentialNetState[EssentialItem::BeamAxe]);
  addNetElement(&m_essentialNetState[EssentialItem::WireTool]);
  addNetElement(&m_essentialNetState[EssentialItem::PaintTool]);
  addNetElement(&m_essentialNetState[EssentialItem::InspectionTool]);
}

ItemPtr PlayerInventory::itemsAt(InventorySlot const& slot) const {
  return retrieve(slot);
}

ItemPtr PlayerInventory::stackWith(InventorySlot const& slot, ItemPtr const& items) {
  if (!items || items->empty())
    return {};

  if (auto es = slot.ptr<EquipmentSlot>()) {
    auto& itemSlot = retrieve(slot);
    if (!itemSlot && itemAllowedAsEquipment(items, *es))
      m_equipment[*es] = items->take(1);
  } else {
    auto& dest = retrieve(slot);
    if (dest && dest->stackableWith(items))
      dest->stackWith(items);
    if (!dest)
      dest = items->take(items->count());
  }

  if (items->empty())
    return {};

  return items;
}

ItemPtr PlayerInventory::takeSlot(InventorySlot const& slot) {
  if (slot.is<SwapSlot>())
    m_swapReturnSlot = {};
  auto& item = retrieve(slot);
  item->markTaken();
  return take(item);
}

bool PlayerInventory::exchangeItems(InventorySlot const& first, InventorySlot const& second) {
  auto& firstItems = retrieve(first);
  auto& secondItems = retrieve(second);

  auto itemAllowed = [&](ItemPtr const& item, String const& bagType) -> bool {
    if (InventorySetting(AllowAnyItemInBags))
      return true;
    return itemAllowedInBag(item, bagType);
  };

  if (first.is<BagSlot>() && !itemAllowed(secondItems, first.get<BagSlot>().first))
    return false;
  if (second.is<BagSlot>() && !itemAllowed(firstItems, second.get<BagSlot>().first))
    return false;
  if (first.is<EquipmentSlot>() && (secondItems->count() > 1 || !itemAllowedAsEquipment(secondItems, first.get<EquipmentSlot>())))
    return false;
  if (second.is<EquipmentSlot>() && (firstItems->count() > 1 || !itemAllowedAsEquipment(firstItems, second.get<EquipmentSlot>())))
    return false;

  swap(firstItems, secondItems);
  swapCustomBarLinks(first, second);

  return true;
}

bool PlayerInventory::setItem(InventorySlot const& slot, ItemPtr const& item) {
  auto itemAllowed = [&](ItemPtr const& item, String const& bagType) -> bool {
    if (InventorySetting(AllowAnyItemInBags))
      return true;
    return itemAllowedInBag(item, bagType);
  };

  if (auto currencyItem = as<CurrencyItem>(item)) {
    m_currencies[currencyItem->currencyType()] += currencyItem->totalValue();
    return true;
  } else if (auto es = slot.ptr<EquipmentSlot>()) {
    if (itemAllowedAsEquipment(item, *es)) {
      m_equipment[*es] = item;
      return true;
    }
  } else if (slot.is<SwapSlot>()) {
    m_swapSlot = item;
    return true;
  } else if (slot.is<TrashSlot>()) {
    m_trashSlot = item;
    return true;
  } else {
    auto bs = slot.get<BagSlot>();
    if (itemAllowed(item, bs.first)) {
      m_bags[bs.first]->setItem(bs.second, item);
      return true;
    }
  }

  return false;
}

bool PlayerInventory::consumeSlot(InventorySlot const& slot, uint64_t count) {
  if (count == 0)
    return true;

  auto& item = retrieve(slot);
  if (!item)
    return false;

  bool consumed = item->consume(count);
  if (item->empty())
    item = {};

  return consumed;
}

ItemPtr PlayerInventory::addItems(ItemPtr items) {
  if (!items || items->empty())
    return {};

  // First, add coins as monetary value.
  if (auto currencyItem = as<CurrencyItem>(items)) {
    addCurrency(currencyItem->currencyType(), currencyItem->totalValue());
    return {};
  }

  // Then, try adding equipment to the equipment slots.

  if (is<HeadArmor>(items) && !headArmor())
    m_equipment[EquipmentSlot::Head] = items->take(1);
  if (is<ChestArmor>(items) && !chestArmor())
    m_equipment[EquipmentSlot::Chest] = items->take(1);
  if (is<LegsArmor>(items) && !legsArmor())
    m_equipment[EquipmentSlot::Legs] = items->take(1);
  if (is<BackArmor>(items) && !backArmor())
    m_equipment[EquipmentSlot::Back] = items->take(1);

  if (InventorySetting(AddToCosmetics) && items) {
    if (!items->empty()) {
      if (is<HeadArmor>(items) && !headCosmetic())
        m_equipment[EquipmentSlot::HeadCosmetic] = items->take(1);
      if (is<ChestArmor>(items) && !chestCosmetic())
        m_equipment[EquipmentSlot::ChestCosmetic] = items->take(1);
      if (is<LegsArmor>(items) && !legsCosmetic())
        m_equipment[EquipmentSlot::LegsCosmetic] = items->take(1);
      if (is<BackArmor>(items) && !backCosmetic())
        m_equipment[EquipmentSlot::BackCosmetic] = items->take(1);
    }
  }

  // Then, finally the bags
  return addToBags(std::move(items));
}

ItemPtr PlayerInventory::addToBags(ItemPtr items) {
  if (!items || items->empty())
    return {};

  auto itemAllowed = [&](ItemPtr const& item, String const& bagType) -> bool {
    if (InventorySetting(AllowAnyItemInBags) && InventorySetting(NoBagPreferences))
      return true;
    return itemAllowedInBag(item, bagType);
  };

  for (auto pair : m_bags) {
    if (!itemAllowed(items, pair.first))
      continue;

    items = pair.second->stackItems(items);
    if (!items)
      break;

    for (size_t i = 0; i < pair.second->size(); ++i) {
      if (!pair.second->at(i)) {
        pair.second->setItem(i, take(items));
        autoAddToCustomBar(BagSlot(pair.first, i));
        break;
      }
    }
  }

  if (items && InventorySetting(AllowAnyItemInBags) && !InventorySetting(NoBagPreferences)) {
    for (auto pair : m_bags) {
      items = pair.second->stackItems(items);
      if (!items)
        break;

      for (size_t i = 0; i < pair.second->size(); ++i) {
        if (!pair.second->at(i)) {
          pair.second->setItem(i, take(items));
          autoAddToCustomBar(BagSlot(pair.first, i));
          break;
        }
      }
    }
  }

  return items;
}

uint64_t PlayerInventory::itemsCanFit(ItemPtr const& items) const {
  if (!items || items->empty())
    return 0;

  if (is<CurrencyItem>(items))
    return items->count();

  uint64_t canFit = 0;

  // First, check the equipment slots
  if (is<HeadArmor>(items) && !headArmor())
    ++canFit;
  if (is<ChestArmor>(items) && !chestArmor())
    ++canFit;
  if (is<LegsArmor>(items) && !legsArmor())
    ++canFit;
  if (is<BackArmor>(items) && !backArmor())
    ++canFit;

  // FezzedOne: Check cosmetic slots if automatic stacking into them is allowed.
  if (InventorySetting(AddToCosmetics)) {
    if (is<HeadArmor>(items) && !headCosmetic())
      ++canFit;
    if (is<ChestArmor>(items) && !chestCosmetic())
      ++canFit;
    if (is<LegsArmor>(items) && !legsCosmetic())
      ++canFit;
    if (is<BackArmor>(items) && !backCosmetic())
      ++canFit;
  }

  auto itemAllowed = [&](ItemPtr const& item, String const& bagType) -> bool {
    if (InventorySetting(AllowAnyItemInBags))
      return true;
    return itemAllowedInBag(item, bagType);
  };

  // Then add into bags
  for (auto const& pair : m_bags) {
    if (itemAllowed(items, pair.first))
      canFit += pair.second->itemsCanFit(items);
  }

  return min(canFit, items->count());
}

bool PlayerInventory::hasItem(ItemDescriptor const& descriptor, bool exactMatch) const {
  return hasCountOfItem(descriptor, exactMatch) >= descriptor.count();
}

uint64_t PlayerInventory::hasCountOfItem(ItemDescriptor const& descriptor, bool exactMatch) const {
  auto one = descriptor.singular();

  uint64_t count = 0;
  auto countItem = [&](ItemPtr const& ptr) {
    if (ptr)
      count += ptr->matches(one, exactMatch) ? ptr->count() : 0;
  };

  countItem(m_swapSlot);
  countItem(m_trashSlot);
  for (auto const& p : m_equipment)
    countItem(p.second);

  for (auto const& pair : m_bags)
    count += pair.second->available(one, exactMatch);

  return count;
}

bool PlayerInventory::consumeItems(ItemDescriptor const& descriptor, bool exactMatch) {
  if (descriptor.count() == 0)
    return true;

  auto one = descriptor.singular();

  Map<String, uint64_t> consumeFromItemBags;
  for (auto pair : m_bags)
    consumeFromItemBags[pair.first] = pair.second->available(one);

  uint64_t consumeFromEquipment = 0;
  for (auto const& p : m_equipment) {
    if (p.second)
      consumeFromEquipment += p.second->matches(one, exactMatch) ? p.second->count() : 0;
  }

  uint64_t consumeFromSwap = 0;
  if (m_swapSlot)
    consumeFromSwap += m_swapSlot->matches(one, exactMatch) ? m_swapSlot->count() : 0;

  uint64_t consumeFromTrash = 0;
  if (m_trashSlot)
    consumeFromTrash += m_trashSlot->matches(one, exactMatch) ? m_trashSlot->count() : 0;

  auto totalAvailable = consumeFromEquipment + consumeFromSwap + consumeFromTrash;
  for (auto pair : consumeFromItemBags)
    totalAvailable += pair.second;

  if (totalAvailable < descriptor.count())
    return false;

  uint64_t leftoverCount = descriptor.count();
  uint64_t quantity;
  for (auto pair : m_bags) {
    quantity = min(leftoverCount, consumeFromItemBags[pair.first]);
    if (quantity > 0) {
      auto res = pair.second->consumeItems(one.multiply(quantity), exactMatch);
      _unused(res);
      starAssert(res);
      leftoverCount -= quantity;
    }
  }

  quantity = min(leftoverCount, consumeFromEquipment);
  if (quantity > 0) {
    auto leftoverQuantity = quantity;
    for (auto const& p : m_equipment) {
      if (p.second && p.second->matches(one, exactMatch)) {
        auto toConsume = min(p.second->count(), quantity);
        auto res = p.second->consume(toConsume);
        _unused(res);
        starAssert(res);

        leftoverQuantity -= toConsume;
      }
    }
    starAssert(leftoverQuantity == 0);
    leftoverCount -= quantity;
  }

  quantity = std::min(leftoverCount, consumeFromSwap);
  if (quantity > 0) {
    if (m_swapSlot && m_swapSlot->matches(one, exactMatch)) {
      auto toConsume = std::min(m_swapSlot->count(), quantity);
      auto res = m_swapSlot->consume(toConsume);
      _unused(res);
      starAssert(res);

      quantity -= toConsume;
      starAssert(quantity == 0);
    }
    leftoverCount -= std::min(leftoverCount, consumeFromSwap);
  }

  quantity = std::min(leftoverCount, consumeFromTrash);
  if (quantity > 0) {
    if (m_trashSlot && m_trashSlot->matches(one, exactMatch)) {
      auto toConsume = std::min(m_trashSlot->count(), quantity);
      auto res = m_trashSlot->consume(toConsume);
      _unused(res);
      starAssert(res);

      quantity -= toConsume;
      starAssert(quantity == 0);
    }
    leftoverCount -= std::min(leftoverCount, consumeFromTrash);
  }

  starAssert(leftoverCount == 0);
  return true;
}

ItemDescriptor PlayerInventory::takeItems(ItemDescriptor const& descriptor, bool takePartial, bool exactMatch) {
  uint64_t hasCount = hasCountOfItem(descriptor, exactMatch);

  if (hasCount >= descriptor.count() || (takePartial && hasCount > 0)) {
    ItemDescriptor consumeDescriptor = descriptor.withCount(min(descriptor.count(), hasCount));
    consumeItems(consumeDescriptor, exactMatch);
    return consumeDescriptor;
  }

  return {};
}

HashMap<ItemDescriptor, uint64_t> PlayerInventory::availableItems() const {
  return ItemDatabase::normalizeBag(allItems());
}

HeadArmorPtr PlayerInventory::headArmor() const {
  return as<HeadArmor>(m_equipment.value(EquipmentSlot::Head));
}

ChestArmorPtr PlayerInventory::chestArmor() const {
  return as<ChestArmor>(m_equipment.value(EquipmentSlot::Chest));
}

LegsArmorPtr PlayerInventory::legsArmor() const {
  return as<LegsArmor>(m_equipment.value(EquipmentSlot::Legs));
}

BackArmorPtr PlayerInventory::backArmor() const {
  return as<BackArmor>(m_equipment.value(EquipmentSlot::Back));
}

HeadArmorPtr PlayerInventory::headCosmetic() const {
  return as<HeadArmor>(m_equipment.value(EquipmentSlot::HeadCosmetic));
}

ChestArmorPtr PlayerInventory::chestCosmetic() const {
  return as<ChestArmor>(m_equipment.value(EquipmentSlot::ChestCosmetic));
}

LegsArmorPtr PlayerInventory::legsCosmetic() const {
  return as<LegsArmor>(m_equipment.value(EquipmentSlot::LegsCosmetic));
}

BackArmorPtr PlayerInventory::backCosmetic() const {
  return as<BackArmor>(m_equipment.value(EquipmentSlot::BackCosmetic));
}

ItemBagConstPtr PlayerInventory::bagContents(String const& type) const {
  return m_bags.get(type);
}

void PlayerInventory::condenseBagStacks(String const& bagType) {
  auto bag = m_bags[bagType];

  bag->condenseStacks();

  m_customBar.forEach([&](auto const& index, CustomBarLink& link) {
    if (link.first) {
      if (auto bs = link.first->ptr<BagSlot>()) {
        if (bs->first == bagType && !bag->at(bs->second))
          link.first = {};
      }
    }
    if (link.second) {
      if (auto bs = link.second->ptr<BagSlot>()) {
        if (bs->first == bagType && !bag->at(bs->second))
          link.second = {};
      }
    }
  });
}

void PlayerInventory::sortBag(String const& bagType) {
  auto bag = m_bags[bagType];

  // When sorting bags, we need to record where all the action bar links were
  // pointing if any of them were pointing to the bag we are about to sort.
  MultiArray<pair<ItemPtr, ItemPtr>, 2> savedCustomBar(m_customBar.size());
  m_customBar.forEach([&](auto const& index, CustomBarLink const& link) {
    if (link.first) {
      if (auto bs = link.first->ptr<BagSlot>()) {
        if (bs->first == bagType)
          savedCustomBar(index).first = bag->at(bs->second);
      }
    }
    if (link.second) {
      if (auto bs = link.second->ptr<BagSlot>()) {
        if (bs->first == bagType)
          savedCustomBar(index).second = bag->at(bs->second);
      }
    }
  });

  auto itemDatabase = Root::singletonPtr()->itemDatabase();
  bag->items().sort([itemDatabase](ItemPtr const& a, ItemPtr const& b) {
    if (a && !b)
      return true;
    if (!a)
      return false;

    auto aType = itemDatabase->itemType(a->name());
    auto bType = itemDatabase->itemType(b->name());
    if (aType != bType)
      return aType < bType;

    if (a->rarity() != b->rarity())
      return a->rarity() > b->rarity();

    if (a->name().compare(b->name()) != 0)
      return a->name().compare(b->name()) < 0;

    if (a->count() != b->count())
      return a->count() > b->count();

    return false;
  });

  // Once we are done sorting, we need to restore the potentially action bar
  // links to point to where the item with the same identity is now residing.

  Map<ItemPtr, size_t> itemIndexes;
  for (size_t i = 0; i < bag->size(); ++i) {
    if (auto item = bag->at(i))
      itemIndexes[item] = i;
  }

  savedCustomBar.forEach([&](auto const& index, auto const& savedItems) {
    if (savedItems.first)
      m_customBar.at(index).first.set(BagSlot(bagType, itemIndexes.get(savedItems.first)));
    if (savedItems.second)
      m_customBar.at(index).second.set(BagSlot(bagType, itemIndexes.get(savedItems.second)));
  });
}

void PlayerInventory::shiftSwap(InventorySlot const& slot) {
  // FezzedOne: Replacement item swap function that also returns whether items were actually swapped or just stacked.
  auto canSwapItems = [&](ItemBagPtr bag, size_t pos, ItemPtr items) -> std::tuple<ItemPtr, bool> {
    auto& storedItem = bag->at(pos);
    bool didSwap = true;
    auto swapItems = items;
    if (!swapItems || swapItems->empty()) {
      swapItems = storedItem;
      storedItem = {};
    } else if (storedItem) {
      if (!storedItem->stackWith(swapItems))
        std::swap(storedItem, swapItems);
      else
        didSwap = false;
    } else {
      storedItem = swapItems;
      swapItems = {};
    }
    return std::tuple(swapItems, didSwap);
  };

  if (auto es = slot.ptr<EquipmentSlot>()) {
    if (itemAllowedAsEquipment(m_swapSlot, *es)) {
      auto& equipSlot = m_equipment[*es];
      if (itemSafeCount(m_swapSlot) <= 1) {
        swap(m_swapSlot, equipSlot);
        swapCustomBarLinks(SwapSlot(), slot);
      } else if (itemSafeCount(equipSlot) == 0) {
        equipSlot = m_swapSlot->take(1);
      }
    }
  } else if (slot.is<TrashSlot>()) {
    swap(m_swapSlot, m_trashSlot);
    swapCustomBarLinks(SwapSlot(), slot);
  } else if (auto bs = slot.ptr<BagSlot>()) {
    auto itemAllowed = [&](ItemPtr const& item, String const& bagType) -> bool {
      if (InventorySetting(AllowAnyItemInBags))
        return true;
      return itemAllowedInBag(item, bagType);
    };

    if (itemAllowed(m_swapSlot, bs->first)) {
      bool didSwap = false;
      std::tie(m_swapSlot, didSwap) = canSwapItems(m_bags[bs->first], bs->second, m_swapSlot);
      // m_swapSlot = m_bags[bs->first]->swapItems(bs->second, m_swapSlot);
      if (didSwap)
        swapCustomBarLinks(SwapSlot(), slot);
    }
  }

  if (!m_swapSlot)
    m_swapReturnSlot = {};
  else
    m_swapReturnSlot = slot;
}

bool PlayerInventory::clearSwap() {
  auto itemAllowed = [&](ItemPtr const& item, String const& bagType) -> bool {
    if (InventorySetting(AllowAnyItemInBags))
      return true;
    return itemAllowedInBag(item, bagType);
  };

  auto trySlot = [&](InventorySlot slot) {
    if (!m_swapSlot)
      return;

    m_swapSlot = stackWith(slot, m_swapSlot);
    if (!m_swapSlot)
      swapCustomBarLinks(SwapSlot(), slot);
  };

  auto tryBag = [&](String const& bagType) {
    for (size_t i = 0; i < m_bags[bagType]->size(); ++i) {
      if (!m_swapSlot || !itemAllowed(m_swapSlot, bagType))
        break;
      trySlot(BagSlot(bagType, i));
    }
  };

  if (m_swapReturnSlot)
    trySlot(m_swapReturnSlot.take());

  trySlot(EquipmentSlot::Head);
  trySlot(EquipmentSlot::Chest);
  trySlot(EquipmentSlot::Legs);
  trySlot(EquipmentSlot::Back);

  if (InventorySetting(AddToCosmetics)) {
    trySlot(EquipmentSlot::HeadCosmetic);
    trySlot(EquipmentSlot::ChestCosmetic);
    trySlot(EquipmentSlot::LegsCosmetic);
    trySlot(EquipmentSlot::BackCosmetic);
  }

  for (auto bagType : m_bags.keys())
    tryBag(bagType);

  return !m_swapSlot;
}

ItemPtr PlayerInventory::swapSlotItem() const {
  return m_swapSlot;
}

void PlayerInventory::setSwapSlotItem(ItemPtr const& items) {
  if (auto currencyItem = as<CurrencyItem>(items)) {
    addCurrency(currencyItem->currencyType(), currencyItem->totalValue());
    m_swapSlot = {};
  } else {
    m_swapSlot = items;
    autoAddToCustomBar(SwapSlot());
  }
}

ItemPtr PlayerInventory::essentialItem(EssentialItem essentialItem) const {
  return m_essential.value(essentialItem);
}

void PlayerInventory::setEssentialItem(EssentialItem essentialItem, ItemPtr item) {
  m_essential[essentialItem] = item;
}

StringMap<uint64_t> PlayerInventory::availableCurrencies() const {
  return m_currencies;
}

uint64_t PlayerInventory::currency(String const& currencyType) const {
  return m_currencies.value(currencyType, 0);
}

void PlayerInventory::addCurrency(String const& currencyType, uint64_t amount) {
  uint64_t previousTotal = m_currencies[currencyType];
  uint64_t newTotal = previousTotal + amount;
  if (newTotal < previousTotal)
    newTotal = highest<uint64_t>();
  m_currencies[currencyType] = min(Root::singleton().assets()->json("/currencies.config").get(currencyType).getUInt("playerMax", highest<uint64_t>()), newTotal);
}

bool PlayerInventory::consumeCurrency(String const& currencyType, uint64_t amount) {
  if (m_currencies[currencyType] >= amount) {
    m_currencies[currencyType] -= amount;
    return true;
  } else {
    return false;
  }
}

Maybe<InventorySlot> PlayerInventory::customBarPrimarySlot(CustomBarIndex customBarIndex) const {
  return m_customBar.at(m_customBarGroup, customBarIndex).first;
}

Maybe<InventorySlot> PlayerInventory::customBarSecondarySlot(CustomBarIndex customBarIndex) const {
  return m_customBar.at(m_customBarGroup, customBarIndex).second;
}

void PlayerInventory::setCustomBarPrimarySlot(CustomBarIndex customBarIndex, Maybe<InventorySlot> slot) {
  // The primary slot is not allowed to point to an empty item.
  if (slot) {
    if (!itemsAt(*slot))
      slot = {};
  }

  auto& cbl = m_customBar.at(m_customBarGroup, customBarIndex);
  if (slot && cbl.second == slot) {
    // If we match the secondary slot, just swap the slots for primary and
    // secondary
    swap(cbl.first, cbl.second);
  } else {
    cbl.first = slot;
  }
}

void PlayerInventory::setCustomBarSecondarySlot(CustomBarIndex customBarIndex, Maybe<InventorySlot> slot) {
  auto& cbl = m_customBar.at(m_customBarGroup, customBarIndex);
  // The secondary slot is not allowed to point to an empty item or a two
  // handed item.
  if (slot) {
    if (!itemsAt(*slot) || itemSafeTwoHanded(itemsAt(*slot)))
      slot = {};
  }

  if (cbl.first && cbl.first == slot && !itemSafeTwoHanded(itemsAt(*cbl.first))) {
    // If we match the primary slot and the primary slot is not a two handed
    // item, then just swap the two slots.
    swap(cbl.first, cbl.second);
  } else {
    cbl.second = slot;
    // If the primary slot was two handed, it is no longer valid so clear it.
    if (cbl.first && itemSafeTwoHanded(itemsAt(*cbl.first)))
      cbl.first = {};
  }
}

void PlayerInventory::addToCustomBar(InventorySlot slot) {
  for (size_t j = 0; j < m_customBar.size(1); ++j) {
    auto& cbl = m_customBar.at(m_customBarGroup, j);
    if (!cbl.first && !cbl.second) {
      cbl.first.set(slot);
      break;
    }
  }
}

uint8_t PlayerInventory::customBarGroup() const {
  return m_customBarGroup;
}

void PlayerInventory::setCustomBarGroup(uint8_t group) {
  m_customBarGroup = group;
}

uint8_t PlayerInventory::customBarGroups() const {
  return m_customBar.size(0);
}

uint8_t PlayerInventory::customBarIndexes() const {
  return m_customBar.size(1);
}

SelectedActionBarLocation PlayerInventory::selectedActionBarLocation() const {
  return m_selectedActionBar;
}

void PlayerInventory::selectActionBarLocation(SelectedActionBarLocation location) {
  m_selectedActionBar = location;
}

ItemPtr PlayerInventory::primaryHeldItem() const {
  if (m_swapSlot)
    return m_swapSlot;

  if (m_selectedActionBar.is<EssentialItem>())
    return m_essential.value(m_selectedActionBar.get<EssentialItem>());

  if (m_selectedActionBar.is<CustomBarIndex>()) {
    if (auto slot = m_customBar.at(m_customBarGroup, m_selectedActionBar.get<CustomBarIndex>()).first)
      return itemsAt(*slot);
  }

  return {};
}

ItemPtr PlayerInventory::secondaryHeldItem() const {
  auto pri = primaryHeldItem();
  if (itemSafeTwoHanded(pri) || m_swapSlot || !m_selectedActionBar || m_selectedActionBar.is<EssentialItem>())
    return {};

  auto const& cbl = m_customBar.at(m_customBarGroup, m_selectedActionBar.get<CustomBarIndex>());

  if (cbl.first && itemSafeTwoHanded(itemsAt(*cbl.first)))
    return {};

  if (cbl.second)
    return itemsAt(*cbl.second);

  return {};
}

Maybe<InventorySlot> PlayerInventory::primaryHeldSlot() const {
  if (m_swapSlot)
    return InventorySlot(SwapSlot());
  if (m_selectedActionBar.is<CustomBarIndex>())
    return customBarPrimarySlot(m_selectedActionBar.get<CustomBarIndex>());
  return {};
}

Maybe<InventorySlot> PlayerInventory::secondaryHeldSlot() const {
  if (m_swapSlot || itemSafeTwoHanded(primaryHeldItem()))
    return {};
  if (m_selectedActionBar.is<CustomBarIndex>())
    return customBarSecondarySlot(m_selectedActionBar.get<CustomBarIndex>());
  return {};
}

// From WasabiRaptor's PR: Function to take overflowed items from a player's inventory whenever it gets resized by a mod.
List<ItemPtr> PlayerInventory::clearOverflow() {
  auto list = m_inventoryLoadOverflow;
  m_inventoryLoadOverflow.clear();
  return list;
}

bool PlayerInventory::slotValid(InventorySlot const& slot) const {
  if (auto bagSlot = slot.ptr<BagSlot>()) {
    if (auto bag = m_bags.get(bagSlot->first)) {
      if ((size_t)bagSlot->second >= bag->size())
        return false;
    } else
      return false;
  }
  return true;
}

void PlayerInventory::load(Json const& store) {
  auto itemDatabase = Root::singleton().itemDatabase();

  m_equipment[EquipmentSlot::Head] = itemDatabase->diskLoad(store.get("headSlot"));
  m_equipment[EquipmentSlot::Chest] = itemDatabase->diskLoad(store.get("chestSlot"));
  m_equipment[EquipmentSlot::Legs] = itemDatabase->diskLoad(store.get("legsSlot"));
  m_equipment[EquipmentSlot::Back] = itemDatabase->diskLoad(store.get("backSlot"));
  m_equipment[EquipmentSlot::HeadCosmetic] = itemDatabase->diskLoad(store.get("headCosmeticSlot"));
  m_equipment[EquipmentSlot::ChestCosmetic] = itemDatabase->diskLoad(store.get("chestCosmeticSlot"));
  m_equipment[EquipmentSlot::LegsCosmetic] = itemDatabase->diskLoad(store.get("legsCosmeticSlot"));
  m_equipment[EquipmentSlot::BackCosmetic] = itemDatabase->diskLoad(store.get("backCosmeticSlot"));

  auto itemBags = store.get("itemBags").toObject();
  eraseWhere(m_bags, [&](auto const& p) { return !itemBags.contains(p.first); });
  // FezzedOne: Load any saved overflow.
  m_inventoryLoadOverflow.clear();
  if (store.contains("overflow")) {
    Json overflow = store.get("overflow");
    if (overflow.type() == Json::Type::Array) {
      m_inventoryLoadOverflow = overflow.toArray().transformed([itemDatabase](Json const& item) { return itemDatabase->diskLoad(item); });
    }
  }

  // FezzedOne: Load the configured bags from `player.config`.
  auto config = Root::singleton().assets()->json("/player.config:inventory");
  auto bags = config.get("itemBags").toObject();

  for (auto const& p : itemBags) {
    auto& bagType = p.first;
    auto newBag = ItemBag::loadStore(p.second);
    if (bags.keys().contains(bagType)) { // If the bag exists in the config, only take overflowed items.
      auto& bag = m_bags[bagType];
      auto size = bag.get()->size();
      if (bag)
        *bag = std::move(newBag);
      else
        bag = make_shared<ItemBag>(std::move(newBag));
      m_inventoryLoadOverflow.appendAll(bag.get()->resize(bags.get(bagType).getUInt("size")));
    } else { // If the bag *doesn't* exist in the config, take all items from it as overflow.
      m_inventoryLoadOverflow.appendAll(ItemBag(newBag).items());
    }
  }

  /* FezzedOne: Create any missing inventory «bags». */
  // auto bagOrder = bags.keys().sorted([&bags](String const& a, String const& b) {
  //   return bags.get(a).getInt("priority", 0) < bags.get(b).getInt("priority", 0);
  // });
  for (auto name : bags.keys()) {
    size_t size = bags.get(name).getUInt("size");
    if (!m_bags.keys().contains(name)) {
      m_bags[name] = make_shared<ItemBag>(size);
      // m_bagsNetState[name].resize(size);
    }
  }

  m_swapSlot = itemDatabase->diskLoad(store.get("swapSlot"));
  m_trashSlot = itemDatabase->diskLoad(store.get("trashSlot"));

  m_currencies = jsonToMapV<StringMap<uint64_t>>(store.get("currencies"), mem_fn(&Json::toUInt));

  // FezzedOne: Create any missing currencies.
  // auto currenciesConfig = Root::singleton().assets()->json("/currencies.config");
  // for (auto p : currenciesConfig.iterateObject()) {
  //   if (!m_currencies.keys().contains(name))
  //     m_currencies[p.first] = 0;
  // }

  m_customBarGroup = store.getUInt("customBarGroup");

  for (size_t i = 0; i < m_customBar.size(0); ++i) {
    for (size_t j = 0; j < m_customBar.size(1); ++j) {
      Json cbl = store.get("customBar").get(i, JsonArray()).get(j, JsonArray());
      // From WasabiRaptor's PR: Function to check whether a hotbar «slot» is valid.
      auto validateLink = [this](Json link) -> Json {
        if ((link.isType(Json::Type::Object)) && (m_bags.keys().contains(link.getString("type"))) && (m_bags[link.getString("type")].get()->size() > link.getUInt("location")))
          return link;
        return Json();
      };
      m_customBar.at(i, j) = CustomBarLink{
          jsonToMaybe<InventorySlot>(validateLink(cbl.get(0, Json())), jsonToInventorySlot),
          jsonToMaybe<InventorySlot>(validateLink(cbl.get(1, Json())), jsonToInventorySlot)};
    }
  }

  m_selectedActionBar = jsonToSelectedActionBarLocation(store.get("selectedActionBar"));

  m_essential.clear();
  m_essential[EssentialItem::BeamAxe] = itemDatabase->diskLoad(store.get("beamAxe"));
  m_essential[EssentialItem::WireTool] = itemDatabase->diskLoad(store.get("wireTool"));
  m_essential[EssentialItem::PaintTool] = itemDatabase->diskLoad(store.get("paintTool"));
  m_essential[EssentialItem::InspectionTool] = itemDatabase->diskLoad(store.get("inspectionTool"));
}

Json PlayerInventory::store() const {
  auto itemDatabase = Root::singleton().itemDatabase();

  JsonArray customBar;
  for (size_t i = 0; i < m_customBar.size(0); ++i) {
    JsonArray customBarGroup;
    for (size_t j = 0; j < m_customBar.size(1); ++j) {
      auto const& cbl = m_customBar.at(i, j);
      customBarGroup.append(JsonArray{jsonFromMaybe(cbl.first, jsonFromInventorySlot), jsonFromMaybe(cbl.second, jsonFromInventorySlot)});
    }
    customBar.append(take(customBarGroup));
  }

  // FezzedOne: Saved any unspawned overflow to the player file.
  bool overflowExists = false;
  Json overflow = Json();
  if (!m_inventoryLoadOverflow.empty()) {
    overflowExists = true;
    overflow = m_inventoryLoadOverflow.transformed([itemDatabase](ItemConstPtr const& item) { return itemDatabase->diskStore(item); });
  }

  JsonObject itemBags;
  for (auto bag : m_bags)
    itemBags.add(bag.first, bag.second->diskStore());

  if (overflowExists) {
    return JsonObject{
        {"headSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::Head))},
        {"chestSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::Chest))},
        {"legsSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::Legs))},
        {"backSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::Back))},
        {"headCosmeticSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::HeadCosmetic))},
        {"chestCosmeticSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::ChestCosmetic))},
        {"legsCosmeticSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::LegsCosmetic))},
        {"backCosmeticSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::BackCosmetic))},
        {"itemBags", itemBags},
        {"swapSlot", itemDatabase->diskStore(m_swapSlot)},
        {"trashSlot", itemDatabase->diskStore(m_trashSlot)},
        {"currencies", jsonFromMap(m_currencies)},
        {"customBarGroup", m_customBarGroup},
        {"customBar", std::move(customBar)},
        {"selectedActionBar", jsonFromSelectedActionBarLocation(m_selectedActionBar)},
        {"beamAxe", itemDatabase->diskStore(m_essential.value(EssentialItem::BeamAxe))},
        {"wireTool", itemDatabase->diskStore(m_essential.value(EssentialItem::WireTool))},
        {"paintTool", itemDatabase->diskStore(m_essential.value(EssentialItem::PaintTool))},
        {"inspectionTool", itemDatabase->diskStore(m_essential.value(EssentialItem::InspectionTool))},
        {"overflow", overflow}};
  } else {
    return JsonObject{
        {"headSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::Head))},
        {"chestSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::Chest))},
        {"legsSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::Legs))},
        {"backSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::Back))},
        {"headCosmeticSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::HeadCosmetic))},
        {"chestCosmeticSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::ChestCosmetic))},
        {"legsCosmeticSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::LegsCosmetic))},
        {"backCosmeticSlot", itemDatabase->diskStore(m_equipment.value(EquipmentSlot::BackCosmetic))},
        {"itemBags", itemBags},
        {"swapSlot", itemDatabase->diskStore(m_swapSlot)},
        {"trashSlot", itemDatabase->diskStore(m_trashSlot)},
        {"currencies", jsonFromMap(m_currencies)},
        {"customBarGroup", m_customBarGroup},
        {"customBar", std::move(customBar)},
        {"selectedActionBar", jsonFromSelectedActionBarLocation(m_selectedActionBar)},
        {"beamAxe", itemDatabase->diskStore(m_essential.value(EssentialItem::BeamAxe))},
        {"wireTool", itemDatabase->diskStore(m_essential.value(EssentialItem::WireTool))},
        {"paintTool", itemDatabase->diskStore(m_essential.value(EssentialItem::PaintTool))},
        {"inspectionTool", itemDatabase->diskStore(m_essential.value(EssentialItem::InspectionTool))}};
  }
}

void PlayerInventory::forEveryItem(function<void(InventorySlot const&, ItemPtr&)> function) {
  auto checkedFunction = [function = std::move(function)](InventorySlot const& slot, ItemPtr& item) {
    if (item)
      function(slot, item);
  };

  for (auto& p : m_equipment)
    checkedFunction(p.first, p.second);
  for (auto const& p : m_bags) {
    for (size_t i = 0; i < p.second->size(); ++i)
      checkedFunction(BagSlot(p.first, i), p.second->at(i));
  }
  checkedFunction(SwapSlot(), m_swapSlot);
  checkedFunction(TrashSlot(), m_trashSlot);
}

void PlayerInventory::forEveryItem(function<void(InventorySlot const&, ItemPtr const&)> function) const {
  return const_cast<PlayerInventory*>(this)->forEveryItem([function = std::move(function)](InventorySlot const& slot, ItemPtr& item) {
    function(slot, item);
  });
}

List<ItemPtr> PlayerInventory::allItems() const {
  List<ItemPtr> items;
  forEveryItem([&items](InventorySlot const&, ItemPtr const& item) {
    items.append(item);
  });
  return items;
}

Map<String, uint64_t> PlayerInventory::itemSummary() const {
  Map<String, uint64_t> result;
  forEveryItem([&result](auto const&, auto const& item) {
    result[item->name()] += item->count();
  });
  return result;
}

void PlayerInventory::cleanup() {
  for (auto pair : m_bags) {
    if (pair.second)
      pair.second->cleanup();
  }

  for (auto& p : m_equipment)
    if (p.second && p.second->empty())
      p.second = ItemPtr();

  if (m_swapSlot && m_swapSlot->empty())
    m_swapSlot = ItemPtr();

  if (m_trashSlot && m_trashSlot->empty())
    m_trashSlot = ItemPtr();

  m_customBar.forEach([this](Array2S const&, CustomBarLink& p) {
    ItemPtr primary = p.first ? retrieve(*p.first) : ItemPtr();
    ItemPtr secondary = p.second ? retrieve(*p.second) : ItemPtr();

    // Reset the primary and secondary action bar link if the item is gone
    if (!primary)
      p.first.reset();
    if (!secondary)
      p.second.reset();

    // If the primary hand item is two handed, the secondary hand should not be
    // set
    if (itemSafeTwoHanded(primary))
      p.second.reset();
    // Two handed items are not allowed in the secondary slot
    if (itemSafeTwoHanded(secondary))
      p.second.reset();
  });
}

bool PlayerInventory::checkInventoryFilter(ItemPtr const& items, String const& filterName) {
  auto config = Root::singleton().assets()->json("/player.config:inventoryFilters");

  // filter by item type if an itemTypes filter is set
  auto itemDatabase = Root::singleton().itemDatabase();
  auto filterConfig = config.get(filterName);
  auto itemTypeName = ItemTypeNames.getRight(itemDatabase->itemType(items->name()));
  if (filterConfig.contains("typeWhitelist") && !filterConfig.getArray("typeWhitelist").contains(itemTypeName))
    return false;

  if (filterConfig.contains("typeBlacklist") && filterConfig.getArray("typeBlacklist").contains(itemTypeName))
    return false;

  // filter by item tags if an itemTags filter is set
  // this is an inclusive filter
  auto itemTags = itemDatabase->itemTags(items->name());
  if (filterConfig.contains("tagWhitelist")) {
    auto whitelistedTags = filterConfig.getArray("tagWhitelist").filtered([itemTags](Json const& tag) {
      return itemTags.contains(tag.toString());
    });
    if (whitelistedTags.size() == 0)
      return false;
  }

  if (filterConfig.contains("tagBlacklist")) {
    auto blacklistedTags = filterConfig.getArray("tagBlacklist").filtered([itemTags](Json const& tag) {
      return itemTags.contains(tag.toString());
    });
    if (blacklistedTags.size() > 0)
      return false;
  }

  auto itemCategory = items->category();
  if (auto categoryWhitelist = filterConfig.optArray("categoryWhitelist")) {
    auto categoryWhiteset = jsonToStringSet(*categoryWhitelist);
    if (!categoryWhiteset.contains(itemCategory))
      return false;
  }

  if (auto categoryBlacklist = filterConfig.optArray("categoryBlacklist")) {
    auto categoryBlackset = jsonToStringSet(*categoryBlacklist);
    if (categoryBlackset.contains(itemCategory))
      return false;
  }

  return true;
}

ItemPtr const& PlayerInventory::retrieve(InventorySlot const& slot) const {
  return const_cast<PlayerInventory*>(this)->retrieve(slot);
}

ItemPtr& PlayerInventory::retrieve(InventorySlot const& slot) {
  auto guardEmpty = [](ItemPtr& item) -> ItemPtr& {
    if (item && item->empty())
      item = {};
    return item;
  };

  if (auto es = slot.ptr<EquipmentSlot>())
    return guardEmpty(m_equipment[*es]);
  else if (auto bs = slot.ptr<BagSlot>()) {
    // FezzedOne: «Downstreamed» from OpenStarbound, but also made the exception thrown on out-of-bounds access a bit more consistent.
    if (auto bag = m_bags.ptr(bs->first)) {
      if (bs->second < (*bag)->size())
        return guardEmpty((*bag)->at(bs->second));
    }
    throw ItemException::format("Invalid inventory slot {}", jsonFromInventorySlot(slot));
  } else if (slot.is<SwapSlot>())
    return guardEmpty(m_swapSlot);
  else
    return guardEmpty(m_trashSlot);
}

void PlayerInventory::swapCustomBarLinks(InventorySlot a, InventorySlot b) {
  m_customBar.forEach([&](Array2S const&, CustomBarLink& p) {
    if (p.first == a)
      p.first = b;
    else if (p.first == b)
      p.first = a;

    if (p.second == a)
      p.second = b;
    else if (p.second == b)
      p.second = a;
  });
}

void PlayerInventory::autoAddToCustomBar(InventorySlot slot) {
  if (!Root::singleton().configuration()->getPath("inventory.pickupToActionBar").toBool())
    return;

  auto items = itemsAt(slot);
  if (items && !items->empty() && checkInventoryFilter(items, "autoAddToCustomBar"))
    addToCustomBar(slot);
}

void PlayerInventory::netElementsNeedLoad(bool) {
  auto itemDatabase = Root::singleton().itemDatabase();

  auto deserializeItem = [&itemDatabase](NetElementData<ItemDescriptor>& netState, ItemPtr& item) {
    if (netState.pullUpdated())
      itemDatabase->loadItem(netState.get(), item);
  };

  auto deserializeItemList = [&](List<NetElementData<ItemDescriptor>>& netStatesList, List<ItemPtr>& itemList) {
    // FezzedOne: If the actual inventory is larger than the networked one, fill the remaining slots with empty items.
    for (size_t i = 0; i < itemList.size(); ++i) {
      if (i < netStatesList.size())
        deserializeItem(netStatesList[i], itemList[i]);
      else
        break;
    }
  };

  auto deserializeItemMap = [&](auto& netStatesMap, auto& itemMap) {
    for (auto k : netStatesMap.keys())
      deserializeItem(netStatesMap[k], itemMap[k]);
  };

  deserializeItemMap(m_equipmentNetState, m_equipment);

  // FezzedOne: Check to make sure the bag actually exists in the real inventory before deserialising anything into it.
  for (auto bagType : m_bagsNetState.keys()) {
    if (m_bags.contains(bagType))
      deserializeItemList(m_bagsNetState[bagType], m_bags[bagType]->items());
  }

  deserializeItem(m_swapSlotNetState, m_swapSlot);
  deserializeItem(m_trashSlotNetState, m_trashSlot);

  m_currencies = m_currenciesNetState.get();

  m_customBarGroup = m_customBarGroupNetState.get();
  m_customBarNetState.forEach([&](Array<size_t, 2> const& index, auto& ns) {
    // FezzedOne: Assume the action bar is fully empty. This prevents a server-side segfault caused by
    // action bar slots being linked to inventory slots the server cannot see.
    // Also don't try to fill custom bar slots that don't exist.
    if (index[0] < m_customBar.size(0) && index[1] < m_customBar.size(1))
      m_customBar.at(index) = CustomBarLink{{}, {}}; // ns.get();
  });

  m_selectedActionBar = m_selectedActionBarNetState.get();

  deserializeItemMap(m_essentialNetState, m_essential);

  cleanup();
}

void PlayerInventory::netElementsNeedStore() {
  cleanup();

  auto const& playerIdentity = m_player->identity();

  const StringMap<String> identityTags{
      {"species", playerIdentity.species},
      {"imagePath", playerIdentity.imagePath ? *playerIdentity.imagePath : playerIdentity.species},
      {"gender", playerIdentity.gender == Gender::Male ? "male" : "female"},
      {"bodyDirectives", playerIdentity.bodyDirectives.string()},
      {"emoteDirectives", playerIdentity.bodyDirectives.string()},
      {"hairGroup", playerIdentity.hairGroup},
      {"hairType", playerIdentity.hairType},
      {"hairDirectives", playerIdentity.hairDirectives.string()},
      {"facialHairGroup", playerIdentity.facialHairGroup},
      {"facialHairType", playerIdentity.facialHairType},
      {"facialHairDirectives", playerIdentity.facialHairDirectives.string()},
      {"facialMaskGroup", playerIdentity.facialMaskGroup},
      {"facialMaskType", playerIdentity.facialMaskType},
      {"facialMaskDirectives", playerIdentity.facialMaskDirectives.string()},
      {"personalityIdle", playerIdentity.personality.idle},
      {"personalityArmIdle", playerIdentity.personality.armIdle},
      {"headOffsetX", strf("{}", playerIdentity.personality.headOffset[0])},
      {"headOffsetY", strf("{}", playerIdentity.personality.headOffset[1])},
      {"armOffsetX", strf("{}", playerIdentity.personality.armOffset[0])},
      {"armOffsetY", strf("{}", playerIdentity.personality.armOffset[1])},
      {"color", Color::rgba(playerIdentity.color).toHex()}};

  auto handleDirectiveTags = [&](ItemDescriptor& armourItem, List<ItemDescriptor> const& overlays) {
    if (auto params = armourItem.parameters(); params.isType(Json::Type::Object)) {
      auto jDirectives = params.opt("directives").value(Json()), jFlipDirectives = params.opt("flipDirectives").value(Json());
      auto processedDirectives = JsonObject{
          {"directives", jDirectives.toString().replaceTags(identityTags, false)},
          {"flipDirectives", jFlipDirectives.toString().replaceTags(identityTags, false)}};
      armourItem.applyParameters(params.toObject());

      if (!overlays.empty()) {
        JsonArray jOverlays{};
        for (auto& item : overlays) {
          if (auto params = item.parameters(); params.isType(Json::Type::Object)) {
            auto jDirectives = params.opt("directives").value(Json()), jFlipDirectives = params.opt("flipDirectives").value(Json());
            auto processedDirectives = JsonObject{
                {"directives", jDirectives.toString().replaceTags(identityTags, false)},
                {"flipDirectives", jFlipDirectives.toString().replaceTags(identityTags, false)}};
            item.applyParameters(params.toObject());
          }
          jOverlays.emplaceAppend(item.toJson());
        }
        armourItem.applyParameters(JsonObject{
            {"stackedItems", jOverlays}});
      }
    }
  };

  auto serializeItem = [&](NetElementData<ItemDescriptor>& netState, ItemPtr& item, bool replaceArmourTags = false) {
    auto armourItem = as<ArmorItem>(item);
    if (replaceArmourTags && armourItem) {
      auto cloneDesc = itemSafeDescriptor(item);
      auto overlays = armourItem ? armourItem->getStackedCosmetics() : List<ItemPtr>{};
      handleDirectiveTags(cloneDesc,
          overlays.transformed([](ItemPtr const& item) { return itemSafeDescriptor(as<ArmorItem>(item)); }));
      netState.set(cloneDesc);
    } else {
      netState.set(itemSafeDescriptor(item));
    }
  };

  auto serializeItemList = [&](List<NetElementData<ItemDescriptor>>& netStatesList, List<ItemPtr>& itemList) {
    for (size_t i = 0; i < netStatesList.size(); ++i) {
      if (i < itemList.size())
        serializeItem(netStatesList[i], itemList[i]);
      else // If the real bag is smaller than the networked one, make the networked item slot look «empty».
        netStatesList[i].set(ItemDescriptor());
    }
  };

  auto serializeEmptyItemList = [&](List<NetElementData<ItemDescriptor>>& netStatesList) {
    for (size_t i = 0; i < netStatesList.size(); ++i) {
      netStatesList[i].set(ItemDescriptor());
    }
  };

  auto serializeItemMap = [&](auto& netStatesMap, auto& itemMap, bool replaceArmourTags = false) {
    for (auto k : netStatesMap.keys())
      serializeItem(netStatesMap[k], itemMap[k], replaceArmourTags);
  };

  serializeItemMap(m_equipmentNetState, m_equipment, true);

  for (auto bagType : m_bagsNetState.keys()) {
    // Check if the bag exists in the player's real inventory. If not, fill it with empty slots.
    if (m_bags.contains(bagType))
      serializeItemList(m_bagsNetState[bagType], m_bags[bagType]->items());
    else
      serializeEmptyItemList(m_bagsNetState[bagType]);
  }

  serializeItem(m_swapSlotNetState, m_swapSlot);
  serializeItem(m_trashSlotNetState, m_trashSlot);

  m_currenciesNetState.set(m_currencies);

  // FezzedOne: Spoof the selected custom bar group.
  m_customBarGroupNetState.set(m_customBarGroup < m_networkedCustomBarGroups ? m_customBarGroup : m_networkedCustomBarGroups - 1);

  // FezzedOne: Spoof the networked custom bars.
  m_customBarNetState.forEach([&](auto const& index, auto& cbl) {
    // FezzedOne: Spoof *all* slots as empty. This prevents the case where an action bar slot linked to an
    // inventory slot the server can't see causes the server to segfault.
    m_customBarNetState.at(index).set(CustomBarLinkCompat{{}, {}});
  });

  // FezzedOne: Spoof the selected custom bar slot.
  if (auto nonEssentialSlot = m_selectedActionBar.maybe<CustomBarIndex>()) {
    if (*nonEssentialSlot < m_networkedCustomBarIndexes)
      m_selectedActionBarNetState.set(m_selectedActionBar);
    else
      m_selectedActionBarNetState.set((CustomBarIndex)(m_networkedCustomBarIndexes - 1));
  } else {
    m_selectedActionBarNetState.set(m_selectedActionBar);
  }


  serializeItemMap(m_essentialNetState, m_essential);
}

} // namespace Star
