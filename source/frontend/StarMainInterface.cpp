#include "StarMainInterface.hpp"
#include "StarActionBar.hpp"
#include "StarActiveItem.hpp"
#include "StarAiInterface.hpp"
#include "StarAiTypes.hpp"
#include "StarAssets.hpp"
#include "StarCanvasWidget.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarCharCreation.hpp"
#include "StarCharSelection.hpp"
#include "StarChat.hpp"
#include "StarChatBubbleManager.hpp"
#include "StarCinematic.hpp"
#include "StarClientCommandProcessor.hpp"
#include "StarClientContext.hpp"
#include "StarCodexInterface.hpp"
#include "StarConfirmationDialog.hpp"
#include "StarContainerEntity.hpp"
#include "StarContainerInteractor.hpp"
#include "StarContainerInterface.hpp"
#include "StarCraftingInterface.hpp"
#include "StarDrawable.hpp"
#include "StarFireableItem.hpp"
#include "StarGuiReader.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarInspectionTool.hpp"
#include "StarItem.hpp"
#include "StarItemDrop.hpp"
#include "StarItemSlotWidget.hpp"
#include "StarJoinRequestDialog.hpp"
#include "StarJsonExtra.hpp"
#include "StarLabelWidget.hpp"
#include "StarLexicalCast.hpp"
#include "StarLogging.hpp"
#include "StarMerchantInterface.hpp"
#include "StarMonster.hpp"
#include "StarNameplatePainter.hpp"
#include "StarNpc.hpp"
#include "StarOptionsMenu.hpp"
#include "StarPaneManager.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerInventory.hpp"
#include "StarPlayerLog.hpp"
#include "StarPlayerUniverseMap.hpp"
#include "StarPopupInterface.hpp"
#include "StarQuestIndicatorPainter.hpp"
#include "StarQuestInterface.hpp"
#include "StarQuestManager.hpp"
#include "StarQuestTracker.hpp"
#include "StarRadioMessagePopup.hpp"
#include "StarRoot.hpp"
#include "StarScriptPane.hpp"
#include "StarSongbookInterface.hpp"
#include "StarStatusPane.hpp"
#include "StarTeamBar.hpp"
#include "StarTeleportDialog.hpp"
#include "StarToolUserEntity.hpp"
#include "StarUniverseClient.hpp"
#include "StarWarpTargetEntity.hpp"
#include "StarWireInterface.hpp"
#include "StarWorldTemplate.hpp"

#if defined TRACY_ENABLE
#include "tracy/Tracy.hpp"
#else
#define ZoneScoped
#define ZoneScopedN(name)
#endif

