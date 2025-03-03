#include "StarBaseScriptPane.hpp"
#include "StarRoot.hpp"
#include "StarAssets.hpp"
#include "StarGuiReader.hpp"
#include "StarJsonExtra.hpp"
#include "StarConfigLuaBindings.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarInterfaceLuaBindings.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarCanvasWidget.hpp"
#include "StarItemTooltip.hpp"
#include "StarItemGridWidget.hpp"
#include "StarSimpleTooltip.hpp"
#include "StarImageWidget.hpp"

namespace Star {

BaseScriptPane::BaseScriptPane(Json config, MainInterface* mainInterface, bool construct, bool removeHoakyChatCallbacks)
: Pane(), m_rawConfig(config), m_removeHoakyChatCallbacks(removeHoakyChatCallbacks), m_mainInterface(mainInterface) {
  auto& root = Root::singleton();
  auto assets = root.assets();

  if (config.type() == Json::Type::Object && config.contains("baseConfig")) {
    auto baseConfig = assets->fetchJson(config.getString("baseConfig"));
    m_config = jsonMerge(baseConfig, config);
  } else {
    m_config = assets->fetchJson(config);
  }
  
  m_dismissable = m_config.optBool("dismissable").value(true);
  m_interactive = m_config.getBool("interactive", true);
  m_reader = make_shared<GuiReader>();
  if (m_dismissable) {
    m_reader->registerCallback("close", [this](Widget*) {
      dismiss();
    });
  }

  for (auto const& callbackName : jsonToStringList(m_config.get("scriptWidgetCallbacks", JsonArray{}))) {
    m_reader->registerCallback(callbackName, [this, callbackName](Widget* widget) {
      m_script.invoke(callbackName, widget->name(), widget->data());
    });
  }

  if (construct)
    this->construct(config);

  m_callbacksAdded = false;
}

void BaseScriptPane::show() {
  Pane::show();
}

void BaseScriptPane::displayed() {
  Pane::displayed();
  if (!m_callbacksAdded) {
    m_script.addCallbacks("pane", makePaneCallbacks());
    m_script.addCallbacks("widget", LuaBindings::makeWidgetCallbacks(this, m_reader));
    m_script.addCallbacks("config", LuaBindings::makeConfigCallbacks( [this](String const& name, Json const& def) {
      return m_config.query(name, def);
    }));
    m_script.addCallbacks("input", LuaBindings::makeInputCallbacks());
    // FezzedOne: The clipboard callbacks don't actually use the main interface, so it's fine to put a null pointer there.
    m_script.addCallbacks("clipboard", LuaBindings::makeClipboardCallbacks(nullptr));
    if (m_mainInterface) {
      m_script.addCallbacks("interface", LuaBindings::makeInterfaceCallbacks(m_mainInterface));
      m_script.addCallbacks("chat",
        LuaBindings::makeChatCallbacks(m_mainInterface, m_removeHoakyChatCallbacks));
    }
    m_callbacksAdded = true;
  }
  m_script.init();
  m_script.invoke("displayed");
}

void BaseScriptPane::dismissed() {
  Pane::dismissed();
  m_script.invoke("dismissed");
  m_script.uninit();
  // m_script.removeCallbacks("widget");
  // m_script.removeCallbacks("config");
  // m_script.removeCallbacks("pane");
}

void BaseScriptPane::tick(float dt) {
  Pane::tick(dt);

  for (auto p : m_canvasClickCallbacks) {
    if (!p.first) continue;
    for (auto const& clickEvent : p.first->pullClickEvents())
      // FezzedOne: Added this for consistency with the one below.
      m_script.invoke(p.second, jsonFromVec2I(clickEvent.position), (uint8_t)clickEvent.button,
        clickEvent.buttonDown, MouseButtonNames.getRight(clickEvent.button));
  }
  for (auto p : m_canvasKeyCallbacks) {
    if (!p.first) continue;
    for (auto const& keyEvent : p.first->pullKeyEvents())
      // FezzedOne: For OpenStarbound script compatibility. 
      m_script.invoke(p.second, (int)keyEvent.key, keyEvent.keyDown, KeyNames.getRight(keyEvent.key));
  }

  m_script.update(m_script.updateDt(dt));
}

bool BaseScriptPane::sendEvent(InputEvent const& event) {
  // Intercept GuiClose before the canvas child so GuiClose always closes
  // BaseScriptPanes without having to support it in the script.
  if (context()->actions(event).contains(InterfaceAction::GuiClose)) {
    if (m_dismissable) {
      dismiss();
      return true;
    }
  }

  return Pane::sendEvent(event);
}

Json const& BaseScriptPane::config() const { return m_config; }
Json const& BaseScriptPane::rawConfig() const { return m_rawConfig; }

bool BaseScriptPane::interactive() const { return m_interactive; }

PanePtr BaseScriptPane::createTooltip(Vec2I const& screenPosition) {
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
      return ItemTooltipBuilder::buildItemTooltip(item);
    return {};
  }
}

Maybe<String> BaseScriptPane::cursorOverride(Vec2I const& screenPosition) {
  auto result = m_script.invoke<Maybe<String>>("cursorOverride", screenPosition);
  if (result)
    return *result;
  else
    return {};
}

GuiReaderPtr BaseScriptPane::reader() {
  return m_reader;
}

void BaseScriptPane::construct(Json config) {
  auto assets = Root::singleton().assets();
  m_reader->construct(assets->fetchJson(config.get("gui")), this);

  for (auto pair : config.getObject("canvasClickCallbacks", {}))
    m_canvasClickCallbacks.set(findChild<CanvasWidget>(pair.first), pair.second.toString());
  for (auto pair : config.getObject("canvasKeyCallbacks", {}))
    m_canvasKeyCallbacks.set(findChild<CanvasWidget>(pair.first), pair.second.toString());

  m_script.setScripts(jsonToStringList(config.get("scripts", JsonArray())));
  m_script.setUpdateDelta(config.getUInt("scriptDelta", 1));
}

}
