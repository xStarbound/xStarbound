#include "StarInterfaceLuaBindings.hpp"
#include "StarChat.hpp"
#include "StarChatTypes.hpp"
#include "StarGuiContext.hpp"
#include "StarJson.hpp"
#include "StarJsonExtra.hpp"
#include "StarLuaGameConverters.hpp"
#include "StarMainInterface.hpp"
#include "StarRoot.hpp"
#include "StarShellParser.hpp"
#include "StarUniverseClient.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarWorldCamera.hpp"

namespace Star {

// For SE compatibility, so that mods don't need to get rewritten.
LuaCallbacks LuaBindings::makeChatCallbacks(MainInterface* mainInterface, bool removeHoakyCallbacks) {
  LuaCallbacks callbacks;

  callbacks.registerCallback("send", [mainInterface](String const& text, Maybe<String> const& sendMode, Maybe<bool> addBubble, Maybe<JsonObject> metadata) {
    String sendModeStr = sendMode.value("Broadcast");
    bool addBubbleBool = addBubble.value(true);
    mainInterface->universeClient()->sendChat(text, sendModeStr, addBubbleBool, metadata);
  });

  // FezzedOne: For compatibility with the StarExtensions callback of the same name.
  callbacks.registerCallback("parseArguments", [mainInterface](String const& argumentString) -> Json {
    ShellParser parser;
    return jsonFromStringList(parser.tokenizeToStringList(argumentString));
  });

  // FezzedOne: Sends a chat message *exactly* as if it were sent through the vanilla chat interface, returning any *client-side*
  // command results as a list of strings. Intended for compatibility with SE's `chat.command`.
  if (removeHoakyCallbacks) {
    // We're in a chat pane script. Avoid a forced call to `sendMode` to reduce wonkiness.
    callbacks.registerCallback("command", [mainInterface](String chatText, Maybe<bool> addToHistory, Maybe<String> sendMode) -> Maybe<List<String>> {
      bool addToHistoryBool = false;
      if (addToHistory) addToHistoryBool = *addToHistory;
      Maybe<ChatSendMode> chatSendMode = sendMode ? ChatSendModeNames.valueLeft(*sendMode, ChatSendMode::Broadcast) : ChatSendMode::Broadcast;
      return mainInterface->doChatCallback(chatText, addToHistoryBool, chatSendMode);
    });
  } else {
    callbacks.registerCallback("command", [mainInterface](String chatText, Maybe<bool> addToHistory, Maybe<String> sendMode) -> Maybe<List<String>> {
      bool addToHistoryBool = false;
      if (addToHistory) addToHistoryBool = *addToHistory;
      Maybe<ChatSendMode> chatSendMode = sendMode ? ChatSendModeNames.maybeLeft(*sendMode) : Maybe<ChatSendMode>{};
      return mainInterface->doChatCallback(chatText, addToHistoryBool, chatSendMode);
    });
  }

  {
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
        JsonObject messageMetadata = newChatMessageConfig.getObject("data", JsonObject{});

        bool showChatBool = newChatMessageConfig.optBool("showPane").value(true);

        ChatReceivedMessage messageToAdd = ChatReceivedMessage(MessageContext(messageMode, messageChannelName),
            messageConnectionId,
            messageNick,
            messageText,
            messagePortrait,
            messageMetadata);
        mainInterface->addChatMessage(messageToAdd, showChatBool);
      } else if (text) {
        ChatReceivedMessage messageToAdd = ChatReceivedMessage(MessageContext(MessageContext::Mode::CommandResult, ""),
            (uint16_t)0,
            "",
            *text,
            "",
            JsonObject{});
        mainInterface->addChatMessage(messageToAdd, true);
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


    callbacks.registerCallback("setInput", [mainInterface](String const& chatInput) -> bool {
      if (mainInterface->chat())
        return mainInterface->chat()->setCurrentChat(chatInput);
      return false;
    });

    callbacks.registerCallback("clear", [mainInterface](Maybe<size_t> numMessages) {
      if (mainInterface->chat())
        mainInterface->chat()->clearMessages(numMessages);
    });
  }

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
    return GuiContext::singleton().maybeSetClipboard(std::move(text));
  });

  return callbacks;
}

