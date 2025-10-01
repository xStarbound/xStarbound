#include "StarArmorWearer.hpp"
#include "StarActivatableItem.hpp"
#include "StarArmors.hpp"
#include "StarAssets.hpp"
#include "StarCasting.hpp"
#include "StarHumanoid.hpp"
#include "StarImageProcessing.hpp"
#include "StarItemDatabase.hpp"
#include "StarLiquidItem.hpp"
#include "StarMaterialItem.hpp"
#include "StarObject.hpp"
#include "StarObjectDatabase.hpp"
#include "StarObjectItem.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerInventory.hpp"
#include "StarRoot.hpp"
#include "StarTools.hpp"
#include "StarWorld.hpp"

namespace Star {

ArmorWearer::ArmorWearer() : m_lastNude(true), m_lastFacingDirection(255) {
  addNetElement(&m_headItemDataNetState);
  addNetElement(&m_chestItemDataNetState);
  addNetElement(&m_legsItemDataNetState);
  addNetElement(&m_backItemDataNetState);
  addNetElement(&m_headCosmeticItemDataNetState);
  addNetElement(&m_chestCosmeticItemDataNetState);
  addNetElement(&m_legsCosmeticItemDataNetState);
  addNetElement(&m_backCosmeticItemDataNetState);

  reset();
}

ArmorWearer::ArmorWearer(bool isPlayer) : ArmorWearer() {
  m_isPlayer = isPlayer;
}

ArmorWearer::ArmorWearer(Player* player) : ArmorWearer(true) {
  m_player = player;
}

void ArmorWearer::setupHumanoidClothingDrawables(Humanoid& humanoid, bool forceNude) {
  bool nudeChanged = m_lastNude != forceNude;
  m_lastNude = forceNude;

  uint8_t currentDirection = (uint8_t)humanoid.facingDirection();
  bool directionChanged = m_lastFacingDirection != currentDirection;
  m_lastFacingDirection = currentDirection;

  bool headNeedsSync = m_headNeedsSync;
  bool chestNeedsSync = m_chestNeedsSync;
  bool legsNeedsSync = m_legsNeedsSync;
  bool backNeedsSync = m_backNeedsSync;

  bool anyNeedsSync = headNeedsSync || chestNeedsSync || legsNeedsSync || backNeedsSync || nudeChanged;
  if (anyNeedsSync)
    netElementsNeedLoad(true);

  bool bodyHidden = false;
  Json humanoidOverrides = JsonObject{};

  auto mergeHumanoidConfig = [&](auto armourItem) {
    Json configToMerge = armourItem->instanceValue("humanoidConfig", Json());
    if (configToMerge.isType(Json::Type::Object))
      humanoidOverrides = jsonMerge(humanoidOverrides, configToMerge);
  };

  auto getDirectives = [&](auto armourItem) -> Directives {
    return currentDirection == (uint8_t)Direction::Left ? armourItem->flippedDirectives() : armourItem->directives();
  };

  ArmorItem::HiddenArmorTypes hiddenArmourSlots = {false, false, false, false};

  List<Humanoid::ArmorEntry> headArmorStack = {};
  List<Humanoid::ArmorEntry> chestArmorStack = {};
  List<Humanoid::ArmorEntry> frontSleeveStack = {};
  List<Humanoid::ArmorEntry> backSleeveStack = {};
  List<Humanoid::ArmorEntry> legsArmorStack = {};
  List<Humanoid::BackEntry> backArmorStack = {};

  bool pulledOpenSbCosmeticUpdate = m_player ? m_player->pulledCosmeticUpdate() : false;
  auto& openSbCosmeticStack = m_player ? m_player->getNetArmorSecrets() : Array<ArmorItemPtr, 12>::filled(nullptr);

  if (anyNeedsSync) { // FezzedOne: Handle armour hiding from stock cosmetic slots.
    if (auto& item = m_headCosmeticItem) {
      auto hiddenSlots = item->armorTypesToHide();
      hiddenArmourSlots.head |= hiddenSlots.head;
      hiddenArmourSlots.chest |= hiddenSlots.chest;
      hiddenArmourSlots.legs |= hiddenSlots.legs;
      hiddenArmourSlots.back |= hiddenSlots.back;
    }
    if (auto& item = m_chestCosmeticItem) {
      auto hiddenSlots = item->armorTypesToHide();
      hiddenArmourSlots.head |= hiddenSlots.head;
      hiddenArmourSlots.chest |= hiddenSlots.chest;
      hiddenArmourSlots.legs |= hiddenSlots.legs;
      hiddenArmourSlots.back |= hiddenSlots.back;
    }
    if (auto& item = m_legsCosmeticItem) {
      auto hiddenSlots = item->armorTypesToHide();
      hiddenArmourSlots.head |= hiddenSlots.head;
      hiddenArmourSlots.chest |= hiddenSlots.chest;
      hiddenArmourSlots.legs |= hiddenSlots.legs;
      hiddenArmourSlots.back |= hiddenSlots.back;
    }
    if (auto& item = m_backCosmeticItem) {
      auto hiddenSlots = item->armorTypesToHide();
      hiddenArmourSlots.head |= hiddenSlots.head;
      hiddenArmourSlots.chest |= hiddenSlots.chest;
      hiddenArmourSlots.legs |= hiddenSlots.legs;
      hiddenArmourSlots.back |= hiddenSlots.back;
    }
  }

  if (m_player && m_player->isSlave() && (pulledOpenSbCosmeticUpdate || anyNeedsSync)) { // FezzedOne: Handle armour hiding from oSB cosmetic slots.
    anyNeedsSync = true;
    for (uint8_t i = 0; i != 12; i++) {
      auto& item = openSbCosmeticStack[i];
      if (!item) continue;
      auto hiddenSlots = item->armorTypesToHide();
      hiddenArmourSlots.head |= hiddenSlots.head || is<HeadArmor>(item);
      hiddenArmourSlots.chest |= hiddenSlots.chest || is<ChestArmor>(item);
      hiddenArmourSlots.legs |= hiddenSlots.legs || is<LegsArmor>(item);
      hiddenArmourSlots.back |= hiddenSlots.back || is<BackArmor>(item);
    }
  }

  anyNeedsSync |= directionChanged;

  uint8_t openSbLayerCount = 0;

  auto shouldShowArmour = [&](auto armour) -> bool {
    return !forceNude || armour->bypassNudity();
  };

  { // FezzedOne: The main armour updating code.
    if (m_headCosmeticItem && !m_headCosmeticItem->hideInStockSlots()) {
      if (anyNeedsSync) {
        if (shouldShowArmour(m_headCosmeticItem)) {
          humanoid.setHeadArmorFrameset(m_headCosmeticItem->frameset(humanoid.identity().gender));
          humanoid.setHeadArmorDirectives(getDirectives(m_headCosmeticItem));
          humanoid.setHelmetMaskDirectives(m_headCosmeticItem->maskDirectives());
          mergeHumanoidConfig(m_headCosmeticItem);
          for (auto& item : m_headCosmeticItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<HeadArmor>(item)) {
                bool hidden = armourItem->hideInStockSlots();
                if (m_player && openSbLayerCount < 12 && m_player->isMaster())
                  m_player->setNetArmorSecret(openSbLayerCount++, hidden ? nullptr : as<ArmorItem>(item));
                headArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->frameset(humanoid.identity().gender),
                    hidden ? Directives("?scale=0") : getDirectives(armourItem),
                    hidden ? Directives() : armourItem->maskDirectives()});
                mergeHumanoidConfig(item);
                bodyHidden |= armourItem->hideBody();
              }
            }
          }
          // humanoid.setHeadArmorStack(headArmorStack);
        } else {
          humanoid.setHeadArmorFrameset("");
          humanoid.setHelmetMaskDirectives("");
        }
      }
      if (m_headItem && !hiddenArmourSlots.head && !m_headItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          if (m_headItem->isUnderlaid() && shouldShowArmour(m_headItem)) {
            humanoid.setHeadArmorUnderlayFrameset(m_headItem->frameset(humanoid.identity().gender));
            humanoid.setHeadArmorUnderlayDirectives(getDirectives(m_headItem));
            humanoid.setHelmetMaskUnderlayDirectives(m_headItem->maskDirectives());
            mergeHumanoidConfig(m_headItem);
            List<Humanoid::ArmorEntry> headUnderlayArmorStack = {};
            for (auto& item : m_headItem->getStackedCosmetics()) {
              if (item) {
                if (auto armourItem = as<HeadArmor>(item)) {
                  bool hidden = armourItem->hideInStockSlots();
                  headUnderlayArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                      armourItem->frameset(humanoid.identity().gender),
                      hidden ? Directives("?scale=0") : getDirectives(armourItem),
                      hidden ? Directives() : armourItem->maskDirectives()});
                  mergeHumanoidConfig(item);
                  bodyHidden |= armourItem->hideBody();
                }
              }
            }
            humanoid.setHeadArmorUnderlayStack(headUnderlayArmorStack);
          } else {
            humanoid.setHeadArmorUnderlayFrameset("");
            humanoid.setHelmetMaskUnderlayDirectives("");
            humanoid.setHeadArmorUnderlayStack({});
          }
        }
        bodyHidden = bodyHidden || (m_headItem->isUnderlaid() && m_headItem->hideBody());
      } else {
        if (anyNeedsSync) {
          humanoid.setHeadArmorUnderlayFrameset("");
          humanoid.setHelmetMaskUnderlayDirectives("");
          humanoid.setHeadArmorUnderlayStack({});
        }
      }
      bodyHidden = bodyHidden || m_headCosmeticItem->hideBody();
    } else if (m_headItem && shouldShowArmour(m_headItem) && !hiddenArmourSlots.head && !m_headItem->hideInStockSlots()) {
      if (anyNeedsSync) {
        humanoid.setHeadArmorFrameset(m_headItem->frameset(humanoid.identity().gender));
        humanoid.setHeadArmorDirectives(getDirectives(m_headItem));
        humanoid.setHelmetMaskDirectives(m_headItem->maskDirectives());
        mergeHumanoidConfig(m_headItem);
        for (auto& item : m_headItem->getStackedCosmetics()) {
          if (item) {
            if (auto armourItem = as<HeadArmor>(item)) {
              bool hidden = armourItem->hideInStockSlots();
              if (m_player && openSbLayerCount < 12 && m_player->isMaster())
                m_player->setNetArmorSecret(openSbLayerCount++, hidden ? nullptr : as<ArmorItem>(item));
              headArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                  armourItem->frameset(humanoid.identity().gender),
                  hidden ? Directives("?scale=0") : getDirectives(armourItem),
                  hidden ? Directives() : armourItem->maskDirectives()});
              mergeHumanoidConfig(item);
              bodyHidden |= armourItem->hideBody();
            }
          }
        }
        // humanoid.setHeadArmorStack(headArmorStack);
        humanoid.setHeadArmorUnderlayFrameset("");
        humanoid.setHelmetMaskUnderlayDirectives("");
        humanoid.setHeadArmorUnderlayStack({});
      }
      bodyHidden = bodyHidden || m_headItem->hideBody();
    } else {
      if (anyNeedsSync) {
        humanoid.setHeadArmorFrameset("");
        humanoid.setHelmetMaskDirectives("");
        humanoid.setHeadArmorUnderlayFrameset("");
        humanoid.setHelmetMaskUnderlayDirectives("");
        // humanoid.setHeadArmorStack(headArmorStack);
        humanoid.setHeadArmorUnderlayStack({});
      }
    }

    {
      List<ChestArmorPtr> chestItems;
      List<LegsArmorPtr> legsItems;

      if (m_chestCosmeticItem && !m_chestCosmeticItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          if (shouldShowArmour(m_chestCosmeticItem)) {
            humanoid.setBackSleeveFrameset(m_chestCosmeticItem->backSleeveFrameset(humanoid.identity().gender));
            humanoid.setFrontSleeveFrameset(m_chestCosmeticItem->frontSleeveFrameset(humanoid.identity().gender));
            humanoid.setChestArmorFrameset(m_chestCosmeticItem->bodyFrameset(humanoid.identity().gender));
            humanoid.setChestArmorDirectives(getDirectives(m_chestCosmeticItem));
            mergeHumanoidConfig(m_chestCosmeticItem);
            for (auto& item : m_chestCosmeticItem->getStackedCosmetics()) {
              if (item) {
                if (auto armourItem = as<ChestArmor>(item)) {
                  bool hidden = armourItem->hideInStockSlots();
                  chestItems.emplace_back(hidden ? nullptr : armourItem);
                  chestArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                      armourItem->bodyFrameset(humanoid.identity().gender),
                      hidden ? Directives("?scale=0") : getDirectives(armourItem),
                      Directives()});
                  frontSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
                      armourItem->frontSleeveFrameset(humanoid.identity().gender),
                      hidden ? Directives("?scale=0") : getDirectives(armourItem),
                      Directives()});
                  backSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
                      armourItem->backSleeveFrameset(humanoid.identity().gender),
                      hidden ? Directives("?scale=0") : getDirectives(armourItem),
                      Directives()});
                  mergeHumanoidConfig(item);
                  bodyHidden |= armourItem->hideBody();
                }
              }
            }
            // humanoid.setChestArmorStack(chestArmorStack);
            // humanoid.setFrontSleeveStack(frontSleeveStack);
            // humanoid.setBackSleeveStack(backSleeveStack);
          } else {
            humanoid.setBackSleeveFrameset("");
            humanoid.setFrontSleeveFrameset("");
            humanoid.setChestArmorFrameset("");
          }
        }
        if (m_chestItem && !hiddenArmourSlots.chest && !m_chestItem->hideInStockSlots()) {
          if (anyNeedsSync) {
            if (m_chestItem->isUnderlaid() && shouldShowArmour(m_chestItem)) {
              humanoid.setBackSleeveUnderlayFrameset(m_chestItem->backSleeveFrameset(humanoid.identity().gender));
              humanoid.setFrontSleeveUnderlayFrameset(m_chestItem->frontSleeveFrameset(humanoid.identity().gender));
              humanoid.setChestArmorUnderlayFrameset(m_chestItem->bodyFrameset(humanoid.identity().gender));
              humanoid.setChestArmorUnderlayDirectives(getDirectives(m_chestItem));
              mergeHumanoidConfig(m_chestItem);
              List<Humanoid::ArmorEntry> chestArmorUnderlayStack = {}, frontSleeveUnderlayStack = {}, backSleeveUnderlayStack = {};
              for (auto& item : m_chestItem->getStackedCosmetics()) {
                if (item) {
                  if (auto armourItem = as<ChestArmor>(item)) {
                    bool hidden = armourItem->hideInStockSlots();
                    chestArmorUnderlayStack.emplaceAppend(Humanoid::ArmorEntry{
                        armourItem->bodyFrameset(humanoid.identity().gender),
                        hidden ? Directives("?scale=0") : getDirectives(armourItem),
                        Directives()});
                    frontSleeveUnderlayStack.emplaceAppend(Humanoid::ArmorEntry{
                        armourItem->frontSleeveFrameset(humanoid.identity().gender),
                        hidden ? Directives("?scale=0") : getDirectives(armourItem),
                        Directives()});
                    backSleeveUnderlayStack.emplaceAppend(Humanoid::ArmorEntry{
                        armourItem->backSleeveFrameset(humanoid.identity().gender),
                        hidden ? Directives("?scale=0") : getDirectives(armourItem),
                        Directives()});
                    mergeHumanoidConfig(item);
                    bodyHidden |= armourItem->hideBody();
                  }
                }
              }
              humanoid.setChestArmorUnderlayStack(chestArmorUnderlayStack);
              humanoid.setFrontSleeveUnderlayStack(frontSleeveUnderlayStack);
              humanoid.setBackSleeveUnderlayStack(backSleeveUnderlayStack);
            } else {
              humanoid.setBackSleeveUnderlayFrameset("");
              humanoid.setFrontSleeveUnderlayFrameset("");
              humanoid.setChestArmorUnderlayFrameset("");
              humanoid.setChestArmorUnderlayStack({});
              humanoid.setFrontSleeveUnderlayStack({});
              humanoid.setBackSleeveUnderlayStack({});
            }
          }
          bodyHidden = bodyHidden || (m_chestItem->isUnderlaid() && m_chestItem->hideBody());
        } else {
          if (anyNeedsSync) {
            humanoid.setBackSleeveUnderlayFrameset("");
            humanoid.setFrontSleeveUnderlayFrameset("");
            humanoid.setChestArmorUnderlayFrameset("");
            humanoid.setChestArmorUnderlayStack({});
            humanoid.setFrontSleeveUnderlayStack({});
            humanoid.setBackSleeveUnderlayStack({});
          }
        }
        bodyHidden = bodyHidden || m_chestCosmeticItem->hideBody();
      } else if (m_chestItem && shouldShowArmour(m_chestItem) && !hiddenArmourSlots.chest && !m_chestItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          humanoid.setBackSleeveFrameset(m_chestItem->backSleeveFrameset(humanoid.identity().gender));
          humanoid.setFrontSleeveFrameset(m_chestItem->frontSleeveFrameset(humanoid.identity().gender));
          humanoid.setChestArmorFrameset(m_chestItem->bodyFrameset(humanoid.identity().gender));
          humanoid.setChestArmorDirectives(getDirectives(m_chestItem));
          mergeHumanoidConfig(m_chestItem);
          for (auto& item : m_chestItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<ChestArmor>(item)) {
                bool hidden = armourItem->hideInStockSlots();
                chestItems.emplace_back(hidden ? nullptr : armourItem);
                chestArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->bodyFrameset(humanoid.identity().gender),
                    hidden ? Directives("?scale=0") : getDirectives(armourItem),
                    Directives()});
                frontSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->frontSleeveFrameset(humanoid.identity().gender),
                    hidden ? Directives("?scale=0") : getDirectives(armourItem),
                    Directives()});
                backSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->backSleeveFrameset(humanoid.identity().gender),
                    hidden ? Directives("?scale=0") : getDirectives(armourItem),
                    Directives()});
                mergeHumanoidConfig(item);
                bodyHidden |= armourItem->hideBody();
              }
            }
          }
          // humanoid.setChestArmorStack(chestArmorStack);
          // humanoid.setFrontSleeveStack(frontSleeveStack);
          // humanoid.setBackSleeveStack(backSleeveStack);
          humanoid.setBackSleeveUnderlayFrameset("");
          humanoid.setFrontSleeveUnderlayFrameset("");
          humanoid.setChestArmorUnderlayFrameset("");
          humanoid.setChestArmorUnderlayStack({});
          humanoid.setFrontSleeveUnderlayStack({});
          humanoid.setBackSleeveUnderlayStack({});
        }
        bodyHidden = bodyHidden || m_chestItem->hideBody();
      } else {
        if (anyNeedsSync) {
          humanoid.setBackSleeveFrameset("");
          humanoid.setFrontSleeveFrameset("");
          humanoid.setChestArmorFrameset("");
          humanoid.setBackSleeveUnderlayFrameset("");
          humanoid.setFrontSleeveUnderlayFrameset("");
          humanoid.setChestArmorUnderlayFrameset("");
          // humanoid.setChestArmorStack(chestArmorStack);
          // humanoid.setFrontSleeveStack(frontSleeveStack);
          // humanoid.setBackSleeveStack(backSleeveStack);
          humanoid.setChestArmorUnderlayStack({});
          humanoid.setFrontSleeveUnderlayStack({});
          humanoid.setBackSleeveUnderlayStack({});
        }
      }

      if (m_legsCosmeticItem && !m_legsCosmeticItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          if (shouldShowArmour(m_legsCosmeticItem)) {
            humanoid.setLegsArmorFrameset(m_legsCosmeticItem->frameset(humanoid.identity().gender));
            humanoid.setLegsArmorDirectives(getDirectives(m_legsCosmeticItem));
            mergeHumanoidConfig(m_legsCosmeticItem);
            for (auto& item : m_legsCosmeticItem->getStackedCosmetics()) {
              if (item) {
                if (auto armourItem = as<LegsArmor>(item)) {
                  bool hidden = armourItem->hideInStockSlots();
                  legsItems.emplace_back(hidden ? nullptr : armourItem);
                  legsArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                      armourItem->frameset(humanoid.identity().gender),
                      hidden ? Directives("?scale=0") : getDirectives(armourItem),
                      Directives()});
                  mergeHumanoidConfig(item);
                  bodyHidden |= armourItem->hideBody();
                }
              }
            }
            // humanoid.setLegsArmorStack(legsArmorStack);
          } else {
            humanoid.setLegsArmorFrameset("");
          }
        }
        if (m_legsItem && !hiddenArmourSlots.legs && !m_legsItem->hideInStockSlots()) {
          if (anyNeedsSync) {
            if (m_legsItem->isUnderlaid() && shouldShowArmour(m_legsItem)) {
              humanoid.setLegsArmorUnderlayFrameset(m_legsItem->frameset(humanoid.identity().gender));
              humanoid.setLegsArmorUnderlayDirectives(getDirectives(m_legsItem));
              mergeHumanoidConfig(m_legsItem);
              List<Humanoid::ArmorEntry> legsArmorUnderlayStack = {};
              for (auto& item : m_legsItem->getStackedCosmetics()) {
                if (item) {
                  if (auto armourItem = as<LegsArmor>(item)) {
                    bool hidden = armourItem->hideInStockSlots();
                    legsArmorUnderlayStack.emplaceAppend(Humanoid::ArmorEntry{
                        armourItem->frameset(humanoid.identity().gender),
                        hidden ? Directives("?scale=0") : getDirectives(armourItem),
                        Directives()});
                    mergeHumanoidConfig(item);
                    bodyHidden |= armourItem->hideBody();
                  }
                }
              }
              humanoid.setLegsArmorUnderlayStack(legsArmorUnderlayStack);
            } else {
              humanoid.setLegsArmorUnderlayFrameset("");
              humanoid.setLegsArmorUnderlayStack({});
            }
          }
          bodyHidden = bodyHidden || (m_legsItem->isUnderlaid() && m_legsItem->hideBody());
        } else {
          if (anyNeedsSync) {
            humanoid.setLegsArmorUnderlayFrameset("");
            humanoid.setLegsArmorUnderlayStack({});
          }
        }
        bodyHidden = bodyHidden || m_legsCosmeticItem->hideBody();
      } else if (m_legsItem && shouldShowArmour(m_legsItem) && !hiddenArmourSlots.legs && !m_legsItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          humanoid.setLegsArmorFrameset(m_legsItem->frameset(humanoid.identity().gender));
          humanoid.setLegsArmorDirectives(getDirectives(m_legsItem));
          mergeHumanoidConfig(m_legsItem);
          for (auto& item : m_legsItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<LegsArmor>(item)) {
                bool hidden = armourItem->hideInStockSlots();
                legsItems.emplace_back(hidden ? nullptr : armourItem);
                legsArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->frameset(humanoid.identity().gender),
                    hidden ? Directives("?scale=0") : getDirectives(armourItem),
                    Directives()});
                mergeHumanoidConfig(item);
                bodyHidden |= armourItem->hideBody();
              }
            }
          }
          // humanoid.setLegsArmorStack(legsArmorStack);
          humanoid.setLegsArmorUnderlayFrameset("");
          humanoid.setLegsArmorUnderlayStack({});
        }
        bodyHidden = bodyHidden || m_legsItem->hideBody();
      } else {
        if (anyNeedsSync) {
          humanoid.setLegsArmorFrameset("");
          humanoid.setLegsArmorUnderlayFrameset("");
          // humanoid.setLegsArmorStack(legsArmorStack);
          humanoid.setLegsArmorUnderlayStack({});
        }
      }

      // FezzedOne: Staggers legs and chest armour items the same way they are visually staggered on xSB
      // when networking to oSB clients.
      size_t chestItemCount = chestItems.size(), legsItemCount = legsItems.size();
      size_t largerCount = std::max<size_t>(legsItemCount, chestItemCount);

      if (largerCount != 0) {
        for (size_t i = 0; i < largerCount; i++) {
          if (i < legsItems.size()) {
            if (m_player && openSbLayerCount < 12 && m_player->isMaster())
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(legsItems[i]));
          }
          if (openSbLayerCount >= 12) break;
          if (i < chestItems.size()) {
            if (m_player && openSbLayerCount < 12 && m_player->isMaster())
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(chestItems[i]));
          }
          if (openSbLayerCount >= 12) break;
        }
      }
    }

    if (m_backCosmeticItem && !m_backCosmeticItem->hideInStockSlots()) {
      if (anyNeedsSync) {
        if (shouldShowArmour(m_backCosmeticItem)) {
          humanoid.setBackArmorFrameset(m_backCosmeticItem->frameset(humanoid.identity().gender));
          humanoid.setBackArmorDirectives(getDirectives(m_backCosmeticItem));
          humanoid.setBackArmorHeadRotation(m_backCosmeticItem->rotateWithHead());
          mergeHumanoidConfig(m_backCosmeticItem);
          for (auto& item : m_backCosmeticItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<BackArmor>(item)) {
                bool hidden = armourItem->hideInStockSlots();
                if (m_player && openSbLayerCount < 12 && m_player->isMaster())
                  m_player->setNetArmorSecret(openSbLayerCount++, hidden ? nullptr : as<ArmorItem>(item));
                backArmorStack.emplaceAppend(Humanoid::BackEntry{
                    armourItem->frameset(humanoid.identity().gender),
                    hidden ? Directives("?scale=0") : getDirectives(armourItem),
                    Directives(),
                    armourItem->rotateWithHead()});
                mergeHumanoidConfig(item);
                bodyHidden |= armourItem->hideBody();
              }
            }
          }
          // humanoid.setBackArmorStack(backArmorStack);
        } else {
          humanoid.setBackArmorFrameset("");
          humanoid.setBackArmorHeadRotation(false);
        }
      }
      if (m_backItem && !hiddenArmourSlots.back && !m_backItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          if (m_backItem->isUnderlaid() && shouldShowArmour(m_backItem)) {
            humanoid.setBackArmorUnderlayFrameset(m_backItem->frameset(humanoid.identity().gender));
            humanoid.setBackArmorUnderlayDirectives(getDirectives(m_backItem));
            humanoid.setBackArmorUnderlayHeadRotation(m_backItem->rotateWithHead());
            mergeHumanoidConfig(m_backItem);
            List<Humanoid::BackEntry> backArmorUnderlayStack = {};
            for (auto& item : m_backItem->getStackedCosmetics()) {
              if (item) {
                if (auto armourItem = as<BackArmor>(item)) {
                  bool hidden = armourItem->hideInStockSlots();
                  backArmorUnderlayStack.emplaceAppend(Humanoid::BackEntry{
                      armourItem->frameset(humanoid.identity().gender),
                      hidden ? Directives("?scale=0") : getDirectives(armourItem),
                      Directives(),
                      armourItem->rotateWithHead()});
                  mergeHumanoidConfig(item);
                  bodyHidden |= armourItem->hideBody();
                }
              }
            }
            humanoid.setBackArmorUnderlayStack(backArmorUnderlayStack);
          } else {
            humanoid.setBackArmorUnderlayFrameset("");
            humanoid.setBackArmorUnderlayStack({});
            humanoid.setBackArmorUnderlayHeadRotation(false);
          }
        }
        bodyHidden = bodyHidden || (m_backItem->isUnderlaid() && m_backItem->hideBody());
      } else {
        if (anyNeedsSync) {
          humanoid.setBackArmorUnderlayFrameset("");
          humanoid.setBackArmorUnderlayStack({});
        }
      }
      bodyHidden = bodyHidden || m_backCosmeticItem->hideBody();
    } else if (m_backItem && shouldShowArmour(m_backItem) && !hiddenArmourSlots.back && !m_backItem->hideInStockSlots()) {
      if (anyNeedsSync) {
        humanoid.setBackArmorFrameset(m_backItem->frameset(humanoid.identity().gender));
        humanoid.setBackArmorDirectives(getDirectives(m_backItem));
        humanoid.setBackArmorHeadRotation(m_backItem->rotateWithHead());
        mergeHumanoidConfig(m_backItem);
        for (auto& item : m_backItem->getStackedCosmetics()) {
          if (item) {
            if (auto armourItem = as<BackArmor>(item)) {
              bool hidden = armourItem->hideInStockSlots();
              if (m_player && openSbLayerCount < 12 && m_player->isMaster())
                m_player->setNetArmorSecret(openSbLayerCount++, hidden ? nullptr : as<ArmorItem>(item));
              backArmorStack.emplaceAppend(Humanoid::BackEntry{
                  armourItem->frameset(humanoid.identity().gender),
                  hidden ? Directives("?scale=0") : getDirectives(armourItem),
                  Directives(),
                  armourItem->rotateWithHead()});
              mergeHumanoidConfig(item);
              bodyHidden |= armourItem->hideBody();
            }
          }
        }
        // humanoid.setBackArmorStack(backArmorStack);
        humanoid.setBackArmorUnderlayFrameset("");
        humanoid.setBackArmorUnderlayHeadRotation(false);
        humanoid.setBackArmorUnderlayStack({});
      }
      bodyHidden = bodyHidden || m_backItem->hideBody();
    } else {
      if (anyNeedsSync) {
        humanoid.setBackArmorFrameset("");
        humanoid.setBackArmorHeadRotation(false);
        humanoid.setBackArmorUnderlayFrameset("");
        humanoid.setBackArmorUnderlayHeadRotation(false);
        // humanoid.setBackArmorStack(backArmorStack);
        humanoid.setBackArmorUnderlayStack({});
      }
    }
  }

  if (m_player && m_player->isSlave() && anyNeedsSync) { // FezzedOne: Reads OpenStarbound cosmetic layers into xSB overlays.
    for (uint8_t i = 0; i != 12; i++) {
      auto& item = openSbCosmeticStack[i];
      if (!item) continue;
      if (auto armourItem = as<HeadArmor>(item)) {
        if (!shouldShowArmour(armourItem)) continue;
        headArmorStack.emplaceAppend(Humanoid::ArmorEntry{
            armourItem->frameset(humanoid.identity().gender),
            getDirectives(armourItem),
            armourItem->maskDirectives(),
            BaseCosmeticOrdering,
            true});
        mergeHumanoidConfig(item);
        bodyHidden |= armourItem->hideBody();
      } else if (auto armourItem = as<ChestArmor>(item)) {
        if (!shouldShowArmour(armourItem)) continue;
        bool hidden = armourItem->hideInStockSlots();
        chestArmorStack.emplaceAppend(Humanoid::ArmorEntry{
            armourItem->bodyFrameset(humanoid.identity().gender),
            getDirectives(armourItem),
            Directives(),
            i});
        frontSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
            armourItem->frontSleeveFrameset(humanoid.identity().gender),
            getDirectives(armourItem),
            Directives(),
            i});
        backSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
            armourItem->backSleeveFrameset(humanoid.identity().gender),
            getDirectives(armourItem),
            Directives(),
            i});
        mergeHumanoidConfig(item);
        bodyHidden |= armourItem->hideBody();
      } else if (auto armourItem = as<LegsArmor>(item)) {
        if (!shouldShowArmour(armourItem)) continue;
        bool hidden = armourItem->hideInStockSlots();
        legsArmorStack.emplaceAppend(Humanoid::ArmorEntry{
            armourItem->frameset(humanoid.identity().gender),
            getDirectives(armourItem),
            Directives(),
            i});
        mergeHumanoidConfig(item);
        bodyHidden |= armourItem->hideBody();
      } else if (auto armourItem = as<BackArmor>(item)) {
        if (!shouldShowArmour(armourItem)) continue;
        bool hidden = armourItem->hideInStockSlots();
        backArmorStack.emplaceAppend(Humanoid::BackEntry{
            armourItem->frameset(humanoid.identity().gender),
            getDirectives(armourItem),
            Directives(),
            armourItem->rotateWithHead()});
        mergeHumanoidConfig(item);
        bodyHidden |= armourItem->hideBody();
      }
    }
  }

  if (anyNeedsSync) {
    humanoid.setHeadArmorStack(headArmorStack);
    humanoid.setChestArmorStack(chestArmorStack);
    humanoid.setFrontSleeveStack(frontSleeveStack);
    humanoid.setBackSleeveStack(backSleeveStack);
    humanoid.setLegsArmorStack(legsArmorStack);
    humanoid.setBackArmorStack(backArmorStack);
    // FezzedOne: Clear any emulated OpenStarbound cosmetic slots after the last xStarbound overlay, if any cosmetic slots are left unfilled.
    if (m_player && m_player->isMaster() && openSbLayerCount < 12) {
      for (uint8_t i = openSbLayerCount; i != 12; i++) {
        m_player->setNetArmorSecret(i, nullptr);
      }
    }
  }

  m_headNeedsSync = m_chestNeedsSync = m_legsNeedsSync = m_backNeedsSync = false;

  if (anyNeedsSync) {
    try {
      humanoid.updateHumanoidConfigOverrides(humanoidOverrides);
    } catch (std::exception const& e) {
      Logger::warn("ArmorWearer: Exception caught while handling humanoid overrides; attempted to restore base humanoid config for player's species. "
                   "Check the \"humanoidConfig\" on your cosmetic items.\n  Exception: {}",
          outputException(e, false));
      humanoid.updateHumanoidConfigOverrides(JsonObject{});
    }
  }

  humanoid.setBodyHidden(bodyHidden);
}

