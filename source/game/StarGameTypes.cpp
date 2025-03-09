#include "StarGameTypes.hpp"

namespace Star {

float GlobalTimescale = 1.0f;
float GlobalTimestep = 1.0f / 60.0f;
float ServerGlobalTimestep = 1.0f / 60.0f;

EnumMap<Direction> const DirectionNames{
  {Direction::Left, "left"},
  {Direction::Right, "right"},
};

EnumMap<Gender> const GenderNames{
  {Gender::Male, "male"},
  {Gender::Female, "female"},
};

EnumMap<FireMode> const FireModeNames{
  {FireMode::None, "none"},
  {FireMode::Primary, "primary"},
  {FireMode::Alt, "alt"}
};

EnumMap<ToolHand> const ToolHandNames{
  {ToolHand::Primary, "primary"},
  {ToolHand::Alt, "alt"}
};

EnumMap<TileLayer> const TileLayerNames{
  {TileLayer::Foreground, "foreground"},
  {TileLayer::Background, "background"}
};

EnumMap<MoveControlType> const MoveControlTypeNames{
  {MoveControlType::Left, "left"},
  {MoveControlType::Right, "right"},
  {MoveControlType::Down, "down"},
  {MoveControlType::Up, "up"},
  {MoveControlType::Jump, "jump"}
};

EnumMap<PortraitMode> const PortraitModeNames{
  {PortraitMode::Head, "head"},
  {PortraitMode::Bust, "bust"},
  {PortraitMode::Full, "full"},
  {PortraitMode::FullNeutral, "fullneutral"},
  {PortraitMode::FullNude, "fullnude"},
  {PortraitMode::FullNeutralNude, "fullneutralnude"}
};

EnumMap<Rarity> const RarityNames{
  {Rarity::Common, "common"},
  {Rarity::Uncommon, "uncommon"},
  {Rarity::Rare, "rare"},
  {Rarity::Legendary, "legendary"},
  {Rarity::Essential, "essential"},
  // FezzedOne: New colour "rarities".
  {Rarity::Colour1, "colour1"},
  {Rarity::Colour2, "colour2"},
  {Rarity::Colour3, "colour3"},
  {Rarity::Colour4, "colour4"},
  {Rarity::Colour5, "colour5"},
  {Rarity::Colour6, "colour6"},
  {Rarity::Colour7, "colour7"},
  {Rarity::Colour8, "colour8"},
  {Rarity::Colour9, "colour9"},
  {Rarity::Colour10, "colour10"},
  {Rarity::Colour11, "colour11"},
  {Rarity::Colour12, "colour12"},
  {Rarity::Colour13, "colour13"},
  {Rarity::Colour14, "colour14"},
  {Rarity::Colour15, "colour15"},
  {Rarity::Colour16, "colour16"}
};

std::pair<EntityId, EntityId> connectionEntitySpace(ConnectionId connectionId) {
  if (connectionId == ServerConnectionId) {
    return {MinServerEntityId + 1, MaxServerEntityId};
  } else if (connectionId >= MinClientConnectionId && connectionId <= MaxClientConnectionId) {
    EntityId beginIdSpace = (EntityId)connectionId * -65536;
    EntityId endIdSpace = beginIdSpace + 65535;
    return {beginIdSpace, endIdSpace};
  } else {
    throw StarException::format("Invalid connection id in clientEntitySpace({})", connectionId);
  }
}

bool entityIdInSpace(EntityId entityId, ConnectionId connectionId) {
  auto pair = connectionEntitySpace(connectionId);
  return entityId >= pair.first && entityId <= pair.second;
}

ConnectionId connectionForEntity(EntityId entityId) {
  if (entityId > 0)
    return ServerConnectionId;
  else
    return (-entityId - 1) / 65536 + 1;
}

pair<float, Direction> getAngleSide(float angle, bool ccRotation) {
  angle = constrainAngle(angle);
  Direction direction = Direction::Right;
  if (angle > Constants::pi / 2) {
    direction = Direction::Left;
    angle = Constants::pi - angle;
  } else if (angle < -Constants::pi / 2) {
    direction = Direction::Left;
    angle = -Constants::pi - angle;
  }

  if (direction == Direction::Left && ccRotation)
    angle *= -1;

  return make_pair(angle, direction);
}

}
