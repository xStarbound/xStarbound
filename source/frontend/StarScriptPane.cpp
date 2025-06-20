#include "StarScriptPane.hpp"
#include "StarAssets.hpp"
#include "StarCanvasWidget.hpp"
#include "StarCelestialLuaBindings.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarConfiguration.hpp"
#include "StarEntityLuaBindings.hpp"
#include "StarGuiReader.hpp"
#include "StarImageWidget.hpp"
#include "StarInterfaceLuaBindings.hpp"
#include "StarItemGridWidget.hpp"
#include "StarItemTooltip.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarNetworkedAnimatorLuaBindings.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerLuaBindings.hpp"
#include "StarRoot.hpp"
#include "StarSimpleTooltip.hpp"
#include "StarStatusControllerLuaBindings.hpp"
#include "StarUniverseClient.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarWorldClient.hpp"

namespace Star {

Mutex ScriptPane::s_globalScriptPaneMutex = {};
List<ScriptPane*> ScriptPane::s_globalClientPaneRegistry = {};

ScriptPane::ScriptPane(UniverseClientPtr client, Json config, EntityId sourceEntityId, MainInterface* mainInterface) : BaseScriptPane(config, mainInterface) {
  auto& root = Root::singleton();
  auto assets = root.assets();

  m_client = std::move(client);
  m_sourceEntityId = sourceEntityId;

  m_script.addCallbacks("entity", LuaBindings::makeEntityCallbacks(as<Entity>(m_client->mainPlayer()).get()));
  m_script.addCallbacks("player", LuaBindings::makePlayerCallbacks(m_client->mainPlayer().get()));
  m_script.addCallbacks("playerAnimator", LuaBindings::makeNetworkedAnimatorCallbacks(m_client->mainPlayer()->effectsAnimator().get()));
  m_script.addCallbacks("status", LuaBindings::makeStatusControllerCallbacks(m_client->mainPlayer()->statusController()));
  m_script.addCallbacks("celestial", LuaBindings::makeCelestialCallbacks(m_client.get()));

  {
    MutexLocker locker(ScriptPane::s_globalScriptPaneMutex);
    ScriptPane::s_globalClientPaneRegistry.append(this);
  }
}

ScriptPane::~ScriptPane() {
  {
    MutexLocker locker(ScriptPane::s_globalScriptPaneMutex);
    ScriptPane::s_globalClientPaneRegistry.remove(this);
  }
}

void ScriptPane::displayed() {
  auto world = m_client->worldClient();
  if (world && world->inWorld()) {
    auto config = Root::singleton().configuration();
    m_script.setLuaRoot(make_shared<LuaRoot>());

    auto assets = Root::singleton().assets();
    Json clientConfig = assets->json("/client.config");

    m_script.luaRoot()->tuneAutoGarbageCollection(clientConfig.getFloat("luaGcPause"), clientConfig.getFloat("luaGcStepMultiplier"));
    m_script.addCallbacks("world", LuaBindings::makeWorldCallbacks(world.get()));
  }
  BaseScriptPane::displayed();
}

void ScriptPane::dismissed() {
  BaseScriptPane::dismissed();
  m_script.removeCallbacks("world");
}

void ScriptPane::tick(float dt) {
  if (m_sourceEntityId != NullEntityId && !m_client->worldClient()->playerCanReachEntity(m_sourceEntityId))
    dismiss();

  BaseScriptPane::tick(dt);
}

PanePtr ScriptPane::createTooltip(Vec2I const& screenPosition) {
  auto result = m_script.invoke<Json>("createTooltip", screenPosition);
  if (result && !result.value().isNull()) {
    if (result->type() == Json::Type::String) {
      return SimpleTooltipBuilder::buildTooltip(result->toString());
    } else {
      PanePtr tooltip = make_shared<Pane>();
      m_reader->construct(*result, tooltip.get());
      return tooltip;
    }
  } else {
    ItemPtr item;
    if (auto child = getChildAt(screenPosition)) {
      if (auto itemSlot = as<ItemSlotWidget>(child))
        item = itemSlot->item();
      if (auto itemGrid = as<ItemGridWidget>(child))
        item = itemGrid->itemAt(screenPosition);
    }
    if (item)
      return ItemTooltipBuilder::buildItemTooltip(item, m_client->mainPlayer());
    return {};
  }
}

LuaCallbacks ScriptPane::makePaneCallbacks() {
  LuaCallbacks callbacks = BaseScriptPane::makePaneCallbacks();
  callbacks.registerCallback("sourceEntity", [this]() { return m_sourceEntityId; });
  return callbacks;
}

bool ScriptPane::openWithInventory() const {
  return m_config.getBool("openWithInventory", false);
}

EntityId ScriptPane::sourceEntityId() const {
  return m_sourceEntityId;
}

Maybe<Json> ScriptPane::receivePaneMessages(const String& message, bool localMessage, const JsonArray& args) {
  Maybe<Json> result = {};
  bool isChatMessage = message == "chatMessage" || message == "newChatMessage";
  JsonArray results = JsonArray{};
  {
    MutexLocker locker(ScriptPane::s_globalScriptPaneMutex);
    for (auto const& pane : ScriptPane::s_globalClientPaneRegistry) {
      if (isChatMessage) {
        if (auto arrayResult = pane->m_script.handleMessage(message, localMessage, args))
          results.append(*arrayResult);
      } else {
        result = pane->m_script.handleMessage(message, localMessage, args);
        if (result) break;
      }
    }
  }
  return isChatMessage ? Json(results) : result;
}

} // namespace Star