void ArmorWearer::effects(EffectEmitter& effectEmitter) {
  if (auto item = as<EffectSourceItem>(m_headCosmeticItem))
    effectEmitter.addEffectSources("headArmor", item->effectSources());
  else if (auto item = as<EffectSourceItem>(m_headItem))
    effectEmitter.addEffectSources("headArmor", item->effectSources());

  if (auto item = as<EffectSourceItem>(m_chestCosmeticItem))
    effectEmitter.addEffectSources("chestArmor", item->effectSources());
  else if (auto item = as<EffectSourceItem>(m_chestItem))
    effectEmitter.addEffectSources("chestArmor", item->effectSources());

  if (auto item = as<EffectSourceItem>(m_legsCosmeticItem))
    effectEmitter.addEffectSources("legsArmor", item->effectSources());
  else if (auto item = as<EffectSourceItem>(m_legsItem))
    effectEmitter.addEffectSources("legsArmor", item->effectSources());

  if (auto item = as<EffectSourceItem>(m_backCosmeticItem))
    effectEmitter.addEffectSources("backArmor", item->effectSources());
  else if (auto item = as<EffectSourceItem>(m_backItem))
    effectEmitter.addEffectSources("backArmor", item->effectSources());
}

Json ArmorWearer::diskStore() const {
  JsonObject res;
  if (m_headItem)
    res["headItem"] = m_headItem->descriptor().diskStore();
  if (m_chestItem)
    res["chestItem"] = m_chestItem->descriptor().diskStore();
  if (m_legsItem)
    res["legsItem"] = m_legsItem->descriptor().diskStore();
  if (m_backItem)
    res["backItem"] = m_backItem->descriptor().diskStore();
  if (m_headCosmeticItem)
    res["headCosmeticItem"] = m_headCosmeticItem->descriptor().diskStore();
  if (m_chestCosmeticItem)
    res["chestCosmeticItem"] = m_chestCosmeticItem->descriptor().diskStore();
  if (m_legsCosmeticItem)
    res["legsCosmeticItem"] = m_legsCosmeticItem->descriptor().diskStore();
  if (m_backCosmeticItem)
    res["backCosmeticItem"] = m_backCosmeticItem->descriptor().diskStore();

  return res;
}

