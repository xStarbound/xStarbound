#ifndef STAR_CONTAINER_INTERFACE_HPP
#define STAR_CONTAINER_INTERFACE_HPP

#include "StarContainerInteractor.hpp"
#include "StarGuiReader.hpp"
#include "StarLuaComponents.hpp"
#include "StarMainInterface.hpp"
#include "StarPane.hpp"
#include "StarThread.hpp"

namespace Star {

STAR_CLASS(ContainerEntity);
STAR_CLASS(Player);
STAR_CLASS(WorldClient);
STAR_CLASS(Item);
STAR_CLASS(ItemGridWidget);
STAR_CLASS(ItemBag);
STAR_CLASS(ContainerPane);

class ContainerPane : public Pane {
public:
  ContainerPane(WorldClientPtr worldClient, PlayerPtr player, ContainerInteractorPtr containerInteractor, MainInterface* mainInterface = nullptr);
  ~ContainerPane();

  void displayed() override;
  void dismissed() override;
  PanePtr createTooltip(Vec2I const& screenPosition) override;

  bool giveContainerResult(ContainerResult result);

  static Maybe<Json> receivePaneMessages(String const& message, bool localMessage, JsonArray const& args);

protected:
  void update(float dt) override;

private:
  enum class ExpectingSwap {
    None,
    Inventory,
    SwapSlot,
    SwapSlotStack
  };

  void swapSlot(ItemGridWidget* grid);
  void startCrafting();
  void stopCrafting();
  void toggleCrafting();
  void clear();
  void burn();

  WorldClientPtr m_worldClient;
  PlayerPtr m_player;
  ContainerInteractorPtr m_containerInteractor;
  ItemBagPtr m_itemBag;
  MainInterface* m_mainInterface;

  ExpectingSwap m_expectingSwap;

  GuiReader m_reader;

  Maybe<LuaMessageHandlingComponent<LuaWorldComponent<LuaUpdatableComponent<LuaBaseComponent>>>> m_script;
  static Mutex s_globalContainerPaneMutex;
  static List<ContainerPane*> s_globalClientPaneRegistry;
};

} // namespace Star

#endif
