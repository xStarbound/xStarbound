#include "StarChatTypes.hpp"
#include "StarCasting.hpp"
#include "StarDataStreamDevices.hpp"

namespace Star {

EnumMap<ChatSendMode> const ChatSendModeNames{
    {ChatSendMode::Broadcast, "Broadcast"},
    {ChatSendMode::Local, "Local"},
    {ChatSendMode::Party, "Party"}};

MessageContext::MessageContext() : mode() {}

MessageContext::MessageContext(Mode mode) : mode(mode) {}

MessageContext::MessageContext(Mode mode, String const& channelName) : mode(mode), channelName(channelName) {}

EnumMap<MessageContext::Mode> const MessageContextModeNames{
    {MessageContext::Mode::Local, "Local"},
    {MessageContext::Mode::Party, "Party"},
    {MessageContext::Mode::Broadcast, "Broadcast"},
    {MessageContext::Mode::Whisper, "Whisper"},
    {MessageContext::Mode::CommandResult, "CommandResult"},
    {MessageContext::Mode::RadioMessage, "RadioMessage"},
    {MessageContext::Mode::World, "World"}};

DataStream& operator>>(DataStream& ds, MessageContext& messageContext) {
  ds.read(messageContext.mode);
  ds.read(messageContext.channelName);

  return ds;
}

DataStream& operator<<(DataStream& ds, MessageContext const& messageContext) {
  ds.write(messageContext.mode);
  ds.write(messageContext.channelName);

  return ds;
}

ChatReceivedMessage::ChatReceivedMessage() : fromConnection() {}

ChatReceivedMessage::ChatReceivedMessage(MessageContext context, ConnectionId fromConnection, String const& fromNick, String const& text, JsonObject const& metadata)
    : context(context), fromConnection(fromConnection), fromNick(fromNick), text(text), metadata(metadata) {}

ChatReceivedMessage::ChatReceivedMessage(MessageContext context, ConnectionId fromConnection, String const& fromNick, String const& text, String const& portrait, JsonObject const& metadata)
    : context(context), fromConnection(fromConnection), fromNick(fromNick), portrait(portrait), text(text), metadata(metadata) {}

ChatReceivedMessage::ChatReceivedMessage(Json const& json) : ChatReceivedMessage() {
  auto jContext = json.get("context");
  context = MessageContext(
      MessageContextModeNames.getLeft(jContext.getString("mode")),
      jContext.getString("channelName", ""));
  fromConnection = json.getUInt("fromConnection", 0);
  fromNick = json.getString("fromNick", "");
  portrait = json.getString("portrait", "");
  text = json.getString("text", "");
  metadata = json.getObject("data", JsonObject{});
}

Json ChatReceivedMessage::toJson() const {
  return JsonObject{
      {"context", JsonObject{
                      {"mode", MessageContextModeNames.getRight(context.mode)},
                      {"channelName", context.channelName.empty() ? Json() : Json(context.channelName)}}},
      {"fromConnection", fromConnection}, {"fromNick", fromNick.empty() ? Json() : fromNick}, {"portrait", portrait.empty() ? Json() : portrait}, {"text", text}, {"data", metadata}};
}

DataStream& operator>>(DataStream& ds, ChatReceivedMessage& receivedMessage) {
  ds.read(receivedMessage.context);
  ds.read(receivedMessage.fromConnection);
  ds.read(receivedMessage.fromNick);
  ds.read(receivedMessage.portrait);
  ds.read(receivedMessage.text);
  ds.read(receivedMessage.metadata);
  return ds;
}

DataStream& operator<<(DataStream& ds, ChatReceivedMessage const& receivedMessage) {
  ds.write(receivedMessage.context);
  ds.write(receivedMessage.fromConnection);
  ds.write(receivedMessage.fromNick);
  ds.write(receivedMessage.portrait);
  ds.write(receivedMessage.text);
  ds.write(receivedMessage.metadata);
  return ds;
}

DataStream& ChatReceivedMessage::readLegacy(DataStream& ds, ChatReceivedMessage& receivedMessage) {
  ds.read(receivedMessage.context);
  ds.read(receivedMessage.fromConnection);
  ds.read(receivedMessage.fromNick);
  ds.read(receivedMessage.portrait);
  ds.read(receivedMessage.text);
  return ds;
}

DataStream& ChatReceivedMessage::writeLegacy(DataStream& ds, ChatReceivedMessage const& receivedMessage) {
  ds.write(receivedMessage.context);
  ds.write(receivedMessage.fromConnection);
  ds.write(receivedMessage.fromNick);
  ds.write(receivedMessage.portrait);
  ds.write(receivedMessage.text);
  return ds;
}

} // namespace Star
