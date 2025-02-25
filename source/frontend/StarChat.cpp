#include "StarChat.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarUniverseClient.hpp"
#include "StarButtonWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarImageStretchWidget.hpp"
#include "StarCanvasWidget.hpp"
#include "StarAssets.hpp"
#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarPlayerStorage.hpp"
#include "StarTeamClient.hpp"

namespace Star {

Chat::Chat(UniverseClientPtr client, Maybe<ChatState> chatState) : m_client(client) {
  m_chatPrevIndex = 0;
  m_historyOffset = 0;

  auto root = Root::singletonPtr();
  auto assets = root->assets();
  m_timeChatLastActive = Time::monotonicMilliseconds();
  auto fontConfig = assets->json("/interface/chat/chat.config:config.font");
  m_fontSize = fontConfig.getInt("baseSize");
  m_fontDirectives = fontConfig.queryString("directives", "");
  m_font = fontConfig.queryString("type", "");
  m_chatLineHeight = assets->json("/interface/chat/chat.config:config.lineHeight").toFloat();
  m_chatVisTime = assets->json("/interface/chat/chat.config:config.visTime").toFloat();
  m_fadeRate = assets->json("/interface/chat/chat.config:config.fadeRate").toDouble();
  m_chatHistoryLimit = assets->json("/interface/chat/chat.config:config.chatHistoryLimit").toInt();

  m_portraitTextOffset = jsonToVec2I(assets->json("/interface/chat/chat.config:config.portraitTextOffset"));
  m_portraitImageOffset = jsonToVec2I(assets->json("/interface/chat/chat.config:config.portraitImageOffset"));
  m_portraitScale = assets->json("/interface/chat/chat.config:config.portraitScale").toFloat();
  m_portraitVerticalMargin = assets->json("/interface/chat/chat.config:config.portraitVerticalMargin").toFloat();
  m_portraitBackground = assets->json("/interface/chat/chat.config:config.portraitBackground").toString();

  m_bodyHeight = assets->json("/interface/chat/chat.config:config.bodyHeight").toInt();
  m_expandedBodyHeight = assets->json("/interface/chat/chat.config:config.expandedBodyHeight").toInt();

  m_colorCodes[MessageContext::Local] = assets->json("/interface/chat/chat.config:config.colors.local").toString();
  m_colorCodes[MessageContext::Party] = assets->json("/interface/chat/chat.config:config.colors.party").toString();
  m_colorCodes[MessageContext::Broadcast] = assets->json("/interface/chat/chat.config:config.colors.broadcast").toString();
  m_colorCodes[MessageContext::Whisper] = assets->json("/interface/chat/chat.config:config.colors.whisper").toString();
  m_colorCodes[MessageContext::CommandResult] = assets->json("/interface/chat/chat.config:config.colors.commandResult").toString();
  m_colorCodes[MessageContext::RadioMessage] = assets->json("/interface/chat/chat.config:config.colors.radioMessage").toString();
  m_colorCodes[MessageContext::World] = assets->json("/interface/chat/chat.config:config.colors.world").toString();

  GuiReader reader;

  reader.registerCallback("textBox", [=](Widget*) { startChat(); });
  reader.registerCallback("upButton", [=](Widget*) { scrollUp(); });
  reader.registerCallback("downButton", [=](Widget*) { scrollDown(); });
  reader.registerCallback("bottomButton", [=](Widget*) { scrollBottom(); });

  reader.registerCallback("filterGroup", [=](Widget* widget) {
      Json data = as<ButtonWidget>(widget)->data();
      auto filter = data.getArray("filter", {});
      int filterId = as<ButtonGroup>(widget->parent())->checkedId();
      m_modeFilter.clear();
      for (auto mode : filter)
        m_modeFilter.insert(MessageContextModeNames.getLeft(mode.toString()));
      m_sendMode = ChatSendModeNames.getLeft(data.getString("sendMode", "Broadcast"));
      m_historyOffset = 0;
      Root::singleton().configuration()->set("savedChatState", JsonObject{
        {"mode", ChatSendModeNames.getRight(m_sendMode)},
        {"expanded", m_expanded},
        {"filter", filterId}
      });
    });

  Maybe<String> defaultSendMode = assets->json("/interface/chat/chat.config").optString("defaultSendMode");
  if (defaultSendMode) {
    // FezzedOne: Allow setting the default chat send mode in `/interface/chat/chat.config`.
    m_sendMode = ChatSendModeNames.valueLeft(*defaultSendMode, ChatSendMode::Broadcast);
  } else {
    m_sendMode = ChatSendMode::Broadcast;
  }

  reader.construct(assets->json("/interface/chat/chat.config:gui"), this);

  m_textBox = fetchChild<TextBoxWidget>("textBox");
  m_say = fetchChild<LabelWidget>("say");

  m_chatLog = fetchChild<CanvasWidget>("chatLog");
  if (auto logPadding = fontConfig.optQuery("padding")) {
    m_chatLogPadding = jsonToVec2I(logPadding.get());
    m_chatLog->setSize(m_chatLog->size() + m_chatLogPadding * 2);
    m_chatLog->setPosition(m_chatLog->position() - m_chatLogPadding);
  }
  else
    m_chatLogPadding = Vec2I();

  m_bottomButton = fetchChild<ButtonWidget>("bottomButton");
  m_upButton = fetchChild<ButtonWidget>("upButton");

  m_chatHistory.appendAll(m_client->playerStorage()->getMetadata("chatHistory").opt().apply(jsonToStringList).value());

  auto messagesFile = root->toStoragePath("messages.json");
  if (File::isFile(messagesFile)) {
    try {
      auto chatMessages = Json::parseJson(File::readFileString(messagesFile)).toArray();
      for (auto& rawMessage : chatMessages) {
        if (auto message = rawMessage.optObject()) {
          String mode = (*message).get("mode").toString(),
                portrait = (*message).get("portrait").toString(),
                text = (*message).get("text").toString();
          m_receivedMessages.prepend({
            MessageContextModeNames.valueLeft(mode, MessageContext::Mode::Local),
            portrait,
            text
          });
        }
      }
      // Logger::info("Read chat messages from '{}'", messagesFile);
    } catch (std::exception const& e) {
      m_receivedMessages.clear();
      Logger::error("Exception while reading chat messages from '{}': {}", messagesFile, e.what());
    }
  }

  m_expanded = false;

  Json chatSettings = root->configuration()->get("savedChatState");
  if (chatSettings.isType(Json::Type::Object)) {
    if (auto savedChatMode = chatSettings.opt("mode")) {
      m_sendMode = savedChatMode->isType(Json::Type::String)
        ? ChatSendModeNames.valueLeft(savedChatMode->toString(), m_sendMode)
        : m_sendMode;
    }
    if (auto savedExpanded = chatSettings.opt("expanded")) {
      m_expanded = savedExpanded->isType(Json::Type::Bool) ? savedExpanded->toBool() : m_expanded;
    }
    if (auto savedFilterId = chatSettings.opt("filter")) {
      if (savedFilterId->isType(Json::Type::Int)) {
        int filterId = (int)savedFilterId->toInt();
        auto tabGroup = fetchChild<ButtonGroup>("filterGroup");
        if (tabGroup->button(filterId))
          tabGroup->select(filterId);
      }
    }
  }

  if (chatState) {
    m_textBox->setText(std::move(chatState->chatText));
    m_timeChatLastActive = chatState->timeChatLastActive;
    m_historyOffset = chatState->historyOffset;
    m_chatPrevIndex = chatState->chatPrevIndex;
    m_sendMode = chatState->sendMode;
    auto tabGroup = fetchChild<ButtonGroup>("filterGroup");
    if (tabGroup->button(chatState->filterId))
      tabGroup->select(chatState->filterId);
    m_expanded = chatState->expanded;
  }

  show();

  updateBottomButton();

  m_background = fetchChild<ImageStretchWidget>("background");
  m_defaultHeight = m_background->size()[1];
  updateSize();
}

void Chat::update(float dt) {
  Pane::update(dt);

  auto team = m_client->teamClient()->currentTeam();
  for (auto button : fetchChild<ButtonGroup>("filterGroup")->buttons()) {
    auto mode = ChatSendModeNames.getLeft(button->data().getString("sendMode", "Broadcast"));
    if (!team.isValid() && m_sendMode == ChatSendMode::Party && mode == ChatSendMode::Broadcast)
      button->check();
    if (mode == ChatSendMode::Party)
      button->setEnabled(team.isValid());
  }
}

void Chat::startChat() {
  show();
  m_textBox->focus();
}

void Chat::startCommand() {
  show();
  m_textBox->setText("/");
  m_textBox->focus();
}

bool Chat::hasFocus() const {
  return m_textBox->hasFocus();
}

void Chat::stopChat() {
  m_textBox->setText("");
  m_textBox->blur();
  m_timeChatLastActive = Time::monotonicMilliseconds();
}

String Chat::currentChat() const {
  return m_textBox->getText();
}

void Chat::setCurrentChat(String const& chat) {
  m_textBox->setText(chat);
}

void Chat::clearCurrentChat() {
  m_textBox->setText("");
  m_chatPrevIndex = 0;
}

ChatSendMode Chat::sendMode() const {
  return m_sendMode;
}

void Chat::incrementIndex() {
  if (!m_chatHistory.empty()) {
    m_chatPrevIndex = std::min(m_chatPrevIndex + 1, (unsigned)m_chatHistory.size());
    m_textBox->setText(m_chatHistory.at(m_chatPrevIndex - 1));
  }
}

void Chat::decrementIndex() {
  if (m_chatPrevIndex > 1 && !m_chatHistory.empty()) {
    --m_chatPrevIndex;
    m_textBox->setText(m_chatHistory.at(m_chatPrevIndex - 1));
  } else {
    m_chatPrevIndex = 0;
    m_textBox->setText("");
  }
}

void Chat::addLine(String const& text, bool showPane) {
  ChatReceivedMessage message = {{MessageContext::CommandResult}, ServerConnectionId, "", text};
  addMessages({message}, showPane);
}

void Chat::addMessages(List<ChatReceivedMessage> const& messages, bool showPane) {
  if (messages.empty())
    return;

  GuiContext& guiContext = GuiContext::singleton();

  for (auto const& message : messages) {
    Maybe<unsigned> wrapWidth;
    if (message.portrait.empty())
      wrapWidth = m_chatLog->size()[0];

    guiContext.setFont(m_font);
    guiContext.setFontSize(m_fontSize);
    StringList lines;
    if (message.fromNick != "" && message.portrait == "")
      // FezzedOne: Since the chat renderer already wraps text, let's try *not* wrapping text twice.
      lines = StringList{strf("<{}^reset;> {}", message.fromNick, message.text)};
      // lines = guiContext.wrapInterfaceText(strf("<{}^reset;> {}", message.fromNick, message.text), wrapWidth);
    else
      lines = StringList{message.text};
      // lines = guiContext.wrapInterfaceText(message.text, wrapWidth);

    for (size_t i = 0; i < lines.size(); ++i) {
      m_receivedMessages.prepend({
          message.context.mode,
          message.portrait,
          std::move(lines[i])
        });
    }

    if (message.fromNick != "")
      Logger::info("Chat: <{}> {}", message.fromNick, message.text);
    else
      Logger::info("Chat: {}", message.text);
  }

  if (showPane) {
    m_timeChatLastActive = Time::monotonicMilliseconds();
    show();
  }

  m_receivedMessages.resize(std::min((unsigned)m_receivedMessages.size(), m_chatHistoryLimit));
}

void Chat::clearMessages(Maybe<size_t> numMessages) {
  if (m_receivedMessages.empty())
    return;

  if (numMessages) {
    size_t n = *numMessages;
    for (size_t i = 0; i < n; ++i) {
      if (m_receivedMessages.empty()) break;
      m_receivedMessages.pop_front();
    }
  } else {
    m_receivedMessages.clear();
  }
}

void Chat::saveMessages() {
  auto root = Root::singletonPtr();
  auto messagesFile = root->toStoragePath("messages.json");
  try {
    JsonArray messagesToSave{};
    for (auto message : m_receivedMessages) {
      messagesToSave.append(JsonObject{
        {"mode", MessageContextModeNames.valueRight(message.mode, "Local")},
        {"portrait", message.portrait},
        {"text", message.text}
      });
    }
    messagesToSave.reverse();
    File::writeFile(Json(messagesToSave).printJson(2), messagesFile);
    // Logger::info("Saved chat messages to '{}'", messagesFile);
  } catch (std::exception const& e) {
    Logger::error("Exception while saving chat messages to '{}': {}", messagesFile, e.what());
  }
}

void Chat::addHistory(String const& chat) {
  if (m_chatHistory.size() > 0 && m_chatHistory.get(0).equals(chat))
    return;

  m_chatHistory.prepend(chat);
  m_chatHistory.resize(std::min((unsigned)m_chatHistory.size(), m_chatHistoryLimit));
  m_timeChatLastActive = Time::monotonicMilliseconds();
  m_client->playerStorage()->setMetadata("chatHistory", JsonArray::from(m_chatHistory));
}

void Chat::renderImpl() {
  Pane::renderImpl();
  if (m_textBox->hasFocus())
    m_timeChatLastActive = Time::monotonicMilliseconds();
  Vec4B fade = {255, 255, 255, 255};
  fade[3] = (uint8_t)(visible() * 255);
  if (!visible()) {
    hide();
    return;
  }

  Color fadeGreen = Color::Green;
  fadeGreen.setAlpha(fade[3]);
  m_say->setColor(fadeGreen);

  m_chatLog->clear();
  Vec2I chatMin = m_chatLogPadding;
  int messageIndex = -m_historyOffset;

  GuiContext& guiContext = GuiContext::singleton();
  guiContext.setFont(m_font);
  guiContext.setFontSize(m_fontSize);
  guiContext.setLineSpacing(m_chatLineHeight);
  for (auto message : m_receivedMessages) {
    if (!m_modeFilter.empty() && !m_modeFilter.contains(message.mode))
      continue;

    messageIndex++;
    if (messageIndex <= 0)
      continue;
    if (chatMin[1] > m_chatLog->size()[1])
      break;

    String channelColorCode = "^reset";
    if (m_colorCodes.contains(message.mode))
      channelColorCode = m_colorCodes[message.mode];
    channelColorCode += "^set;";

    String messageString = channelColorCode + message.text;

    float messageHeight = 0;
    float lineHeightMargin =  + ((m_chatLineHeight * m_fontSize) - m_fontSize);
    unsigned wrapWidth = m_chatLog->size()[0] - m_chatLogPadding[0];

    if (message.portrait != "") {
      TextPositioning tp = {Vec2F(chatMin +  m_portraitTextOffset), HorizontalAnchor::LeftAnchor, VerticalAnchor::VMidAnchor, (wrapWidth - m_portraitTextOffset[0])};
      Vec2F textSize = guiContext.determineInterfaceTextSize(messageString, tp).size().floor();
      Vec2F portraitSize = Vec2F(guiContext.textureSize(m_portraitBackground));
      messageHeight = max(portraitSize[1] + m_portraitVerticalMargin, textSize[1] + lineHeightMargin);

      // Draw both image and text anchored left and centered vertically
      auto imagePosition = chatMin + Vec2I(0, floor(messageHeight / 2)) - Vec2I(0, floor(portraitSize[1] / 2));
      m_chatLog->drawImage(m_portraitBackground, Vec2F(imagePosition), 1.0f, fade);
      m_chatLog->drawImage(message.portrait, Vec2F(imagePosition + m_portraitImageOffset), m_portraitScale, fade);
      tp.pos += Vec2F(0, floor(messageHeight / 2));
      m_chatLog->drawText(messageString, tp, m_fontSize, fade, FontMode::Normal, m_chatLineHeight, m_font, m_fontDirectives);

    } else {
      TextPositioning tp = {Vec2F(chatMin), HorizontalAnchor::LeftAnchor, VerticalAnchor::BottomAnchor, wrapWidth};
      messageHeight = guiContext.determineInterfaceTextSize(messageString, tp).size()[1] + lineHeightMargin;

      m_chatLog->drawText(messageString, tp, m_fontSize, fade, FontMode::Normal, m_chatLineHeight, m_font, m_fontDirectives);
    }

    chatMin[1] += ceil(messageHeight);
  }

  guiContext.setDefaultLineSpacing();
  guiContext.setDefaultFont();
}

void Chat::hide() {
  stopChat();
  Pane::hide();
}

float Chat::visible() const {
  double difference = (Time::monotonicMilliseconds() - m_timeChatLastActive) / 1000.0;
  if (difference < m_chatVisTime)
    return 1;
  return clamp<float>(1 - (difference - m_chatVisTime) / m_fadeRate, 0, 1);
}

bool Chat::sendEvent(InputEvent const& event) {
  if (active()) {
    if (hasFocus()) {
      if (event.is<KeyDownEvent>()) {
        auto actions = context()->actions(event);
        if (actions.contains(InterfaceAction::ChatStop)) {
          stopChat();
          return true;
        } else if (actions.contains(InterfaceAction::ChatPreviousLine)) {
          incrementIndex();
          return true;
        } else if (actions.contains(InterfaceAction::ChatNextLine)) {
          decrementIndex();
          return true;
        } else if (actions.contains(InterfaceAction::ChatPageDown)) {
          scrollDown();
          return true;
        } else if (actions.contains(InterfaceAction::ChatPageUp)) {
          scrollUp();
          return true;
        }
      }
    }

    if (auto mouseWheel = event.ptr<MouseWheelEvent>()) {
      if (inMember(*context()->mousePosition(event))) {
        if (mouseWheel->mouseWheel == MouseWheel::Down)
          scrollDown();
        else
          scrollUp();
        return true;
      }
    }

    if (event.is<MouseMoveEvent>() && inMember(*context()->mousePosition(event)))
      m_timeChatLastActive = Time::monotonicMilliseconds();

    if (event.is<MouseButtonDownEvent>()) {
      if (m_chatLog->inMember(*context()->mousePosition(event))) {
        m_expanded = !m_expanded;
        updateSize();
        Root::singleton().configuration()->set("savedChatState", JsonObject{
          {"mode", ChatSendModeNames.getRight(m_sendMode)},
          {"expanded", m_expanded},
          {"filter", fetchChild<ButtonGroup>("filterGroup")->checkedId()}
        });
        return true;
      }
    }
  }

  return Pane::sendEvent(event);
}

void Chat::scrollUp() {
  auto shownMessages = m_receivedMessages.filtered([=](LogMessage msg) {
      return (m_modeFilter.empty() || m_modeFilter.contains(msg.mode));
    });

  m_historyOffset = std::max(0, std::min((int)shownMessages.size() - 1, m_historyOffset + 1));
  m_timeChatLastActive = Time::monotonicMilliseconds();
  updateBottomButton();
}

void Chat::scrollDown() {
  m_historyOffset = std::max(0, m_historyOffset - 1);
  m_timeChatLastActive = Time::monotonicMilliseconds();
  updateBottomButton();
}

void Chat::scrollBottom() {
  m_historyOffset = 0;
  m_timeChatLastActive = Time::monotonicMilliseconds();
  updateBottomButton();
}

void Chat::updateSize() {
  auto height = m_expanded ? m_expandedBodyHeight : m_bodyHeight;
  m_background->setSize(Vec2I(m_background->size()[0], m_defaultHeight + height));
  m_chatLog->setSize(Vec2I(m_chatLog->size()[0], height));
  m_upButton->setPosition(Vec2I(m_upButton->position()[0], m_chatLog->position()[1] + m_chatLog->size()[1] - m_upButton->size()[1]));
  determineSizeFromChildren();
}

void Chat::updateBottomButton() {
  auto assets = Root::singleton().assets();
  auto bottomConfig = assets->json("/interface/chat/chat.config:bottom");
  if (m_historyOffset == 0)
    m_bottomButton->setImages(bottomConfig.get("atbottom").getString("base"), bottomConfig.get("atbottom").getString("hover"));
  else
    m_bottomButton->setImages(bottomConfig.get("scrolling").getString("base"), bottomConfig.get("scrolling").getString("hover"));
}

ChatState Chat::getState() {
  return ChatState{
    m_textBox->getText(),
    m_timeChatLastActive,
    m_historyOffset,
    m_chatPrevIndex,
    m_sendMode,
    fetchChild<ButtonGroup>("filterGroup")->checkedId(),
    m_expanded
  };
}

}