void ArmorWearer::diskLoad(Json const& diskStore) {
  auto itemDb = Root::singleton().itemDatabase();
  m_headItem = as<HeadArmor>(itemDb->diskLoad(diskStore.get("headItem", {})));
  m_chestItem = as<ChestArmor>(itemDb->diskLoad(diskStore.get("chestItem", {})));
  m_legsItem = as<LegsArmor>(itemDb->diskLoad(diskStore.get("legsItem", {})));
  m_backItem = as<BackArmor>(itemDb->diskLoad(diskStore.get("backItem", {})));
  m_headCosmeticItem = as<HeadArmor>(itemDb->diskLoad(diskStore.get("headCosmeticItem", {})));
  m_chestCosmeticItem = as<ChestArmor>(itemDb->diskLoad(diskStore.get("chestCosmeticItem", {})));
  m_legsCosmeticItem = as<LegsArmor>(itemDb->diskLoad(diskStore.get("legsCosmeticItem", {})));
  m_backCosmeticItem = as<BackArmor>(itemDb->diskLoad(diskStore.get("backCosmeticItem", {})));
}

List<PersistentStatusEffect> ArmorWearer::statusEffects() const {
  List<PersistentStatusEffect> statusEffects;
  auto addStatusFromItem = [&](ItemPtr const& item) {
    if (auto effectItem = as<StatusEffectItem>(item))
      statusEffects.appendAll(effectItem->statusEffects());
  };

  addStatusFromItem(m_headItem);
  addStatusFromItem(m_chestItem);
  addStatusFromItem(m_legsItem);
  addStatusFromItem(m_backItem);

  return statusEffects;
}

