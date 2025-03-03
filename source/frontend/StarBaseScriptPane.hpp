#ifndef STAR_BASE_SCRIPT_PANE_HPP
#define STAR_BASE_SCRIPT_PANE_HPP

#include "StarPane.hpp"
#include "StarLuaComponents.hpp"
#include "StarGuiReader.hpp"
#include "StarMainInterface.hpp"

namespace Star {

STAR_CLASS(CanvasWidget);
STAR_CLASS(BaseScriptPane);

// A more 'raw' script pane that doesn't depend on a world being present.
// Requires a derived class to provide a Lua root.
// Should maybe move into windowing?

class BaseScriptPane : public Pane {
public:
  BaseScriptPane(Json config, MainInterface* mainInterface = nullptr, bool construct = true, bool removeHoakyChatCallbacks = false);

  virtual void show() override;
  void displayed() override;
  void dismissed() override;

  void tick(float dt) override;

  bool sendEvent(InputEvent const& event) override;
  
  Json const& config() const;
  Json const& rawConfig() const;

  void construct(Json config);

  bool interactive() const override;

  PanePtr createTooltip(Vec2I const& screenPosition) override;
  Maybe<String> cursorOverride(Vec2I const& screenPosition) override;
protected:
  virtual GuiReaderPtr reader() override;
  Json m_config;
  Json m_rawConfig;
  bool m_dismissable;

  GuiReaderPtr m_reader;

  Map<CanvasWidgetPtr, String> m_canvasClickCallbacks;
  Map<CanvasWidgetPtr, String> m_canvasKeyCallbacks;

  bool m_interactive;

  bool m_callbacksAdded;
  bool m_removeHoakyChatCallbacks;
  mutable LuaMessageHandlingComponent<LuaUpdatableComponent<LuaBaseComponent>> m_script;
  MainInterface* m_mainInterface; // If null, means the pane wasn't initialised from the main game interface.
};

}

#endif