LuaCallbacks LuaBindings::makeInterfaceCallbacks(MainInterface* mainInterface, bool unsafeVersion) {
  LuaCallbacks callbacks;

  // From OpenStarbound.
  callbacks.registerCallbackWithSignature<bool>(
      "hudVisible", bind(mem_fn(&MainInterface::hudVisible), mainInterface));
  callbacks.registerCallbackWithSignature<void, bool>(
      "setHudVisible", bind(mem_fn(&MainInterface::setHudVisible), mainInterface, _1));

  callbacks.registerCallback("bindCanvas", [mainInterface](String const& canvasName, Maybe<bool> ignoreInterfaceScale) -> Maybe<CanvasWidgetPtr> {
    if (auto canvas = mainInterface->fetchCanvas(canvasName, ignoreInterfaceScale.value(false)))
      return canvas;
    return {};
  });

  if (unsafeVersion) {
    callbacks.registerCallback("bindRegisteredPane", [mainInterface](String const& registeredPaneName) -> Maybe<LuaCallbacks> {
      if (auto pane = mainInterface->paneManager()->maybeRegisteredPane(MainInterfacePanesNames.getLeft(registeredPaneName)))
        return pane->makePaneCallbacks();
      return {};
    });
  } else {
    callbacks.registerCallback("bindRegisteredPane", [mainInterface](String const& registeredPaneName) -> Maybe<LuaCallbacks> {
      return {};
    });
  }

  // FezzedOne: Why was this missing?
  callbacks.registerCallback("displayRegisteredPane", [mainInterface](String const& registeredPaneName) {
    auto pane = MainInterfacePanesNames.getLeft(registeredPaneName);
    auto paneManager = mainInterface->paneManager();
    if (paneManager->maybeRegisteredPane(pane))
      paneManager->displayRegisteredPane(pane);
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
    mainInterface->setCameraPositionOverride(std::move(newPosition));
  });

  callbacks.registerCallback("queueMessage", [mainInterface](String const& message, Maybe<float> cooldown, Maybe<float> springState) {
    mainInterface->queueMessage(message, cooldown, springState.value(0));
  });

  callbacks.registerCallback("setCursorText", [mainInterface](Maybe<String> const& cursorText, Maybe<bool> overrideGameTooltips) {
    mainInterface->setCursorText(cursorText, overrideGameTooltips);
  });

  // FezzedOne: Sends a chat message *exactly* as if it were sent through the vanilla chat interface, returning any *client-side*
  // command results as a list of strings.
  callbacks.registerCallback("doChat", [mainInterface](String chatText, Maybe<bool> addToHistory, Maybe<String> sendMode) -> Maybe<List<String>> {
    bool addToHistoryBool = false;
    if (addToHistory) addToHistoryBool = *addToHistory;
    Maybe<ChatSendMode> chatSendMode = sendMode ? ChatSendModeNames.maybeLeft(*sendMode) : Maybe<ChatSendMode>{};
    return mainInterface->doChatCallback(chatText, addToHistoryBool, chatSendMode);
  });

  callbacks.registerCallback("drawDrawable", [mainInterface](Drawable drawable, Vec2F const& screenPos, float pixelRatio, Maybe<Vec4B> const& colour) {
    Vec4B checkedColour = Vec4B::filled(255);
    if (colour)
      checkedColour = *colour;
    mainInterface->drawDrawable(std::move(drawable), screenPos, pixelRatio, checkedColour);
  });

  callbacks.registerCallback("windowSize", [mainInterface]() -> Vec2U {
    return Vec2U(mainInterface->windowWidth(), mainInterface->windowHeight());
  });

  callbacks.registerCallback("cursorPosition", [mainInterface]() -> Vec2I {
    return mainInterface->cursorPosition();
  });

  // callbacks.registerCallback("addChatMessage", [mainInterface](Json const& chatMessageConfig, Maybe<bool> showChat) {
  //   if (chatMessageConfig) {
  //     bool showChatBool = false;
  //     if (showChat)
  //       showChatBool = *showChat;

  //     Json newChatMessageConfig = JsonObject();
  //     if (chatMessageConfig.type() == Json::Type::Object)
  //       newChatMessageConfig = chatMessageConfig;
  //     Json newContext = newChatMessageConfig.getObject("context", JsonObject());

  //     MessageContext::Mode messageMode = MessageContextModeNames.valueLeft(newContext.getString("mode", "Local"), MessageContext::Mode::Local);
  //     String messageChannelName = newContext.getString("channel", "");

  //     ConnectionId messageConnectionId = (uint16_t)newChatMessageConfig.getInt("connection", 0);
  //     String messageNick = newChatMessageConfig.getString("nick", "");
  //     String messagePortrait = newChatMessageConfig.getString("portrait", "");
  //     String messageText = newChatMessageConfig.getString("message", "");

  //     ChatReceivedMessage messageToAdd = ChatReceivedMessage(MessageContext(messageMode, messageChannelName),
  //                                                            messageConnectionId,
  //                                                            messageNick,
  //                                                            messageText,
  //                                                            messagePortrait);
  //     mainInterface->addChatMessage(messageToAdd, showChatBool);
  //   }
  // });

  return callbacks;
}

// [OpenStarbound] Kae: Added camera bindings.
LuaCallbacks LuaBindings::makeCameraCallbacks(WorldCamera* camera) {
  LuaCallbacks callbacks;

  callbacks.registerCallbackWithSignature<Vec2F>("position", bind(&WorldCamera::centerWorldPosition, camera));
  callbacks.registerCallbackWithSignature<float>("pixelRatio", bind(&WorldCamera::pixelRatio, camera));
  callbacks.registerCallback("setPixelRatio", [camera](float pixelRatio, Maybe<bool> smooth) {
    if (smooth.value())
      camera->setTargetPixelRatio(pixelRatio);
    else
      camera->setPixelRatio(pixelRatio);
    Root::singleton().configuration()->set("zoomLevel", pixelRatio);
  });

  callbacks.registerCallbackWithSignature<Vec2U>("screenSize", bind(&WorldCamera::screenSize, camera));
  callbacks.registerCallbackWithSignature<RectF>("worldScreenRect", bind(&WorldCamera::worldScreenRect, camera));
  callbacks.registerCallbackWithSignature<RectI>("worldTileRect", bind(&WorldCamera::worldTileRect, camera));
  callbacks.registerCallbackWithSignature<Vec2F>("tileMinScreen", bind(&WorldCamera::tileMinScreen, camera));
  callbacks.registerCallbackWithSignature<Vec2F, Vec2F>("screenToWorld", bind(&WorldCamera::screenToWorld, camera, _1));
  callbacks.registerCallbackWithSignature<Vec2F, Vec2F>("worldToScreen", bind(&WorldCamera::worldToScreen, camera, _1));

  return callbacks;
}

} // namespace Star
