#include "StarPlayerLuaBindings.hpp"
#include "StarClientContext.hpp"
#include "StarItem.hpp"
#include "StarItemDatabase.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerInventory.hpp"
#include "StarPlayerTech.hpp"
#include "StarPlayerLog.hpp"
#include "StarQuestManager.hpp"
#include "StarWarping.hpp"
#include "StarStatistics.hpp"
#include "StarPlayerUniverseMap.hpp"
#include "StarJsonExtra.hpp"
#include "StarUniverseClient.hpp"

namespace Star {

LuaCallbacks LuaBindings::makePlayerCallbacks(Player* player) {
  LuaCallbacks callbacks;

  // FezzedOne: `"save"` works identically to `"getPlayerJson"`, and is here for OpenSB compatibility.
  callbacks.registerCallback("save", [player]() -> Json { return player->diskStore(); });
  callbacks.registerCallback("load", [player](Json const& data) {
    auto saved = player->diskStore();
    try { player->diskLoad(data); }
    catch (StarException const&) {
      player->diskLoad(saved);
      throw;
    }
  });

  // FezzedOne: Make sure these callbacks work identically to `"getIdentity"`/`"identity"` and `"setIdentity"`, respectively.
  // Don't need player file corruption. They're here for OpenSB compatibility, of course.
  callbacks.registerCallback(   "humanoidIdentity", [player]()         { return player->getIdentity();  });
  callbacks.registerCallback("setHumanoidIdentity", [player](Json const& id) {  player->setIdentity(id); });

  callbacks.registerCallback(   "bodyDirectives", [player]()   { return player->identity().bodyDirectives;      });
  callbacks.registerCallback("setBodyDirectives", [player](String const& str) { player->setBodyDirectives(str); });

  callbacks.registerCallback(   "emoteDirectives", [player]()   { return player->identity().emoteDirectives;      });
  callbacks.registerCallback("setEmoteDirectives", [player](String const& str) { player->setEmoteDirectives(str); });

  callbacks.registerCallback(   "hairGroup",      [player]()   { return player->identity().hairGroup;      });
  callbacks.registerCallback("setHairGroup",      [player](String const& str) { player->setHairGroup(str); });
  callbacks.registerCallback(   "hairType",       [player]()   { return player->identity().hairType;      });
  callbacks.registerCallback("setHairType",       [player](String const& str) { player->setHairType(str); });
  callbacks.registerCallback(   "hairDirectives", [player]()   { return player->identity().hairDirectives;     });
  callbacks.registerCallback("setHairDirectives", [player](String const& str) { player->setHairDirectives(str); });

  callbacks.registerCallback(   "facialHairGroup",      [player]()   { return player->identity().facialHairGroup;      });
  callbacks.registerCallback("setFacialHairGroup",      [player](String const& str) { player->setFacialHairGroup(str); });
  callbacks.registerCallback(   "facialHairType",       [player]()   { return player->identity().facialHairType;      });
  callbacks.registerCallback("setFacialHairType",       [player](String const& str) { player->setFacialHairType(str); });
  callbacks.registerCallback(   "facialHairDirectives", [player]()   { return player->identity().facialHairDirectives;      });
  callbacks.registerCallback("setFacialHairDirectives", [player](String const& str) { player->setFacialHairDirectives(str); });

  callbacks.registerCallback("hair", [player]() {
    HumanoidIdentity const& identity = player->identity();
    return luaTupleReturn(identity.hairGroup, identity.hairType, identity.hairDirectives);
  });

  callbacks.registerCallback("facialHair", [player]() {
    HumanoidIdentity const& identity = player->identity();
    return luaTupleReturn(identity.facialHairGroup, identity.facialHairType, identity.facialHairDirectives);
  });

  callbacks.registerCallback("facialMask", [player]() {
    HumanoidIdentity const& identity = player->identity();
    return luaTupleReturn(identity.facialMaskGroup, identity.facialMaskType, identity.facialMaskDirectives);
  });

  callbacks.registerCallback("setFacialHair", [player](Maybe<String> const& group, Maybe<String> const& type, Maybe<String> const& directives) {
    if (group && type && directives)
      player->setFacialHair(*group, *type, *directives);
    else {
      if (group)      player->setFacialHairGroup(*group);
      if (type)       player->setFacialHairType(*type);
      if (directives) player->setFacialHairDirectives(*directives);
    }
  });

  callbacks.registerCallback("setFacialMask", [player](Maybe<String> const& group, Maybe<String> const& type, Maybe<String> const& directives) {
    if (group && type && directives)
      player->setFacialMask(*group, *type, *directives);
    else {
      if (group)      player->setFacialMaskGroup(*group);
      if (type)       player->setFacialMaskType(*type);
      if (directives) player->setFacialMaskDirectives(*directives);
    }
  });

  callbacks.registerCallback("setHair", [player](Maybe<String> const& group, Maybe<String> const& type, Maybe<String> const& directives) {
    if (group && type && directives)
      player->setHair(*group, *type, *directives);
    else {
      if (group)      player->setHairGroup(*group);
      if (type)       player->setHairType(*type);
      if (directives) player->setHairDirectives(*directives);
    }
  });

  callbacks.registerCallback(   "name", [player]()                   { return player->name(); });
  callbacks.registerCallback("setName", [player](String const& name) { player->setName(name); });

  callbacks.registerCallback(   "description", [player]()                          { return player->description(); });
  callbacks.registerCallback("setDescription", [player](String const& description) { player->setDescription(description); });

  callbacks.registerCallback(   "species", [player]()                      { return player->species();    });
  callbacks.registerCallback("setSpecies", [player](String const& species) { player->setSpecies(species); });

  callbacks.registerCallback(   "imagePath", [player]()                        { return player->identity().imagePath;    });
  callbacks.registerCallback("setImagePath", [player](Maybe<String> const& imagePath) { player->setImagePath(imagePath); });

  callbacks.registerCallback(   "gender", [player]()                     { return GenderNames.getRight(player->gender());  });
  callbacks.registerCallback("setGender", [player](String const& gender) { player->setGender(GenderNames.getLeft(gender)); });

  callbacks.registerCallback(   "personality", [player]() { return jsonFromPersonality(player->identity().personality); });
  callbacks.registerCallback("setPersonality", [player](Json const& personalityConfig) {
    Personality const& oldPersonality = player->identity().personality;
    Personality newPersonality = oldPersonality;
    player->setPersonality(parsePersonality(newPersonality, personalityConfig));
  });

  // FezzedOne: Added these missing callbacks.
  callbacks.registerCallback(   "favoriteColor", [player]()                       { return player->favoriteColor(); });
  callbacks.registerCallback("setFavoriteColor", [player](Vec4B const& newColour) { player->setFavoriteColor(newColour); });

  // From OpenStarbound/Kae: Callbacks for getting and setting the player difficulty mode.
  callbacks.registerCallback(   "mode", [player]()                       { return PlayerModeNames.getRight(player->modeType());    });
  callbacks.registerCallback("setMode", [player](String const& modeName) { player->setModeType(PlayerModeNames.getLeft(modeName)); });

  callbacks.registerCallback(   "identity", [player]() -> Json { return player->getIdentity(); });
  callbacks.registerCallback("getIdentity", [player]() -> Json { return player->getIdentity(); }); // Exists for mod compatibility reasons.
  callbacks.registerCallback("setIdentity", [player](Json const &newIdentity) { player->setIdentity(newIdentity); });

  callbacks.registerCallback(   "interactRadius", [player]()         { return player->interactRadius();       });
  callbacks.registerCallback("setInteractRadius", [player](float radius) { player->setInteractRadius(radius); });

  callbacks.registerCallback("id",       [player]() { return player->entityId(); });
  callbacks.registerCallback("uniqueId", [player]() { return player->uniqueId(); });
  callbacks.registerCallback("isAdmin",  [player]() { return player->isAdmin();  });

  callbacks.registerCallback("interact", [player](String const& type, Json const& configData, Maybe<EntityId> const& sourceEntityId) {
      player->interact(InteractAction(type, sourceEntityId.value(NullEntityId), configData));
    });

  callbacks.registerCallback("shipUpgrades", [player]() { return player->shipUpgrades().toJson(); });
  callbacks.registerCallback("upgradeShip", [player](Json const& upgrades) { player->applyShipUpgrades(upgrades); });

  callbacks.registerCallback("setUniverseFlag", [player](String const& flagName) {
      player->clientContext()->rpcInterface()->invokeRemote("universe.setFlag", flagName);
    });

  callbacks.registerCallback("giveBlueprint", [player](Json const& item) { player->addBlueprint(ItemDescriptor(item)); });

  callbacks.registerCallback("blueprintKnown", [player](Json const& item) { return player->blueprintKnown(ItemDescriptor(item)); });

  callbacks.registerCallback("makeTechAvailable", [player](String const& tech) {
      player->techs()->makeAvailable(tech);
    });
  callbacks.registerCallback("makeTechUnavailable", [player](String const& tech) {
      player->techs()->makeUnavailable(tech);
    });
  callbacks.registerCallback("enableTech", [player](String const& tech) {
      player->techs()->enable(tech);
    });
  callbacks.registerCallback("equipTech", [player](String const& tech) {
      player->techs()->equip(tech);
    });
  callbacks.registerCallback("unequipTech", [player](String const& tech) {
      player->techs()->unequip(tech);
    });
  callbacks.registerCallback("availableTechs", [player]() {
      return player->techs()->availableTechs();
    });
  callbacks.registerCallback("enabledTechs", [player]() {
      return player->techs()->enabledTechs();
    });
  callbacks.registerCallback("equippedTech", [player](String typeName) {
      return player->techs()->equippedTechs().maybe(TechTypeNames.getLeft(typeName));
    });

  callbacks.registerCallback("currency", [player](String const& currencyType) { return player->currency(currencyType); });
  callbacks.registerCallback("addCurrency", [player](String const& currencyType, uint64_t amount) {
      player->inventory()->addCurrency(currencyType, amount);
    });
  callbacks.registerCallback("consumeCurrency", [player](String const& currencyType, uint64_t amount) {
      return player->inventory()->consumeCurrency(currencyType, amount);
    });

  callbacks.registerCallback("cleanupItems", [player]() {
      player->inventory()->cleanup();
    });

  callbacks.registerCallback("giveItem", [player](Json const& item) {
      player->giveItem(ItemDescriptor(item));
    });

  callbacks.registerCallback("giveEssentialItem", [player](String const& slotName, Json const& item) {
      auto itemDatabase = Root::singleton().itemDatabase();
      player->inventory()->setEssentialItem(EssentialItemNames.getLeft(slotName), itemDatabase->item(ItemDescriptor(item)));
    });

  callbacks.registerCallback("essentialItem", [player](String const& slotName) {
      return itemSafeDescriptor(player->inventory()->essentialItem(EssentialItemNames.getLeft(slotName))).toJson();
    });

  callbacks.registerCallback("removeEssentialItem", [player](String const& slotName) {
      player->inventory()->setEssentialItem(EssentialItemNames.getLeft(slotName), {});
    });

  callbacks.registerCallback("setEquippedItem", [player](String const& slotName, Json const& item) {
      auto itemDatabase = Root::singleton().itemDatabase();
      auto slot = InventorySlot(EquipmentSlotNames.getLeft(slotName));
      player->inventory()->setItem(slot, itemDatabase->item(ItemDescriptor(item)));
    });

  callbacks.registerCallback("equippedItem", [player](String const& slotName) {
      auto slot = InventorySlot(EquipmentSlotNames.getLeft(slotName));
      if (auto item = player->inventory()->itemsAt(slot))
        return item->descriptor().toJson();
      return Json();
    });

  callbacks.registerCallback("hasItem", [player](Json const& item, Maybe<bool> exactMatch) {
      return player->hasItem(ItemDescriptor(item), exactMatch.value(false));
    });

  callbacks.registerCallback("hasCountOfItem", [player](Json const& item, Maybe<bool> exactMatch) {
      return player->hasCountOfItem(ItemDescriptor(item), exactMatch.value(false));
    });

  callbacks.registerCallback("consumeItem", [player](Json const& item, Maybe<bool> consumePartial, Maybe<bool> exactMatch) {
      return player->takeItem(ItemDescriptor(item), consumePartial.value(false), exactMatch.value(false)).toJson();
    });

  callbacks.registerCallback("inventoryTags", [player]() {
      StringMap<size_t> inventoryTags;
      for (auto const& item : player->inventory()->allItems()) {
        for (auto tag : item->itemTags())
          ++inventoryTags[tag];
      }
      return inventoryTags;
    });

  callbacks.registerCallback("itemsWithTag", [player](String const& tag) {
      JsonArray items;
      for (auto const& item : player->inventory()->allItems()) {
        for (auto itemTag : item->itemTags()) {
          if (itemTag == tag)
            items.append(item->descriptor().toJson());
        }
      }
      return items;
    });

  callbacks.registerCallback("consumeTaggedItem", [player](String const& itemTag, uint64_t count) {
      for (auto const& item : player->inventory()->allItems()) {
        if (item->hasItemTag(itemTag)) {
          uint64_t takeCount = min(item->count(), count);
          player->takeItem(item->descriptor().singular().multiply(takeCount));
          count -= takeCount;
          if (count == 0)
            break;
        }
      }
    });

  callbacks.registerCallback("hasItemWithParameter", [player](String const& parameterName, Json const& parameterValue) {
      for (auto const& item : player->inventory()->allItems()) {
        if (item->instanceValue(parameterName, Json()) == parameterValue)
          return true;
      }
      return false;
    });

  callbacks.registerCallback("consumeItemWithParameter", [player](String const& parameterName, Json const& parameterValue, uint64_t count) {
      for (auto const& item : player->inventory()->allItems()) {
        if (item->instanceValue(parameterName, Json()) == parameterValue) {
          uint64_t takeCount = min(item->count(), count);
          player->takeItem(item->descriptor().singular().multiply(takeCount));
          count -= takeCount;
          if (count == 0)
            break;
        }
      }
    });

  callbacks.registerCallback("getItemWithParameter", [player](String const& parameterName, Json const& parameterValue) -> Json {
      for (auto const& item : player->inventory()->allItems()) {
        if (item->instanceValue(parameterName, Json()) == parameterValue)
          return item->descriptor().toJson();
      }
      return {};
    });

  callbacks.registerCallback("primaryHandItem", [player]() -> Maybe<Json> {
      if (!player->primaryHandItem())
        return {};
      return player->primaryHandItem()->descriptor().toJson();
    });

  callbacks.registerCallback("altHandItem", [player]() -> Maybe<Json> {
      if (!player->altHandItem())
        return {};
      return player->altHandItem()->descriptor().toJson();
    });

  callbacks.registerCallback("primaryHandItemTags", [player]() -> StringSet {
      if (!player->primaryHandItem())
        return {};
      return player->primaryHandItem()->itemTags();
    });

  callbacks.registerCallback("altHandItemTags", [player]() -> StringSet {
      if (!player->altHandItem())
        return {};
      return player->altHandItem()->itemTags();
    });

  callbacks.registerCallback("swapSlotItem", [player]() -> Maybe<Json> {
      if (!player->inventory()->swapSlotItem())
        return {};
      return player->inventory()->swapSlotItem()->descriptor().toJson();
    });

  callbacks.registerCallback("setSwapSlotItem", [player](Json const& item) {
      auto itemDatabase = Root::singleton().itemDatabase();
      player->inventory()->setSwapSlotItem(itemDatabase->item(ItemDescriptor(item)));
    });

  callbacks.registerCallback("canStartQuest",
      [player](Json const& quest) { return player->questManager()->canStart(QuestArcDescriptor::fromJson(quest)); });

  callbacks.registerCallback("startQuest", [player](Json const& quest, Maybe<String> const& serverUuid, Maybe<String> const& worldId) {
      auto questArc = QuestArcDescriptor::fromJson(quest);
      auto followUp = make_shared<Quest>(questArc, 0, player);
      if (serverUuid)
        followUp->setServerUuid(Uuid(*serverUuid));
      if (worldId)
        followUp->setWorldId(parseWorldId(*worldId));
      player->questManager()->offer(followUp);
      return followUp->questId();
    });

  callbacks.registerCallback("hasQuest", [player](String const& questId) {
      return player->questManager()->hasQuest(questId);
    });

  callbacks.registerCallback("hasAcceptedQuest", [player](String const& questId) {
      return player->questManager()->hasAcceptedQuest(questId);
    });

  callbacks.registerCallback("hasActiveQuest", [player](String const& questId) {
      return player->questManager()->isActive(questId);
    });

  callbacks.registerCallback("hasCompletedQuest", [player](String const& questId) {
      return player->questManager()->hasCompleted(questId);
    });

  callbacks.registerCallback("currentQuestWorld", [player]() -> Maybe<String> {
      auto maybeQuest = player->questManager()->currentQuest();
      if (maybeQuest) {
        auto quest = *maybeQuest;
        if (auto worldId = quest->worldId())
          return printWorldId(*worldId);
      }
      return {};
    });

  callbacks.registerCallback("questWorlds", [player]() -> List<pair<String, bool>> {
      List<pair<String, bool>> res;
      auto maybeCurrentQuest = player->questManager()->currentQuest();
      for (auto q : player->questManager()->listActiveQuests()) {
        if (auto worldId = q->worldId()) {
          bool isCurrentQuest = maybeCurrentQuest && maybeCurrentQuest.get()->questId() == q->questId();
          res.append(pair<String, bool>(printWorldId(*worldId), isCurrentQuest));
        }
      }
      return res;
    });

  callbacks.registerCallback("currentQuestLocation", [player]() -> Json {
      auto maybeQuest = player->questManager()->currentQuest();
      if (maybeQuest) {
        if (auto questLocation = maybeQuest.get()->location())
          return JsonObject{{"system", jsonFromVec3I(questLocation->first)}, {"location", jsonFromSystemLocation(questLocation->second)}};
      }
      return {};
    });

  callbacks.registerCallback("questLocations", [player]() -> List<pair<Json, bool>> {
      List<pair<Json, bool>> res;
      auto maybeCurrentQuest = player->questManager()->currentQuest();
      for (auto q : player->questManager()->listActiveQuests()) {
        if (auto questLocation = q->location()) {
          bool isCurrentQuest = maybeCurrentQuest && maybeCurrentQuest.get()->questId() == q->questId();
          auto locationJson = JsonObject{{"system", jsonFromVec3I(questLocation->first)}, {"location", jsonFromSystemLocation(questLocation->second)}};
          res.append(pair<Json, bool>(locationJson, isCurrentQuest));
        }
      }
      return res;
    });

  callbacks.registerCallback("enableMission", [player](String const& mission) {
      AiState& aiState = player->aiState();
      if (!aiState.completedMissions.contains(mission))
        aiState.availableMissions.add(mission);
    });

  callbacks.registerCallback("completeMission", [player](String const& mission) {
      AiState& aiState = player->aiState();
      aiState.availableMissions.remove(mission);
      aiState.completedMissions.add(mission);
    });

  callbacks.registerCallback("hasCompletedMission", [player](String const& mission) -> bool {
      return player->aiState().completedMissions.contains(mission);
    });

  callbacks.registerCallback("radioMessage", [player](Json const& messageConfig, Maybe<float> const& delay) {
      player->queueRadioMessage(messageConfig, delay.value(0));
    });

  callbacks.registerCallback("worldId", [player]() { return printWorldId(player->clientContext()->playerWorldId()); });

  callbacks.registerCallback("serverUuid", [player]() { return player->clientContext()->serverUuid().hex(); });

  callbacks.registerCallback("ownShipWorldId", [player]() { return printWorldId(ClientShipWorldId(player->uuid())); });

  callbacks.registerCallback("lounge", [player](EntityId entityId, Maybe<size_t> anchorIndex) {
      return player->lounge(entityId, anchorIndex.value(0));
    });
  callbacks.registerCallback("isLounging", [player]() { return (bool)player->loungingIn(); });
  callbacks.registerCallback("loungingIn", [player]() -> Maybe<EntityId> {
      if (auto anchorState = player->loungingIn())
        return anchorState->entityId;
      return {};
    });
  callbacks.registerCallback("stopLounging", [player]() { player->stopLounging(); });

  callbacks.registerCallback("playTime", [player]() { return player->log()->playTime(); });

  callbacks.registerCallback("introComplete", [player]() { return player->log()->introComplete(); });
  callbacks.registerCallback("setIntroComplete", [player](bool complete) {
      return player->log()->setIntroComplete(complete);
    });

  callbacks.registerCallback("warp", [player](String action, Maybe<String> animation, Maybe<bool> deploy) {
      player->setPendingWarp(action, animation, deploy.value(false));
    });

  callbacks.registerCallback("canDeploy", [player]() {
      return player->canDeploy();
    });

  callbacks.registerCallback("isDeployed", [player]() -> bool {
      return player->isDeployed();
    });

  callbacks.registerCallback("confirm", [player](Json dialogConfig) {
      auto pair = RpcPromise<Json>::createPair();
      player->queueConfirmation(dialogConfig, pair.second);
      return pair.first;
    });

  callbacks.registerCallback("playCinematic", [player](Json const& cinematic, Maybe<bool> unique) {
      player->setPendingCinematic(cinematic, unique.value(false));
    });

  callbacks.registerCallback("recordEvent", [player](String const& eventName, Json const& fields) {
      player->statistics()->recordEvent(eventName, fields);
    });

  callbacks.registerCallback("worldHasOrbitBookmark", [player](Json const& coords) -> bool {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      return player->universeMap()->worldBookmark(coordinate).isValid();
    });

  callbacks.registerCallback("orbitBookmarks", [player]() -> List<pair<Vec3I, Json>> {
      return player->universeMap()->orbitBookmarks().transformed([](pair<Vec3I, OrbitBookmark> const& p) -> pair<Vec3I, Json> {
        return {p.first, p.second.toJson()};
      });
    });

  callbacks.registerCallback("systemBookmarks", [player](Json const& coords) -> List<Json> {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      return player->universeMap()->systemBookmarks(coordinate).transformed([](OrbitBookmark const& bookmark) {
          return bookmark.toJson();
        });
    });

  callbacks.registerCallback("addOrbitBookmark", [player](Json const& system, Json const& bookmarkConfig) {
      CelestialCoordinate coordinate = CelestialCoordinate(system);
      return player->universeMap()->addOrbitBookmark(coordinate, OrbitBookmark::fromJson(bookmarkConfig));
    });

  callbacks.registerCallback("removeOrbitBookmark", [player](Json const& system, Json const& bookmarkConfig) {
      CelestialCoordinate coordinate = CelestialCoordinate(system);
      return player->universeMap()->removeOrbitBookmark(coordinate, OrbitBookmark::fromJson(bookmarkConfig));
    });

  callbacks.registerCallback("teleportBookmarks", [player]() -> List<Json> {
      return player->universeMap()->teleportBookmarks().transformed([](TeleportBookmark const& bookmark) -> Json {
        return bookmark.toJson();
      });
    });

  callbacks.registerCallback("addTeleportBookmark", [player](Json const& bookmarkConfig) {
      return player->universeMap()->addTeleportBookmark(TeleportBookmark::fromJson(bookmarkConfig));
    });

  callbacks.registerCallback("removeTeleportBookmark", [player](Json const& bookmarkConfig) {
      player->universeMap()->removeTeleportBookmark(TeleportBookmark::fromJson(bookmarkConfig));
    });

  callbacks.registerCallback("isMapped", [player](Json const& coords) {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      return player->universeMap()->isMapped(coordinate);
    });

  callbacks.registerCallback("mappedObjects", [player](Json const& coords) -> Json {
      CelestialCoordinate coordinate = CelestialCoordinate(coords);
      JsonObject json;
      for (auto p : player->universeMap()->mappedObjects(coordinate)) {
        JsonObject object = {
          {"typeName", p.second.typeName},
          {"orbit", jsonFromMaybe<CelestialOrbit>(p.second.orbit, [](CelestialOrbit const& o) { return o.toJson(); })},
          {"parameters", p.second.parameters}
        };
        json.set(p.first.hex(), object);
      }
      return json;
    });

  callbacks.registerCallback("collectables", [player](String const& collection) {
      return player->log()->collectables(collection);
    });

  callbacks.registerCallback("getProperty", [player](String const& name, Maybe<Json> const& defaultValue) -> Json {
      return player->getGenericProperty(name, defaultValue.value(Json()));
    });

  callbacks.registerCallback("setProperty", [player](String const& name, Json const& value) {
      player->setGenericProperty(name, value);
    });

  callbacks.registerCallback("addScannedObject", [player](String const& objectName) -> bool {
      return player->log()->addScannedObject(objectName);
    });

  callbacks.registerCallback("removeScannedObject", [player](String const& objectName) {
      player->log()->removeScannedObject(objectName);
    });

  callbacks.registerCallback("sendChat", [player](String const &text, Maybe<String> const &sendMode, Maybe<bool> suppressBubble) {
      String sendModeStr = sendMode.value("Local");
      bool suppressBubbleBool = suppressBubble.value(false);
      player->getUniverseClient()->sendChat(text, sendModeStr, suppressBubbleBool); });

  callbacks.registerCallback("queueStatusMessage", [player](String const &newMessage) {
      player->queueUIMessage(newMessage);
    });

  callbacks.registerCallback("addChatBubble", [player](String const &bubbleMessage, Maybe<String> const &portrait, Maybe<EntityId> const &sourceEntityId, Maybe<Json> const &bubbleConfig) {
      player->addChatMessage(bubbleMessage, portrait, sourceEntityId, bubbleConfig);
    });

  callbacks.registerCallback("setChatBubbleConfig", [player](Maybe<Json> const &bubbleConfig) {
      player->setChatBubbleConfig(bubbleConfig);
    });

  callbacks.registerCallback("getChatBubbleConfig", [player]() -> Json {
      return player->getChatBubbleConfig();
    });

  callbacks.registerCallback("getPlayerJson", [player]() -> Json { return player->diskStore(); });

  callbacks.registerCallback("dropEverything", [player]() { player->dropEverything(); });

  // FezzedOne: Minor compatibility break with xSB-2 v2.0.0 and v2.0.0a to ensure OpenSB compability.
  callbacks.registerCallback("emote", [player](String const &emote, Maybe<float> cooldown, Maybe<bool> gentleRequest) {
      bool isGentleRequest = gentleRequest.value(false);
      if (isGentleRequest) {
        player->requestEmote(emote, cooldown);
      } else {
        HumanoidEmote emoteState = HumanoidEmoteNames.valueLeft(emote, HumanoidEmote::Idle);
        player->addEmote(emoteState, cooldown);
      }
    });

  // Kae: New OpenSB callback.
  callbacks.registerCallback("currentEmote", [player]() {
    auto currentEmote = player->currentEmote();
    return luaTupleReturn(HumanoidEmoteNames.getRight(currentEmote.first), currentEmote.second);
  });

  callbacks.registerCallback("addEffectEmitters", [player](Json const &effectEmitters) {
      bool isValid = true;
      StringSet effectEmitterSet;
      try {
        effectEmitterSet = jsonToStringSet(effectEmitters);
      } catch (JsonException const& e) {
        isValid = false;
        Logger::error("addEffectEmitters: {}", e.what());
      }
      if (isValid)
        player->addEffectEmitters(effectEmitterSet);
    });

  callbacks.registerCallback("setExternalWarpsIgnored", [player](bool ignored)
                             { player->setExternalWarpsIgnored(ignored); });

  callbacks.registerCallback("setExternalRadioMessagesIgnored", [player](bool ignored)
                             { player->setExternalRadioMessagesIgnored(ignored); });

  callbacks.registerCallback("setExternalCinematicsIgnored", [player](bool ignored)
                             { player->setExternalCinematicsIgnored(ignored); });

  callbacks.registerCallback("setPhysicsEntitiesIgnored", [player](bool ignored)
                             { player->setPhysicsEntitiesIgnored(ignored); });

  callbacks.registerCallback("setNudityIgnored", [player](bool ignored)
                             { player->setForcedNudityIgnored(ignored); });

  callbacks.registerCallback("setTechOverridesIgnored", [player](bool ignored)
                             { player->setTechOverridesIgnored(ignored); });

  callbacks.registerCallback("toggleOverreach", [player](bool toggle)
                             { player->setCanReachAll(toggle); });

  callbacks.registerCallback("toggleInWorldRespawn", [player](bool toggle)
                             { player->setAlwaysRespawnInWorld(toggle); });

  callbacks.registerCallback("setIgnoreItemPickups", [player](bool ignore)
                             { player->setIgnoreItemPickups(ignore); });

  callbacks.registerCallback("setIgnoreShipUpdates", [player](bool ignore)
                             { player->setIgnoreShipUpdates(ignore); });

  callbacks.registerCallback("toggleFastWarp", [player](bool ignore)
                             { player->setFastRespawn(ignore); });

  callbacks.registerCallback("externalWarpsIgnored", [player]() -> bool
                             { return player->externalWarpsIgnored(); });

  callbacks.registerCallback("externalRadioMessagesIgnored", [player]() -> bool
                             { return player->externalRadioMessagesIgnored(); });

  callbacks.registerCallback("externalCinematicsIgnored", [player]() -> bool
                             { return player->externalCinematicsIgnored(); });

  callbacks.registerCallback("physicsEntitiesIgnored", [player]() -> bool
                             { return player->physicsEntitiesIgnored(); });

  callbacks.registerCallback("nudityIgnored", [player]() -> bool
                             { return player->forcedNudityIgnored(); });

  callbacks.registerCallback("overreach", [player]() -> bool
                             { return player->canReachAll(); });

  callbacks.registerCallback("techOverridesIgnored", [player]() -> bool
                             { return player->ignoreAllTechOverrides(); });

  callbacks.registerCallback("itemPickupsIgnored", [player]() -> bool
                             { return player->ignoreItemPickups(); });

  callbacks.registerCallback("fastWarp", [player]() -> bool
                             { return player->fastRespawn(); });

  callbacks.registerCallback("inWorldRespawn", [player]() -> bool
                             { return player->alwaysRespawnInWorld(); });

  callbacks.registerCallback("shipUpdatesIgnored", [player]() -> bool
                             { return player->shipUpdatesIgnored(); });

  callbacks.registerCallback("getChatText", [player]() -> String
                             { return player->chatText(); });

  callbacks.registerCallback("chatHasFocus", [player]() -> bool
                             { return player->chatOpen(); });

  callbacks.registerCallback("overrideTypingIndicator", [player](bool overridden)
                             { player->overrideChatIndicator(overridden); });

  callbacks.registerCallback("overrideMenuIndicator", [player](bool overridden)
                             { player->overrideMenuIndicator(overridden); });

  callbacks.registerCallback("aimPosition", [player]() -> Vec2F
                             { return player->aimPosition(); });

  // Kae: OpenSB callbacks.
  callbacks.registerCallback("actionBarGroup", [player]() {
    return luaTupleReturn(player->inventory()->customBarGroup() + 1, player->inventory()->customBarGroups());
  });

  callbacks.registerCallback("setActionBarGroup", [player](int group) {
    player->inventory()->setCustomBarGroup((group - 1) % (unsigned)player->inventory()->customBarGroups());
  });

  callbacks.registerCallback("selectedActionBarSlot", [player](LuaEngine& engine) -> Maybe<LuaValue> {
    if (auto barLocation = player->inventory()->selectedActionBarLocation()) {
      if (auto index = barLocation.ptr<CustomBarIndex>())
        return engine.luaFrom<CustomBarIndex>(*index + 1);
      else
        return engine.luaFrom<String>(EssentialItemNames.getRight(barLocation.get<EssentialItem>()));
    }
    else {
      return {};
    }
  });

  callbacks.registerCallback("setSelectedActionBarSlot", [player](MVariant<int, String> const& slot) {
    auto inventory = player->inventory();
    if (!slot)
      inventory->selectActionBarLocation(SelectedActionBarLocation());
    else if (auto index = slot.ptr<int>()) { 
      CustomBarIndex wrapped = (*index - 1) % (unsigned)inventory->customBarIndexes();
      inventory->selectActionBarLocation(SelectedActionBarLocation(wrapped));
    } else {
      EssentialItem const& item = EssentialItemNames.getLeft(slot.get<String>());
      inventory->selectActionBarLocation(SelectedActionBarLocation(item));
    }
  });
  // [end]

  // FezzedOne: Added `"say"`, functionally identical to `"addChatBubble"`, for OpenSB compability.
  // Can be invoked with only a `bubbleMessage`, emulating OpenSB behaviour.
  callbacks.registerCallback("say", [player](String const &bubbleMessage, Maybe<String> const &portrait, Maybe<EntityId> const &sourceEntityId, Maybe<Json> const &bubbleConfig) {
      player->addChatMessage(bubbleMessage, portrait, sourceEntityId, bubbleConfig);
    });

  // FezzedOne: {NOTE] The OpenSB version needs to be called every tick, and even then, may not work anyway.
  // The xSB-2 version here "sticks" and is saved to the player file. In any case, calling this version
  // every tick should not cause issues - just make sure to reset it on `uninit`.
  callbacks.registerCallback("setDamageTeam", [player](Maybe<String> teamType, Maybe<uint16_t> teamNumber) {
      TeamType checkedTeamType = TeamType::Friendly;
      if (teamType)
        checkedTeamType = TeamTypeNames.valueLeft(teamType.get(), TeamType::Friendly);
      TeamNumber checkedTeamNumber = 0;
      if (teamNumber)
        checkedTeamNumber = teamNumber.get();
      if (teamType || teamNumber) {
        player->setDamageTeam(EntityDamageTeam(checkedTeamType, checkedTeamNumber));
      } else {
        player->setDamageTeam();
      }
    });

  callbacks.registerCallback("setOverrideState", [player](Maybe<String> newState) {
    if (newState) {
      Maybe<Humanoid::State> newHumanoidState = Humanoid::StateNames.valueLeft(*newState, Humanoid::State::Idle);
      player->setOverrideState(newHumanoidState);
    } else {
      player->setOverrideState(Maybe<Humanoid::State>{});
    }
  });

  callbacks.registerCallback("overrideCameraPosition", [player](Maybe<Vec2F> newCameraPosition)
                             { player->overrideCameraPosition(newCameraPosition); });

  // FezzedOne: Addresses the inability of status effects to suppress tool usage.
  // Note: Tool usage is suppressed if *either* this callback or `tech.setToolUsageSuppressed` is called
  // with `true`, regardless of what's passed to the other callback. Make sure to clear suppression
  // by calling this with `false`, `nil` or no argument on `uninit` in status effects, since it
  // otherwise persists until the next `Player::init` call.
  callbacks.registerCallback("setToolUsageSuppressed", [player](Maybe<bool> suppressed)
                             { player->setToolUsageSuppressed(suppressed); });

  callbacks.registerCallback("teamMembers", [player]() -> Json
                             { return player->teamMembers(); });

  callbacks.registerCallback("controlAimPosition", [player](Vec2F const& newAimPosition)
                             { player->aim(std::move(newAimPosition)); });

  callbacks.registerCallback("controlShifting", [player](Maybe<bool> const& shifting)
                             { if (shifting) player->setShifting(*shifting);
                               else          player->setShifting(false); });

  callbacks.registerCallback("controlTrigger", [player](Maybe<bool> const& trigger) {
    if (trigger) {
      if (*trigger)
        player->beginTrigger();
      else
        player->endTrigger();
    } else {
      player->endTrigger();
    }
  });

  callbacks.registerCallback("controlFire", [player](Maybe<String> const& fireMode) {
    if (fireMode) {
      String mode = (*fireMode).toLower();
      if (mode == "primary" || mode == "beginPrimary")
        player->beginPrimaryFire();
      else if (mode == "endPrimary")
        player->endPrimaryFire();
      else if (mode == "alt" || mode == "beginAlt")
        player->beginAltFire();
      else if (mode == "endAlt")
        player->endAltFire();
    } else {
      player->endPrimaryFire();
      player->endAltFire();
    }
  });

  callbacks.registerCallback("controlSpecialAction", [player](int specialAction) {
    player->special(specialAction);
  });

  callbacks.registerCallback("controlAction", [player](Maybe<String> const& action) {
    const CaseInsensitiveStringMap<std::function<void()>> playerActions = {
      {"left", bind(&Player::moveLeft, player)},
      {"right", bind(&Player::moveRight, player)},
      {"down", bind(&Player::moveDown, player)},
      {"up", bind(&Player::moveUp, player)},
      {"jump", bind(&Player::jump, player)},
      {"beginTrigger", bind(&Player::beginTrigger, player)},
      {"endTrigger", bind(&Player::endTrigger, player)},
      {"beginPrimaryFire", bind(&Player::beginPrimaryFire, player)},
      {"endPrimaryFire", bind(&Player::endPrimaryFire, player)},
      {"beginAltFire", bind(&Player::beginAltFire, player)},
      {"endAltFire", bind(&Player::endAltFire, player)},
      {"shift", bind(&Player::setShifting, player, true)},
      {"unshift", bind(&Player::setShifting, player, false)},
      {"special1", bind(&Player::special, player, 1)},
      {"special2", bind(&Player::special, player, 2)},
      {"special3", bind(&Player::special, player, 3)},
      {"dropItem", bind(&Player::dropItem, player)}
    };
    if (action) {
      if (auto actionBind = playerActions.maybe(*action)) {
        (*actionBind)();
      }
    }
  });

  return callbacks;
}

}
