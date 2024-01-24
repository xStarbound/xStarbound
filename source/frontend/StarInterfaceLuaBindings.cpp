#include "StarInterfaceLuaBindings.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarJson.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarMainInterface.hpp"
#include "StarGuiContext.hpp"
#include "StarChatTypes.hpp"
#include "StarUniverseClient.hpp"
#include "StarChat.hpp"

namespace Star {

// For SE compatibility, so that mods don't need to get rewritten.
LuaCallbacks LuaBindings::makeChatCallbacks(MainInterface* mainInterface) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("send", [mainInterface](String const &text, Maybe<String> const &sendMode, Maybe<bool> suppressBubble) {
      String sendModeStr = sendMode.value("Broadcast");
      bool suppressBubbleBool = suppressBubble.value(false);
      mainInterface->universeClient()->sendChat(text, sendModeStr, suppressBubbleBool);
    });

  // FezzedOne: Sends a chat message *exactly* as if it were sent through the vanilla chat interface, returning any *client-side*
  // command results as a list of strings. Intended for compatibility with SE's `chat.command`.
  callbacks.registerCallback("command", [mainInterface](String chatText, Maybe<bool> addToHistory) -> Maybe<List<String>> {
    bool addToHistoryBool = false;
    if (addToHistory) addToHistoryBool = *addToHistory;
    return mainInterface->doChatCallback(chatText, addToHistoryBool);
  });

  callbacks.registerCallback("addMessage", [mainInterface](Maybe<String> const& text, Json const& chatMessageConfig) {
    if (chatMessageConfig) {
      Json newChatMessageConfig = JsonObject();
      if (chatMessageConfig.type() == Json::Type::Object)
        newChatMessageConfig = chatMessageConfig;

      MessageContext::Mode messageMode = MessageContextModeNames.valueLeft(newChatMessageConfig.getString("mode", "CommandResult"), MessageContext::Mode::CommandResult);
      String messageChannelName = newChatMessageConfig.getString("channel", "");

      ConnectionId messageConnectionId = (uint16_t)newChatMessageConfig.getInt("connection", 0);
      String messageNick = newChatMessageConfig.getString("fromNick", "");
      String messagePortrait = newChatMessageConfig.getString("portrait", "");
      String messageText;
      if (text)
        messageText = *text;
      else
        messageText = newChatMessageConfig.getString("text", "");

      bool showChatBool = newChatMessageConfig.optBool("showPane").value(true);

      ChatReceivedMessage messageToAdd = ChatReceivedMessage(MessageContext(messageMode, messageChannelName),
                                                             messageConnectionId,
                                                             messageNick,
                                                             messageText,
                                                             messagePortrait);
      mainInterface->addChatMessage(messageToAdd, showChatBool);
    } else if (text) {
      bool showChatBool = true;
      if (showChat)
        showChatBool = *showChat;
      ChatReceivedMessage messageToAdd = ChatReceivedMessage(MessageContext(MessageContext::Mode::CommandResult, ""),
                                                             (uint16_t)0,
                                                             "",
                                                             *text,
                                                             "");
      mainInterface->addChatMessage(messageToAdd, showChatBool);
    }
  });

  callbacks.registerCallback("input", [mainInterface]() -> String {
    if (mainInterface->chat())
      return mainInterface->chat()->currentChat();
    else
      return "";
  });

  callbacks.registerCallback("mode", [mainInterface]() -> Maybe<String> {
    if (mainInterface->chat())
      return Maybe<String>(ChatSendModeNames.getRight(mainInterface->chat()->sendMode()));
    else
      return {};
  });

  callbacks.registerCallback("setInput", [mainInterface](String const& chatInput) {
    if (mainInterface->chat())
      return mainInterface->chat()->sendMode(chatInput);
  });

  callbacks.registerCallback("clear", [mainInterface](Maybe<size_t> numMessages) {
    if (mainInterface->chat())
      return mainInterface->chat()->clearMessages(numMessages);
  });

  return callbacks;
}

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

  callbacks.registerCallback("setScale", [mainInterface](float newScale) {
    Root::singleton().configuration()->set("interfaceScale", std::clamp(newScale, 0.5f, 100.0f));
  });

  callbacks.registerCallback("worldPixelRatio", [mainInterface]() -> float {
    return Root::singleton().configuration()->get("zoomLevel").toFloat();
  });

  callbacks.registerCallback("setWorldPixelRatio", [mainInterface](float newPixelRatio) {
    Root::singleton().configuration()->set("zoomLevel", std::clamp(newPixelRatio, 0.25f, 100.0f));
  });

  callbacks.registerCallback("cameraPosition", [mainInterface]() -> Vec2F {
    return mainInterface->cameraPosition();
  });

  callbacks.registerCallback("overrideCameraPosition", [mainInterface](Vec2F newPosition) {
    mainInterface->setCameraPositionOverride(move(newPosition));
  });

  callbacks.registerCallback("queueMessage", [mainInterface](String const& message, Maybe<float> cooldown, Maybe<float> springState) {
    mainInterface->queueMessage(message, cooldown, springState.value(0));
  });

  callbacks.registerCallback("setCursorText", [mainInterface](Maybe<String> const& cursorText, Maybe<bool> overrideGameTooltips) {
    mainInterface->setCursorText(cursorText, overrideGameTooltips);
  });

  // FezzedOne: Sends a chat message *exactly* as if it were sent through the vanilla chat interface, returning any *client-side*
  // command results as a list of strings.
  callbacks.registerCallback("doChat", [mainInterface](String chatText, Maybe<bool> addToHistory) -> Maybe<List<String>> {
    bool addToHistoryBool = false;
    if (addToHistory) addToHistoryBool = *addToHistory;
    return mainInterface->doChatCallback(chatText, addToHistoryBool);
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
