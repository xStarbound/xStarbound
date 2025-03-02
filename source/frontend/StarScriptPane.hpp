#ifndef STAR_SCRIPT_PANE_HPP
#define STAR_SCRIPT_PANE_HPP

#include "StarBaseScriptPane.hpp"
#include "StarMainInterface.hpp"
#include "StarThread.hpp"

namespace Star {

STAR_CLASS(CanvasWidget);
STAR_CLASS(ScriptPane);
STAR_CLASS(UniverseClient);

class ScriptPane : public BaseScriptPane {
public:
  ScriptPane(UniverseClientPtr client, Json config, EntityId sourceEntityId = NullEntityId, MainInterface* mainInterface = nullptr);
  ~ScriptPane();

  void displayed() override;
  void dismissed() override;

  void tick(float dt) override;

  PanePtr createTooltip(Vec2I const& screenPosition) override;

  bool openWithInventory() const;

  EntityId sourceEntityId() const;

  LuaCallbacks makePaneCallbacks() override;

  static Maybe<Json> receivePaneMessages(String const& message, bool localMessage, JsonArray const& args);

private:
  UniverseClientPtr m_client;
  EntityId m_sourceEntityId;
  static Mutex s_globalScriptPaneMutex;
  static List<ScriptPane*> s_globalClientPaneRegistry;
};

}

#endif
