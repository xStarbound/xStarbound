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
#include "StarNpc.hpp"
#include "StarObject.hpp"
#include "StarObjectDatabase.hpp"
#include "StarObjectItem.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerInventory.hpp"
#include "StarRoot.hpp"
#include "StarTools.hpp"
#include "StarWorld.hpp"

#if defined TRACY_ENABLE
#include "tracy/Tracy.hpp"
#else
#define ZoneScoped
#define ZoneScopedN(name)
#endif

namespace Star {

ItemDescriptor ArmorWearer::setUpArmourItemNetworking(StringMap<String> const& identityTags, StringMap<String> const& visualIdentityTags, StringMap<String> const& netIdentityTags,
    ArmorItemPtr const& armourItem, Direction facingDirection) {
  ZoneScoped;
  auto processDirectives = [&](ArmorItemPtr const& armourItem, JsonObject& processedDirectives, Maybe<String> const& directives, Maybe<String> const& flipDirectives) {
    bool facingLeft = facingDirection == Direction::Left;

    auto shouldUseOverrides = armourItem->instanceValue("directivesUseOverrides");
    StringMap<String> const& tagsToUse = shouldUseOverrides.isType(Json::Type::Bool) ? (shouldUseOverrides.toBool() ? visualIdentityTags : identityTags) : netIdentityTags;

    String const& species = tagsToUse.maybe("imagePath").value("human");
    String const& gender = tagsToUse.maybe("gender").value("male");
    auto jSpeciesDirectives = armourItem->instanceValue(species + "Directives"),
         jGenderDirectives = armourItem->instanceValue(gender + "Directives"),
         jFlippedSpeciesDirectives = armourItem->instanceValue(species + "FlipDirectives"),
         jFlippedGenderDirectives = armourItem->instanceValue(gender + "FlipDirectives");
    auto speciesDirectives = jSpeciesDirectives.isType(Json::Type::String) ? jSpeciesDirectives.toString() : "",
         genderDirectives = jGenderDirectives.isType(Json::Type::String) ? jGenderDirectives.toString() : "";
    auto flippedSpeciesDirectives = facingLeft && jFlippedSpeciesDirectives.isType(Json::Type::String) ? jFlippedSpeciesDirectives.toString() : speciesDirectives,
         flippedGenderDirectives = facingLeft && jFlippedGenderDirectives.isType(Json::Type::String) ? jFlippedGenderDirectives.toString() : genderDirectives;

    const StringMap<String> directiveTags{
        {"speciesDirectives", flippedSpeciesDirectives},
        {"genderDirectives", flippedGenderDirectives}};


    // FezzedOne: Network flip directives as normal directives, allowing stock clients to see flipping.
    if (facingLeft && flipDirectives) {
      processedDirectives["directives"] = flipDirectives->replaceTags(directiveTags, false).replaceTags(tagsToUse, false);
    } else if (directives) {
      processedDirectives["directives"] = directives->replaceTags(directiveTags, false).replaceTags(tagsToUse, false);
    }
    processedDirectives["flipDirectives"] = Json();
    processedDirectives["xSBdirectives"] = Json();
    processedDirectives["xSBflipDirectives"] = Json();
  };

  auto handleDirectiveTags = [&](ArmorItemPtr const& armourItem) -> ItemDescriptor {
    if (!armourItem) return ItemDescriptor();
    auto armourItemDesc = itemSafeDescriptor(as<Item>(armourItem));
    auto params = armourItemDesc.parameters();
    auto processedDirectives = JsonObject{};

    auto directives = armourItem->xSBdirectives();
    auto flipDirectives = armourItem->xSBflippedDirectives();

    processDirectives(armourItem, processedDirectives, directives, flipDirectives);

    armourItemDesc = armourItemDesc.applyParameters(processedDirectives);
    auto overlays = armourItem->getStackedCosmetics();

    if (!overlays.empty()) {
      JsonArray jOverlays{};
      for (auto& item : overlays) {
        if (auto armourOverlay = as<ArmorItem>(item)) {
          ItemDescriptor newItem = itemSafeDescriptor(item);
          if (auto params = newItem.parameters(); params.isType(Json::Type::Object)) {
            auto processedDirectives = JsonObject{};

            auto directives = armourOverlay->xSBdirectives();
            auto flipDirectives = armourOverlay->xSBflippedDirectives();

            processDirectives(armourOverlay, processedDirectives, directives, flipDirectives);

            // FezzedOne: Network flip directives as normal directives, allowing stock clients to see flipping.
            newItem = newItem.applyParameters(processedDirectives);
          }
          jOverlays.emplaceAppend(newItem.diskStore());
        }
      }
      armourItemDesc = armourItemDesc.applyParameters(JsonObject{{"stackedItems", jOverlays}});
    }

    return armourItemDesc;
  };

  if (armourItem && !armourItem->hideInStockSlots()) {
    ItemDescriptor desc;
    if (!identityTags.empty())
      desc = handleDirectiveTags(armourItem);
    else
      desc = itemSafeDescriptor(as<Item>(armourItem));
    return desc;
  }
  return ItemDescriptor();
}


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

  m_openSbOverrides = {};
  auto jOpenSbOverrides = Root::singleton().assets()->json("/humanoid.config").get("openSbOverrides", Json());
  if (jOpenSbOverrides.isType(Json::Type::Object)) {
    m_openSbOverrides = jOpenSbOverrides.toObject();
  }

  m_isOpenSb = false;
  m_warned = false;
}

ArmorWearer::ArmorWearer(bool isPlayer) : ArmorWearer() {
  m_isPlayer = isPlayer;
}

ArmorWearer::ArmorWearer(Player* player) : ArmorWearer(false) {
  m_player = player;
}

ArmorWearer::ArmorWearer(Npc* npc) : ArmorWearer(false) {
  m_npc = npc;
}