void ArmorWearer::reset() {
  // FezzedOne: {NOTE] Kae's fix for some visual issues with armour.
  m_headNeedsSync = m_chestNeedsSync = m_legsNeedsSync = m_backNeedsSync = true;
  m_headItem.reset();
  m_chestItem.reset();
  m_legsItem.reset();
  m_backItem.reset();
  m_headCosmeticItem.reset();
  m_chestCosmeticItem.reset();
  m_legsCosmeticItem.reset();
  m_backCosmeticItem.reset();
}

void ArmorWearer::setHeadItem(HeadArmorPtr headItem) {
  if (Item::itemsEqual(m_headItem, headItem))
    return;
  m_headItem = headItem;
  // m_headNeedsSync |= !m_headCosmeticItem;
  m_headNeedsSync = true;
}

void ArmorWearer::setHeadCosmeticItem(HeadArmorPtr headCosmeticItem) {
  if (Item::itemsEqual(m_headCosmeticItem, headCosmeticItem))
    return;
  m_headCosmeticItem = headCosmeticItem;
  m_headNeedsSync = true;
}

void ArmorWearer::setChestItem(ChestArmorPtr chestItem) {
  if (Item::itemsEqual(m_chestItem, chestItem))
    return;
  m_chestItem = chestItem;
  m_chestNeedsSync = true;
}

