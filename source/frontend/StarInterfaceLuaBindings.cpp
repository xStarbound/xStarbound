#include "StarInterfaceLuaBindings.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarJson.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarMainInterface.hpp"
#include "StarGuiContext.hpp"
#include "StarChatTypes.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeInterfaceCallbacks(MainInterface* mainInterface) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("bindCanvas", [mainInterface](String const& canvasName, Maybe<bool> ignoreInterfaceScale) -> Maybe<CanvasWidgetPtr> {
    if (auto canvas = mainInterface->fetchCanvas(canvasName, ignoreInterfaceScale.value(false)))
      return canvas;
    return {};
  });

  callbacks.registerCallback("bindRegisteredPane", [mainInterface](String const& registeredPaneName) -> Maybe<LuaCallbacks> {
    if (auto pane = mainInterface->paneManager()->maybeRegisteredPane(MainInterfacePanesNames.getLeft(registeredPaneName)))
      return pane->makePaneCallbacks();
    return {};
  });

  callbacks.registerCallback("scale", [mainInterface]() -> int {
    return GuiContext::singleton().interfaceScale();
  });

  callbacks.registerCallback("queueMessage", [mainInterface](String const& message, Maybe<float> cooldown, Maybe<float> springState) {
    mainInterface->queueMessage(message, cooldown, springState.value(0));
  });

  callbacks.registerCallback("setCursorText", [mainInterface](Maybe<String> const& cursorText, Maybe<bool> overrideGameTooltips) {
    mainInterface->setCursorText(cursorText, overrideGameTooltips);
  });

  callbacks.registerCallback("addChatMessage", [mainInterface](Json const& chatMessageConfig, Maybe<bool> showChat) {
    if (chatMessageConfig) {
      bool showChatBool = false;
      if (showChat)
        showChatBool = *showChat;

      Json newChatMessageConfig = JsonObject();
      if (chatMessageConfig.type() == Json::Type::Object)
        newChatMessageConfig = chatMessageConfig;
      Json newContext = newChatMessageConfig.getObject("context", JsonObject());

      MessageContext::Mode messageMode = MessageContextModeNames.valueLeft(newContext.getString("mode", "Local"), MessageContext::Mode::Local);
      String messageChannelName = newContext.getString("channel", "");

      ConnectionId messageConnectionId = (uint16_t)newChatMessageConfig.getInt("connection", 0);
      String messageNick = newChatMessageConfig.getString("nick", "");
      String messagePortrait = newChatMessageConfig.getString("portrait", "");
      String messageText = newChatMessageConfig.getString("string", "");

      ChatReceivedMessage messageToAdd = ChatReceivedMessage(MessageContext(messageMode, messageChannelName),
                                                             messageConnectionId,
                                                             messageNick,
                                                             messageText,
                                                             messagePortrait);
      mainInterface->addChatMessage(messageToAdd, showChatBool);
    }
  });

  return callbacks;
}

}