void ArmorWearer::setupHumanoidClothingDrawables(Humanoid& humanoid, bool forceNude, bool forceSync, Maybe<Direction> facingDirection) {
  ZoneScoped;
  bool nudeChanged = false;
  if (m_player) {
    nudeChanged = m_lastNude != forceNude;
    m_lastNude = forceNude;
  } else {
    m_lastNude = false;
  }

  uint8_t currentDirection = facingDirection ? (uint8_t)(*facingDirection) : (uint8_t)humanoid.facingDirection();
  bool directionChanged = m_lastFacingDirection != currentDirection;
  m_lastFacingDirection = currentDirection;

  bool headNeedsSync = m_headNeedsSync;
  bool chestNeedsSync = m_chestNeedsSync;
  bool legsNeedsSync = m_legsNeedsSync;
  bool backNeedsSync = m_backNeedsSync;

  bool anyNeedsSync = forceSync || headNeedsSync || chestNeedsSync || legsNeedsSync || backNeedsSync || nudeChanged;
  if (anyNeedsSync) {
    m_warned = false;
    netElementsNeedLoad(true);
  }

  bool bodyHidden = false;
  Json humanoidOverrides = JsonObject{};

  auto getDirectiveString = [](Json const& json, String const& key) -> String {
    Json jValue = json.opt(key).value(Json());
    return jValue.isType(Json::Type::String) ? jValue.toString() : "";
  };

  auto checkDirectiveString = [](Json const& json, String const& key) -> bool {
    Json jValue = json.opt(key).value(Json());
    return jValue.isType(Json::Type::String);
  };

  auto generateIdentityTags = [](HumanoidIdentity const& identity) -> StringMap<String> const {
    // FezzedOne: Substitution tags that can be used in both armour directives and overridden identity directives.
    // Directive tag substitutions are done internally by xClient and the post-substitution directives «replicated» to other clients in a vanilla-compatible manner,
    // so *any* client can see the automatically substituted directives.
    return StringMap<String>{
        {"species", identity.species},
        {"imagePath", identity.imagePath ? *identity.imagePath : identity.species},
        {"gender", identity.gender == Gender::Male ? "male" : "female"},
        {"bodyDirectives", identity.bodyDirectives.string()},
        {"emoteDirectives", identity.emoteDirectives.string()},
        {"hairGroup", identity.hairGroup},
        {"hairType", identity.hairType},
        {"hairDirectives", identity.hairDirectives.string()},
        {"facialHairGroup", identity.facialHairGroup},
        {"facialHairType", identity.facialHairType},
        {"facialHairDirectives", identity.facialHairDirectives.string()},
        {"facialMaskGroup", identity.facialMaskGroup},
        {"facialMaskType", identity.facialMaskType},
        {"facialMaskDirectives", identity.facialMaskDirectives.string()},
        {"personalityIdle", identity.personality.idle},
        {"personalityArmIdle", identity.personality.armIdle},
        {"headOffsetX", strf("{}", identity.personality.headOffset[0])},
        {"headOffsetY", strf("{}", identity.personality.headOffset[1])},
        {"armOffsetX", strf("{}", identity.personality.armOffset[0])},
        {"armOffsetY", strf("{}", identity.personality.armOffset[1])},
        {"color", Color::rgba(identity.color).toHex()}};
  };

  // FezzedOne: The base identity tags are generated before any cosmetics passes because items can't alter the base identity anyway.
  StringMap<String> const identityTags = generateIdentityTags(humanoid.identity());
  StringMap<String> visualIdentityTags = {};
  StringMap<String> netIdentityTags = {};

  auto replaceBaseTag = [&](String const& merger, String const& base) -> String {
    return merger.replaceTags(StringMap<String>{{"base", base}}, false);
  };

  auto mergeDirectives = [&](Json const& base, String const& key, Json& merger) {
    if (checkDirectiveString(merger, key)) {
      String value;
      if (checkDirectiveString(base, key))
        value = replaceBaseTag(getDirectiveString(merger, key), getDirectiveString(base, key));
      else
        value = getDirectiveString(merger, key);
      value = value.replaceTags(identityTags, false);
      merger = merger.set(key, value);
    }
  };

  // FezzedOne: Fixed «lag» in applying humanoid gender overrides. Requires multi-pass cosmetic processing.
  Gender newGender = humanoid.identity().gender;

  auto mergeHumanoidConfig = [&](auto armourItem, bool const skip) {
    if (skip) return;
    String gender = newGender == Gender::Male ? "male" : "female";
    Json configsToMerge[3] = {
        armourItem ? armourItem->instanceValue("humanoidConfig", Json()) : m_scriptedHumanoidConfig,
        armourItem ? armourItem->instanceValue(gender + "HumanoidConfig", Json()) : Json(),
        armourItem ? armourItem->instanceValue(humanoid.identity().species + "HumanoidConfig", Json()) : Json()};
    for (int i = 0; i < 3; i++) {
      if (configsToMerge[i].isType(Json::Type::Object)) {
        if (Json identityToMerge = configsToMerge[i].opt("identity").value(Json()); identityToMerge.isType(Json::Type::Object)) {
          Json jBaseIdentity = humanoidOverrides.opt("identity").value(Json());
          Json baseIdentity = jBaseIdentity.isType(Json::Type::Object) ? jBaseIdentity : JsonObject{};
          mergeDirectives(baseIdentity, "bodyDirectives", identityToMerge);
          mergeDirectives(baseIdentity, "hairDirectives", identityToMerge);
          mergeDirectives(baseIdentity, "emoteDirectives", identityToMerge);
          mergeDirectives(baseIdentity, "facialHairDirectives", identityToMerge);
          mergeDirectives(baseIdentity, "facialMaskDirectives", identityToMerge);
          baseIdentity = jsonMerge(baseIdentity, identityToMerge);
          humanoidOverrides = jsonMerge(humanoidOverrides, configsToMerge[i]);
          humanoidOverrides = humanoidOverrides.set("identity", baseIdentity);
          Json jNewGender = baseIdentity.opt("gender").value();
          if (jNewGender.isType(Json::Type::String)) {
            String newGenderStr = jNewGender.toString();
            if (newGenderStr.toLower() == "male" || newGenderStr.toLower() == "female")
              newGender = newGenderStr == "male" ? Gender::Male : Gender::Female;
          }
        } else {
          humanoidOverrides = jsonMerge(humanoidOverrides, configsToMerge[i]);
        }
      }
    }
  };

  auto getDirectives = [&](auto armourItem) -> Directives {
    // Check to ensure this isn't a remotely controlled player to avoid an unnecessary (and useless) tag replacement on items whose tags were already replaced by the other player's xClient client.
    // auto remotePlayer = m_player && m_player->isSlave();
    // if (!remotePlayer) {
    auto shouldUseOverrides = armourItem->instanceValue("directivesUseOverrides");
    StringMap<String> const& tagsToUse = shouldUseOverrides.isType(Json::Type::Bool) ? (shouldUseOverrides.toBool() ? visualIdentityTags : identityTags) : netIdentityTags;

    bool facingLeft = currentDirection == (uint8_t)Direction::Left;
    Maybe<String> directives = facingLeft ? armourItem->xSBflippedDirectives() : armourItem->xSBdirectives();
    if (directives) {
      String const& species = tagsToUse.maybe("imagePath").value("human");
      String const& gender = tagsToUse.maybe("gender").value("male");
      auto jSpeciesDirectives = armourItem->instanceValue(species + "Directives"),
           jGenderDirectives = armourItem->instanceValue(gender + "Directives"),
           jFlippedSpeciesDirectives = armourItem->instanceValue(species + "FlipDirectives"),
           jFlippedGenderDirectives = armourItem->instanceValue(gender + "FlipDirectives");
      auto speciesDirectives = jSpeciesDirectives.isType(Json::Type::String) ? jSpeciesDirectives.toString() : "",
           genderDirectives = jGenderDirectives.isType(Json::Type::String) ? jGenderDirectives.toString() : "";
      auto flippedSpeciesDirectives = facingLeft && jFlippedSpeciesDirectives.isType(Json::Type::String) ? jFlippedSpeciesDirectives.toString() : speciesDirectives,
           flippedGenderDirectives = facingLeft && jFlippedGenderDirectives.isType(Json::Type::String) ? jFlippedGenderDirectives.toString() : genderDirectives;

      const StringMap<String> directiveTags{
          {"speciesDirectives", flippedSpeciesDirectives},
          {"genderDirectives", flippedGenderDirectives}};

      return Directives(directives->replaceTags(directiveTags, false).replaceTags(tagsToUse, false));
    } else {
      return facingLeft ? armourItem->flippedDirectives() : armourItem->directives();
    }
    // } else {
    //   return currentDirection == (uint8_t)Direction::Left ? armourItem->flippedDirectives() : armourItem->directives();
    // }
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
      if (!item->hideInStockSlots()) {
        auto hiddenSlots = item->armorTypesToHide();
        hiddenArmourSlots.head |= hiddenSlots.head;
        hiddenArmourSlots.chest |= hiddenSlots.chest;
        hiddenArmourSlots.legs |= hiddenSlots.legs;
        hiddenArmourSlots.back |= hiddenSlots.back;
      }
    }
    if (auto& item = m_chestCosmeticItem) {
      if (!item->hideInStockSlots()) {
        auto hiddenSlots = item->armorTypesToHide();
        hiddenArmourSlots.head |= hiddenSlots.head;
        hiddenArmourSlots.chest |= hiddenSlots.chest;
        hiddenArmourSlots.legs |= hiddenSlots.legs;
        hiddenArmourSlots.back |= hiddenSlots.back;
      }
    }
    if (auto& item = m_legsCosmeticItem) {
      if (!item->hideInStockSlots()) {
        auto hiddenSlots = item->armorTypesToHide();
        hiddenArmourSlots.head |= hiddenSlots.head;
        hiddenArmourSlots.chest |= hiddenSlots.chest;
        hiddenArmourSlots.legs |= hiddenSlots.legs;
        hiddenArmourSlots.back |= hiddenSlots.back;
      }
    }
    if (auto& item = m_backCosmeticItem) {
      if (!item->hideInStockSlots()) {
        auto hiddenSlots = item->armorTypesToHide();
        hiddenArmourSlots.head |= hiddenSlots.head;
        hiddenArmourSlots.chest |= hiddenSlots.chest;
        hiddenArmourSlots.legs |= hiddenSlots.legs;
        hiddenArmourSlots.back |= hiddenSlots.back;
      }
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

  auto shouldShowArmour = [&](auto const& armour) -> bool {
    return (bool)m_npc || !forceNude || armour->bypassNudity();
  };

  mergeHumanoidConfig(ArmorItemPtr(nullptr), false);

  auto resolveCosmeticVisuals = [&](auto secondPassType) { // FezzedOne: The main armour updating code.
    // This does two passes now.
    // 1. Update humanoid overrides.
    // 2. Set up all armour/cosmetic framesets and directives, and network items to oSB clients.
    // The second pass is needed because cosmetic appearances can depend on the gender and species from humanoid overrides applied by the same or other cosmetics.

    constexpr bool secondPass = secondPassType.value;

    List<HeadArmorPtr> headItems;

    auto append = [secondPass](auto& stack, auto&& item) {
      if (secondPass) stack.emplace_back(item);
    };

    if (m_headCosmeticItem && !m_headCosmeticItem->hideInStockSlots()) {
      if (anyNeedsSync) {
        if (shouldShowArmour(m_headCosmeticItem)) {
          if (secondPass) {
            humanoid.setHeadArmorFrameset(m_headCosmeticItem->frameset(newGender));
            humanoid.setHeadArmorDirectives(getDirectives(m_headCosmeticItem));
            humanoid.setHelmetMaskDirectives(m_headCosmeticItem->maskDirectives());
          }
          mergeHumanoidConfig(m_headCosmeticItem, secondPass);
          for (auto& item : m_headCosmeticItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<HeadArmor>(item)) {
                bool hidden = armourItem->hideInStockSlots();
                if (m_player && m_player->isMaster())
                  append(headItems, hidden ? nullptr : armourItem);
                append(headArmorStack, Humanoid::ArmorEntry{
                                           armourItem->frameset(newGender),
                                           hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                           hidden ? Directives() : armourItem->maskDirectives()});
                mergeHumanoidConfig(item, secondPass);
                if (!secondPass) bodyHidden |= armourItem->hideBody();
              }
            }
          }
          // humanoid.setHeadArmorStack(headArmorStack);
        } else {
          if (secondPass) {
            humanoid.setHeadArmorFrameset("");
            humanoid.setHeadArmorDirectives("");
            humanoid.setHelmetMaskDirectives("");
          }
        }
      }
      if (m_headItem && !hiddenArmourSlots.head && !m_headItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          if (m_headItem->isUnderlaid() && shouldShowArmour(m_headItem)) {
            if (secondPass) {
              humanoid.setHeadArmorUnderlayFrameset(m_headItem->frameset(newGender));
              humanoid.setHeadArmorUnderlayDirectives(getDirectives(m_headItem));
              humanoid.setHelmetMaskUnderlayDirectives(m_headItem->maskDirectives());
            }
            mergeHumanoidConfig(m_headItem, secondPass);
            List<Humanoid::ArmorEntry> headUnderlayArmorStack = {};
            for (auto& item : m_headItem->getStackedCosmetics()) {
              if (item) {
                if (auto armourItem = as<HeadArmor>(item)) {
                  bool hidden = armourItem->hideInStockSlots();
                  append(headUnderlayArmorStack, Humanoid::ArmorEntry{
                                                     armourItem->frameset(newGender),
                                                     hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                                     hidden ? Directives() : armourItem->maskDirectives()});
                  mergeHumanoidConfig(item, secondPass);
                  if (!secondPass) bodyHidden |= armourItem->hideBody();
                }
              }
            }
            if (secondPass) {
              humanoid.setHeadArmorUnderlayStack(headUnderlayArmorStack);
            }
          } else {
            if (secondPass) {
              humanoid.setHeadArmorUnderlayFrameset("");
              humanoid.setHeadArmorUnderlayDirectives("");
              humanoid.setHelmetMaskUnderlayDirectives("");
              humanoid.setHeadArmorUnderlayStack({});
            }
          }
        }
        if (!secondPass) bodyHidden = bodyHidden || (m_headItem->isUnderlaid() && m_headItem->hideBody());
      } else {
        if (anyNeedsSync && secondPass) {
          humanoid.setHeadArmorUnderlayFrameset("");
          humanoid.setHeadArmorUnderlayDirectives("");
          humanoid.setHelmetMaskUnderlayDirectives("");
          humanoid.setHeadArmorUnderlayStack({});
        }
      }
      if (!secondPass) bodyHidden = bodyHidden || m_headCosmeticItem->hideBody();
    } else if (m_headItem && shouldShowArmour(m_headItem) && !hiddenArmourSlots.head && !m_headItem->hideInStockSlots()) {
      if (anyNeedsSync) {
        if (secondPass) {
          humanoid.setHeadArmorFrameset(m_headItem->frameset(newGender));
          humanoid.setHeadArmorDirectives(getDirectives(m_headItem));
          humanoid.setHelmetMaskDirectives(m_headItem->maskDirectives());
        }
        mergeHumanoidConfig(m_headItem, secondPass);
        for (auto& item : m_headItem->getStackedCosmetics()) {
          if (item) {
            if (auto armourItem = as<HeadArmor>(item)) {
              bool hidden = armourItem->hideInStockSlots();
              if (m_player && m_player->isMaster())
                append(headItems, hidden ? nullptr : armourItem);
              append(headArmorStack, Humanoid::ArmorEntry{
                                         armourItem->frameset(newGender),
                                         hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                         hidden ? Directives() : armourItem->maskDirectives()});
              mergeHumanoidConfig(item, secondPass);
              if (!secondPass) bodyHidden |= armourItem->hideBody();
            }
          }
        }
        // humanoid.setHeadArmorStack(headArmorStack);
        if (secondPass) {
          humanoid.setHeadArmorUnderlayFrameset("");
          humanoid.setHeadArmorUnderlayDirectives("");
          humanoid.setHelmetMaskUnderlayDirectives("");
          humanoid.setHeadArmorUnderlayStack({});
        }
      }
      if (!secondPass) bodyHidden = bodyHidden || m_headItem->hideBody();
    } else {
      if (anyNeedsSync && secondPass) {
        humanoid.setHeadArmorFrameset("");
        humanoid.setHeadArmorDirectives("");
        humanoid.setHelmetMaskDirectives("");
        humanoid.setHeadArmorUnderlayFrameset("");
        humanoid.setHeadArmorUnderlayDirectives("");
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
            if (secondPass) {
              humanoid.setBackSleeveFrameset(m_chestCosmeticItem->backSleeveFrameset(newGender));
              humanoid.setFrontSleeveFrameset(m_chestCosmeticItem->frontSleeveFrameset(newGender));
              humanoid.setChestArmorFrameset(m_chestCosmeticItem->bodyFrameset(newGender));
              humanoid.setChestArmorDirectives(getDirectives(m_chestCosmeticItem));
            }
            mergeHumanoidConfig(m_chestCosmeticItem, secondPass);
            for (auto& item : m_chestCosmeticItem->getStackedCosmetics()) {
              if (item) {
                if (auto armourItem = as<ChestArmor>(item)) {
                  bool hidden = armourItem->hideInStockSlots();
                  if (m_player && m_player->isMaster())
                    append(chestItems, hidden ? nullptr : armourItem);
                  append(chestArmorStack, Humanoid::ArmorEntry{
                                              armourItem->bodyFrameset(newGender),
                                              hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                              Directives()});
                  append(frontSleeveStack, Humanoid::ArmorEntry{
                                               armourItem->frontSleeveFrameset(newGender),
                                               hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                               Directives()});
                  append(backSleeveStack, Humanoid::ArmorEntry{
                                              armourItem->backSleeveFrameset(newGender),
                                              hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                              Directives()});
                  mergeHumanoidConfig(item, secondPass);
                  if (!secondPass) bodyHidden |= armourItem->hideBody();
                }
              }
            }
            // humanoid.setChestArmorStack(chestArmorStack);
            // humanoid.setFrontSleeveStack(frontSleeveStack);
            // humanoid.setBackSleeveStack(backSleeveStack);
          } else {
            if (secondPass) {
              humanoid.setBackSleeveFrameset("");
              humanoid.setFrontSleeveFrameset("");
              humanoid.setChestArmorFrameset("");
              humanoid.setChestArmorDirectives("");
            }
          }
        }
        if (m_chestItem && !hiddenArmourSlots.chest && !m_chestItem->hideInStockSlots()) {
          if (anyNeedsSync) {
            if (m_chestItem->isUnderlaid() && shouldShowArmour(m_chestItem)) {
              if (secondPass) {
                humanoid.setBackSleeveUnderlayFrameset(m_chestItem->backSleeveFrameset(newGender));
                humanoid.setFrontSleeveUnderlayFrameset(m_chestItem->frontSleeveFrameset(newGender));
                humanoid.setChestArmorUnderlayFrameset(m_chestItem->bodyFrameset(newGender));
                humanoid.setChestArmorUnderlayDirectives(getDirectives(m_chestItem));
              }
              mergeHumanoidConfig(m_chestItem, secondPass);
              List<Humanoid::ArmorEntry> chestArmorUnderlayStack = {}, frontSleeveUnderlayStack = {}, backSleeveUnderlayStack = {};
              for (auto& item : m_chestItem->getStackedCosmetics()) {
                if (item) {
                  if (auto armourItem = as<ChestArmor>(item)) {
                    bool hidden = armourItem->hideInStockSlots();
                    append(chestArmorUnderlayStack, Humanoid::ArmorEntry{
                                                        armourItem->bodyFrameset(newGender),
                                                        hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                                        Directives()});
                    append(frontSleeveUnderlayStack, Humanoid::ArmorEntry{
                                                         armourItem->frontSleeveFrameset(newGender),
                                                         hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                                         Directives()});
                    append(backSleeveUnderlayStack, Humanoid::ArmorEntry{
                                                        armourItem->backSleeveFrameset(newGender),
                                                        hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                                        Directives()});
                    mergeHumanoidConfig(item, secondPass);
                    if (!secondPass) bodyHidden |= armourItem->hideBody();
                  }
                }
              }
              if (secondPass) {
                humanoid.setChestArmorUnderlayStack(chestArmorUnderlayStack);
                humanoid.setFrontSleeveUnderlayStack(frontSleeveUnderlayStack);
                humanoid.setBackSleeveUnderlayStack(backSleeveUnderlayStack);
              }
            } else {
              if (secondPass) {
                humanoid.setBackSleeveUnderlayFrameset("");
                humanoid.setFrontSleeveUnderlayFrameset("");
                humanoid.setChestArmorUnderlayFrameset("");
                humanoid.setChestArmorUnderlayDirectives("");
                humanoid.setChestArmorUnderlayStack({});
                humanoid.setFrontSleeveUnderlayStack({});
                humanoid.setBackSleeveUnderlayStack({});
              }
            }
          }
          if (!secondPass) bodyHidden = bodyHidden || (m_chestItem->isUnderlaid() && m_chestItem->hideBody());
        } else {
          if (secondPass && anyNeedsSync) {
            humanoid.setBackSleeveUnderlayFrameset("");
            humanoid.setFrontSleeveUnderlayFrameset("");
            humanoid.setChestArmorUnderlayFrameset("");
            humanoid.setChestArmorUnderlayDirectives("");
            humanoid.setChestArmorUnderlayStack({});
            humanoid.setFrontSleeveUnderlayStack({});
            humanoid.setBackSleeveUnderlayStack({});
          }
        }
        if (!secondPass) bodyHidden = bodyHidden || m_chestCosmeticItem->hideBody();
      } else if (m_chestItem && shouldShowArmour(m_chestItem) && !hiddenArmourSlots.chest && !m_chestItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          if (secondPass) {
            humanoid.setBackSleeveFrameset(m_chestItem->backSleeveFrameset(newGender));
            humanoid.setFrontSleeveFrameset(m_chestItem->frontSleeveFrameset(newGender));
            humanoid.setChestArmorFrameset(m_chestItem->bodyFrameset(newGender));
            humanoid.setChestArmorDirectives(getDirectives(m_chestItem));
          }
          mergeHumanoidConfig(m_chestItem, secondPass);
          for (auto& item : m_chestItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<ChestArmor>(item)) {
                bool hidden = armourItem->hideInStockSlots();
                if (m_player && m_player->isMaster())
                  append(chestItems, hidden ? nullptr : armourItem);
                append(chestArmorStack, Humanoid::ArmorEntry{
                                            armourItem->bodyFrameset(newGender),
                                            hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                            Directives()});
                append(frontSleeveStack, Humanoid::ArmorEntry{
                                             armourItem->frontSleeveFrameset(newGender),
                                             hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                             Directives()});
                append(backSleeveStack, Humanoid::ArmorEntry{
                                            armourItem->backSleeveFrameset(newGender),
                                            hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                            Directives()});
                mergeHumanoidConfig(item, secondPass);
                if (!secondPass) bodyHidden |= armourItem->hideBody();
              }
            }
          }
          if (secondPass) {
            // humanoid.setChestArmorStack(chestArmorStack);
            // humanoid.setFrontSleeveStack(frontSleeveStack);
            // humanoid.setBackSleeveStack(backSleeveStack);
            humanoid.setBackSleeveUnderlayFrameset("");
            humanoid.setFrontSleeveUnderlayFrameset("");
            humanoid.setChestArmorUnderlayFrameset("");
            humanoid.setChestArmorUnderlayDirectives("");
            humanoid.setChestArmorUnderlayStack({});
            humanoid.setFrontSleeveUnderlayStack({});
            humanoid.setBackSleeveUnderlayStack({});
          }
        }
        if (!secondPass) bodyHidden = bodyHidden || m_chestItem->hideBody();
      } else {
        if (secondPass && anyNeedsSync) {
          humanoid.setBackSleeveFrameset("");
          humanoid.setFrontSleeveFrameset("");
          humanoid.setChestArmorFrameset("");
          humanoid.setChestArmorDirectives("");
          humanoid.setBackSleeveUnderlayFrameset("");
          humanoid.setFrontSleeveUnderlayFrameset("");
          humanoid.setChestArmorUnderlayFrameset("");
          humanoid.setChestArmorUnderlayDirectives("");
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
            if (secondPass) {
              humanoid.setLegsArmorFrameset(m_legsCosmeticItem->frameset(newGender));
              humanoid.setLegsArmorDirectives(getDirectives(m_legsCosmeticItem));
            }
            mergeHumanoidConfig(m_legsCosmeticItem, secondPass);
            for (auto& item : m_legsCosmeticItem->getStackedCosmetics()) {
              if (item) {
                if (auto armourItem = as<LegsArmor>(item)) {
                  bool hidden = armourItem->hideInStockSlots();
                  if (m_player && m_player->isMaster())
                    append(legsItems, hidden ? nullptr : armourItem);
                  append(legsArmorStack, Humanoid::ArmorEntry{
                                             armourItem->frameset(newGender),
                                             hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                             Directives()});
                  mergeHumanoidConfig(item, secondPass);
                  if (!secondPass) bodyHidden |= armourItem->hideBody();
                }
              }
            }
            // humanoid.setLegsArmorStack(legsArmorStack);
          } else {
            if (secondPass) {
              humanoid.setLegsArmorFrameset("");
              humanoid.setLegsArmorDirectives("");
            }
          }
        }
        if (m_legsItem && !hiddenArmourSlots.legs && !m_legsItem->hideInStockSlots()) {
          if (anyNeedsSync) {
            if (m_legsItem->isUnderlaid() && shouldShowArmour(m_legsItem)) {
              if (secondPass) {
                humanoid.setLegsArmorUnderlayFrameset(m_legsItem->frameset(newGender));
                humanoid.setLegsArmorUnderlayDirectives(getDirectives(m_legsItem));
              }
              mergeHumanoidConfig(m_legsItem, secondPass);
              List<Humanoid::ArmorEntry> legsArmorUnderlayStack = {};
              for (auto& item : m_legsItem->getStackedCosmetics()) {
                if (item) {
                  if (auto armourItem = as<LegsArmor>(item)) {
                    bool hidden = armourItem->hideInStockSlots();
                    append(legsArmorUnderlayStack, Humanoid::ArmorEntry{
                                                       armourItem->frameset(newGender),
                                                       hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                                       Directives()});
                    mergeHumanoidConfig(item, secondPass);
                    if (!secondPass) bodyHidden |= armourItem->hideBody();
                  }
                }
              }
              if (secondPass) {
                humanoid.setLegsArmorUnderlayStack(legsArmorUnderlayStack);
              }
            } else {
              if (secondPass) {
                humanoid.setLegsArmorUnderlayFrameset("");
                humanoid.setLegsArmorUnderlayDirectives("");
                humanoid.setLegsArmorUnderlayStack({});
              }
            }
          }
          if (!secondPass) bodyHidden = bodyHidden || (m_legsItem->isUnderlaid() && m_legsItem->hideBody());
        } else {
          if (secondPass && anyNeedsSync) {
            humanoid.setLegsArmorUnderlayFrameset("");
            humanoid.setLegsArmorUnderlayDirectives("");
            humanoid.setLegsArmorUnderlayStack({});
          }
        }
        if (!secondPass) bodyHidden = bodyHidden || m_legsCosmeticItem->hideBody();
      } else if (m_legsItem && shouldShowArmour(m_legsItem) && !hiddenArmourSlots.legs && !m_legsItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          if (secondPass) {
            humanoid.setLegsArmorFrameset(m_legsItem->frameset(newGender));
            humanoid.setLegsArmorDirectives(getDirectives(m_legsItem));
          }
          mergeHumanoidConfig(m_legsItem, secondPass);
          for (auto& item : m_legsItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<LegsArmor>(item)) {
                bool hidden = armourItem->hideInStockSlots();
                if (m_player && m_player->isMaster())
                  append(legsItems, hidden ? nullptr : armourItem);
                append(legsArmorStack, Humanoid::ArmorEntry{
                                           armourItem->frameset(newGender),
                                           hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                           Directives()});
                mergeHumanoidConfig(item, secondPass);
                if (!secondPass) bodyHidden |= armourItem->hideBody();
              }
            }
          }
          if (secondPass) {
            // humanoid.setLegsArmorStack(legsArmorStack);
            humanoid.setLegsArmorUnderlayFrameset("");
            humanoid.setLegsArmorUnderlayDirectives("");
            humanoid.setLegsArmorUnderlayStack({});
          }
        }
        if (!secondPass) bodyHidden = bodyHidden || m_legsItem->hideBody();
      } else {
        if (secondPass && anyNeedsSync) {
          humanoid.setLegsArmorFrameset("");
          humanoid.setLegsArmorDirectives("");
          humanoid.setLegsArmorUnderlayFrameset("");
          humanoid.setLegsArmorUnderlayDirectives("");
          // humanoid.setLegsArmorStack(legsArmorStack);
          humanoid.setLegsArmorUnderlayStack({});
        }
      }

      if (secondPass) {
        // FezzedOne: Staggers legs and chest armour items the same way they are visually staggered on xSB
        // when networking to oSB clients.
        size_t chestItemCount = chestItems.size(), legsItemCount = legsItems.size();
        size_t largerCount = std::max<size_t>(legsItemCount, chestItemCount);

        if (largerCount != 0) {
          for (size_t i = 0; i < largerCount; i++) {
            if (i < legsItems.size()) {
              if (m_player && openSbLayerCount < 12 && m_player->isMaster())
                m_player->setNetArmorSecret(identityTags, visualIdentityTags, netIdentityTags, openSbLayerCount++, as<ArmorItem>(legsItems[i]));
            }
            if (openSbLayerCount >= 12) break;
            if (i < chestItems.size()) {
              if (m_player && openSbLayerCount < 12 && m_player->isMaster())
                m_player->setNetArmorSecret(identityTags, visualIdentityTags, netIdentityTags, openSbLayerCount++, as<ArmorItem>(chestItems[i]));
            }
            if (openSbLayerCount >= 12) break;
          }
        }
      }
    }

    if (m_backCosmeticItem && !m_backCosmeticItem->hideInStockSlots()) {
      if (anyNeedsSync) {
        if (shouldShowArmour(m_backCosmeticItem)) {
          if (secondPass) {
            humanoid.setBackArmorFrameset(m_backCosmeticItem->frameset(newGender));
            humanoid.setBackArmorDirectives(getDirectives(m_backCosmeticItem));
            humanoid.setBackArmorHeadRotation(m_backCosmeticItem->rotateWithHead());
          }
          mergeHumanoidConfig(m_backCosmeticItem, secondPass);
          for (auto& item : m_backCosmeticItem->getStackedCosmetics()) {
            if (item) {
              if (auto armourItem = as<BackArmor>(item)) {
                bool hidden = armourItem->hideInStockSlots();
                if (secondPass && m_player && openSbLayerCount < 12 && m_player->isMaster())
                  m_player->setNetArmorSecret(identityTags, visualIdentityTags, netIdentityTags, openSbLayerCount++, hidden ? nullptr : as<ArmorItem>(item));
                append(backArmorStack, Humanoid::BackEntry{
                                           armourItem->frameset(newGender),
                                           hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                           Directives(),
                                           armourItem->rotateWithHead()});
                mergeHumanoidConfig(item, secondPass);
                if (!secondPass) bodyHidden |= armourItem->hideBody();
              }
            }
          }
          // humanoid.setBackArmorStack(backArmorStack);
        } else {
          if (secondPass) {
            humanoid.setBackArmorFrameset("");
            humanoid.setBackArmorDirectives("");
            humanoid.setBackArmorHeadRotation(false);
          }
        }
      }
      if (m_backItem && !hiddenArmourSlots.back && !m_backItem->hideInStockSlots()) {
        if (anyNeedsSync) {
          if (m_backItem->isUnderlaid() && shouldShowArmour(m_backItem)) {
            if (secondPass) {
              humanoid.setBackArmorUnderlayFrameset(m_backItem->frameset(newGender));
              humanoid.setBackArmorUnderlayDirectives(getDirectives(m_backItem));
              humanoid.setBackArmorUnderlayHeadRotation(m_backItem->rotateWithHead());
            }
            mergeHumanoidConfig(m_backItem, secondPass);
            List<Humanoid::BackEntry> backArmorUnderlayStack = {};
            for (auto& item : m_backItem->getStackedCosmetics()) {
              if (item) {
                if (auto armourItem = as<BackArmor>(item)) {
                  bool hidden = armourItem->hideInStockSlots();
                  append(backArmorUnderlayStack, Humanoid::BackEntry{
                                                     armourItem->frameset(newGender),
                                                     hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                                     Directives(),
                                                     armourItem->rotateWithHead()});
                  mergeHumanoidConfig(item, secondPass);
                  if (!secondPass) bodyHidden |= armourItem->hideBody();
                }
              }
            }
            if (secondPass) {
              humanoid.setBackArmorUnderlayStack(backArmorUnderlayStack);
            }
          } else {
            if (secondPass) {
              humanoid.setBackArmorUnderlayFrameset("");
              humanoid.setBackArmorUnderlayDirectives("");
              humanoid.setBackArmorUnderlayStack({});
              humanoid.setBackArmorUnderlayHeadRotation(false);
            }
          }
        }
        if (!secondPass) bodyHidden = bodyHidden || (m_backItem->isUnderlaid() && m_backItem->hideBody());
      } else {
        if (secondPass && anyNeedsSync) {
          humanoid.setBackArmorUnderlayFrameset("");
          humanoid.setBackArmorUnderlayDirectives("");
          humanoid.setBackArmorUnderlayStack({});
          humanoid.setBackArmorUnderlayHeadRotation(false);
        }
      }
      if (!secondPass) bodyHidden = bodyHidden || m_backCosmeticItem->hideBody();
    } else if (m_backItem && shouldShowArmour(m_backItem) && !hiddenArmourSlots.back && !m_backItem->hideInStockSlots()) {
      if (anyNeedsSync) {
        if (secondPass) {
          humanoid.setBackArmorFrameset(m_backItem->frameset(newGender));
          humanoid.setBackArmorDirectives(getDirectives(m_backItem));
          humanoid.setBackArmorHeadRotation(m_backItem->rotateWithHead());
        }
        mergeHumanoidConfig(m_backItem, secondPass);
        for (auto& item : m_backItem->getStackedCosmetics()) {
          if (item) {
            if (auto armourItem = as<BackArmor>(item)) {
              bool hidden = armourItem->hideInStockSlots();
              if (secondPass && m_player && openSbLayerCount < 12 && m_player->isMaster())
                m_player->setNetArmorSecret(identityTags, visualIdentityTags, netIdentityTags, openSbLayerCount++, hidden ? nullptr : as<ArmorItem>(item));
              append(backArmorStack, Humanoid::BackEntry{
                                         armourItem->frameset(newGender),
                                         hidden ? Directives("?scale=0") : getDirectives(armourItem),
                                         Directives(),
                                         armourItem->rotateWithHead()});
              mergeHumanoidConfig(item, secondPass);
              if (!secondPass) bodyHidden |= armourItem->hideBody();
            }
          }
        }
        // humanoid.setBackArmorStack(backArmorStack);
        if (secondPass) {
          humanoid.setBackArmorUnderlayFrameset("");
          humanoid.setBackArmorUnderlayDirectives("");
          humanoid.setBackArmorUnderlayHeadRotation(false);
          humanoid.setBackArmorUnderlayStack({});
        }
      }
      if (!secondPass) bodyHidden = bodyHidden || m_backItem->hideBody();
    } else {
      if (secondPass && anyNeedsSync) {
        humanoid.setBackArmorFrameset("");
        humanoid.setBackArmorDirectives("");
        humanoid.setBackArmorHeadRotation(false);
        humanoid.setBackArmorUnderlayFrameset("");
        humanoid.setBackArmorUnderlayDirectives("");
        humanoid.setBackArmorUnderlayHeadRotation(false);
        // humanoid.setBackArmorStack(backArmorStack);
        humanoid.setBackArmorUnderlayStack({});
      }
    }

    // Push head items last, to increase the chance they end up in the first four OpenStarbound slots, since the chest item quirk doesn't affect head or back items.
    size_t headItemCount = headItems.size();
    if (secondPass && headItemCount != 0) {
      for (size_t j = 0; j < headItemCount; j++) {
        if (m_player && openSbLayerCount < 12 && m_player->isMaster())
          m_player->setNetArmorSecret(identityTags, visualIdentityTags, netIdentityTags, openSbLayerCount++, as<ArmorItem>(headItems[j]));
      }
    }

    if (m_player && m_player->isSlave() && anyNeedsSync) { // FezzedOne: Reads OpenStarbound cosmetic layers into xSB overlays.
      for (uint8_t j = 0; j != 12; j++) {
        auto& item = openSbCosmeticStack[secondPass];
        if (!item) continue;
        if (auto armourItem = as<HeadArmor>(item)) {
          if (!shouldShowArmour(armourItem)) continue;
          append(headArmorStack, Humanoid::ArmorEntry{
                                     armourItem->frameset(newGender),
                                     getDirectives(armourItem),
                                     armourItem->maskDirectives(),
                                     BaseCosmeticOrdering,
                                     true});
          mergeHumanoidConfig(item, secondPass);
          if (!secondPass) bodyHidden |= armourItem->hideBody();
        } else if (auto armourItem = as<ChestArmor>(item)) {
          if (!shouldShowArmour(armourItem)) continue;
          append(chestArmorStack, Humanoid::ArmorEntry{
                                      armourItem->bodyFrameset(newGender),
                                      getDirectives(armourItem),
                                      Directives(),
                                      j});
          append(frontSleeveStack, Humanoid::ArmorEntry{
                                       armourItem->frontSleeveFrameset(newGender),
                                       getDirectives(armourItem),
                                       Directives(),
                                       j});
          append(backSleeveStack, Humanoid::ArmorEntry{
                                      armourItem->backSleeveFrameset(newGender),
                                      getDirectives(armourItem),
                                      Directives(),
                                      j});
          mergeHumanoidConfig(item, secondPass);
          if (!secondPass) bodyHidden |= armourItem->hideBody();
        } else if (auto armourItem = as<LegsArmor>(item)) {
          if (!shouldShowArmour(armourItem)) continue;
          append(legsArmorStack, Humanoid::ArmorEntry{
                                     armourItem->frameset(newGender),
                                     getDirectives(armourItem),
                                     Directives(),
                                     j});
          mergeHumanoidConfig(item, secondPass);
          if (!secondPass) bodyHidden |= armourItem->hideBody();
        } else if (auto armourItem = as<BackArmor>(item)) {
          if (!shouldShowArmour(armourItem)) continue;
          append(backArmorStack, Humanoid::BackEntry{
                                     armourItem->frameset(newGender),
                                     getDirectives(armourItem),
                                     Directives(),
                                     armourItem->rotateWithHead()});
          mergeHumanoidConfig(item, secondPass);
          if (!secondPass) bodyHidden |= armourItem->hideBody();
        }
      }
    }

    if (!secondPass) { // FezzedOne: Update overrides before the second pass. The critical step that requires two passes.
      if (anyNeedsSync) {
        try {
          humanoid.updateHumanoidConfigOverrides(humanoidOverrides, forceSync);
        } catch (std::exception const& e) {
          if (!m_warned) {
            Logger::warn("ArmorWearer: Exception caught while handling humanoid overrides; attempted to restore base humanoid config for player's species. "
                         "Check the \"humanoidConfig\" on your cosmetic items and any scripted humanoid overrides in scripts.\n  Exception: {}",
                outputException(e, false));
            m_warned = true;
          }
          humanoid.updateHumanoidConfigOverrides(JsonObject{}, true);
        }
      }

      humanoid.setBodyHidden(bodyHidden);

      // FezzedOne: These tags aren't generated until the end of the first pass because they need to take identity overrides into account.
      visualIdentityTags = generateIdentityTags(humanoid.visualIdentity());
      netIdentityTags = generateIdentityTags(humanoid.netIdentity());
    }
  };

  resolveCosmeticVisuals(std::false_type());
  resolveCosmeticVisuals(std::true_type());

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
        m_player->setNetArmorSecret(identityTags, visualIdentityTags, netIdentityTags, i, nullptr);
      }
    }
  }

  m_headNeedsSync = m_chestNeedsSync = m_legsNeedsSync = m_backNeedsSync = false;
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
  if ((m_npc && m_npc->isMaster()) || (m_player && m_player->isMaster())) return; // FezzedOne: Required sanity check. Mastered entities should not have enslaved armour wearers. `m_isPlayer` is actually a «is mastered» check here.

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

  auto generateIdentityTags = [](HumanoidIdentity const& identity) -> StringMap<String> const {
    return StringMap<String>{
        {"species", identity.species},
        {"imagePath", identity.imagePath ? *identity.imagePath : identity.species},
        {"gender", identity.gender == Gender::Male ? "male" : "female"},
        {"bodyDirectives", identity.bodyDirectives.string()},
        {"emoteDirectives", identity.emoteDirectives.string()},
        {"hairGroup", identity.hairGroup},
        {"hairType", identity.hairType},
        {"hairDirectives", identity.hairDirectives.string()},
        {"facialHairGroup", identity.facialHairGroup},
        {"facialHairType", identity.facialHairType},
        {"facialHairDirectives", identity.facialHairDirectives.string()},
        {"facialMaskGroup", identity.facialMaskGroup},
        {"facialMaskType", identity.facialMaskType},
        {"facialMaskDirectives", identity.facialMaskDirectives.string()},
        {"personalityIdle", identity.personality.idle},
        {"personalityArmIdle", identity.personality.armIdle},
        {"headOffsetX", strf("{}", identity.personality.headOffset[0])},
        {"headOffsetY", strf("{}", identity.personality.headOffset[1])},
        {"armOffsetX", strf("{}", identity.personality.armOffset[0])},
        {"armOffsetY", strf("{}", identity.personality.armOffset[1])},
        {"color", Color::rgba(identity.color).toHex()}};
  };

  if (m_player) {
    auto const identityTags = generateIdentityTags(m_player->humanoid()->identity());
    auto const visualIdentityTags = generateIdentityTags(m_player->humanoid()->visualIdentity());
    auto const netIdentityTags = generateIdentityTags(m_player->humanoid()->netIdentity());
    // FezzedOne: Substitution tags that can be used in both armour directives and overridden identity directives.
    // Directive tag substitutions are done internally by xClient and the post-substitution directives «replicated» to other clients in a vanilla-compatible manner,
    // so *any* client can see the automatically substituted directives.

    m_headItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_headItem), (Direction)m_lastFacingDirection));
    m_chestItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_chestItem), (Direction)m_lastFacingDirection));
    m_legsItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_legsItem), (Direction)m_lastFacingDirection));
    m_backItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_backItem), (Direction)m_lastFacingDirection));

    m_headCosmeticItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_headCosmeticItem), (Direction)m_lastFacingDirection));
    m_chestCosmeticItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_chestCosmeticItem), (Direction)m_lastFacingDirection));
    m_legsCosmeticItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_legsCosmeticItem), (Direction)m_lastFacingDirection));
    m_backCosmeticItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_backCosmeticItem), (Direction)m_lastFacingDirection));
  } else if (m_npc) {
    auto const identityTags = generateIdentityTags(m_npc->humanoid().identity());
    auto const visualIdentityTags = generateIdentityTags(m_npc->humanoid().visualIdentity());
    auto const netIdentityTags = generateIdentityTags(m_npc->humanoid().netIdentity());

    m_headItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_headItem), (Direction)m_lastFacingDirection));
    m_chestItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_chestItem), (Direction)m_lastFacingDirection));
    m_legsItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_legsItem), (Direction)m_lastFacingDirection));
    m_backItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_backItem), (Direction)m_lastFacingDirection));

    m_headCosmeticItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_headCosmeticItem), (Direction)m_lastFacingDirection));
    m_chestCosmeticItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_chestCosmeticItem), (Direction)m_lastFacingDirection));
    m_legsCosmeticItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_legsCosmeticItem), (Direction)m_lastFacingDirection));
    m_backCosmeticItemDataNetState.set(setUpArmourItemNetworking(identityTags, visualIdentityTags, netIdentityTags, as<ArmorItem>(m_backCosmeticItem), (Direction)m_lastFacingDirection));
  }
}

void ArmorWearer::setScriptedHumanoidConfig(Json const& newConfig) {
  m_scriptedHumanoidConfig = newConfig;
}

Json ArmorWearer::scriptedHumanoidConfig() const {
  return m_scriptedHumanoidConfig;
}

} // namespace Star