void ArmorWearer::setChestCosmeticItem(ChestArmorPtr chestCosmeticItem) {
  if (Item::itemsEqual(m_chestCosmeticItem, chestCosmeticItem))
    return;
  m_chestCosmeticItem = chestCosmeticItem;
  m_chestNeedsSync = true;
}

void ArmorWearer::setLegsItem(LegsArmorPtr legsItem) {
  if (Item::itemsEqual(m_legsItem, legsItem))
    return;
  m_legsItem = legsItem;
  m_legsNeedsSync = true;
}

void ArmorWearer::setLegsCosmeticItem(LegsArmorPtr legsCosmeticItem) {
  if (Item::itemsEqual(m_legsCosmeticItem, legsCosmeticItem))
    return;
  m_legsCosmeticItem = legsCosmeticItem;
  m_legsNeedsSync = true;
}

void ArmorWearer::setBackItem(BackArmorPtr backItem) {
  if (Item::itemsEqual(m_backItem, backItem))
    return;
  m_backItem = backItem;
  m_backNeedsSync = true;
}

void ArmorWearer::setBackCosmeticItem(BackArmorPtr backCosmeticItem) {
  if (Item::itemsEqual(m_backCosmeticItem, backCosmeticItem))
    return;
  m_backCosmeticItem = backCosmeticItem;
  m_backNeedsSync = true;
}

