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

  auto mergeHumanoidConfig = [&](ArmorItemPtr armourItem) {
    Json configToMerge = armourItem->instanceValue("humanoidConfig", Json());
    if (configToMerge.isType(Json::Type::Object))
      humanoidOverrides = jsonMerge(humanoidOverrides, configToMerge);
  };

  auto getDirectives = [&](ArmorItemPtr armourItem) -> Directives {
    return currentDirection == (uint8_t)Direction::Left ? armourItem->flippedDirectives() : armourItem->directives();
  };

  List<Humanoid::ArmorEntry> headArmorStack = {};
  List<Humanoid::ArmorEntry> chestArmorStack = {};
  List<Humanoid::ArmorEntry> frontSleeveStack = {};
  List<Humanoid::ArmorEntry> backSleeveStack = {};
  List<Humanoid::ArmorEntry> legsArmorStack = {};
  List<Humanoid::BackEntry> backArmorStack = {};

  anyNeedsSync |= directionChanged;

  uint8_t openSbLayerCount = 0;

  if (m_headCosmeticItem && !forceNude) {
    if (anyNeedsSync) {
      humanoid.setHeadArmorFrameset(m_headCosmeticItem->frameset(humanoid.identity().gender));
      humanoid.setHeadArmorDirectives(getDirectives(as<ArmorItem>(m_headCosmeticItem)));
      humanoid.setHelmetMaskDirectives(m_headCosmeticItem->maskDirectives());
      mergeHumanoidConfig(as<ArmorItem>(m_headCosmeticItem));
      for (auto& item : m_headCosmeticItem->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<HeadArmor>(item)) {
            if (m_player and openSbLayerCount < 12)
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(item));
            headArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->frameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                armourItem->maskDirectives()});
            mergeHumanoidConfig(as<ArmorItem>(item));
          }
        }
      }
      // humanoid.setHeadArmorStack(headArmorStack);
    }
    if (m_headItem) {
      if (anyNeedsSync) {
        if (m_headItem->isUnderlaid()) {
          humanoid.setHeadArmorUnderlayFrameset(m_headItem->frameset(humanoid.identity().gender));
          humanoid.setHeadArmorUnderlayDirectives(getDirectives(as<ArmorItem>(m_headItem)));
          humanoid.setHelmetMaskUnderlayDirectives(m_headItem->maskDirectives());
          mergeHumanoidConfig(as<ArmorItem>(m_headItem));
          List<Humanoid::ArmorEntry> headUnderlayArmorStack = {};
          for (auto& item : m_headItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<HeadArmor>(item)) {
                headUnderlayArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->frameset(humanoid.identity().gender),
                    getDirectives(as<ArmorItem>(armourItem)),
                    armourItem->maskDirectives()});
                mergeHumanoidConfig(as<ArmorItem>(item));
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
  } else if (m_headItem && !forceNude) {
    if (anyNeedsSync) {
      humanoid.setHeadArmorFrameset(m_headItem->frameset(humanoid.identity().gender));
      humanoid.setHeadArmorDirectives(getDirectives(as<ArmorItem>(m_headItem)));
      humanoid.setHelmetMaskDirectives(m_headItem->maskDirectives());
      mergeHumanoidConfig(as<ArmorItem>(m_headItem));
      for (auto& item : m_headItem->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<HeadArmor>(item)) {
            if (m_player and openSbLayerCount < 12)
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(item));
            headArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->frameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                armourItem->maskDirectives()});
            mergeHumanoidConfig(as<ArmorItem>(item));
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

  if (m_chestCosmeticItem && !forceNude) {
    if (anyNeedsSync) {
      humanoid.setBackSleeveFrameset(m_chestCosmeticItem->backSleeveFrameset(humanoid.identity().gender));
      humanoid.setFrontSleeveFrameset(m_chestCosmeticItem->frontSleeveFrameset(humanoid.identity().gender));
      humanoid.setChestArmorFrameset(m_chestCosmeticItem->bodyFrameset(humanoid.identity().gender));
      humanoid.setChestArmorDirectives(getDirectives(as<ArmorItem>(m_chestCosmeticItem)));
      mergeHumanoidConfig(as<ArmorItem>(m_chestCosmeticItem));
      for (auto& item : m_chestCosmeticItem->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<ChestArmor>(item)) {
            if (m_player and openSbLayerCount < 12)
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(item));
            chestArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->bodyFrameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives()});
            frontSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->frontSleeveFrameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives()});
            backSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->backSleeveFrameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives()});
            mergeHumanoidConfig(as<ArmorItem>(item));
          }
        }
      }
      // humanoid.setChestArmorStack(chestArmorStack);
      // humanoid.setFrontSleeveStack(frontSleeveStack);
      // humanoid.setBackSleeveStack(backSleeveStack);
    }
    if (m_chestItem) {
      if (anyNeedsSync) {
        if (m_chestItem->isUnderlaid()) {
          humanoid.setBackSleeveUnderlayFrameset(m_chestItem->backSleeveFrameset(humanoid.identity().gender));
          humanoid.setFrontSleeveUnderlayFrameset(m_chestItem->frontSleeveFrameset(humanoid.identity().gender));
          humanoid.setChestArmorUnderlayFrameset(m_chestItem->bodyFrameset(humanoid.identity().gender));
          humanoid.setChestArmorUnderlayDirectives(getDirectives(as<ArmorItem>(m_chestItem)));
          mergeHumanoidConfig(as<ArmorItem>(m_chestItem));
          List<Humanoid::ArmorEntry> chestArmorUnderlayStack = {}, frontSleeveUnderlayStack = {}, backSleeveUnderlayStack = {};
          for (auto& item : m_chestItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<ChestArmor>(item)) {
                chestArmorUnderlayStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->bodyFrameset(humanoid.identity().gender),
                    getDirectives(as<ArmorItem>(armourItem)),
                    Directives()});
                frontSleeveUnderlayStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->frontSleeveFrameset(humanoid.identity().gender),
                    getDirectives(as<ArmorItem>(armourItem)),
                    Directives()});
                backSleeveUnderlayStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->backSleeveFrameset(humanoid.identity().gender),
                    getDirectives(as<ArmorItem>(armourItem)),
                    Directives()});
                mergeHumanoidConfig(as<ArmorItem>(item));
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
  } else if (m_chestItem && !forceNude) {
    if (anyNeedsSync) {
      humanoid.setBackSleeveFrameset(m_chestItem->backSleeveFrameset(humanoid.identity().gender));
      humanoid.setFrontSleeveFrameset(m_chestItem->frontSleeveFrameset(humanoid.identity().gender));
      humanoid.setChestArmorFrameset(m_chestItem->bodyFrameset(humanoid.identity().gender));
      humanoid.setChestArmorDirectives(getDirectives(as<ArmorItem>(m_chestItem)));
      mergeHumanoidConfig(as<ArmorItem>(m_chestItem));
      for (auto& item : m_chestItem->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<ChestArmor>(item)) {
            if (m_player and openSbLayerCount < 12)
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(item));
            chestArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->bodyFrameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives()});
            frontSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->frontSleeveFrameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives()});
            backSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->backSleeveFrameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives()});
            mergeHumanoidConfig(as<ArmorItem>(item));
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

  if (m_legsCosmeticItem && !forceNude) {
    if (anyNeedsSync) {
      humanoid.setLegsArmorFrameset(m_legsCosmeticItem->frameset(humanoid.identity().gender));
      humanoid.setLegsArmorDirectives(getDirectives(as<ArmorItem>(m_legsCosmeticItem)));
      mergeHumanoidConfig(as<ArmorItem>(m_legsCosmeticItem));
      for (auto& item : m_legsCosmeticItem->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<LegsArmor>(item)) {
            if (m_player and openSbLayerCount < 12)
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(item));
            legsArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->frameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives()});
            mergeHumanoidConfig(as<ArmorItem>(item));
          }
        }
      }
      // humanoid.setLegsArmorStack(legsArmorStack);
    }
    if (m_legsItem) {
      if (anyNeedsSync) {
        if (m_legsItem->isUnderlaid()) {
          humanoid.setLegsArmorUnderlayFrameset(m_legsItem->frameset(humanoid.identity().gender));
          humanoid.setLegsArmorUnderlayDirectives(getDirectives(as<ArmorItem>(m_legsItem)));
          mergeHumanoidConfig(as<ArmorItem>(m_legsItem));
          List<Humanoid::ArmorEntry> legsArmorUnderlayStack = {};
          for (auto& item : m_legsItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<LegsArmor>(item)) {
                legsArmorUnderlayStack.emplaceAppend(Humanoid::ArmorEntry{
                    armourItem->frameset(humanoid.identity().gender),
                    getDirectives(as<ArmorItem>(armourItem)),
                    Directives()});
                mergeHumanoidConfig(as<ArmorItem>(item));
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
  } else if (m_legsItem && !forceNude) {
    if (anyNeedsSync) {
      humanoid.setLegsArmorFrameset(m_legsItem->frameset(humanoid.identity().gender));
      humanoid.setLegsArmorDirectives(getDirectives(as<ArmorItem>(m_legsItem)));
      mergeHumanoidConfig(as<ArmorItem>(m_legsItem));
      for (auto& item : m_legsItem->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<LegsArmor>(item)) {
            if (m_player and openSbLayerCount < 12)
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(item));
            legsArmorStack.emplaceAppend(Humanoid::ArmorEntry{
                armourItem->frameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives()});
            mergeHumanoidConfig(as<ArmorItem>(item));
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

  if (m_backCosmeticItem && !forceNude) {
    if (anyNeedsSync) {
      humanoid.setBackArmorFrameset(m_backCosmeticItem->frameset(humanoid.identity().gender));
      humanoid.setBackArmorDirectives(getDirectives(as<ArmorItem>(m_backCosmeticItem)));
      humanoid.setBackArmorHeadRotation(m_backCosmeticItem->rotateWithHead());
      mergeHumanoidConfig(as<ArmorItem>(m_backCosmeticItem));
      for (auto& item : m_backCosmeticItem->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<BackArmor>(item)) {
            if (m_player and openSbLayerCount < 12)
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(item));
            backArmorStack.emplaceAppend(Humanoid::BackEntry{
                armourItem->frameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives(),
                armourItem->rotateWithHead()});
            mergeHumanoidConfig(as<ArmorItem>(item));
          }
        }
      }
      // humanoid.setBackArmorStack(backArmorStack);
    }
    if (m_backItem) {
      if (anyNeedsSync) {
        if (m_backItem->isUnderlaid()) {
          humanoid.setBackArmorUnderlayFrameset(m_backItem->frameset(humanoid.identity().gender));
          humanoid.setBackArmorUnderlayDirectives(getDirectives(as<ArmorItem>(m_backItem)));
          humanoid.setBackArmorUnderlayHeadRotation(m_backItem->rotateWithHead());
          mergeHumanoidConfig(as<ArmorItem>(m_backItem));
          List<Humanoid::BackEntry> backArmorUnderlayStack = {};
          for (auto& item : m_backItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<BackArmor>(item)) {
                backArmorUnderlayStack.emplaceAppend(Humanoid::BackEntry{
                    armourItem->frameset(humanoid.identity().gender),
                    getDirectives(as<ArmorItem>(armourItem)),
                    Directives(),
                    armourItem->rotateWithHead()});
                mergeHumanoidConfig(as<ArmorItem>(item));
              }
            }
          }
          humanoid.setBackArmorUnderlayStack(backArmorUnderlayStack);
        } else {
          humanoid.setBackArmorUnderlayFrameset("");
          humanoid.setBackArmorUnderlayStack({});
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
  } else if (m_backItem && !forceNude) {
    if (anyNeedsSync) {
      humanoid.setBackArmorFrameset(m_backItem->frameset(humanoid.identity().gender));
      humanoid.setBackArmorDirectives(getDirectives(as<ArmorItem>(m_backItem)));
      humanoid.setBackArmorHeadRotation(m_backItem->rotateWithHead());
      mergeHumanoidConfig(as<ArmorItem>(m_backItem));
      for (auto& item : m_backItem->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<BackArmor>(item)) {
            if (m_player and openSbLayerCount < 12)
              m_player->setNetArmorSecret(openSbLayerCount++, as<ArmorItem>(item));
            backArmorStack.emplaceAppend(Humanoid::BackEntry{
                armourItem->frameset(humanoid.identity().gender),
                getDirectives(as<ArmorItem>(armourItem)),
                Directives(),
                armourItem->rotateWithHead()});
            mergeHumanoidConfig(as<ArmorItem>(item));
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

  if (m_player) {
    if (m_player->isSlave() && m_player->pulledCosmeticUpdate()) { // FezzedOne: Reads OpenStarbound cosmetic layers into xSB overlays.
      anyNeedsSync = true;
      auto& openSbCosmeticStack = m_player->getNetArmorSecrets();
      for (uint8_t i = 0; i != 12; i++) {
        auto& item = openSbCosmeticStack[i];
        if (!item) continue;
        if (auto armourItem = as<HeadArmor>(item)) {
          headArmorStack.emplaceAppend(Humanoid::ArmorEntry{
              armourItem->frameset(humanoid.identity().gender),
              getDirectives(as<ArmorItem>(armourItem)),
              armourItem->maskDirectives()});
          mergeHumanoidConfig(as<ArmorItem>(item));
        } else if (auto armourItem = as<ChestArmor>(item)) {
          chestArmorStack.emplaceAppend(Humanoid::ArmorEntry{
              armourItem->bodyFrameset(humanoid.identity().gender),
              getDirectives(as<ArmorItem>(armourItem)),
              Directives()});
          frontSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
              armourItem->frontSleeveFrameset(humanoid.identity().gender),
              getDirectives(as<ArmorItem>(armourItem)),
              Directives()});
          backSleeveStack.emplaceAppend(Humanoid::ArmorEntry{
              armourItem->backSleeveFrameset(humanoid.identity().gender),
              getDirectives(as<ArmorItem>(armourItem)),
              Directives()});
          mergeHumanoidConfig(as<ArmorItem>(item));
        } else if (auto armourItem = as<LegsArmor>(item)) {
          legsArmorStack.emplaceAppend(Humanoid::ArmorEntry{
              armourItem->frameset(humanoid.identity().gender),
              getDirectives(as<ArmorItem>(armourItem)),
              Directives()});
          mergeHumanoidConfig(as<ArmorItem>(item));
        } else if (auto armourItem = as<BackArmor>(item)) {
          backArmorStack.emplaceAppend(Humanoid::BackEntry{
              armourItem->frameset(humanoid.identity().gender),
              getDirectives(as<ArmorItem>(armourItem)),
              Directives(),
              armourItem->rotateWithHead()});
          mergeHumanoidConfig(as<ArmorItem>(item));
        }
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
    humanoid.updateHumanoidConfigOverrides(humanoidOverrides);
    // FezzedOne: Clear any emulated OpenStarbound cosmetic slots after the last xStarbound overlay, if any cosmetic slots are left unfilled.
    if (m_player && m_player->isMaster() && openSbLayerCount < 12) {
      for (uint8_t i = openSbLayerCount; i != 12; i++) {
        m_player->setNetArmorSecret(i, nullptr);
      }
    }
  }

  m_headNeedsSync = m_chestNeedsSync = m_legsNeedsSync = m_backNeedsSync = false;

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
  m_headItemDataNetState.set(itemSafeDescriptor(m_headItem));
  m_chestItemDataNetState.set(itemSafeDescriptor(m_chestItem));
  m_legsItemDataNetState.set(itemSafeDescriptor(m_legsItem));
  m_backItemDataNetState.set(itemSafeDescriptor(m_backItem));

  m_headCosmeticItemDataNetState.set(itemSafeDescriptor(m_headCosmeticItem));
  m_chestCosmeticItemDataNetState.set(itemSafeDescriptor(m_chestCosmeticItem));
  m_legsCosmeticItemDataNetState.set(itemSafeDescriptor(m_legsCosmeticItem));
  m_backCosmeticItemDataNetState.set(itemSafeDescriptor(m_backCosmeticItem));
}

} // namespace Star
