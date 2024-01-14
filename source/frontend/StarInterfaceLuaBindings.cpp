#include "StarInterfaceLuaBindings.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarJson.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarMainInterface.hpp"
#include "StarGuiContext.hpp"
#include "StarChatTypes.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeClipboardCallbacks(MainInterface* mainInterface) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("hasText", [mainInterface]() -> bool {
    return GuiContext::singleton().clipboardHasText();
  });
  callbacks.registerCallback("getText", [mainInterface]() -> Maybe<String> {
    return GuiContext::singleton().maybeGetClipboard();
  });
  callbacks.registerCallback("setText", [mainInterface](String text) -> Maybe<String> {
    return GuiContext::singleton().maybeSetClipboard(move(text));
  });

  return callbacks;
}

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

  callbacks.registerCallback("scale", [mainInterface]() -> float {
    return GuiContext::singleton().interfaceScale();
  });

  callbacks.registerCallback("queueMessage", [mainInterface](String const& message, Maybe<float> cooldown, Maybe<float> springState) {
    mainInterface->queueMessage(message, cooldown, springState.value(0));
  });

  callbacks.registerCallback("setCursorText", [mainInterface](Maybe<String> const& cursorText, Maybe<bool> overrideGameTooltips) {
    mainInterface->setCursorText(cursorText, overrideGameTooltips);
  });

  // FezzedOne: Sends a chat message *exactly* as if it were sent through the vanilla chat interface, returning any *client-side*
  // command results as a list of strings.
  callbacks.registerCallback("doChat", [mainInterface](String chatText, bool addToHistory) -> Maybe<List<String>> {
    return mainInterface->doChatCallback(chatText, addToHistory);
  });

  callbacks.registerCallback("drawDrawable", [mainInterface](Drawable drawable, Vec2F const& screenPos, float pixelRatio, Maybe<Vec4B> const& colour) {
    Vec4B checkedColour = Vec4B::filled(255);
    if (colour)
      checkedColour = *colour;
    mainInterface->drawDrawable(move(drawable), screenPos, pixelRatio, checkedColour);
  });

  callbacks.registerCallback("windowSize", [mainInterface]() -> Vec2U {
    return Vec2U(mainInterface->windowWidth(), mainInterface->windowHeight());
  });

  callbacks.registerCallback("cursorPosition", [mainInterface]() -> Vec2I {
    return mainInterface->cursorPosition();
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
      String messageText = newChatMessageConfig.getString("message", "");

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