HeadArmorPtr ArmorWearer::headItem() const {
  return m_headItem;
}

HeadArmorPtr ArmorWearer::headCosmeticItem() const {
  return m_headCosmeticItem;
}

ChestArmorPtr ArmorWearer::chestItem() const {
  return m_chestItem;
}

ChestArmorPtr ArmorWearer::chestCosmeticItem() const {
  return m_chestCosmeticItem;
}

LegsArmorPtr ArmorWearer::legsItem() const {
  return m_legsItem;
}

LegsArmorPtr ArmorWearer::legsCosmeticItem() const {
  return m_legsCosmeticItem;
}

BackArmorPtr ArmorWearer::backItem() const {
  return m_backItem;
}

BackArmorPtr ArmorWearer::backCosmeticItem() const {
  return m_backCosmeticItem;
}

ItemDescriptor ArmorWearer::headItemDescriptor() const {
  if (m_headItem)
    return m_headItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::headCosmeticItemDescriptor() const {
  if (m_headCosmeticItem)
    return m_headCosmeticItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::chestItemDescriptor() const {
  if (m_chestItem)
    return m_chestItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::chestCosmeticItemDescriptor() const {
  if (m_chestCosmeticItem)
    return m_chestCosmeticItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::legsItemDescriptor() const {
  if (m_legsItem)
    return m_legsItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::legsCosmeticItemDescriptor() const {
  if (m_legsCosmeticItem)
    return m_legsCosmeticItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::backItemDescriptor() const {
  if (m_backItem)
    return m_backItem->descriptor();
  return {};
}

ItemDescriptor ArmorWearer::backCosmeticItemDescriptor() const {
  if (m_backCosmeticItem)
    return m_backCosmeticItem->descriptor();
  return {};
}

void ArmorWearer::netElementsNeedLoad(bool) {
  auto itemDatabase = Root::singleton().itemDatabase();

  if (m_headCosmeticItemDataNetState.pullUpdated())
    m_headNeedsSync = itemDatabase->loadItem(m_headCosmeticItemDataNetState.get(), m_headCosmeticItem) || m_headNeedsSync;
  if (m_chestCosmeticItemDataNetState.pullUpdated())
    m_chestNeedsSync = itemDatabase->loadItem(m_chestCosmeticItemDataNetState.get(), m_chestCosmeticItem) || m_chestNeedsSync;
  if (m_legsCosmeticItemDataNetState.pullUpdated())
    m_legsNeedsSync = itemDatabase->loadItem(m_legsCosmeticItemDataNetState.get(), m_legsCosmeticItem) || m_legsNeedsSync;
  if (m_backCosmeticItemDataNetState.pullUpdated())
    m_backNeedsSync = itemDatabase->loadItem(m_backCosmeticItemDataNetState.get(), m_backCosmeticItem) || m_backNeedsSync;

  if (m_headItemDataNetState.pullUpdated())
    m_headNeedsSync = (itemDatabase->loadItem(m_headItemDataNetState.get(), m_headItem)) || m_headNeedsSync;
  if (m_chestItemDataNetState.pullUpdated())
    m_chestNeedsSync = (itemDatabase->loadItem(m_chestItemDataNetState.get(), m_chestItem)) || m_chestNeedsSync;
  if (m_legsItemDataNetState.pullUpdated())
    m_legsNeedsSync = (itemDatabase->loadItem(m_legsItemDataNetState.get(), m_legsItem)) || m_legsNeedsSync;
  if (m_backItemDataNetState.pullUpdated())
    m_backNeedsSync = (itemDatabase->loadItem(m_backItemDataNetState.get(), m_backItem)) || m_backNeedsSync;
}

void ArmorWearer::netElementsNeedStore() {
  auto itemDatabase = Root::singleton().itemDatabase();
  auto checkItem = [](auto armourItem) {
    if (armourItem && !armourItem->hideInStockSlots())
      return armourItem;
    return (decltype(armourItem))nullptr;
  };

  m_headItemDataNetState.set(itemSafeDescriptor(checkItem(m_headItem)));
  m_chestItemDataNetState.set(itemSafeDescriptor(checkItem(m_chestItem)));
  m_legsItemDataNetState.set(itemSafeDescriptor(checkItem(m_legsItem)));
  m_backItemDataNetState.set(itemSafeDescriptor(checkItem(m_backItem)));

  m_headCosmeticItemDataNetState.set(itemSafeDescriptor(checkItem(m_headCosmeticItem)));
  m_chestCosmeticItemDataNetState.set(itemSafeDescriptor(checkItem(m_chestCosmeticItem)));
  m_legsCosmeticItemDataNetState.set(itemSafeDescriptor(checkItem(m_legsCosmeticItem)));
  m_backCosmeticItemDataNetState.set(itemSafeDescriptor(checkItem(m_backCosmeticItem)));
}

} // namespace Star