namespace Star {

GuiMessage::GuiMessage() : message(), cooldown(), springState() {}

GuiMessage::GuiMessage(String const& message, float cooldown, float spring)
    : message(message), cooldown(cooldown), springState(spring) {}

MainInterface::MainInterface(UniverseClientPtr client, WorldPainterPtr painter, CinematicPtr cinematicOverlay) {
  m_guiContext = GuiContext::singletonPtr();

  m_client = client;
  m_worldPainter = painter;
  m_cinematicOverlay = cinematicOverlay;

  reset();
}

MainInterface::~MainInterface() {
  clean();
}

void MainInterface::clean() {
  m_client->interfaceMessageCallback() = {};
  if (m_chat) {
    m_chat->saveMessages();
    m_persistedChatState = m_chat->getState();
  }
  m_client->saveCallback() = {};
  m_paneManager.dismissAllPanes();
}

void MainInterface::deregisterPanes() {
  m_paneManager.deregisterAllPanes();
}

void MainInterface::reset() { // *Completely* reset the interface.
  m_state = Running;

  m_disableHud = false;

  m_cursorScreenPos = Vec2I();
  m_overrideDefaultTooltip = false;
  m_state = Running;

  m_config = MainInterfaceConfig::loadFromAssets();

  m_containerInteractor = make_shared<ContainerInteractor>();

  GuiReader itemSlotReader;
  m_cursorItem = convert<ItemSlotWidget>(itemSlotReader.makeSingle("cursorItemSlot", m_config->cursorItemSlot));

  m_planetNameTimer = GameTimer(m_config->planetNameTime);

  m_debugSpatialClearTimer = GameTimer(m_config->debugSpatialClearTime);
  m_debugMapClearTimer = GameTimer(m_config->debugMapClearTime);
  m_debugTextRect = RectF::null();

  m_lastMouseoverTarget = NullEntityId;
  m_stickyTargetingTimer = GameTimer(m_config->monsterHealthBarTime);

  // FezzedOne: Has to be moved here because `interface.bindRegisteredPane` can leave lingering callback references in `uninit` in universe client scripts.
  // A lingering reference is required for Save Inventory Position to work correctly on xClient.
  m_paneManager.deregisterAllPanes();

  m_inventoryWindow = make_shared<InventoryPane>(this, m_client->mainPlayer(), m_containerInteractor);
  m_paneManager.registerPane(MainInterfacePanes::Inventory, PaneLayer::Window, m_inventoryWindow, [this](PanePtr const&) {
    if (auto player = m_client->mainPlayer())
      player->clearSwap();
    if (m_containerPane) {
      m_containerPane->dismiss();
      m_containerPane = {};
      m_containerInteractor->closeContainer();
    }
    for (EntityId id : m_interactionScriptPanes.keys()) {
      if (m_paneManager.isDisplayed(m_interactionScriptPanes[id]) && as<ScriptPane>(m_interactionScriptPanes[id])->openWithInventory())
        m_interactionScriptPanes[id]->dismiss();
    }
  });

  m_overflowMessage = make_shared<GuiMessage>("", 0);

  m_plainCraftingWindow = make_shared<CraftingPane>(m_client->worldClient(), m_client->mainPlayer(), JsonObject{{"filter", JsonArray{"plain"}}}, m_client->mainPlayer()->entityId());
  m_paneManager.registerPane(MainInterfacePanes::CraftingPlain, PaneLayer::Window, m_plainCraftingWindow);

  m_paneManager.registerPane(MainInterfacePanes::EscapeDialog, PaneLayer::ModalWindow, createEscapeDialog());

  auto songbookInterface = make_shared<SongbookInterface>(m_client->mainPlayer());
  m_paneManager.registerPane(MainInterfacePanes::Songbook, PaneLayer::Window, songbookInterface);

  m_questLogInterface = make_shared<QuestLogInterface>(m_client->questManager(), m_client->mainPlayer(), m_cinematicOverlay, m_client);
  m_paneManager.registerPane(MainInterfacePanes::QuestLog, PaneLayer::Window, m_questLogInterface);

  auto aiInterface = make_shared<AiInterface>(m_client, m_cinematicOverlay, &m_paneManager);
  m_paneManager.registerPane(MainInterfacePanes::Ai, PaneLayer::Window, aiInterface);

  m_codexInterface = make_shared<CodexInterface>(m_client->mainPlayer());
  m_paneManager.registerPane(MainInterfacePanes::Codex, PaneLayer::Window, m_codexInterface);

  m_optionsMenu = make_shared<OptionsMenu>(&m_paneManager);
  m_paneManager.registerPane(MainInterfacePanes::Options, PaneLayer::ModalWindow, m_optionsMenu);

  m_popupInterface = make_shared<PopupInterface>();
  m_paneManager.registerPane(MainInterfacePanes::Popup, PaneLayer::Window, m_popupInterface);

  m_confirmationDialog = make_shared<ConfirmationDialog>();
  m_paneManager.registerPane(MainInterfacePanes::Confirmation, PaneLayer::ModalWindow, m_confirmationDialog);

  m_joinRequestDialog = make_shared<JoinRequestDialog>();
  m_paneManager.registerPane(MainInterfacePanes::JoinRequest, PaneLayer::ModalWindow, m_joinRequestDialog);

  m_actionBar = make_shared<ActionBar>(&m_paneManager, m_client->mainPlayer());
  m_paneManager.registerPane(MainInterfacePanes::ActionBar, PaneLayer::Hud, m_actionBar);

  m_questTracker = make_shared<QuestTrackerPane>();
  m_paneManager.registerPane(MainInterfacePanes::QuestTracker, PaneLayer::Hud, m_questTracker);

  m_mmUpgrade = make_shared<ScriptPane>(m_client, "/interface/scripted/mmupgrade/mmupgradegui.config", NullEntityId, this);
  m_paneManager.registerPane(MainInterfacePanes::MmUpgrade, PaneLayer::Window, m_mmUpgrade);

  m_collections = make_shared<ScriptPane>(m_client, "/interface/scripted/collections/collectionsgui.config", NullEntityId, this);
  m_paneManager.registerPane(MainInterfacePanes::Collections, PaneLayer::Window, m_collections);

  m_chat = make_shared<Chat>(this, m_client, m_persistedChatState, Root::singleton().assets()->json("/interface/chat/chat.config"));
  m_paneManager.registerPane(MainInterfacePanes::Chat, PaneLayer::Hud, m_chat);
  m_clientCommandProcessor = make_shared<ClientCommandProcessor>(m_client, m_cinematicOverlay, &m_paneManager, m_config->macroCommands);

  m_radioMessagePopup = make_shared<RadioMessagePopup>();
  m_paneManager.registerPane(MainInterfacePanes::RadioMessagePopup, PaneLayer::Hud, m_radioMessagePopup);

  m_wireInterface = make_shared<WirePane>(m_client->worldClient(), m_client->mainPlayer(), m_worldPainter);
  m_paneManager.registerPane(MainInterfacePanes::WireInterface, PaneLayer::World, m_wireInterface);
  m_client->mainPlayer()->setWireConnector(m_wireInterface.get());

  auto teamBar = make_shared<TeamBar>(this, m_client);
  m_paneManager.registerPane(MainInterfacePanes::TeamBar, PaneLayer::Hud, teamBar);

  auto statusPane = make_shared<StatusPane>(&m_paneManager, m_client);
  m_paneManager.registerPane(MainInterfacePanes::StatusPane, PaneLayer::Hud, statusPane);

  auto planetName = make_shared<Pane>();
  m_planetText = make_shared<LabelWidget>();
  m_planetText->setFontSize(m_config->planetNameFontSize);
  m_planetText->setFontMode(FontMode::Normal);
  m_planetText->setAnchor(HorizontalAnchor::HMidAnchor, VerticalAnchor::VMidAnchor);
  m_planetText->setDirectives(m_config->planetNameDirectives);
  planetName->disableScissoring();
  planetName->setPosition(m_config->planetNameOffset);
  planetName->setAnchor(PaneAnchor::Center);
  planetName->addChild("planetText", m_planetText);
  m_paneManager.registerPane(MainInterfacePanes::PlanetText, PaneLayer::Hud, planetName);

  m_nameplatePainter = make_shared<NameplatePainter>();
  m_questIndicatorPainter = make_shared<QuestIndicatorPainter>(m_client);
  m_chatBubbleManager = make_shared<ChatBubbleManager>();

  m_paneManager.displayRegisteredPane(MainInterfacePanes::ActionBar);
  m_paneManager.displayRegisteredPane(MainInterfacePanes::TeamBar);
  m_paneManager.displayRegisteredPane(MainInterfacePanes::StatusPane);

  m_charSwapPane = make_shared<CharSelectionPane>(m_client->playerStorage(), [&]() {}, // Creation callback.
      [&](PlayerPtr const& player) {
      if (m_client && player)
        m_client->switchPlayer(player->uuid()); },                                                // Selection callback.
      [&](Uuid uuid) {                                                                 // Deletion callback.
        if (m_client)
          m_client->removePlayer(uuid);
      },
      [&](Uuid const& uuid) -> bool { // Filter callback.
        return uuid != m_client->mainPlayer()->uuid();
      });
  {
    m_charSwapPane->setAnchor(PaneAnchor::Center);
    m_charSwapPane->unlockPosition();
  }
  m_paneManager.registerPane(MainInterfacePanes::CharacterSwap, PaneLayer::ModalWindow, m_charSwapPane);

  m_charAddPane = make_shared<CharSelectionPane>(m_client->playerStorage(), [&]() {}, // Creation callback.
      [&](PlayerPtr const& player) {
      if (m_client && player)
        m_client->addPlayer(player->uuid()); },                                               // Selection callback.
      [&](Uuid uuid) {                                                                // Deletion callback.
        if (m_client)
          m_client->removePlayer(uuid);
      },
      [&](Uuid const& uuid) -> bool { // Filter callback.
        return uuid != m_client->mainPlayer()->uuid() && !m_client->controlledPlayers().contains(uuid);
      });
  {
    m_charAddPane->setAnchor(PaneAnchor::Center);
    m_charAddPane->unlockPosition();
  }
  m_paneManager.registerPane(MainInterfacePanes::CharacterAdd, PaneLayer::ModalWindow, m_charAddPane);

  m_charRemovePane = make_shared<CharSelectionPane>(m_client->playerStorage(), [&]() {}, // Creation callback.
      [&](PlayerPtr const& player) {
      if (m_client && player)
        m_client->removePlayer(player->uuid()); },                                                  // Selection callback.
      [&](Uuid uuid) {                                                                   // Deletion callback.
        if (m_client)
          m_client->removePlayer(uuid);
      },
      [&](Uuid const& uuid) -> bool { // Filter callback.
        return uuid != m_client->mainPlayer()->uuid() && m_client->controlledPlayers().contains(uuid);
      });
  {
    m_charRemovePane->setAnchor(PaneAnchor::Center);
    m_charRemovePane->unlockPosition();
  }
  m_paneManager.registerPane(MainInterfacePanes::CharacterRemove, PaneLayer::ModalWindow, m_charRemovePane);

  m_client->saveCallback() = [&]() {
    m_chat->saveMessages();
  };

  m_client->interfaceMessageCallback() = [&](const String& message, bool localMessage, const JsonArray& args) -> Maybe<Json> {
    Maybe<Json> result = {};
    JsonArray results = {};
    bool isChatMessage = message == "chatMessage" || message == "newChatMessage";
    if (isChatMessage) {
      for (auto r : ScriptPane::receivePaneMessages(message, localMessage, args).value(JsonArray{}).toArray())
        results.append(r);
      for (auto r : ContainerPane::receivePaneMessages(message, localMessage, args).value(JsonArray{}).toArray())
        results.append(r);
      if (auto r = m_chat->receiveEntityMessage(message, localMessage, args))
        results.append(*r);
    } else {
      result = ScriptPane::receivePaneMessages(message, localMessage, args);
      if (!result) result = ContainerPane::receivePaneMessages(message, localMessage, args);
      if (!result) result = m_chat->receiveEntityMessage(message, localMessage, args);
    }
    return isChatMessage ? Json(results) : result;
  };

  m_charEditor = make_shared<CharCreationPane>([=](PlayerPtr editedPlayer) {
    m_paneManager.dismissRegisteredPane(MainInterfacePanes::CharacterEdit);
  },
      m_client->mainPlayer());
  m_charEditor->setAnchor(PaneAnchor::Center);
  m_charEditor->unlockPosition();
  m_paneManager.registerPane(MainInterfacePanes::CharacterEdit, PaneLayer::ModalWindow, m_charEditor);
}

MainInterface::RunningState MainInterface::currentState() const {
  return m_state;
}

MainInterfacePaneManager* MainInterface::paneManager() {
  return &m_paneManager;
}

bool MainInterface::escapeDialogOpen() const {
  return m_paneManager.registeredPaneIsDisplayed(MainInterfacePanes::EscapeDialog) || m_paneManager.registeredPaneIsDisplayed(MainInterfacePanes::Options);
}

void MainInterface::openCraftingWindow(Json const& config, EntityId sourceEntityId) {
  if (m_craftingWindow && m_paneManager.isDisplayed(m_craftingWindow)) {
    m_paneManager.dismissPane(m_craftingWindow);
    if (m_craftingWindow->sourceEntityId() == sourceEntityId) {
      m_craftingWindow.reset();
      return;
    }
  }

  m_craftingWindow = make_shared<CraftingPane>(m_client->worldClient(), m_client->mainPlayer(), config, sourceEntityId);
  m_paneManager.displayPane(PaneLayer::Window, m_craftingWindow, [this](PanePtr const&) {
    if (auto player = m_client->mainPlayer())
      player->clearSwap();
  });
}

void MainInterface::openMerchantWindow(Json const& config, EntityId sourceEntityId) {
  if (m_merchantWindow && m_paneManager.isDisplayed(m_merchantWindow)) {
    m_paneManager.dismissPane(m_merchantWindow);
    if (m_merchantWindow->sourceEntityId() == sourceEntityId) {
      m_merchantWindow.reset();
      return;
    }
  }

  m_merchantWindow = make_shared<MerchantPane>(m_client->worldClient(), m_client->mainPlayer(), config, sourceEntityId);
  m_paneManager.displayPane(PaneLayer::Window,
      m_merchantWindow,
      [this](PanePtr const&) {
        if (auto player = m_client->mainPlayer())
          player->clearSwap();
        m_paneManager.dismissRegisteredPane(MainInterfacePanes::Inventory);
      });
  m_paneManager.displayRegisteredPane(MainInterfacePanes::Inventory);

  m_paneManager.bringPaneAdjacent(m_paneManager.registeredPane(MainInterfacePanes::Inventory),
      m_merchantWindow, Root::singleton().assets()->json("/interface.config:bringAdjacentWindowGap").toFloat());
}

void MainInterface::togglePlainCraftingWindow() {
  m_paneManager.toggleRegisteredPane(MainInterfacePanes::CraftingPlain);

  if (m_craftingWindow && m_craftingWindow->isDisplayed() && m_craftingWindow != m_paneManager.registeredPane(MainInterfacePanes::CraftingPlain))
    m_paneManager.dismissPane(m_craftingWindow);

  m_craftingWindow = m_plainCraftingWindow;
}

bool MainInterface::windowsOpen() const {
  return (bool)m_paneManager.topPane({PaneLayer::Window});
}

MerchantPanePtr MainInterface::activeMerchantPane() const {
  if (m_paneManager.isDisplayed(m_merchantWindow))
    return m_merchantWindow;
  else
    return {};
}

bool MainInterface::handleInputEvent(InputEvent const& event) {
  auto player = m_client->mainPlayer();
  auto inv = player->inventory();
  auto& root = Root::singleton();

  if (auto mouseMove = event.ptr<MouseMoveEvent>())
    m_cursorScreenPos = mouseMove->mousePosition;

  if (m_paneManager.sendInputEvent(event)) {
    if (!event.is<MouseButtonUpEvent>() && !event.is<KeyUpEvent>())
      return true;
  }

  if (event.is<KeyDownEvent>()) {
    if (m_chat->hasFocus()) {
      if (m_guiContext->actions(event).contains(InterfaceAction::ChatSendLine)) {
        doChat(m_chat->currentChat(), true);
        m_chat->clearCurrentChat();
        m_chat->stopChat();
        return true;
      }
    } else if (!m_paneManager.keyboardCapturedPane()) {
      Maybe<InventorySlot> swapSlot;

      for (auto action : m_guiContext->actions(event)) {
        switch (action) {
          default:
            break;
          case InterfaceAction::GuiShifting:
            m_guiContext->setShiftHeld(true);
            break;
          case InterfaceAction::ChatBegin:
            m_chat->startChat();
            break;

          case InterfaceAction::InterfaceHideHud:
            m_disableHud = !m_disableHud;
            break;

          case InterfaceAction::InterfaceRepeatCommand:
            if (!m_lastCommand.empty())
              doChat(m_lastCommand, false);
            break;

          case InterfaceAction::InterfaceToggleFullscreen:
            m_optionsMenu->toggleFullscreen();
            break;

          case InterfaceAction::InterfaceReload:
            root.reload();
            root.fullyLoad();
            break;

          case InterfaceAction::ChatBeginCommand:
            m_chat->startCommand();
            break;

          case InterfaceAction::InterfaceEscapeMenu:
            m_paneManager.toggleRegisteredPane(MainInterfacePanes::EscapeDialog);
            break;

          case InterfaceAction::InterfaceInventory:
            m_paneManager.toggleRegisteredPane(MainInterfacePanes::Inventory);
            break;

          case InterfaceAction::InterfaceCodex:
            m_paneManager.toggleRegisteredPane(MainInterfacePanes::Codex);
            break;

          case InterfaceAction::InterfaceQuest:
            m_paneManager.toggleRegisteredPane(MainInterfacePanes::QuestLog);
            break;

          case InterfaceAction::InterfaceCrafting:
            togglePlainCraftingWindow();
            break;
        }
      }
    }

    return false;

  } else if (auto keyUp = event.ptr<KeyUpEvent>()) {
    if (m_guiContext->actionsForKey(keyUp->key).contains(InterfaceAction::GuiShifting))
      m_guiContext->setShiftHeld(false);

    return false;

  } else if (auto mouseDown = event.ptr<MouseButtonDownEvent>()) {
    if (mouseDown->mouseButton == MouseButton::Left || mouseDown->mouseButton == MouseButton::Right || mouseDown->mouseButton == MouseButton::Middle)
      return overlayClick(mouseDown->mousePosition, mouseDown->mouseButton);
    else
      return false;

  } else if (auto mouseUp = event.ptr<MouseButtonUpEvent>()) {
    if (mouseUp->mouseButton == MouseButton::Left)
      player->endPrimaryFire();
    if (mouseUp->mouseButton == MouseButton::Right)
      player->endAltFire();
    if (mouseUp->mouseButton == MouseButton::Middle)
      player->endTrigger();
  }

  for (auto& pair : m_canvases)
    pair.second->sendEvent(event);

  return true;
}

bool MainInterface::inputFocus() const {
  return (bool)m_paneManager.keyboardCapturedPane();
}

bool MainInterface::textInputActive() const {
  return m_paneManager.keyboardCapturedForTextInput();
}

void MainInterface::handleInteractAction(InteractAction interactAction) {
  auto assets = Root::singleton().assets();
  auto world = m_client->worldClient();

  // FezzedOne: If we're in the middle of swapping players, don't allow player-based panes to be opened to prevent visual bugs.
  if (!m_client->switchingPlayer()) {
    if (interactAction.type == InteractActionType::OpenContainer) {
      // If we're currently displaying this container, close it.
      if (m_containerPane && m_containerInteractor->openContainerId() == interactAction.entityId) {
        m_paneManager.dismissPane(m_containerPane);
        return;
      }

      // If we're currently displaying another container, close it before we open.
      if (m_containerPane)
        m_paneManager.dismissPane(m_containerPane);

      auto containerEntity = world->get<ContainerEntity>(interactAction.entityId);
      if (!containerEntity)
        return;

      m_containerInteractor->openContainer(containerEntity);

      m_paneManager.displayRegisteredPane(MainInterfacePanes::Inventory);

      m_containerPane = make_shared<ContainerPane>(world, m_client->mainPlayer(), m_containerInteractor, this);
      m_paneManager.displayPane(PaneLayer::Window, m_containerPane, [this](PanePtr const&) {
        if (auto player = m_client->mainPlayer())
          player->clearSwap();
        m_paneManager.dismissRegisteredPane(MainInterfacePanes::Inventory);
      });

      m_paneManager.bringPaneAdjacent(m_paneManager.registeredPane(MainInterfacePanes::Inventory),
          m_containerPane, Root::singleton().assets()->json("/interface.config:bringAdjacentWindowGap").toFloat());
    } else if (interactAction.type == InteractActionType::SitDown) {
      m_client->mainPlayer()->lounge(interactAction.entityId, interactAction.data.toUInt());
    } else if (interactAction.type == InteractActionType::OpenCraftingInterface) {
      if (!world->entity(interactAction.entityId))
        return;

      openCraftingWindow(interactAction.data, interactAction.entityId);
    } else if (interactAction.type == InteractActionType::OpenSongbookInterface) {
      m_paneManager.displayRegisteredPane(MainInterfacePanes::Songbook);
    } else if (interactAction.type == InteractActionType::OpenNpcCraftingInterface) {
      if (!world->entity(interactAction.entityId))
        return;

      openCraftingWindow(interactAction.data, interactAction.entityId);
    } else if (interactAction.type == InteractActionType::OpenMerchantInterface) {
      if (!world->entity(interactAction.entityId))
        return;

      openMerchantWindow(interactAction.data, interactAction.entityId);
    } else if (interactAction.type == InteractActionType::OpenAiInterface) {
      as<AiInterface>(m_paneManager.registeredPane(MainInterfacePanes::Ai))->setSourceEntityId(interactAction.entityId);
      m_paneManager.displayRegisteredPane(MainInterfacePanes::Ai);
    } else if (interactAction.type == InteractActionType::OpenTeleportDialog) {
      if (m_teleportDialog)
        m_teleportDialog->dismiss();

      if (!m_client->canTeleport())
        return;

      auto currentLocation = TeleportBookmark();

      auto config = assets->fetchJson(interactAction.data);
      if (config.getBool("canBookmark", false)) {
        if (auto entity = world->entity(interactAction.entityId)) {
          if (auto uniqueEntityId = entity->uniqueId()) {
            auto worldTemplate = m_client->worldClient()->currentTemplate();

            if (!worldTemplate) return; // FezzedOne: Got rid of potential segfault here.

            String icon, planetName;
            if (m_client->playerWorld().is<ClientShipWorldId>()) {
              icon = "ship";
              planetName = "Player Ship";
            } else if (m_client->playerWorld().is<CelestialWorldId>()) {
              icon = worldTemplate->worldParameters()->typeName;
              planetName = worldTemplate->worldName();
            } else if (m_client->playerWorld().is<InstanceWorldId>()) {
              icon = worldTemplate->worldParameters()->typeName;
              planetName = worldTemplate->worldName();
            } else {
              icon = "default";
              planetName = "???";
            }

            currentLocation = TeleportBookmark{
                {m_client->playerWorld(), SpawnTargetUniqueEntity(*uniqueEntityId)},
                planetName,
                config.getString("bookmarkName", ""),
                icon};

            if (!m_client->mainPlayer()->universeMap()->teleportBookmarks().contains(currentLocation) || !config.getBool("canTeleport", true)) {
              auto editBookmarkDialog = make_shared<EditBookmarkDialog>(m_client->mainPlayer()->universeMap());
              editBookmarkDialog->setBookmark(currentLocation);
              m_paneManager.displayPane(PaneLayer::ModalWindow, editBookmarkDialog);
              return;
            }
          }
        }
      }

      if (config.getBool("canTeleport", true)) {
        m_teleportDialog = make_shared<TeleportDialog>(m_client, &m_paneManager, interactAction.data, interactAction.entityId, currentLocation);
        m_paneManager.displayPane(PaneLayer::ModalWindow, m_teleportDialog);
      }
    } else if (interactAction.type == InteractActionType::ShowPopup) {
      m_paneManager.displayRegisteredPane(MainInterfacePanes::Popup);
      m_popupInterface->displayMessage(interactAction.data.getString("message"), interactAction.data.getString("title", ""), interactAction.data.getString("subtitle", ""), interactAction.data.optString("sound"));
    } else if (interactAction.type == InteractActionType::ScriptPane) {
      auto sourceEntity = interactAction.entityId;
      // dismiss if there's already a scriptpane open for this source entity
      if (sourceEntity != NullEntityId && m_interactionScriptPanes.contains(sourceEntity) && m_paneManager.isDisplayed(m_interactionScriptPanes[sourceEntity]))
        m_paneManager.dismissPane(m_interactionScriptPanes[sourceEntity]);

      ScriptPanePtr scriptPane = make_shared<ScriptPane>(m_client, interactAction.data, sourceEntity, this);
      displayScriptPane(scriptPane, sourceEntity);

    } else if (interactAction.type == InteractActionType::Message) {
      m_client->mainPlayer()->receiveMessage(connectionForEntity(interactAction.entityId),
          interactAction.data.getString("messageType"), interactAction.data.getArray("messageArgs"));
    }
  }
}

void MainInterface::update(float dt) {
  ZoneScoped;

  // FezzedOne: Need to make sure the entity map is correctly initialised first
  // because some chat scripts expect to be able to send entity messages on `init`.
  if (m_client->worldClient()->inWorld())
    m_paneManager.displayRegisteredPane(MainInterfacePanes::Chat);

  m_paneManager.update(dt);
  m_cursor.update(dt);

  m_questLogInterface->pollDialog(&m_paneManager);

  if (!m_paneManager.topPane({PaneLayer::ModalWindow}) && m_codexInterface->showNewCodex())
    m_paneManager.displayRegisteredPane(MainInterfacePanes::Codex);

  auto player = m_client->mainPlayer();
  auto cursorWorldPos = cursorWorldPosition();
  if (!m_client->paused())
    player->aim(cursorWorldPos);
  if (player->wireToolInUse()) {
    m_paneManager.displayRegisteredPane(MainInterfacePanes::WireInterface);
    player->setWireConnector(m_wireInterface.get());
  } else {
    m_paneManager.dismissRegisteredPane(MainInterfacePanes::WireInterface);
  }

  // FezzedOne: Update chat text and whether the chat pane is open. Ugly, I know.
  player->passChatText(m_chat->currentChat());
  player->passChatOpen(m_chat->hasFocus());

  // update inventory pane items, to know if item slots changed
  m_inventoryWindow->updateItems();

  // update mouseover target
  EntityId newMouseOverTarget = NullEntityId;
  m_stickyTargetingTimer.tick(dt);
  auto mouseoverEntities = m_client->worldClient()->query<DamageBarEntity>(RectF::withCenter(cursorWorldPos, Vec2F(1, 1)), [=](shared_ptr<DamageBarEntity> const& entity) {
    return entity != player && entity->damageBar() == DamageBarType::Default && (entity->getTeam().type == TeamType::Enemy || entity->getTeam().type == TeamType::PVP) && m_client->worldClient()->lightLevel(entity->position()) > 0;
  });
  sortByComputedValue(mouseoverEntities, [&](DamageBarEntityPtr const& a) {
    return m_client->worldClient()->geometry().diff(a->position(), cursorWorldPos).magnitude();
  });
  if (mouseoverEntities.size() > 0) {
    newMouseOverTarget = mouseoverEntities[0]->entityId();
  } else if (m_lastMouseoverTarget == NullEntityId && player->lastDamagedTarget() != NullEntityId && player->timeSinceLastGaveDamage() < m_stickyTargetingTimer.time / 2) {
    if (auto targetEntity = as<DamageBarEntity>(m_client->worldClient()->entity(player->lastDamagedTarget()))) {
      if (targetEntity->damageBar() == DamageBarType::Default && (targetEntity->getTeam().type == TeamType::Enemy || targetEntity->getTeam().type == TeamType::PVP)) {
        newMouseOverTarget = targetEntity->entityId();
      }
    }
  }
  if (newMouseOverTarget != NullEntityId && newMouseOverTarget != m_lastMouseoverTarget) {
    m_lastMouseoverTarget = newMouseOverTarget;
    m_portraitScale = 0.0f;
    m_stickyTargetingTimer.reset();
  }

  if (m_stickyTargetingTimer.ready())
    m_lastMouseoverTarget = NullEntityId;

  // special damage bar entity
  if (m_specialDamageBarTarget != NullEntityId) {
    auto damageBarEntity = as<DamageBarEntity>(m_client->worldClient()->entity(m_specialDamageBarTarget));
    if (damageBarEntity && damageBarEntity->damageBar() == DamageBarType::Special) {
      float targetHealth = damageBarEntity->health() / damageBarEntity->maxHealth();
      float fillSpeed = 1.0f / Root::singleton().assets()->json("/interface.config:specialDamageBar.fillTime").toFloat();
      if (abs(targetHealth - m_specialDamageBarValue) < fillSpeed * dt)
        m_specialDamageBarValue = targetHealth;
      else
        m_specialDamageBarValue += copysign(1.0f, targetHealth - m_specialDamageBarValue) * fillSpeed * dt;
    } else {
      m_specialDamageBarTarget = NullEntityId;
    }
  }

  if (m_specialDamageBarTarget == NullEntityId)
    m_specialDamageBarValue = 0.0f;

  if (m_specialDamageBarTarget == NullEntityId && m_client->mainPlayer()->inWorld()) {
    List<DamageBarEntityPtr> specialDamageTargets;
    m_client->worldClient()->forAllEntities([&specialDamageTargets](EntityPtr const& entity) {
      if (auto damageBarEntity = as<DamageBarEntity>(entity))
        if (damageBarEntity->damageBar() == DamageBarType::Special)
          specialDamageTargets.append(damageBarEntity);
    });
    sortByComputedValue(specialDamageTargets, [&](DamageBarEntityPtr entity) {
      return m_client->worldClient()->geometry().diff(entity->position(), m_client->mainPlayer()->position());
    });

    if (specialDamageTargets.size() > 0)
      m_specialDamageBarTarget = specialDamageTargets[0]->entityId();
  }

  for (auto const& message : m_client->mainPlayer()->pullQueuedMessages())
    queueMessage(message);

  auto chatHeight = (m_chat->active() && m_chat->visible() > 0.1) ? m_chat->size()[1] : 0;
  m_radioMessagePopup->setChatHeight(chatHeight);
  if (!m_cinematicOverlay || m_cinematicOverlay->completed()) {
    if (m_client->mainPlayer()->interruptRadioMessage())
      m_radioMessagePopup->interrupt();
    if (!m_radioMessagePopup->messageActive()) {
      if (auto radioMessage = m_client->mainPlayer()->pullPendingRadioMessage()) {
        m_radioMessagePopup->setMessage(*radioMessage);
        m_paneManager.displayRegisteredPane(MainInterfacePanes::RadioMessagePopup);
        ChatReceivedMessage message = {
            {MessageContext::RadioMessage},
            ServerConnectionId,
            Text::stripEscapeCodes(radioMessage->senderName),
            Text::stripEscapeCodes(radioMessage->text),
            Text::stripEscapeCodes(radioMessage->portraitImage.replace("<frame>", "0"))};
        m_chat->addMessages({message}, false);
      } else {
        m_paneManager.dismissRegisteredPane(MainInterfacePanes::RadioMessagePopup);
      }
    }

    m_client->mainPlayer()->setInCinematic(false);
  } else {
    m_client->mainPlayer()->setInCinematic(true);
  }

  for (auto const& drop : m_client->mainPlayer()->pullQueuedItemDrops())
    queueItemPickupText(drop);

  m_chat->addMessages(m_client->pullChatMessages());

  if (auto worldClient = m_client->worldClient()) {
    if (worldClient->inWorld()) {
      if (auto cinematic = m_client->mainPlayer()->pullPendingCinematic()) {
        if (*cinematic)
          m_cinematicOverlay->load(Root::singleton().assets()->fetchJson(cinematic.take()));
        else
          m_cinematicOverlay->stop();
      }
    }
  }

  if (!m_confirmationDialog->isDisplayed()) {
    if (auto confirmation = m_client->mainPlayer()->pullPendingConfirmation()) {
      m_paneManager.displayRegisteredPane(MainInterfacePanes::Confirmation);
      m_confirmationDialog->displayConfirmation(confirmation->first, confirmation->second);
    }
  } else {
    auto confirmationSource = m_confirmationDialog->sourceEntityId();
    if (confirmationSource && !m_client->worldClient()->playerCanReachEntity(*confirmationSource))
      m_confirmationDialog->dismiss();
  }

  if (!m_joinRequestDialog->isDisplayed()) {
    if (auto req = m_queuedJoinRequests.maybeTakeLast()) {
      m_paneManager.displayRegisteredPane(MainInterfacePanes::JoinRequest);
      m_joinRequestDialog->displayRequest(req->first, [req](P2PJoinRequestReply reply) mutable {
        req->second.fulfill(reply);
      });
    }
  }

  for (EntityId id : m_interactionScriptPanes.keys()) {
    if (!m_paneManager.isDisplayed(m_interactionScriptPanes[id]))
      m_interactionScriptPanes.remove(id);
  }

  if (!m_messages.contains(m_overflowMessage))
    m_messageOverflow = 0;
  unsigned maxMessages = m_messageOverflow == 0 ? m_config->maxMessageCount : m_config->maxMessageCount + 1; // exclude overflow message
  if (m_messages.size() > maxMessages) {
    if (m_messageOverflow == 0) {
      m_messages.prepend(m_overflowMessage);
    }

    m_messageOverflow++;
    m_overflowMessage->message = m_config->overflowMessageText.replace("<count>", toString(m_messageOverflow));
    m_overflowMessage->cooldown = m_config->messageTime;
    if (auto oldest = m_messages.sorted([](GuiMessagePtr a, GuiMessagePtr b) { return a->cooldown < b->cooldown; }).maybeFirst())
      m_overflowMessage->cooldown = oldest.value()->cooldown;

    if (auto bottom = m_messages.filtered([this](GuiMessagePtr m) { return m != m_overflowMessage; }).maybeFirst())
      bottom.value()->cooldown = 0;
  }

  for (auto it = m_messages.begin(); it != m_messages.end();) {
    auto& message = *it;
    message->cooldown -= dt;
    if (message->cooldown < 0)
      it = m_messages.erase(it);
    else
      it++;
  }

  for (auto it = m_itemDropMessages.begin(); it != m_itemDropMessages.end();) {
    auto& message = *it;
    if (message.second.second->cooldown < 0)
      it = m_itemDropMessages.erase(it);
    else
      it++;
  }

  bool playerInWorld = m_client->mainPlayer()->inWorld();
  if (m_cinematicOverlay->completed()) {
    if (m_planetNameTimer.tick(dt)) {
      m_paneManager.dismissRegisteredPane(MainInterfacePanes::PlanetText);
    } else {
      if (playerInWorld) {
        String worldName;
        if (auto worldTemplate = m_client->worldClient()->currentTemplate())
          worldName = worldTemplate->worldName();

        if (!worldName.empty()) {
          m_planetText->setText(strf(m_config->planetNameFormatString.utf8Ptr(), worldName));
          m_paneManager.displayRegisteredPane(MainInterfacePanes::PlanetText);
        }
      }

      Color textColor = Color::White; // probably need to make this jsonable
      float fadeTimer = m_planetNameTimer.timer;
      if (fadeTimer < m_config->planetNameFadeTime)
        textColor.setAlphaF(fadeTimer / m_config->planetNameFadeTime);

      m_planetText->setColor(textColor);
    }
  } else if (!playerInWorld) {
    m_planetNameTimer.reset();
    m_paneManager.dismissRegisteredPane(MainInterfacePanes::PlanetText);
  }

  for (auto& containerResult : m_containerInteractor->pullContainerResults()) {
    if (!m_containerPane || !m_containerPane->giveContainerResult(containerResult)) {
      if (!m_inventoryWindow->giveContainerResult(containerResult)) {
        for (auto item : containerResult) {
          if (m_containerInteractor->containerOpen())
            m_containerInteractor->addToContainer(m_client->mainPlayer()->pickupItems(item));
          else
            m_client->mainPlayer()->giveItem(item);
        }
      }
    }
  }

  if (auto currentQuest = m_client->questManager()->currentQuest()) {
    m_paneManager.displayRegisteredPane(MainInterfacePanes::QuestTracker);
    m_questTracker->setQuest(*currentQuest);
  } else {
    m_paneManager.dismissRegisteredPane(MainInterfacePanes::QuestTracker);
  }

  updateCursor();

  m_nameplatePainter->update(dt, m_client->worldClient(), m_worldPainter->camera(), m_client->worldClient()->interactiveHighlightMode());
  m_questIndicatorPainter->update(dt, m_client->worldClient(), m_worldPainter->camera());

  m_chatBubbleManager->setCamera(m_worldPainter->camera());
  if (auto worldClient = m_client->worldClient()) {
    auto chatActions = worldClient->pullPendingChatActions();
    auto portraitActions = chatActions.filtered([](ChatAction action) { return action.is<PortraitChatAction>(); });

    for (auto action : portraitActions) {
      PortraitChatAction portraitAction = action.get<PortraitChatAction>();

      String name;
      if (auto npc = as<Npc>(worldClient->entity(portraitAction.entity)))
        name = npc->name();

      ChatReceivedMessage message = {
          {MessageContext::World},
          ServerConnectionId,
          Text::stripEscapeCodes(name),
          Text::stripEscapeCodes(portraitAction.text),
          Text::stripEscapeCodes(portraitAction.portrait.replace("<frame>", "0"))};
      m_chat->addMessages({message}, false);
    }

    m_chatBubbleManager->addChatActions(chatActions);
    m_chatBubbleManager->update(dt, worldClient);
  }

  if (auto container = m_client->worldClient()->get<ContainerEntity>(m_containerInteractor->openContainerId())) {
    if (!m_client->worldClient()->playerCanReachEntity(container->entityId()) || !container->isInteractive())
      m_containerInteractor->closeContainer();
  }

  if (m_paneManager.topPane({PaneLayer::Window, PaneLayer::ModalWindow}) && !m_client->mainPlayer()->menuIndicatorOverridden())
    m_client->mainPlayer()->setBusyState(PlayerBusyState::Menu);
  else if (m_chat->hasFocus() && !m_client->mainPlayer()->chatIndicatorOverridden())
    m_client->mainPlayer()->setBusyState(PlayerBusyState::Chatting);
  else
    m_client->mainPlayer()->setBusyState(PlayerBusyState::None);

  for (auto& pair : m_canvases) {
    pair.second->setPosition(Vec2I());
    if (pair.second->ignoreInterfaceScale())
      pair.second->setSize(Vec2I(m_guiContext->windowSize()));
    else
      pair.second->setSize(Vec2I(m_guiContext->windowInterfaceSize()));
    pair.second->update(dt);
  }
}

void MainInterface::renderInWorldElements() {
  if (m_disableHud)
    return;

  m_guiContext->setDefaultFont();
  m_guiContext->setFontProcessingDirectives("");
  m_guiContext->setFontColor(Vec4B::filled(255));
  m_questIndicatorPainter->render();
  m_nameplatePainter->render();
  m_chatBubbleManager->render();
}

void MainInterface::render() {
  if (m_disableHud)
    return;

  m_guiContext->setDefaultFont();
  m_guiContext->setFontProcessingDirectives("");
  m_guiContext->setFontColor(Vec4B::filled(255));

  renderQueuedDrawables();

  renderBreath();
  renderMessages();
  renderMonsterHealthBar();
  renderSpecialDamageBar();
  renderMainBar();
  renderDebug();

  RectI screenRect = RectI::withSize(Vec2I(), Vec2I(m_guiContext->windowSize()));
  for (auto& pair : m_canvases)
    pair.second->render(screenRect);

  renderWindows();
  renderCursor();
}

Vec2F MainInterface::cursorWorldPosition() const {
  return m_worldPainter->camera().screenToWorld(Vec2F(m_cursorScreenPos));
}

bool MainInterface::isDebugDisplayed() {
  return m_clientCommandProcessor->debugDisplayEnabled();
}

void MainInterface::drawDrawable(Drawable drawable, Vec2F const& screenPos, float pixelRatio, Vec4B const& color) {
  m_queuedDrawables.push_back({std::move(drawable), screenPos, pixelRatio, color});
}

void MainInterface::doChat(String const& chat, bool addToHistory) {
  if (chat.empty())
    return;

  if (chat.beginsWith("/")) {
    m_lastCommand = chat;

    for (auto const& result : m_clientCommandProcessor->handleCommand(chat))
      m_chat->addLine(result);
  } else {
    m_client->sendChat(chat, m_chat->sendMode());
  }

  if (addToHistory)
    m_chat->addHistory(chat);
}

Maybe<List<String>> MainInterface::doChatCallback(String& chat, bool addToHistory, Maybe<ChatSendMode> sendMode) {
  if (chat.empty())
    return {};

  bool gotCommandResult = false;
  List<String> finalResult = {};

  if (chat.beginsWith("/")) {
    for (auto const& result : m_clientCommandProcessor->handleCommand(chat)) {
      m_chat->addLine(result);
      if (!gotCommandResult) gotCommandResult = true;
      finalResult.emplace_back(result);
    }

    m_lastCommand = std::move(chat);
  } else {
    m_client->sendChat(chat, sendMode ? *sendMode : m_chat->sendMode());
  }

  if (addToHistory)
    m_chat->addHistory(chat);

  return gotCommandResult ? finalResult : Maybe<List<String>>{};
}

void MainInterface::addChatMessage(ChatReceivedMessage const& message, bool showChat) {
  m_chat->addMessages({message}, showChat);
}

void MainInterface::queueMessage(String const& message, Maybe<float> cooldown, float spring) {
  auto guiMessage = make_shared<GuiMessage>(message, cooldown.value(m_config->messageTime), spring);
  m_messages.append(guiMessage);
}

void MainInterface::queueMessage(String const& message) {
  queueMessage(message, m_config->messageTime, 0.0f);
}

void MainInterface::queueJoinRequest(pair<String, RpcPromiseKeeper<P2PJoinRequestReply>> request) {
  m_queuedJoinRequests.push_back(request);
}

void MainInterface::setCursorText(Maybe<String> const& cursorText, Maybe<bool> overrideGameTooltips) {
  m_overrideTooltip = cursorText;
  if (overrideGameTooltips)
    m_overrideDefaultTooltip = *overrideGameTooltips;
}

void MainInterface::queueItemPickupText(ItemPtr const& item) {
  auto descriptor = item->descriptor();
  if (m_itemDropMessages.contains(descriptor.singular())) {
    auto countMessPair = m_itemDropMessages.get(descriptor.singular());
    auto newCount = item->count() + countMessPair.first;
    auto message = countMessPair.second;
    message->message = strf("{} - {}", item->friendlyName(), newCount);
    message->cooldown = m_config->messageTime;
    m_itemDropMessages[descriptor.singular()] = {newCount, message};
  } else {
    auto message = make_shared<GuiMessage>(strf("{} - {}", item->friendlyName(), item->count()), m_config->messageTime);
    m_messages.append(message);
    m_itemDropMessages[descriptor.singular()] = {item->count(), message};
  }
}

bool MainInterface::fixedCamera() const {
  return m_clientCommandProcessor->fixedCameraEnabled();
}

bool MainInterface::hudVisible() const {
  return !m_disableHud;
}

void MainInterface::setHudVisible(bool visible) {
  m_disableHud = !visible;
}

void MainInterface::warpToOrbitedWorld(bool deploy) {
  if (m_client->canBeamDown(deploy)) {
    if (deploy)
      m_client->warpPlayer(WarpAlias::OrbitedWorld, true, "deploy", true);
    else
      m_client->warpPlayer(WarpAlias::OrbitedWorld, true, "beam");
    return;
  }
  m_guiContext->playAudio("/sfx/interface/clickon_error.ogg");
}

void MainInterface::warpToOwnShip() {
  if (m_client->canBeamUp()) {
    warpTo(WarpAlias::OwnShip);
  } else {
    m_guiContext->playAudio("/sfx/interface/clickon_error.ogg");
  }
}

void MainInterface::warpTo(WarpAction const& warpAction) {
  if (m_client->beamUpRule() == BeamUpRule::AnywhereWithWarning) {
    if (m_confirmationDialog->isDisplayed())
      m_confirmationDialog->dismiss();

    m_paneManager.displayRegisteredPane(MainInterfacePanes::Confirmation);
    m_confirmationDialog->displayConfirmation("/interface/windowconfig/beamupconfirmation.config", [this, warpAction](Widget*) { m_client->warpPlayer(warpAction, true, "beam"); }, [](Widget*) {});
  } else {
    m_client->warpPlayer(warpAction, true, "beam");
  }
}

CanvasWidgetPtr MainInterface::fetchCanvas(String const& canvasName, bool ignoreInterfaceScale) {
  CanvasWidgetPtr canvas;

  if (auto canvasPtr = m_canvases.ptr(canvasName))
    canvas = *canvasPtr;
  else {
    m_canvases.emplace(canvasName, canvas = make_shared<CanvasWidget>());
    canvas->setPosition(Vec2I());
    if (ignoreInterfaceScale)
      canvas->setSize(Vec2I(m_guiContext->windowSize()));
    else
      canvas->setSize(Vec2I(m_guiContext->windowInterfaceSize()));
  }

  canvas->setIgnoreInterfaceScale(ignoreInterfaceScale);
  return canvas;
}

// For when the player swaps characters. We need to completely reload ScriptPanes,
// because a lot of ScriptPanes do not expect the character to suddenly change and may break or spill data over.
void MainInterface::takeScriptPanes(List<ScriptPaneInfo>& out) {
  m_paneManager.dismissWhere([&](PanePtr const& pane) {
    if (auto scriptPane = as<ScriptPane>(pane)) {
      if (scriptPane->isDismissed())
        return false;
      auto sourceEntityId = scriptPane->sourceEntityId();
      m_interactionScriptPanes.remove(sourceEntityId);
      auto& info = out.emplaceAppend();
      info.scriptPane = scriptPane;
      info.config = scriptPane->rawConfig();
      info.sourceEntityId = sourceEntityId;
      info.visible = scriptPane->visibility();
      info.position = scriptPane->relativePosition();

      return true;
    }
    return false;
  });
}

void MainInterface::reviveScriptPanes(List<ScriptPaneInfo>& panes) {
  for (auto& info : panes) { // this is evil and stupid
    info.scriptPane->~ScriptPane();
    new (info.scriptPane.get()) ScriptPane(m_client, info.config, info.sourceEntityId);
    info.scriptPane->setVisibility(info.visible);
    displayScriptPane(info.scriptPane, info.sourceEntityId);
    info.scriptPane->setPosition(info.position);
  }
}

PanePtr MainInterface::createEscapeDialog() {
  auto assets = Root::singleton().assets();

  auto escapeDialog = make_shared<Pane>();
  auto escapeDialogPtr = escapeDialog.get();

  GuiReader escapeDialogReader;
  escapeDialogReader.registerCallback("returnToGame", [escapeDialogPtr](Widget*) {
    escapeDialogPtr->dismiss();
  });
  escapeDialogReader.registerCallback("showOptions", [escapeDialogPtr, this](Widget*) {
    escapeDialogPtr->dismiss();
    m_paneManager.displayRegisteredPane(MainInterfacePanes::Options);
  });
  escapeDialogReader.registerCallback("saveAndQuit", [escapeDialogPtr, this](Widget*) {
    m_state = ReturnToTitle;
    escapeDialogPtr->dismiss();
  });

  escapeDialogReader.construct(assets->json("/interface.config:escapeDialog"), escapeDialogPtr);

  escapeDialog->fetchChild<LabelWidget>("lblversion")->setText(/* Dummy comment to force MSVC rebuild. */
      strf("^font=iosevka-extrabold,set;^#822;xClient^reset,#999,font=iosevka-semibold,set; v{} [{}]\nStarbound v{}\n{}", xSbVersionString, StarArchitectureString, StarVersionString, "^font=iosevka-semibold,set;By ^#800;FezzedOne^reset,#999;, xStarbound and OpenStarbound\ncontributors, and Chucklefish"));
  escapeDialog->fetchChild<LabelWidget>("lblcopyright")->setText("");
  return escapeDialog;
}

float MainInterface::interfaceScale() const {
  return m_guiContext->interfaceScale();
}

unsigned MainInterface::windowHeight() const {
  return m_guiContext->windowHeight();
}

unsigned MainInterface::windowWidth() const {
  return m_guiContext->windowWidth();
}

Vec2I MainInterface::mainBarPosition() const {
  return Vec2I(windowWidth(), windowHeight()) - Vec2I(Vec2F(m_config->mainBarSize) * interfaceScale());
}

void MainInterface::renderBreath() {
  auto assets = Root::singleton().assets();
  auto imgMetadata = Root::singleton().imageMetadataDatabase();

  Vec2I breathBarSize = Vec2I(Vec2F(m_guiContext->textureSize("/interface/breath/empty.png")) * interfaceScale());
  Vec2I breathOffset = jsonToVec2I(assets->json("/interface.config:breathPos"));

  Vec2F breathBackgroundCenterPos(windowWidth() * 0.5f + ((float)(breathOffset[0]) * interfaceScale()), windowHeight() - ((float)(breathOffset[1]) * interfaceScale()));
  Vec2F breathBarPos = breathBackgroundCenterPos + Vec2F(jsonToVec2I(assets->json("/interface.config:breathBarPos"))) * interfaceScale();

  float breath = m_client->mainPlayer()->breath();
  float breathMax = m_client->mainPlayer()->maxBreath();

  size_t blocks = round((10 * breath) / breathMax);

  if (blocks < 10) {
    String breathPath = "/interface/breath/breath.png";
    m_guiContext->drawQuad(breathPath, RectF::withCenter(breathBackgroundCenterPos, Vec2F(imgMetadata->imageSize(breathPath)) * interfaceScale()));
    for (size_t i = 0; i < 10; i++) {
      if (i >= blocks) {
        if (blocks == 0 && Time::monotonicMilliseconds() % 500 > 250)
          m_guiContext->drawQuad("/interface/breath/warning.png", breathBarPos + Vec2F(breathBarSize[0] * i, 0), interfaceScale());
        else
          m_guiContext->drawQuad("/interface/breath/empty.png", breathBarPos + Vec2F(breathBarSize[0] * i, 0), interfaceScale());
      } else {
        m_guiContext->drawQuad("/interface/breath/breathbar.png", breathBarPos + Vec2F(breathBarSize[0] * i, 0), interfaceScale());
      }
    }
  }
}

void MainInterface::renderMessages() {
  if (m_messages.empty())
    return;

  Vec2F totalOffset = {};
  auto imgMetadata = Root::singleton().imageMetadataDatabase();
  unsigned bottomOffset = Root::singleton().configuration()->getPath("inventory.bottomActionBar").optBool().value(false) ? 32 : 0;
  for (auto& message : m_messages) {
    Vec2F hiddenOffset = Vec2F(m_config->messageHiddenOffset);
    Vec2F activeOffset = Vec2F(m_config->messageActiveOffset);
    if (bottomOffset) {
      activeOffset[1] += bottomOffset;
      bottomOffset = 0;
    }
    Vec2F messageOffset = lerp(message->springState, Vec2F(), activeOffset - hiddenOffset);
    totalOffset += messageOffset;
    messageOffset = totalOffset + hiddenOffset;

    Vec2F backgroundCenterPos = Vec2F(windowWidth() * 0.5f + messageOffset[0] * interfaceScale(), messageOffset[1] * interfaceScale());

    Vec2F backgroundTextCenterPos = backgroundCenterPos + Vec2F(m_config->messageTextContainerOffset) * interfaceScale();
    Vec2F messageTextOffset = backgroundTextCenterPos + Vec2F(m_config->messageTextOffset) * interfaceScale();

    if (message->cooldown > m_config->messageHideTime)
      message->springState = (message->springState * m_config->messageWindowSpring + 1.0f) / (m_config->messageWindowSpring + 1.0f);
    else
      message->springState = (message->springState * m_config->messageWindowSpring) / (m_config->messageWindowSpring + 1.0f);

    m_guiContext->drawQuad(m_config->messageTextContainer,
        RectF::withCenter(backgroundTextCenterPos, Vec2F(imgMetadata->imageSize(m_config->messageTextContainer)) * interfaceScale()));

    m_guiContext->setFont(m_config->font);
    m_guiContext->setFontSize(m_config->fontSize);
    m_guiContext->setFontColor(Color::White.toRgba());
    m_guiContext->renderText(message->message, {messageTextOffset, HorizontalAnchor::HMidAnchor, VerticalAnchor::VMidAnchor});
  }
}

void MainInterface::renderMonsterHealthBar() {
  auto assets = Root::singleton().assets();
  auto imgMetadata = Root::singleton().imageMetadataDatabase();
  if (m_lastMouseoverTarget != NullEntityId && !m_stickyTargetingTimer.ready()) {
    auto world = m_client->worldClient();

    auto entity = world->entity(m_lastMouseoverTarget);
    auto showDamageEntity = as<DamageBarEntity>(entity);

    if (!showDamageEntity) {
      m_lastMouseoverTarget = NullEntityId;
      return;
    }

    Vec2F backgroundCenterPos = Vec2F(windowWidth() / 2.0f, windowHeight());

    auto container = assets->json("/interface.config:monsterHealth.container").toString();
    auto offset = jsonToVec2F(assets->json("/interface.config:monsterHealth.offset")) * interfaceScale();
    m_guiContext->drawQuad(container, RectF::withCenter(backgroundCenterPos + offset, Vec2F(imgMetadata->imageSize(container)) * interfaceScale()));

    auto nameTextOffset = jsonToVec2F(assets->json("/interface.config:monsterHealth.nameTextOffset")) * interfaceScale();
    m_guiContext->setFont(m_config->font);
    m_guiContext->setFontSize(m_config->fontSize);
    m_guiContext->setFontColor(Color::White.toRgba());
    m_guiContext->renderText(showDamageEntity->name(), backgroundCenterPos + nameTextOffset);

    auto empty = assets->json("/interface.config:monsterHealth.progressEmpty").toString();
    auto filled = assets->json("/interface.config:monsterHealth.progressFilled").toString();
    auto progressBarOffset = jsonToVec2F(assets->json("/interface.config:monsterHealth.progressBarOffset")) * interfaceScale();
    auto chunks = assets->json("/interface.config:monsterHealth.progressChunks").toInt();
    int blocks = round(showDamageEntity->health() / showDamageEntity->maxHealth() * chunks);
    Vec2F barPos = backgroundCenterPos + progressBarOffset;
    Vec2F barItemOffset = Vec2F(imgMetadata->imageSize(filled)) * interfaceScale();
    barItemOffset[1] = 0;

    m_guiContext->drawQuad(empty, RectF::withSize(backgroundCenterPos + barPos, Vec2F(imgMetadata->imageSize(empty) * interfaceScale())));

    for (int i = 0; i < blocks; i++)
      m_guiContext->drawQuad(filled, barPos + barItemOffset * i, interfaceScale());

    auto portraitOffset = jsonToVec2F(assets->json("/interface.config:monsterHealth.portraitOffset")) * interfaceScale();
    auto portraitScale = assets->json("/interface.config:monsterHealth.portraitScale").toFloat() * interfaceScale();

    auto portraitScissorRect = jsonToRectF(assets->json("/interface.config:monsterHealth.portraitScissorRect")).scaled(interfaceScale());
    auto rect = portraitScissorRect.translated(backgroundCenterPos + portraitOffset);
    m_guiContext->setInterfaceScissorRect(RectI(RectF(rect).scaled(1.0f / interfaceScale())));
    auto portraitMaxSize = jsonToVec2I(assets->json("/interface.config:monsterHealth.portraitMaxSize"));
    List<Drawable> portrait = showDamageEntity->portrait(PortraitMode::Full);

    auto bounds = Drawable::boundBoxAll(portrait, true);
    if (m_portraitScale <= 0.0f)
      m_portraitScale = max(0.0625f, max(bounds.size().x() / portraitMaxSize.x(), bounds.size().y() / portraitMaxSize.y()));
    Drawable::translateAll(portrait, {-bounds.xMin() - (bounds.width() * 0.5f), -bounds.yMin()}); // crop out whitespace, align bottom center
    Drawable::scaleAll(portrait, 1.0f / (m_portraitScale * 2.0f));

    for (auto drawable : portrait)
      m_guiContext->drawDrawable(std::move(drawable), backgroundCenterPos + portraitOffset, portraitScale);

    m_guiContext->resetInterfaceScissorRect();
  }
}

void MainInterface::renderSpecialDamageBar() {
  if (m_specialDamageBarTarget == NullEntityId)
    return;

  auto assets = Root::singleton().assets();
  auto imgMetadata = Root::singleton().imageMetadataDatabase();

  if (auto target = as<DamageBarEntity>(m_client->worldClient()->entity(m_specialDamageBarTarget))) {
    Vec2F bottomCenter = Vec2F(windowWidth() / 2.0f, 0);

    auto barConfig = assets->json("/interface.config:specialDamageBar");

    auto background = barConfig.getString("background");
    auto backgroundOffset = jsonToVec2F(barConfig.get("backgroundOffset")) * interfaceScale();
    auto screenPos = RectF::withSize(bottomCenter + backgroundOffset, Vec2F(imgMetadata->imageSize(background)) * interfaceScale());
    m_guiContext->drawQuad(background, screenPos);

    auto fill = barConfig.getString("fill");
    auto fillOffset = jsonToVec2F(barConfig.get("fillOffset")) * interfaceScale();
    Vec2F size = Vec2F(barConfig.getInt("fillWidth") * m_specialDamageBarValue, imgMetadata->imageSize(fill).y());
    m_guiContext->drawQuad(fill, RectF::withSize(bottomCenter + fillOffset, size * interfaceScale()));

    auto nameOffset = jsonToVec2F(barConfig.get("nameOffset")) * interfaceScale();
    m_guiContext->setFontColor(jsonToColor(barConfig.get("nameColor")).toRgba());
    m_guiContext->setFontSize(barConfig.getUInt("nameSize"));
    m_guiContext->setFontProcessingDirectives(barConfig.getString("nameDirectives"));
    m_guiContext->renderText(target->name(), TextPositioning(bottomCenter + nameOffset, HorizontalAnchor::HMidAnchor, VerticalAnchor::BottomAnchor));
    m_guiContext->setFontProcessingDirectives("");
  }
}

void MainInterface::renderMainBar() {
  Vec2I barPos = mainBarPosition();

  m_cursorTooltip = {};

  auto assets = Root::singleton().assets();

  Vec2I inventoryButtonPos = barPos + Vec2I(Vec2F(m_config->mainBarInventoryButtonOffset) * interfaceScale());
  if (m_paneManager.registeredPaneIsDisplayed(MainInterfacePanes::Inventory)) {
    if (overButton(m_config->mainBarInventoryButtonPoly, m_cursorScreenPos)) {
      m_guiContext->drawQuad(m_config->inventoryImageOpenHover, Vec2F(inventoryButtonPos), interfaceScale());
      m_cursorTooltip = assets->json("/interface.config:cursorTooltip.inventoryText").toString();
    } else {
      m_guiContext->drawQuad(m_config->inventoryImageOpen, Vec2F(inventoryButtonPos), interfaceScale());
    }
  } else if (overButton(m_config->mainBarInventoryButtonPoly, m_cursorScreenPos)) {
    if (m_inventoryWindow->containsNewItems())
      m_guiContext->drawQuad(m_config->inventoryImageGlowHover, Vec2F(inventoryButtonPos), interfaceScale());
    else
      m_guiContext->drawQuad(m_config->inventoryImageHover, Vec2F(inventoryButtonPos), interfaceScale());
    m_cursorTooltip = assets->json("/interface.config:cursorTooltip.inventoryText").toString();
  } else {
    if (m_inventoryWindow->containsNewItems())
      m_guiContext->drawQuad(m_config->inventoryImageGlow, Vec2F(inventoryButtonPos), interfaceScale());
    else
      m_guiContext->drawQuad(m_config->inventoryImage, Vec2F(inventoryButtonPos), interfaceScale());
  }

  auto drawStateButton = [this](MainInterfacePanes paneType, Vec2I pos, PolyI poly,
                             String image, String hoverImage, String openImage, String hoverOpenImage, String toolTip) {
    if (m_paneManager.registeredPaneIsDisplayed(paneType)) {
      if (overButton(poly, m_cursorScreenPos)) {
        m_guiContext->drawQuad(hoverOpenImage, Vec2F(pos), interfaceScale());
        m_cursorTooltip = toolTip;
      } else {
        m_guiContext->drawQuad(openImage, Vec2F(pos), interfaceScale());
      }
    } else if (overButton(poly, m_cursorScreenPos)) {
      m_guiContext->drawQuad(hoverImage, Vec2F(pos), interfaceScale());
      m_cursorTooltip = toolTip;
    } else {
      m_guiContext->drawQuad(image, Vec2F(pos), interfaceScale());
    }
  };

  Vec2I craftButtonPos = barPos + Vec2I(Vec2F(m_config->mainBarCraftButtonOffset) * interfaceScale());
  drawStateButton(MainInterfacePanes::CraftingPlain,
      craftButtonPos,
      m_config->mainBarCraftButtonPoly,
      m_config->craftImage,
      m_config->craftImageHover,
      m_config->craftImageOpen,
      m_config->craftImageOpenHover,
      assets->json("/interface.config:cursorTooltip.craftingText").toString());

  Vec2I codexButtonPos = barPos + Vec2I(Vec2F(m_config->mainBarCodexButtonOffset) * interfaceScale());
  drawStateButton(MainInterfacePanes::Codex,
      codexButtonPos,
      m_config->mainBarCodexButtonPoly,
      m_config->codexImage,
      m_config->codexImageHover,
      m_config->codexImageOpen,
      m_config->codexImageHoverOpen,
      assets->json("/interface.config:cursorTooltip.codexText").toString());

  Vec2I mmUpgradeButtonPos = barPos + Vec2I(Vec2F(m_config->mainBarMmUpgradeButtonOffset) * interfaceScale());
  if (m_client->mainPlayer()->inventory()->essentialItem(EssentialItem::BeamAxe)) {
    drawStateButton(MainInterfacePanes::MmUpgrade,
        mmUpgradeButtonPos,
        m_config->mainBarMmUpgradeButtonPoly,
        m_config->mmUpgradeImage,
        m_config->mmUpgradeImageHover,
        m_config->mmUpgradeImageOpen,
        m_config->mmUpgradeImageHoverOpen,
        assets->json("/interface.config:cursorTooltip.mmUpgradeText").toString());
  } else {
    drawStateButton(MainInterfacePanes::MmUpgrade,
        mmUpgradeButtonPos,
        m_config->mainBarMmUpgradeButtonPoly,
        m_config->mmUpgradeImageDisabled,
        m_config->mmUpgradeImageDisabled,
        m_config->mmUpgradeImageDisabled,
        m_config->mmUpgradeImageDisabled,
        assets->json("/interface.config:cursorTooltip.disabledText").toString());
  }

  Vec2I collectionsButtonPos = barPos + Vec2I(Vec2F(m_config->mainBarCollectionsButtonOffset) * interfaceScale());
  drawStateButton(MainInterfacePanes::Collections,
      collectionsButtonPos,
      m_config->mainBarCollectionsButtonPoly,
      m_config->collectionsImage,
      m_config->collectionsImageHover,
      m_config->collectionsImageOpen,
      m_config->collectionsImageHoverOpen,
      assets->json("/interface.config:cursorTooltip.collectionsText").toString());

  // when the player can't deploy or beam, show the deploy button disabled
  // when the player can beam up they can't deploy down, show beaming up button in deploy button's place
  // when the player can only deploy, only show deploy button
  // when the player can deploy or beam down, show both buttons

  Vec2F deployButtonPos(Vec2F(barPos) + Vec2F(m_config->mainBarDeployButtonOffset) * interfaceScale());
  if (m_client->canBeamUp()) {
    if (overButton(m_config->mainBarDeployButtonPoly, m_cursorScreenPos)) {
      m_guiContext->drawQuad(m_config->beamUpImageHover, deployButtonPos, interfaceScale());
      m_cursorTooltip = assets->json("/interface.config:cursorTooltip.beamUpText").toString();
    } else {
      m_guiContext->drawQuad(m_config->beamUpImage, deployButtonPos, interfaceScale());
    }
  } else if (m_client->canBeamDown(true)) {
    if (overButton(m_config->mainBarDeployButtonPoly, m_cursorScreenPos)) {
      m_guiContext->drawQuad(m_config->deployImageHover, deployButtonPos, interfaceScale());
      m_cursorTooltip = assets->json("/interface.config:cursorTooltip.deployText").toString();
    } else {
      m_guiContext->drawQuad(m_config->deployImage, deployButtonPos, interfaceScale());
    }
  } else {
    m_guiContext->drawQuad(m_config->deployImageDisabled, deployButtonPos, interfaceScale());
  }

  Vec2F beamButtonPos(Vec2F(barPos) + Vec2F(m_config->mainBarBeamButtonOffset) * interfaceScale());
  if (m_client->canBeamDown()) {
    if (overButton(m_config->mainBarBeamButtonPoly, m_cursorScreenPos)) {
      m_guiContext->drawQuad(m_config->beamDownImageHover, beamButtonPos, interfaceScale());
      m_cursorTooltip = assets->json("/interface.config:cursorTooltip.beamDownText").toString();
    } else {
      m_guiContext->drawQuad(m_config->beamDownImage, beamButtonPos, interfaceScale());
    }
  }

  Vec2I questLogButtonPos = barPos + Vec2I(Vec2F(m_config->mainBarQuestLogButtonOffset) * interfaceScale());
  drawStateButton(MainInterfacePanes::QuestLog,
      questLogButtonPos,
      m_config->mainBarQuestLogButtonPoly,
      m_config->questLogImage,
      m_config->questLogImageHover,
      m_config->questLogImageOpen,
      m_config->questLogImageHoverOpen,
      assets->json("/interface.config:cursorTooltip.questsText").toString());

  if (m_overrideTooltip) {
    if (m_overrideDefaultTooltip || !m_cursorTooltip) {
      m_cursorTooltip = m_overrideTooltip;
    }
  }

  m_overrideTooltip = {};
  m_overrideDefaultTooltip = false;
}

void MainInterface::renderWindows() {
  m_paneManager.render();
}

void MainInterface::renderDebug() {
  if (!isDebugDisplayed()) {
    SpatialLogger::clear();
    m_debugTextRect = RectF::null();
    LogMap::clear();
    SpatialLogger::setObserved(false);
    return;
  }
  SpatialLogger::setObserved(true);

  if (m_clientCommandProcessor->debugHudEnabled()) {
    auto assets = Root::singleton().assets();
    m_guiContext->setFontSize(m_config->debugFontSize);
    m_guiContext->setFont(m_config->debugFont);
    m_guiContext->setLineSpacing(0.5f);
    m_guiContext->setFontProcessingDirectives(m_config->debugFontDirectives);
    m_guiContext->setFontColor(Color::White.toRgba());
    m_guiContext->setFontMode(FontMode::Normal);

    Vec2I debugOffset = Vec2I(Vec2F(m_config->debugOffset) * (interfaceScale() * 0.5f));

    bool clearMap = m_debugMapClearTimer.wrapTick();
    auto logMapValues = LogMap::getValues();
    if (clearMap)
      LogMap::clear();

    List<String> formatted;
    formatted.reserve(logMapValues.size());

    int counter = 0;
    for (auto const& pair : logMapValues) {
      TextPositioning positioning = {Vec2F(debugOffset[0], windowHeight() - debugOffset[1] - m_config->fontSize * interfaceScale() * counter++)};
      String& text = formatted.emplace_back(strf("{}^lightgray;:^green,set; {}", pair.first, pair.second));
      m_debugTextRect.combine(m_guiContext->determineTextSize(text, positioning).padded(m_config->debugBackgroundPad));
    }

    if (!m_debugTextRect.isNull()) {
      RenderQuad& quad = m_guiContext->renderer()->immediatePrimitives().emplace_back(std::in_place_type_t<RenderQuad>(), m_debugTextRect, m_config->debugBackgroundColor.toRgba(), 0.0f).get<RenderQuad>();

      quad.b.color[3] = quad.c.color[3] = 0;
    };

    m_debugTextRect = RectF::null();

    counter = 0;
    for (auto const& pair : logMapValues) {
      TextPositioning positioning = {Vec2F(debugOffset[0], windowHeight() - debugOffset[1] - m_config->fontSize * interfaceScale() * counter)};
      m_guiContext->renderText(formatted[counter], positioning);
      ++counter;
    }
    m_guiContext->setFontSize(8);
    m_guiContext->setDefaultFont();
    m_guiContext->setDefaultLineSpacing();
    m_guiContext->setFontColor(Vec4B::filled(255));
    m_guiContext->setFontProcessingDirectives("");
  }

  auto const& camera = m_worldPainter->camera();

  bool clearSpatial = m_debugSpatialClearTimer.wrapTick();

  for (auto const& line : SpatialLogger::getLines("world", clearSpatial)) {
    Vec2F begin = camera.worldToScreen(line.begin);
    Vec2F end = camera.worldGeometry().diff(line.end, line.begin) * camera.pixelRatio() * TilePixels + begin;
    m_guiContext->drawLine(begin, end, line.color, 1);
  }

  for (auto const& line : SpatialLogger::getLines("screen", clearSpatial))
    m_guiContext->drawLine(Vec2F(line.begin), Vec2F(line.end), line.color, 1);

  for (auto const& point : SpatialLogger::getPoints("world", clearSpatial)) {
    auto position = camera.worldToScreen(point.position);
    m_guiContext->drawLine(position + Vec2F(-2, -2), position + Vec2F(-2, 2), point.color, 1);
    m_guiContext->drawLine(position + Vec2F(-2, 2), position + Vec2F(2, 2), point.color, 1);
    m_guiContext->drawLine(position + Vec2F(2, 2), position + Vec2F(2, -2), point.color, 1);
    m_guiContext->drawLine(position + Vec2F(2, -2), position + Vec2F(-2, -2), point.color, 1);
  }

  for (auto const& point : SpatialLogger::getPoints("screen", clearSpatial)) {
    auto position = point.position;
    m_guiContext->drawLine(position + Vec2F(-2, -2), position + Vec2F(-2, 2), point.color, 1);
    m_guiContext->drawLine(position + Vec2F(-2, 2), position + Vec2F(2, 2), point.color, 1);
    m_guiContext->drawLine(position + Vec2F(2, 2), position + Vec2F(2, -2), point.color, 1);
    m_guiContext->drawLine(position + Vec2F(2, -2), position + Vec2F(-2, -2), point.color, 1);
  }

  m_guiContext->setFontSize(m_config->debugFontSize);

  for (auto const& logText : SpatialLogger::getText("world", clearSpatial)) {
    m_guiContext->setFontColor(logText.color);
    m_guiContext->renderText(logText.text.utf8Ptr(), camera.worldToScreen(logText.position));
  }

  for (auto const& logText : SpatialLogger::getText("screen", clearSpatial)) {
    m_guiContext->setFontColor(logText.color);
    m_guiContext->renderText(logText.text.utf8Ptr(), logText.position);
  }
  m_guiContext->setFontColor(Vec4B::filled(255));
}

void MainInterface::updateCursor() {
  Maybe<String> cursorOverride = m_actionBar->cursorOverride(m_cursorScreenPos);

  if (!cursorOverride) {
    if (auto pane = m_paneManager.getPaneAt(Vec2I(Vec2F(m_cursorScreenPos) / interfaceScale()))) {
      cursorOverride = cursorOverride.orMaybe(pane->cursorOverride(Vec2I(Vec2F(m_cursorScreenPos) / interfaceScale())));
    } else {
      auto player = m_client->mainPlayer();
      if (auto anchorState = m_client->mainPlayer()->loungingIn()) {
        if (auto loungeable = m_client->worldClient()->get<LoungeableEntity>(anchorState->entityId)) {
          if (auto loungeAnchor = loungeable->loungeAnchor(anchorState->positionIndex))
            cursorOverride = cursorOverride.orMaybe(loungeAnchor->cursorOverride);
        }
      }
      if (!cursorOverride) {
        for (auto item : {player->primaryHandItem(), player->altHandItem()}) {
          if (auto activeItem = as<ActiveItem>(item)) {
            if (auto cursor = activeItem->cursor()) {
              cursorOverride = cursor;
              break;
            }
          } else if (auto inspectionTool = as<InspectionTool>(item)) {
            cursorOverride = String("/cursors/inspect.cursor");
            break;
          }
        }
      }
    }
  }

  if (cursorOverride)
    m_cursor.setCursor(cursorOverride.take());
  else
    m_cursor.resetCursor();
}

void MainInterface::renderCursor() {
  // if we're currently playing a cinematic, we should not render the mouse.
  if (m_cinematicOverlay && !m_cinematicOverlay->completed())
    return m_guiContext->applicationController()->setCursorVisible(false);

  Vec2I cursorPos = m_cursorScreenPos;
  Vec2I cursorSize = m_cursor.size();
  Vec2I cursorOffset = m_cursor.offset();
  unsigned int cursorScale = m_cursor.scale(interfaceScale());
  Drawable cursorDrawable = m_cursor.drawable();

  cursorPos[0] -= cursorOffset[0] * cursorScale;
  cursorPos[1] -= (cursorSize[1] - cursorOffset[1]) * cursorScale;

  bool needsToDrawCursor = false,
       hardwareCursorDisabled = Root::singleton().configuration()->get("disableHardwareCursor").optBool().value(false);
  if (hardwareCursorDisabled)
    needsToDrawCursor = true;
  else
    needsToDrawCursor = !m_guiContext->trySetCursor(cursorDrawable, cursorOffset, cursorScale);
  if (needsToDrawCursor) m_guiContext->drawDrawable(cursorDrawable, Vec2F(cursorPos), cursorScale);
  m_guiContext->applicationController()->setCursorVisible(!needsToDrawCursor);

  if (m_cursorTooltip) {
    auto assets = Root::singleton().assets();
    auto imgDb = Root::singleton().imageMetadataDatabase();

    auto backgroundImage = assets->json("/interface.config:cursorTooltip.background").toString();
    auto rawCursorOffset = jsonToVec2I(assets->json("/interface.config:cursorTooltip.offset"));

    Vec2F tooltipSize = Vec2F(imgDb->imageSize(backgroundImage)) * interfaceScale();
    Vec2I cursorOffset = (Vec2I{0, -m_cursor.size().y()} + rawCursorOffset) * cursorScale;
    Vec2I tooltipOffset = m_cursorScreenPos + cursorOffset;
    size_t fontSize = assets->json("/interface.config:cursorTooltip.fontSize").toUInt();
    String font = assets->json("/interface.config:cursorTooltip.font").toString();
    Vec4B fontColor = jsonToColor(assets->json("/interface.config:cursorTooltip.color")).toRgba();

    m_guiContext->drawQuad(backgroundImage, Vec2F(tooltipOffset) + Vec2F(-tooltipSize.x(), 0), interfaceScale());
    m_guiContext->setFontSize(fontSize);
    m_guiContext->setFontColor(fontColor);
    m_guiContext->setFont(font);
    m_guiContext->renderText(*m_cursorTooltip,
        TextPositioning(Vec2F(tooltipOffset) + Vec2F(-tooltipSize.x(), tooltipSize.y()) / 2.0f,
            HorizontalAnchor::HMidAnchor,
            VerticalAnchor::VMidAnchor));
  }

  m_cursorItem->setPosition(Vec2I(Vec2F(m_cursorScreenPos) / interfaceScale() + Vec2F(m_config->inventoryItemMouseOffset)));

  if (auto swapItem = m_client->mainPlayer()->inventory()->swapSlotItem())
    m_cursorItem->setItem(swapItem);
  else
    m_cursorItem->setItem({});

  m_cursorItem->render(RectI::withSize({}, {(int)windowWidth(), (int)windowHeight()}));
  m_guiContext->resetInterfaceScissorRect();
}

void MainInterface::renderQueuedDrawables() {
  if (!m_queuedDrawables.empty()) {
    for (auto q : m_queuedDrawables) {
      m_guiContext->drawDrawable(std::move(q.drawable), q.screenPosition, q.pixelRatio, q.colour);
    }
    m_queuedDrawables.clear();
  }
}

bool MainInterface::overButton(PolyI buttonPoly, Vec2I const& mousePos) const {
  Vec2I barPos = mainBarPosition();
  buttonPoly.translate(barPos);
  PolyF buttonPolyF(buttonPoly);
  buttonPolyF.scale(interfaceScale(), Vec2F(barPos));
  return buttonPolyF.contains(Vec2F(mousePos));
}

bool MainInterface::overlayClick(Vec2I const& mousePos, MouseButton mouseButton) {
  PolyI mainBarPoly = m_config->mainBarPoly;
  Vec2I barPos = mainBarPosition();
  mainBarPoly.translate(barPos);
  PolyF mainBarPolyF(mainBarPoly);
  mainBarPolyF.scale(interfaceScale(), Vec2F(barPos));
  mainBarPoly = PolyI(mainBarPolyF);

  if (overButton(m_config->mainBarInventoryButtonPoly, mousePos)) {
    m_paneManager.toggleRegisteredPane(MainInterfacePanes::Inventory);
    return true;
  }

  if (overButton(m_config->mainBarCraftButtonPoly, mousePos)) {
    togglePlainCraftingWindow();
    return true;
  }

  if (overButton(m_config->mainBarCodexButtonPoly, mousePos)) {
    m_paneManager.toggleRegisteredPane(MainInterfacePanes::Codex);
    return true;
  }

  if (overButton(m_config->mainBarDeployButtonPoly, mousePos)) {
    if (m_client->canBeamDown(true))
      warpToOrbitedWorld(true);
    else if (m_client->canBeamUp())
      warpToOwnShip();
    return true;
  }

  if (overButton(m_config->mainBarBeamButtonPoly, mousePos)) {
    if (m_client->canBeamDown())
      warpToOrbitedWorld();
    return true;
  }

  if (overButton(m_config->mainBarQuestLogButtonPoly, mousePos)) {
    m_paneManager.toggleRegisteredPane(MainInterfacePanes::QuestLog);
    return true;
  }

  if (overButton(m_config->mainBarMmUpgradeButtonPoly, mousePos)) {
    if (m_client->mainPlayer()->inventory()->essentialItem(EssentialItem::BeamAxe))
      m_paneManager.toggleRegisteredPane(MainInterfacePanes::MmUpgrade);
    return true;
  }

  if (overButton(m_config->mainBarCollectionsButtonPoly, mousePos)) {
    m_paneManager.toggleRegisteredPane(MainInterfacePanes::Collections);
    return true;
  }

  if (mouseButton == MouseButton::Left)
    m_client->mainPlayer()->beginPrimaryFire();
  if (mouseButton == MouseButton::Right)
    m_client->mainPlayer()->beginAltFire();
  if (mouseButton == MouseButton::Middle)
    m_client->mainPlayer()->beginTrigger();

  return false;
}

void MainInterface::displayScriptPane(ScriptPanePtr& scriptPane, EntityId sourceEntity) {
  // keep any number of script panes open with null source entities
  if (sourceEntity != NullEntityId)
    m_interactionScriptPanes[sourceEntity] = scriptPane;

  PaneLayer layer = PaneLayer::Window;
  if (auto layerName = scriptPane->config().optString("paneLayer"))
    layer = PaneLayerNames.getLeft(*layerName);

  if (scriptPane->openWithInventory()) {
    m_paneManager.displayPane(layer, scriptPane, [this](PanePtr const&) {
      if (auto player = m_client->mainPlayer())
        player->clearSwap();
      m_paneManager.dismissRegisteredPane(MainInterfacePanes::Inventory);
    });
    m_paneManager.displayRegisteredPane(MainInterfacePanes::Inventory);
    m_paneManager.bringPaneAdjacent(m_paneManager.registeredPane(MainInterfacePanes::Inventory),
        scriptPane, Root::singleton().assets()->json("/interface.config:bringAdjacentWindowGap").toFloat());
  } else {
    m_paneManager.displayPane(layer, scriptPane);
  }
}

Vec2I MainInterface::cursorPosition() const {
  return m_cursorScreenPos;
}

Vec2F MainInterface::cameraPosition() const {
  return m_cameraPosition;
}

void MainInterface::passCameraPosition(Vec2F cameraPosition) {
  m_cameraPosition = std::move(cameraPosition);
}

Maybe<Vec2F> MainInterface::cameraPositionOverride() const {
  return m_cameraPositionOverride;
}

void MainInterface::setCameraPositionOverride(Maybe<Vec2F> newCameraOverride) {
  m_cameraPositionOverride = std::move(newCameraOverride);
}

UniverseClientPtr MainInterface::universeClient() const {
  return m_client;
}

ChatPtr MainInterface::chat() const {
  return m_chat;
}

WorldPainterPtr MainInterface::worldPainter() const {
  return m_worldPainter;
}

} // namespace Star
