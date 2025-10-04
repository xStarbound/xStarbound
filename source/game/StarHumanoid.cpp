#include "StarHumanoid.hpp"
#include "StarArmors.hpp"
#include "StarAssets.hpp"
#include "StarDanceDatabase.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarJsonExtra.hpp"
#include "StarParticleDatabase.hpp"
#include "StarRoot.hpp"
#include "StarSpeciesDatabase.hpp"
#include <cstddef>

namespace Star {

extern EnumMap<HumanoidEmote> const HumanoidEmoteNames{
    {HumanoidEmote::Idle, "Idle"},
    {HumanoidEmote::Blabbering, "Blabbering"},
    {HumanoidEmote::Shouting, "Shouting"},
    {HumanoidEmote::Happy, "Happy"},
    {HumanoidEmote::Sad, "Sad"},
    {HumanoidEmote::NEUTRAL, "NEUTRAL"},
    {HumanoidEmote::Laugh, "Laugh"},
    {HumanoidEmote::Annoyed, "Annoyed"},
    {HumanoidEmote::Oh, "Oh"},
    {HumanoidEmote::OOOH, "OOOH"},
    {HumanoidEmote::Blink, "Blink"},
    {HumanoidEmote::Wink, "Wink"},
    {HumanoidEmote::Eat, "Eat"},
    {HumanoidEmote::Sleep, "Sleep"}};

Personality parsePersonalityArray(Json const& config) {
  return Personality{config.getString(0), config.getString(1), jsonToVec2F(config.get(2)), jsonToVec2F(config.get(3))};
}

Personality& parsePersonality(Personality& personality, Json const& config) {
  if (auto idle = config.opt("idle"))
    personality.idle = idle->toString();
  if (auto armIdle = config.opt("armIdle"))
    personality.armIdle = armIdle->toString();
  if (auto headOffset = config.opt("headOffset"))
    personality.headOffset = jsonToVec2F(*headOffset);
  if (auto armOffset = config.opt("armOffset"))
    personality.armOffset = jsonToVec2F(*armOffset);

  return personality;
}

Personality parsePersonality(Json const& config) {
  Personality personality;
  return parsePersonality(personality, config);
}

Json jsonFromPersonality(Personality const& personality) {
  return JsonObject{
      {"idle", personality.idle},
      {"armIdle", personality.armIdle},
      {"headOffset", jsonFromVec2F(personality.headOffset)},
      {"armOffset", jsonFromVec2F(personality.armOffset)}};
}

HumanoidIdentity::HumanoidIdentity(Json config) {
  if (config.isNull())
    config = JsonObject();

  name = config.getString("name", "Humanoid");
  species = config.getString("species", "human");
  gender = GenderNames.getLeft(config.getString("gender", "male"));
  hairGroup = config.getString("hairGroup", "hair");
  hairType = config.getString("hairType", "male1");
  hairDirectives = config.getString("hairDirectives", "");
  bodyDirectives = config.getString("bodyDirectives", "");
  if (auto jEmoteDirectives = config.optString("emoteDirectives")) // Passing Directives as a default arg would be inefficient
    emoteDirectives = jEmoteDirectives.take();
  else
    emoteDirectives = bodyDirectives;

  facialHairGroup = config.getString("facialHairGroup", "");
  facialHairType = config.getString("facialHairType", "");
  facialHairDirectives = config.getString("facialHairDirectives", "");
  facialMaskGroup = config.getString("facialMaskGroup", "");
  facialMaskType = config.getString("facialMaskType", "");
  facialMaskDirectives = config.getString("facialMaskDirectives", "");

  personality.idle = config.getString("personalityIdle", "idle.1");
  personality.armIdle = config.getString("personalityArmIdle", "idle.1");
  personality.headOffset = jsonToVec2F(config.get("personalityHeadOffset", JsonArray{0, 0}));
  personality.armOffset = jsonToVec2F(config.get("personalityArmOffset", JsonArray{0, 0}));

  color = jsonToColor(config.get("color", "white")).toRgba();

  imagePath = config.optString("imagePath");
}

Json HumanoidIdentity::toJson() const {
  auto result = JsonObject{
      {"name", name},
      {"species", species},
      {"gender", GenderNames.getRight(gender)},
      {"hairGroup", hairGroup},
      {"hairType", hairType},
      {"hairDirectives", hairDirectives.string()},
      {"bodyDirectives", bodyDirectives.string()},
      {"emoteDirectives", emoteDirectives.string()},
      {"facialHairGroup", facialHairGroup},
      {"facialHairType", facialHairType},
      {"facialHairDirectives", facialHairDirectives.string()},
      {"facialMaskGroup", facialMaskGroup},
      {"facialMaskType", facialMaskType},
      {"facialMaskDirectives", facialMaskDirectives.string()},
      {"personalityIdle", personality.idle},
      {"personalityArmIdle", personality.armIdle},
      {"personalityHeadOffset", jsonFromVec2F(personality.headOffset)},
      {"personalityArmOffset", jsonFromVec2F(personality.armOffset)},
      {"color", jsonFromColor(Color::rgba(color))}};
  if (imagePath)
    result["imagePath"] = *imagePath;
  return result;
}

DataStream& operator>>(DataStream& ds, HumanoidIdentity& identity) {
  ds.read(identity.name);
  ds.read(identity.species);
  ds.read(identity.gender);
  ds.read(identity.hairGroup);
  ds.read(identity.hairType);
  ds.read(identity.hairDirectives);
  ds.read(identity.bodyDirectives);
  ds.read(identity.emoteDirectives);
  ds.read(identity.facialHairGroup);
  ds.read(identity.facialHairType);
  ds.read(identity.facialHairDirectives);
  ds.read(identity.facialMaskGroup);
  ds.read(identity.facialMaskType);
  ds.read(identity.facialMaskDirectives);
  ds.read(identity.personality.idle);
  ds.read(identity.personality.armIdle);
  ds.read(identity.personality.headOffset);
  ds.read(identity.personality.armOffset);
  ds.read(identity.color);
  ds.read(identity.imagePath);

  return ds;
}

DataStream& operator<<(DataStream& ds, HumanoidIdentity const& identity) {
  ds.write(identity.name);
  ds.write(identity.species);
  ds.write(identity.gender);
  ds.write(identity.hairGroup);
  ds.write(identity.hairType);
  ds.write(identity.hairDirectives);
  ds.write(identity.bodyDirectives);
  ds.write(identity.emoteDirectives);
  ds.write(identity.facialHairGroup);
  ds.write(identity.facialHairType);
  ds.write(identity.facialHairDirectives);
  ds.write(identity.facialMaskGroup);
  ds.write(identity.facialMaskType);
  ds.write(identity.facialMaskDirectives);
  ds.write(identity.personality.idle);
  ds.write(identity.personality.armIdle);
  ds.write(identity.personality.headOffset);
  ds.write(identity.personality.armOffset);
  ds.write(identity.color);
  ds.write(identity.imagePath);

  return ds;
}

Humanoid::HumanoidTiming::HumanoidTiming(Json config) {
  if (config.type() != Json::Type::Object) {
    auto assets = Root::singleton().assets();
    config = assets->json("/humanoid.config:humanoidTiming");
  }

  if (config.contains("stateCycle"))
    stateCycle = jsonToArrayF<STATESIZE>(config.get("stateCycle"));
  if (config.contains("stateFrames"))
    stateFrames = jsonToArrayU<STATESIZE>(config.get("stateFrames"));

  if (config.contains("emoteCycle"))
    emoteCycle = jsonToArrayF<EmoteSize>(config.get("emoteCycle"));
  if (config.contains("emoteFrames"))
    emoteFrames = jsonToArrayU<EmoteSize>(config.get("emoteFrames"));
}

bool Humanoid::HumanoidTiming::cyclicState(State state) {
  switch (state) {
    case State::Walk:
    case State::Run:
    case State::Swim:
      return true;
    default:
      return false;
  }
}

bool Humanoid::HumanoidTiming::cyclicEmoteState(HumanoidEmote state) {
  switch (state) {
    case HumanoidEmote::Blabbering:
    case HumanoidEmote::Shouting:
    case HumanoidEmote::Sad:
    case HumanoidEmote::Laugh:
    case HumanoidEmote::Eat:
    case HumanoidEmote::Sleep:
      return true;
    default:
      return false;
  }
}

int Humanoid::HumanoidTiming::stateSeq(float timer, State state) const {
  return genericSeq(timer, stateCycle[state], stateFrames[state], cyclicState(state));
}

int Humanoid::HumanoidTiming::emoteStateSeq(float timer, HumanoidEmote state) const {
  return genericSeq(timer, emoteCycle[(size_t)state], emoteFrames[(size_t)state], cyclicEmoteState(state));
}

int Humanoid::HumanoidTiming::danceSeq(float timer, DancePtr dance) const {
  return genericSeq(timer, dance->cycle, dance->steps.size(), dance->cyclic) - 1;
}

int Humanoid::HumanoidTiming::genericSeq(float timer, float cycle, unsigned frames, bool cyclic) const {
  if (cyclic) {
    timer = fmod(timer, cycle);
  }
  return clamp<int>(timer * frames / cycle, 0, frames - 1) + 1;
}

EnumMap<Humanoid::State> const Humanoid::StateNames{
    {Humanoid::State::Idle, "idle"},
    {Humanoid::State::Walk, "walk"},
    {Humanoid::State::Run, "run"},
    {Humanoid::State::Jump, "jump"},
    {Humanoid::State::Fall, "fall"},
    {Humanoid::State::Swim, "swim"},
    {Humanoid::State::SwimIdle, "swimIdle"},
    {Humanoid::State::Duck, "duck"},
    {Humanoid::State::Sit, "sit"},
    {Humanoid::State::Lay, "lay"},
};

Humanoid::Humanoid(Json const& config) {
  m_baseHumanoidConfig = config;
  m_previousOverrides = Json();
  updateHumanoidConfigOverrides(JsonObject{});

  m_twoHanded = false;
  m_primaryHand.holdingItem = false;
  m_altHand.holdingItem = false;

  m_movingBackwards = false;
  m_altHand.angle = 0;
  m_facingDirection = Direction::Left;
  m_rotation = 0;
  m_drawVaporTrail = false;
  m_state = State::Idle;
  m_emoteState = HumanoidEmote::Idle;
  m_dance = {};
  m_emoteAnimationTimer = 0;

  m_primaryHand.angle = 0;
  m_animationTimer = 0.0f;

  m_humanoidRotationSettings = HumanoidRotationSettings::Null;

  m_broadcastToStock = false;
}

Humanoid::Humanoid(HumanoidIdentity const& identity)
    : Humanoid(Root::singleton().speciesDatabase()->species(identity.species)->humanoidConfig()) {
  setIdentity(identity);
}

void Humanoid::setIdentity(HumanoidIdentity const& identity, Maybe<HumanoidIdentity> const& visualOverrides) {
  m_identity = identity;
  if (visualOverrides)
    m_visualIdentity = *visualOverrides;
  else
    m_visualIdentity = identity;
  m_headFrameset = getHeadFromIdentity();
  m_bodyFrameset = getBodyFromIdentity();
  m_emoteFrameset = getFacialEmotesFromIdentity();
  m_hairFrameset = getHairFromIdentity();
  m_facialHairFrameset = getFacialHairFromIdentity();
  m_facialMaskFrameset = getFacialMaskFromIdentity();
  m_backArmFrameset = getBackArmFromIdentity();
  m_frontArmFrameset = getFrontArmFromIdentity();
  m_vaporTrailFrameset = getVaporTrailFrameset();

  auto const& visualIdentity = visualOverrides ? *visualOverrides : identity;
  auto speciesDatabase = Root::singleton().speciesDatabase();
  Json config = JsonObject{};
  String resolvedSpecies = visualIdentity.imagePath ? *visualIdentity.imagePath : visualIdentity.species;
  if (speciesDatabase->allSpecies().contains(resolvedSpecies))
    config = speciesDatabase->species(resolvedSpecies)->humanoidConfig();

  bool useBodyMask = config.optBool("useBodyMask").value(false);
  bool useBodyHeadMask = config.optBool("useBodyHeadMask").value(false);

  m_humanoidRotationSettings = HumanoidRotationSettings::Null;
  m_humanoidRotationSettings |= useBodyMask ? HumanoidRotationSettings::UseBodyMask : HumanoidRotationSettings::Null;
  m_humanoidRotationSettings |= useBodyHeadMask ? HumanoidRotationSettings::UseBodyHeadMask : HumanoidRotationSettings::Null;
}

HumanoidIdentity const& Humanoid::identity() const {
  return m_identity;
}

HumanoidIdentity const& Humanoid::netIdentity() const {
  return m_broadcastToStock ? m_visualIdentity : m_identity;
}

void Humanoid::setHeadArmorDirectives(Directives directives) {
  m_headArmorDirectives = std::move(directives);
}

void Humanoid::setHeadArmorFrameset(String headFrameset) {
  m_headArmorFrameset = std::move(headFrameset);
}

void Humanoid::setChestArmorDirectives(Directives directives) {
  m_chestArmorDirectives = std::move(directives);
}

void Humanoid::setChestArmorFrameset(String chest) {
  m_chestArmorFrameset = std::move(chest);
}

void Humanoid::setBackSleeveFrameset(String backSleeveFrameset) {
  m_backSleeveFrameset = std::move(backSleeveFrameset);
}

void Humanoid::setFrontSleeveFrameset(String frontSleeveFrameset) {
  m_frontSleeveFrameset = std::move(frontSleeveFrameset);
}

void Humanoid::setLegsArmorDirectives(Directives directives) {
  m_legsArmorDirectives = std::move(directives);
}

void Humanoid::setLegsArmorFrameset(String legsFrameset) {
  m_legsArmorFrameset = std::move(legsFrameset);
}

void Humanoid::setBackArmorDirectives(Directives directives) {
  m_backArmorDirectives = std::move(directives);
}

void Humanoid::setBackArmorHeadRotation(bool rotation) {
  m_backArmorHeadRotation = rotation;
}

void Humanoid::setBackArmorFrameset(String backFrameset) {
  m_backArmorFrameset = std::move(backFrameset);
}

void Humanoid::setHelmetMaskDirectives(Directives helmetMaskDirectives) {
  m_helmetMaskDirectives = std::move(helmetMaskDirectives);
}

void Humanoid::setHeadArmorUnderlayDirectives(Directives directives) {
  m_headArmorUnderlayDirectives = std::move(directives);
}

void Humanoid::setHeadArmorUnderlayFrameset(String headFrameset) {
  m_headArmorUnderlayFrameset = std::move(headFrameset);
}

void Humanoid::setChestArmorUnderlayDirectives(Directives directives) {
  m_chestArmorUnderlayDirectives = std::move(directives);
}

void Humanoid::setChestArmorUnderlayFrameset(String chest) {
  m_chestArmorUnderlayFrameset = std::move(chest);
}

void Humanoid::setBackSleeveUnderlayFrameset(String backSleeveFrameset) {
  m_backSleeveUnderlayFrameset = std::move(backSleeveFrameset);
}

void Humanoid::setFrontSleeveUnderlayFrameset(String frontSleeveFrameset) {
  m_frontSleeveUnderlayFrameset = std::move(frontSleeveFrameset);
}

void Humanoid::setLegsArmorUnderlayDirectives(Directives directives) {
  m_legsArmorUnderlayDirectives = std::move(directives);
}

void Humanoid::setLegsArmorUnderlayFrameset(String legsFrameset) {
  m_legsArmorUnderlayFrameset = std::move(legsFrameset);
}

void Humanoid::setBackArmorUnderlayDirectives(Directives directives) {
  m_backArmorUnderlayDirectives = std::move(directives);
}

void Humanoid::setBackArmorUnderlayHeadRotation(bool rotation) {
  m_backArmorUnderlayHeadRotation = rotation;
}

void Humanoid::setBackArmorUnderlayFrameset(String backFrameset) {
  m_backArmorUnderlayFrameset = std::move(backFrameset);
}

void Humanoid::setHelmetMaskUnderlayDirectives(Directives helmetMaskDirectives) {
  m_helmetMaskUnderlayDirectives = std::move(helmetMaskDirectives);
}

void Humanoid::setBackArmorStack(List<BackEntry> const& newStack) {
  m_backArmorStack = std::move(newStack);
}

void Humanoid::setBackSleeveStack(List<ArmorEntry> const& newStack) {
  m_backSleeveStack = std::move(newStack);
}

void Humanoid::setLegsArmorStack(List<ArmorEntry> const& newStack) {
  m_legsArmorStack = std::move(newStack);
}

void Humanoid::setChestArmorStack(List<ArmorEntry> const& newStack) {
  m_chestArmorStack = std::move(newStack);
}

void Humanoid::setHeadArmorStack(List<ArmorEntry> const& newStack) {
  m_headArmorStack = std::move(newStack);
}

void Humanoid::setFrontSleeveStack(List<ArmorEntry> const& newStack) {
  m_frontSleeveStack = std::move(newStack);
}

void Humanoid::setBackArmorUnderlayStack(List<BackEntry> const& newStack) {
  m_backArmorUnderlayStack = std::move(newStack);
}

void Humanoid::setBackSleeveUnderlayStack(List<ArmorEntry> const& newStack) {
  m_backSleeveUnderlayStack = std::move(newStack);
}

void Humanoid::setLegsArmorUnderlayStack(List<ArmorEntry> const& newStack) {
  m_legsArmorUnderlayStack = std::move(newStack);
}

void Humanoid::setChestArmorUnderlayStack(List<ArmorEntry> const& newStack) {
  m_chestArmorUnderlayStack = std::move(newStack);
}

void Humanoid::setHeadArmorUnderlayStack(List<ArmorEntry> const& newStack) {
  m_headArmorUnderlayStack = std::move(newStack);
}

void Humanoid::setFrontSleeveUnderlayStack(List<ArmorEntry> const& newStack) {
  m_frontSleeveUnderlayStack = std::move(newStack);
}

void Humanoid::setBodyHidden(bool hidden) {
  m_bodyHidden = hidden;
}

void Humanoid::setState(State state) {
  if (m_state != state)
    m_animationTimer = 0.0f;
  m_state = state;
}

void Humanoid::setEmoteState(HumanoidEmote state) {
  if (m_emoteState != state)
    m_emoteAnimationTimer = 0.0f;
  m_emoteState = state;
}

void Humanoid::setDance(Maybe<String> const& dance) {
  if (m_dance != dance)
    m_danceTimer = 0.0f;
  m_dance = dance;
}

void Humanoid::setFacingDirection(Direction facingDirection) {
  m_facingDirection = facingDirection;
}

void Humanoid::setMovingBackwards(bool movingBackwards) {
  m_movingBackwards = movingBackwards;
}

void Humanoid::setRotation(float rotation) {
  m_rotation = rotation;
}

void Humanoid::setVaporTrail(bool enabled) {
  m_drawVaporTrail = enabled;
}

Humanoid::State Humanoid::state() const {
  return m_state;
}

HumanoidEmote Humanoid::emoteState() const {
  return m_emoteState;
}

Maybe<String> Humanoid::dance() const {
  return m_dance;
}

bool Humanoid::danceCyclicOrEnded() const {
  if (!m_dance)
    return false;

  auto danceDatabase = Root::singleton().danceDatabase();
  auto dance = danceDatabase->getDance(*m_dance);
  return dance->cyclic || m_danceTimer > dance->duration;
}

Direction Humanoid::facingDirection() const {
  return m_facingDirection;
}

bool Humanoid::movingBackwards() const {
  return m_movingBackwards;
}

void Humanoid::setPrimaryHandParameters(bool holdingItem, float angle, float itemAngle, bool twoHanded,
    bool recoil, bool outsideOfHand) {
  m_primaryHand.holdingItem = holdingItem;
  m_primaryHand.angle = angle;
  m_primaryHand.itemAngle = itemAngle;
  m_twoHanded = twoHanded;
  m_primaryHand.recoil = recoil;
  m_primaryHand.outsideOfHand = outsideOfHand;
}

void Humanoid::setPrimaryHandFrameOverrides(String backFrameOverride, String frontFrameOverride) {
  // Novaenia: some users stick directives in these?? better make sure they don't break with custom clothing
  // FezzedOne: This splits override directives from frame specifier and moved them behind clothing directives.
  // Unless the directives begin with a `??`.
  size_t backEnd = backFrameOverride.utf8().find('?');
  size_t frontEnd = frontFrameOverride.utf8().find('?');
  std::string backSubString = backEnd == NPos ? "" : backFrameOverride.utf8().substr(backEnd);
  std::string frontSubString = frontEnd == NPos ? "" : frontFrameOverride.utf8().substr(frontEnd);
  m_primaryHand.skipBackSplice = (backSubString.size() >= 2) && (backSubString[0] == '?') && (backSubString[1] == '?');
  m_primaryHand.skipFrontSplice = (frontSubString.size() >= 2) && (frontSubString[0] == '?') && (frontSubString[1] == '?');
  Directives backDirectives = Directives(backSubString);
  Directives frontDirectives = Directives(frontSubString);
  if (backEnd != NPos) backFrameOverride = backFrameOverride.utf8().substr(0, backEnd);
  if (frontEnd != NPos) frontFrameOverride = frontFrameOverride.utf8().substr(0, frontEnd);
  m_primaryHand.backFrame = !backFrameOverride.empty() ? std::move(backFrameOverride) : "rotation";
  m_primaryHand.frontFrame = !frontFrameOverride.empty() ? std::move(frontFrameOverride) : "rotation";
  m_primaryHand.backDirectives = !backDirectives.empty() ? std::move(backDirectives) : Directives();
  m_primaryHand.frontDirectives = !frontDirectives.empty() ? std::move(frontDirectives) : Directives();
}

void Humanoid::setPrimaryHandDrawables(List<Drawable> drawables) {
  m_primaryHand.itemDrawables = std::move(drawables);
}

void Humanoid::setPrimaryHandNonRotatedDrawables(List<Drawable> drawables) {
  m_primaryHand.nonRotatedDrawables = std::move(drawables);
}

void Humanoid::setAltHandParameters(bool holdingItem, float angle, float itemAngle, bool recoil,
    bool outsideOfHand) {
  m_altHand.holdingItem = holdingItem;
  m_altHand.angle = angle;
  m_altHand.itemAngle = itemAngle;
  m_altHand.recoil = recoil;
  m_altHand.outsideOfHand = outsideOfHand;
}

void Humanoid::setAltHandFrameOverrides(String backFrameOverride, String frontFrameOverride) {
  // Novaenia: Split override directives from frame specifier and moved them behind clothing directives.
  // FezzedOne: Unless the directives begin with a `??`.
  size_t backEnd = backFrameOverride.utf8().find('?');
  size_t frontEnd = frontFrameOverride.utf8().find('?');
  std::string backSubString = backEnd == NPos ? "" : backFrameOverride.utf8().substr(backEnd);
  std::string frontSubString = frontEnd == NPos ? "" : frontFrameOverride.utf8().substr(frontEnd);
  m_altHand.skipBackSplice = (backSubString.size() >= 2) && (backSubString[0] == '?') && (backSubString[1] == '?');
  m_altHand.skipFrontSplice = (frontSubString.size() >= 2) && (frontSubString[0] == '?') && (frontSubString[1] == '?');
  Directives backDirectives = Directives(backSubString);
  Directives frontDirectives = Directives(frontSubString);
  if (backEnd != NPos) backFrameOverride = backFrameOverride.utf8().substr(0, backEnd);
  if (frontEnd != NPos) frontFrameOverride = frontFrameOverride.utf8().substr(0, frontEnd);
  m_altHand.backFrame = !backFrameOverride.empty() ? std::move(backFrameOverride) : "rotation";
  m_altHand.frontFrame = !frontFrameOverride.empty() ? std::move(frontFrameOverride) : "rotation";
  m_altHand.backDirectives = !backDirectives.empty() ? std::move(backDirectives) : Directives();
  m_altHand.frontDirectives = !frontDirectives.empty() ? std::move(frontDirectives) : Directives();
}

void Humanoid::setAltHandDrawables(List<Drawable> drawables) {
  m_altHand.itemDrawables = std::move(drawables);
}

void Humanoid::setAltHandNonRotatedDrawables(List<Drawable> drawables) {
  m_altHand.nonRotatedDrawables = std::move(drawables);
}

void Humanoid::animate(float dt) {
  m_animationTimer += dt;
  m_emoteAnimationTimer += dt;
  m_danceTimer += dt;
}

void Humanoid::resetAnimation() {
  m_animationTimer = 0.0f;
  m_emoteAnimationTimer = 0.0f;
  m_danceTimer = 0.0f;
}

List<Drawable> Humanoid::render(bool withItems, bool withRotation, Maybe<float> aimAngle) {
  List<Drawable> drawables;

  int armStateSeq = getArmStateSequence();
  int bodyStateSeq = getBodyStateSequence();
  int emoteStateSeq = getEmoteStateSequence();
  float bobYOffset = getBobYOffset();
  Maybe<DancePtr> dance = getDance();
  Maybe<DanceStep> danceStep = {};
  if (dance.isValid()) {
    if (!(*dance)->states.contains(StateNames.getRight(m_state)))
      dance = {};
    else
      danceStep = (*dance)->steps[m_timing.danceSeq(m_danceTimer, *dance)];
  }

  auto frontHand = (m_facingDirection == Direction::Left || m_twoHanded) ? m_primaryHand : m_altHand;
  auto backHand = (m_facingDirection == Direction::Right || m_twoHanded) ? m_primaryHand : m_altHand;

  Maybe<float> aimAngleToUse = {};
  if (frontHand.holdingItem || backHand.holdingItem)
    aimAngleToUse = aimAngle;

  Vec2F frontArmFrameOffset = Vec2F(0, bobYOffset);
  if (frontHand.recoil)
    frontArmFrameOffset += m_recoilOffset;
  Vec2F backArmFrameOffset = Vec2F(0, bobYOffset);
  if (backHand.recoil)
    backArmFrameOffset += m_recoilOffset;

  auto addDrawable = [&](Drawable drawable, bool forceFullbright = false) {
    if (m_facingDirection == Direction::Left)
      drawable.scale(Vec2F(-1, 1));
    drawable.fullbright = drawable.fullbright || forceFullbright;
    drawables.append(std::move(drawable));
  };

  auto backArmDrawable = [&](String const& frameSet, Directives const& directives) -> Drawable {
    String image = strf("{}:{}{}", frameSet, backHand.backFrame, backHand.backDirectives.prefix());
    Drawable backArm = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, backArmFrameOffset);
    if (backHand.skipBackSplice) backArm.imagePart().addDirectives(backHand.backDirectives, true);
    backArm.imagePart().addDirectives(directives, true);
    if (!backHand.skipBackSplice) backArm.imagePart().addDirectives(backHand.backDirectives, true);
    backArm.rotate(backHand.angle, backArmFrameOffset + m_backArmRotationCenter + m_backArmOffset);
    return backArm;
  };

  Vec2F headRotationOffset = Vec2F(0.0f, 0.0f);
  if (aimAngleToUse) {
    headRotationOffset = Vec2F::filled(*aimAngleToUse / (0.5f * Constants::pi)).piecewiseMultiply(m_maximumHeadRotationOffset);
    headRotationOffset[1] = std::abs(headRotationOffset[1]) * (m_maximumHeadRotationOffset[1] < 0.0f ? -1.0f : 1.0f);
  }

  Vec2F headPosition(0.0f, bobYOffset);
  if (dance.isValid())
    headPosition += danceStep->headOffset / TilePixels;
  else if (m_state == Idle)
    headPosition += m_identity.personality.headOffset / TilePixels;
  else if (m_state == Run)
    headPosition += m_headRunOffset;
  else if (m_state == Swim || m_state == SwimIdle)
    headPosition += m_headSwimOffset;
  else if (m_state == Duck)
    headPosition += m_headDuckOffset;
  else if (m_state == Sit)
    headPosition += m_headSitOffset;
  else if (m_state == Lay)
    headPosition += m_headLayOffset;

  if (!m_backArmorUnderlayFrameset.empty()) {
    auto frameGroup = frameBase(m_state);
    if (m_movingBackwards && (m_state == State::Run))
      frameGroup = "runbackwards";
    String image;
    auto directives = getBackUnderlayDirectives();
    if (dance.isValid() && danceStep->bodyFrame)
      image = strf("{}:{}{}", m_backArmorUnderlayFrameset, *danceStep->bodyFrame, directives.prefix());
    else if (m_state == Idle)
      image = strf("{}:{}{}", m_backArmorUnderlayFrameset, m_identity.personality.idle, directives.prefix());
    else
      image = strf("{}:{}.{}{}", m_backArmorUnderlayFrameset, frameGroup, bodyStateSeq, directives.prefix());

    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, Vec2F());
    drawable.imagePart().addDirectives(directives, true);
    if (m_backArmorUnderlayHeadRotation && aimAngleToUse) {
      drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
      drawable.translate(headRotationOffset);
    }
    addDrawable(std::move(drawable));
  }

  if (!m_backArmorUnderlayStack.empty()) {
    for (BackEntry entry : m_backArmorUnderlayStack) {
      auto frameGroup = frameBase(m_state);
      if (m_movingBackwards && (m_state == State::Run))
        frameGroup = "runbackwards";
      String image;
      if (dance.isValid() && danceStep->bodyFrame)
        image = strf("{}:{}{}", entry.frameset, *danceStep->bodyFrame, entry.directives.prefix());
      else if (m_state == Idle)
        image = strf("{}:{}{}", entry.frameset, m_identity.personality.idle, entry.directives.prefix());
      else
        image = strf("{}:{}.{}{}", entry.frameset, frameGroup, bodyStateSeq, entry.directives.prefix());

      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, Vec2F());
      drawable.imagePart().addDirectives(entry.directives, true);
      if (entry.rotateWithHead && aimAngleToUse) {
        drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
        drawable.translate(headRotationOffset);
      }
      addDrawable(std::move(drawable));
    }
  }

  if (!m_backArmorFrameset.empty()) {
    auto frameGroup = frameBase(m_state);
    if (m_movingBackwards && (m_state == State::Run))
      frameGroup = "runbackwards";
    String image;
    auto directives = getBackDirectives();
    if (dance.isValid() && danceStep->bodyFrame)
      image = strf("{}:{}{}", m_backArmorFrameset, *danceStep->bodyFrame, directives.prefix());
    else if (m_state == Idle)
      image = strf("{}:{}{}", m_backArmorFrameset, m_identity.personality.idle, directives.prefix());
    else
      image = strf("{}:{}.{}{}", m_backArmorFrameset, frameGroup, bodyStateSeq, directives.prefix());

    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, Vec2F());
    drawable.imagePart().addDirectives(directives, true);
    if (m_backArmorHeadRotation && aimAngleToUse) {
      drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
      drawable.translate(headRotationOffset);
    }
    addDrawable(std::move(drawable));
  }

  if (!m_backArmorStack.empty()) {
    for (BackEntry entry : m_backArmorStack) {
      auto frameGroup = frameBase(m_state);
      if (m_movingBackwards && (m_state == State::Run))
        frameGroup = "runbackwards";
      String image;
      if (dance.isValid() && danceStep->bodyFrame)
        image = strf("{}:{}{}", entry.frameset, *danceStep->bodyFrame, entry.directives.prefix());
      else if (m_state == Idle)
        image = strf("{}:{}{}", entry.frameset, m_identity.personality.idle, entry.directives.prefix());
      else
        image = strf("{}:{}.{}{}", entry.frameset, frameGroup, bodyStateSeq, entry.directives.prefix());

      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, Vec2F());
      drawable.imagePart().addDirectives(entry.directives, true);
      if (entry.rotateWithHead && aimAngleToUse) {
        drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
        drawable.translate(headRotationOffset);
      }
      addDrawable(std::move(drawable));
    }
  }

  if (backHand.holdingItem && !dance.isValid() && withItems) {
    auto drawItem = [&]() {
      for (auto& backHandItem : backHand.itemDrawables) {
        backHandItem.translate(m_frontHandPosition + backArmFrameOffset + m_backArmOffset);
        backHandItem.rotate(backHand.itemAngle, backArmFrameOffset + m_backArmRotationCenter + m_backArmOffset);
        addDrawable(std::move(backHandItem));
      }
    };
    if (!m_twoHanded && backHand.outsideOfHand)
      drawItem();
    if (!m_backArmFrameset.empty() && !m_bodyHidden)
      addDrawable(backArmDrawable(m_backArmFrameset, getBodyDirectives()), m_bodyFullbright);
    if (!m_backSleeveUnderlayFrameset.empty())
      addDrawable(backArmDrawable(m_backSleeveUnderlayFrameset, getChestUnderlayDirectives()));
    if (!m_backSleeveUnderlayStack.empty()) {
      for (ArmorEntry entry : m_backSleeveUnderlayStack) {
        addDrawable(backArmDrawable(entry.frameset, entry.directives));
      }
    }
    if (!m_backSleeveFrameset.empty())
      addDrawable(backArmDrawable(m_backSleeveFrameset, getChestDirectives()));
    if (!m_backSleeveStack.empty()) {
      for (ArmorEntry entry : m_backSleeveStack) {
        addDrawable(backArmDrawable(entry.frameset, entry.directives));
      }
    }
    if (!m_twoHanded && !backHand.outsideOfHand)
      drawItem();
  } else {
    if (!m_backArmFrameset.empty() && !m_bodyHidden) {
      String image;
      Vec2F position;
      auto directives = getBodyDirectives();
      if (dance.isValid() && danceStep->backArmFrame) {
        image = strf("{}:{}{}", m_backArmFrameset, *danceStep->backArmFrame, directives.prefix());
        position = danceStep->backArmOffset / TilePixels;
      } else if (m_state == Idle) {
        image = strf("{}:{}{}", m_backArmFrameset, m_identity.personality.armIdle, directives.prefix());
        position = m_identity.personality.armOffset / TilePixels;
      } else
        image = strf("{}:{}.{}{}", m_backArmFrameset, frameBase(m_state), armStateSeq, directives.prefix());
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
      drawable.imagePart().addDirectives(getBodyDirectives(), true);
      if (dance.isValid())
        drawable.rotate(danceStep->backArmRotation);
      addDrawable(std::move(drawable), m_bodyFullbright);
    }

    if (!m_backSleeveUnderlayFrameset.empty()) {
      String image;
      Vec2F position;
      auto directives = getChestUnderlayDirectives();
      if (dance.isValid() && danceStep->backArmFrame) {
        image = strf("{}:{}{}", m_backSleeveUnderlayFrameset, *danceStep->backArmFrame, directives.prefix());
        position = danceStep->backArmOffset / TilePixels;
      } else if (m_state == Idle) {
        image = strf("{}:{}{}", m_backSleeveUnderlayFrameset, m_identity.personality.armIdle, directives.prefix());
        position = m_identity.personality.armOffset / TilePixels;
      } else
        image = strf("{}:{}.{}{}", m_backSleeveUnderlayFrameset, frameBase(m_state), armStateSeq, directives.prefix());
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
      drawable.imagePart().addDirectives(directives, true);
      if (dance.isValid())
        drawable.rotate(danceStep->backArmRotation);
      addDrawable(std::move(drawable));
    }

    if (!m_backSleeveUnderlayStack.empty()) {
      for (ArmorEntry entry : m_backSleeveUnderlayStack) {
        String image;
        Vec2F position;
        if (dance.isValid() && danceStep->backArmFrame) {
          image = strf("{}:{}{}", entry.frameset, *danceStep->backArmFrame, entry.directives.prefix());
          position = danceStep->backArmOffset / TilePixels;
        } else if (m_state == Idle) {
          image = strf("{}:{}{}", entry.frameset, m_identity.personality.armIdle, entry.directives.prefix());
          position = m_identity.personality.armOffset / TilePixels;
        } else
          image = strf("{}:{}.{}{}", entry.frameset, frameBase(m_state), armStateSeq, entry.directives.prefix());
        auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
        drawable.imagePart().addDirectives(entry.directives, true);
        if (dance.isValid())
          drawable.rotate(danceStep->backArmRotation);
        addDrawable(std::move(drawable));
      }
    }

    if (!m_backSleeveFrameset.empty()) {
      String image;
      Vec2F position;
      auto directives = getChestDirectives();
      if (dance.isValid() && danceStep->backArmFrame) {
        image = strf("{}:{}{}", m_backSleeveFrameset, *danceStep->backArmFrame, directives.prefix());
        position = danceStep->backArmOffset / TilePixels;
      } else if (m_state == Idle) {
        image = strf("{}:{}{}", m_backSleeveFrameset, m_identity.personality.armIdle, directives.prefix());
        position = m_identity.personality.armOffset / TilePixels;
      } else
        image = strf("{}:{}.{}{}", m_backSleeveFrameset, frameBase(m_state), armStateSeq, directives.prefix());
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
      drawable.imagePart().addDirectives(getChestDirectives(), true);
      if (dance.isValid())
        drawable.rotate(danceStep->backArmRotation);
      addDrawable(std::move(drawable));
    }

    if (!m_backSleeveStack.empty()) {
      for (ArmorEntry entry : m_backSleeveStack) {
        String image;
        Vec2F position;
        if (dance.isValid() && danceStep->backArmFrame) {
          image = strf("{}:{}{}", entry.frameset, *danceStep->backArmFrame, entry.directives.prefix());
          position = danceStep->backArmOffset / TilePixels;
        } else if (m_state == Idle) {
          image = strf("{}:{}{}", entry.frameset, m_identity.personality.armIdle, entry.directives.prefix());
          position = m_identity.personality.armOffset / TilePixels;
        } else
          image = strf("{}:{}.{}{}", entry.frameset, frameBase(m_state), armStateSeq, entry.directives.prefix());
        auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
        drawable.imagePart().addDirectives(entry.directives, true);
        if (dance.isValid())
          drawable.rotate(danceStep->backArmRotation);
        addDrawable(std::move(drawable));
      }
    }
  }

  auto getRotationMaskFromIdentity = [&](String animationFrame, bool isHeadMask) -> String {
    return String(strf("?addmask=/humanoid/{}/{}mask/{}body.png:{}",
        m_identity.imagePath ? *m_identity.imagePath : m_identity.species,
        isHeadMask ? "head" : "",
        GenderNames.getRight(m_identity.gender),
        animationFrame));
  };

  if (!m_headFrameset.empty() && !m_bodyHidden) {
    auto directives = getBodyDirectives();
    String image = strf("{}:normal{}", m_headFrameset, directives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    if (aimAngleToUse) {
      drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
      drawable.translate(headRotationOffset);
    }
    drawable.imagePart().addDirectives(directives, true);
    addDrawable(std::move(drawable), m_bodyFullbright);
  }

  if (!m_emoteFrameset.empty() && !m_bodyHidden) {
    auto directives = getEmoteDirectives();
    String image = strf("{}:{}.{}{}", m_emoteFrameset, emoteFrameBase(m_emoteState), emoteStateSeq, directives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    if (aimAngleToUse) {
      drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
      drawable.translate(headRotationOffset);
    }
    drawable.imagePart().addDirectives(directives, true);
    addDrawable(std::move(drawable), m_bodyFullbright);
  }

  if (!m_hairFrameset.empty() && !m_bodyHidden) {
    auto directives = getHairDirectives();
    String image = strf("{}:normal{}", m_hairFrameset, directives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    if (aimAngleToUse) {
      drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
      drawable.translate(headRotationOffset);
    }
    drawable.imagePart().addDirectives(directives, true).addDirectives(getHelmetMaskUnderlayDirectives(), true);
    if (!m_headArmorUnderlayStack.empty()) {
      for (ArmorEntry entry : m_headArmorUnderlayStack)
        drawable.imagePart().addDirectives(entry.maskDirectives, true);
    }
    drawable.imagePart().addDirectives(getHelmetMaskDirectives(), true);
    if (!m_headArmorStack.empty()) {
      for (ArmorEntry entry : m_headArmorStack)
        drawable.imagePart().addDirectives(entry.maskDirectives, true);
    }
    addDrawable(std::move(drawable), m_bodyFullbright);
  }

  if (!m_bodyFrameset.empty() && !m_bodyHidden) {
    String frame, image;
    auto directives = getBodyDirectives();
    if (dance.isValid() && danceStep->bodyFrame)
      frame = strf("{}{}", *danceStep->bodyFrame, directives.prefix());
    else if (m_state == Idle)
      frame = strf("{}{}", m_identity.personality.idle, directives.prefix());
    else
      frame = strf("{}.{}{}", frameBase(m_state), bodyStateSeq, directives.prefix());
    image = strf("{}:{}", m_bodyFrameset, frame);
    bool bodyMaskToHead = m_humanoidRotationSettings & HumanoidRotationSettings::UseBodyHeadMask;
    auto drawable = Drawable::makeImage(bodyMaskToHead ? image : std::move(image), 1.0f / TilePixels, true, {});
    if (m_humanoidRotationSettings & HumanoidRotationSettings::UseBodyMask)
      drawable.imagePart().addDirectives(getRotationMaskFromIdentity(frame, false));
    drawable.imagePart().addDirectives(directives, true);
    addDrawable(std::move(drawable), m_bodyFullbright);
    if (bodyMaskToHead) {
      auto rotatedDrawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, {});
      rotatedDrawable.imagePart().addDirectives(getRotationMaskFromIdentity(frame, true)).addDirectives(directives, true);
      if (aimAngleToUse) {
        rotatedDrawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
        rotatedDrawable.translate(headRotationOffset);
      }
      addDrawable(std::move(rotatedDrawable), m_bodyFullbright);
    }
  }

  if (!m_legsArmorUnderlayFrameset.empty()) {
    String image;
    auto directives = getLegsUnderlayDirectives();
    if (dance.isValid() && danceStep->bodyFrame)
      image = strf("{}:{}{}", m_legsArmorUnderlayFrameset, *danceStep->bodyFrame, directives.prefix());
    else if (m_state == Idle)
      image = strf("{}:{}{}", m_legsArmorUnderlayFrameset, m_identity.personality.idle, directives.prefix());
    else
      image = strf("{}:{}.{}{}", m_legsArmorUnderlayFrameset, frameBase(m_state), bodyStateSeq, directives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, {});
    drawable.imagePart().addDirectives(directives, true);
    addDrawable(std::move(drawable));
  }

  if (!m_chestArmorUnderlayFrameset.empty()) {
    String image;
    Vec2F position;
    auto directives = getChestUnderlayDirectives();
    if (dance.isValid() && danceStep->bodyFrame)
      image = strf("{}:{}{}", m_chestArmorUnderlayFrameset, *danceStep->bodyFrame, directives.prefix());
    else if (m_state == Run)
      image = strf("{}:run{}", m_chestArmorUnderlayFrameset, directives.prefix());
    else if (m_state == Idle)
      image = strf("{}:{}{}", m_chestArmorUnderlayFrameset, m_identity.personality.idle, directives.prefix());
    else if (m_state == Duck)
      image = strf("{}:duck{}", m_chestArmorUnderlayFrameset, directives.prefix());
    else if ((m_state == Swim) || (m_state == SwimIdle))
      image = strf("{}:swim{}", m_chestArmorUnderlayFrameset, directives.prefix());
    else
      image = strf("{}:chest.1{}", m_chestArmorUnderlayFrameset, directives.prefix());
    if (m_state != Duck)
      position[1] += bobYOffset;
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
    drawable.imagePart().addDirectives(directives, true);
    addDrawable(std::move(drawable));
  }

  if (!m_chestArmorUnderlayStack.empty() || !m_legsArmorUnderlayStack.empty()) {
    size_t legsUnderlayStackSize = m_legsArmorUnderlayStack.size(), chestUnderlayStackSize = m_chestArmorUnderlayStack.size();
    size_t largerStackSize = std::max<size_t>(legsUnderlayStackSize, chestUnderlayStackSize);
    for (size_t i = 0; i < largerStackSize; i++) {
      if (i < legsUnderlayStackSize) {
        String image;
        if (dance.isValid() && danceStep->bodyFrame)
          image = strf("{}:{}{}", m_legsArmorUnderlayStack[i].frameset, *danceStep->bodyFrame, m_legsArmorUnderlayStack[i].directives.prefix());
        else if (m_state == Idle)
          image = strf("{}:{}{}", m_legsArmorUnderlayStack[i].frameset, m_identity.personality.idle, m_legsArmorUnderlayStack[i].directives.prefix());
        else
          image = strf("{}:{}.{}{}", m_legsArmorUnderlayStack[i].frameset, frameBase(m_state), bodyStateSeq, m_legsArmorUnderlayStack[i].directives.prefix());
        auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, {});
        drawable.imagePart().addDirectives(m_legsArmorUnderlayStack[i].directives, true);
        addDrawable(std::move(drawable));
      }

      if (i < chestUnderlayStackSize) {
        String image;
        Vec2F position;
        if (dance.isValid() && danceStep->bodyFrame)
          image = strf("{}:{}{}", m_chestArmorUnderlayStack[i].frameset, *danceStep->bodyFrame, m_chestArmorUnderlayStack[i].directives.prefix());
        else if (m_state == Run)
          image = strf("{}:run{}", m_chestArmorUnderlayStack[i].frameset, m_chestArmorUnderlayStack[i].directives.prefix());
        else if (m_state == Idle)
          image = strf("{}:{}{}", m_chestArmorUnderlayStack[i].frameset, m_identity.personality.idle, m_chestArmorUnderlayStack[i].directives.prefix());
        else if (m_state == Duck)
          image = strf("{}:duck{}", m_chestArmorUnderlayStack[i].frameset, m_chestArmorUnderlayStack[i].directives.prefix());
        else if ((m_state == Swim) || (m_state == SwimIdle))
          image = strf("{}:swim{}", m_chestArmorUnderlayStack[i].frameset, m_chestArmorUnderlayStack[i].directives.prefix());
        else
          image = strf("{}:chest.1{}", m_chestArmorUnderlayStack[i].frameset, m_chestArmorUnderlayStack[i].directives.prefix());
        if (m_state != Duck)
          position[1] += bobYOffset;
        auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
        drawable.imagePart().addDirectives(m_chestArmorUnderlayStack[i].directives, true);
        addDrawable(std::move(drawable));
      }
    }
  }

  if (!m_legsArmorFrameset.empty()) {
    String image;
    auto directives = getLegsDirectives();
    if (dance.isValid() && danceStep->bodyFrame)
      image = strf("{}:{}{}", m_legsArmorFrameset, *danceStep->bodyFrame, directives.prefix());
    else if (m_state == Idle)
      image = strf("{}:{}{}", m_legsArmorFrameset, m_identity.personality.idle, directives.prefix());
    else
      image = strf("{}:{}.{}{}", m_legsArmorFrameset, frameBase(m_state), bodyStateSeq, directives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, {});
    drawable.imagePart().addDirectives(directives, true);
    addDrawable(std::move(drawable));
  }

  HashMap<uint8_t, Drawable> openSbCosmetics;

  if (!m_chestArmorStack.empty() || !m_legsArmorStack.empty()) {
    // FezzedOne: Chest and legs items in the first four OpenStarbound cosmetic slots are rendered at this point in order to emulate OpenStarbound's rendering.
    size_t legsStackSize = m_legsArmorStack.size(), chestStackSize = m_chestArmorStack.size();
    size_t largerStackSize = std::max<size_t>(legsStackSize, chestStackSize);

    for (size_t i = 0; i < largerStackSize; i++) {
      if (i < legsStackSize) {
        auto& ordering = m_legsArmorStack[i].ordering;
        if (ordering < 4) {
          String image;
          if (dance.isValid() && danceStep->bodyFrame)
            image = strf("{}:{}{}", m_legsArmorStack[i].frameset, *danceStep->bodyFrame, m_legsArmorStack[i].directives.prefix());
          else if (m_state == Idle)
            image = strf("{}:{}{}", m_legsArmorStack[i].frameset, m_identity.personality.idle, m_legsArmorStack[i].directives.prefix());
          else
            image = strf("{}:{}.{}{}", m_legsArmorStack[i].frameset, frameBase(m_state), bodyStateSeq, m_legsArmorStack[i].directives.prefix());
          auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, {});
          drawable.imagePart().addDirectives(m_legsArmorStack[i].directives, true);
          openSbCosmetics[ordering] = drawable;
        }
      }

      if (i < chestStackSize) {
        auto& ordering = m_chestArmorStack[i].ordering;
        if (ordering < 4) {
          String image;
          Vec2F position;
          if (dance.isValid() && danceStep->bodyFrame)
            image = strf("{}:{}{}", m_chestArmorStack[i].frameset, *danceStep->bodyFrame, m_chestArmorStack[i].directives.prefix());
          else if (m_state == Run)
            image = strf("{}:run{}", m_chestArmorStack[i].frameset, m_chestArmorStack[i].directives.prefix());
          else if (m_state == Idle)
            image = strf("{}:{}{}", m_chestArmorStack[i].frameset, m_identity.personality.idle, m_chestArmorStack[i].directives.prefix());
          else if (m_state == Duck)
            image = strf("{}:duck{}", m_chestArmorStack[i].frameset, m_chestArmorStack[i].directives.prefix());
          else if ((m_state == Swim) || (m_state == SwimIdle))
            image = strf("{}:swim{}", m_chestArmorStack[i].frameset, m_chestArmorStack[i].directives.prefix());
          else
            image = strf("{}:chest.1{}", m_chestArmorStack[i].frameset, m_chestArmorStack[i].directives.prefix());
          if (m_state != Duck)
            position[1] += bobYOffset;
          auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
          drawable.imagePart().addDirectives(m_chestArmorStack[i].directives, true);
          openSbCosmetics[ordering] = drawable;
        }
      }
    }

    // FezzedOne: Ensures oSB chest and leg cosmetics are rendered in the proper layer order.
    for (size_t i = 0; i != 4; i++) {
      if (auto drawable = openSbCosmetics.ptr(i))
        addDrawable(std::move(*drawable));
    }
  }

  if (!m_chestArmorFrameset.empty()) {
    String image;
    Vec2F position;
    auto directives = getChestDirectives();
    if (dance.isValid() && danceStep->bodyFrame)
      image = strf("{}:{}{}", m_chestArmorFrameset, *danceStep->bodyFrame, directives.prefix());
    else if (m_state == Run)
      image = strf("{}:run{}", m_chestArmorFrameset, directives.prefix());
    else if (m_state == Idle)
      image = strf("{}:{}{}", m_chestArmorFrameset, m_identity.personality.idle, directives.prefix());
    else if (m_state == Duck)
      image = strf("{}:duck{}", m_chestArmorFrameset, directives.prefix());
    else if ((m_state == Swim) || (m_state == SwimIdle))
      image = strf("{}:swim{}", m_chestArmorFrameset, directives.prefix());
    else
      image = strf("{}:chest.1{}", m_chestArmorFrameset, directives.prefix());
    if (m_state != Duck)
      position[1] += bobYOffset;
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
    drawable.imagePart().addDirectives(directives, true);
    addDrawable(std::move(drawable));
  }

  if (!m_chestArmorStack.empty() || !m_legsArmorStack.empty()) {
    size_t legsStackSize = m_legsArmorStack.size(), chestStackSize = m_chestArmorStack.size();
    size_t largerStackSize = std::max<size_t>(legsStackSize, chestStackSize);

    for (size_t i = 0; i < largerStackSize; i++) {
      if (i < legsStackSize) {
        auto& ordering = m_legsArmorStack[i].ordering;
        if (ordering >= 4) {
          String image;
          if (dance.isValid() && danceStep->bodyFrame)
            image = strf("{}:{}{}", m_legsArmorStack[i].frameset, *danceStep->bodyFrame, m_legsArmorStack[i].directives.prefix());
          else if (m_state == Idle)
            image = strf("{}:{}{}", m_legsArmorStack[i].frameset, m_identity.personality.idle, m_legsArmorStack[i].directives.prefix());
          else
            image = strf("{}:{}.{}{}", m_legsArmorStack[i].frameset, frameBase(m_state), bodyStateSeq, m_legsArmorStack[i].directives.prefix());
          auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, {});
          drawable.imagePart().addDirectives(m_legsArmorStack[i].directives, true);
          if (ordering >= 12)
            addDrawable(std::move(drawable));
          else
            openSbCosmetics[ordering] = drawable;
        }
      }

      if (i < chestStackSize) {
        auto& ordering = m_chestArmorStack[i].ordering;
        if (ordering >= 4) {
          String image;
          Vec2F position;
          if (dance.isValid() && danceStep->bodyFrame)
            image = strf("{}:{}{}", m_chestArmorStack[i].frameset, *danceStep->bodyFrame, m_chestArmorStack[i].directives.prefix());
          else if (m_state == Run)
            image = strf("{}:run{}", m_chestArmorStack[i].frameset, m_chestArmorStack[i].directives.prefix());
          else if (m_state == Idle)
            image = strf("{}:{}{}", m_chestArmorStack[i].frameset, m_identity.personality.idle, m_chestArmorStack[i].directives.prefix());
          else if (m_state == Duck)
            image = strf("{}:duck{}", m_chestArmorStack[i].frameset, m_chestArmorStack[i].directives.prefix());
          else if ((m_state == Swim) || (m_state == SwimIdle))
            image = strf("{}:swim{}", m_chestArmorStack[i].frameset, m_chestArmorStack[i].directives.prefix());
          else
            image = strf("{}:chest.1{}", m_chestArmorStack[i].frameset, m_chestArmorStack[i].directives.prefix());
          if (m_state != Duck)
            position[1] += bobYOffset;
          auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
          drawable.imagePart().addDirectives(m_chestArmorStack[i].directives, true);
          if (ordering >= 12)
            addDrawable(std::move(drawable));
          else
            openSbCosmetics[ordering] = drawable;
        }
      }
    }

    // FezzedOne: Ensures oSB chest and leg cosmetics are rendered in the proper layer order.
    for (size_t i = 4; i != 12; i++) {
      if (auto drawable = openSbCosmetics.ptr(i))
        addDrawable(std::move(*drawable));
    }
  }

  if (!m_facialHairFrameset.empty() && !m_bodyHidden) {
    auto directives = getFacialHairDirectives();
    String image = strf("{}:normal{}", m_facialHairFrameset, directives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    if (aimAngleToUse) {
      drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
      drawable.translate(headRotationOffset);
    }
    drawable.imagePart().addDirectives(directives, true).addDirectives(getHelmetMaskUnderlayDirectives(), true);
    if (!m_headArmorUnderlayStack.empty()) {
      for (ArmorEntry entry : m_headArmorUnderlayStack)
        drawable.imagePart().addDirectives(entry.maskDirectives, true);
    }
    drawable.imagePart().addDirectives(getHelmetMaskDirectives(), true);
    if (!m_headArmorStack.empty()) {
      for (ArmorEntry entry : m_headArmorStack)
        drawable.imagePart().addDirectives(entry.maskDirectives, true);
    }
    addDrawable(std::move(drawable), m_bodyFullbright);
  }

  if (!m_facialMaskFrameset.empty() && !m_bodyHidden) {
    auto directives = getFacialMaskDirectives();
    String image = strf("{}:normal{}", m_facialMaskFrameset, directives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    if (aimAngleToUse) {
      drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
      drawable.translate(headRotationOffset);
    }
    drawable.imagePart().addDirectives(directives, true).addDirectives(getHelmetMaskUnderlayDirectives(), true);
    if (!m_headArmorUnderlayStack.empty()) {
      for (ArmorEntry entry : m_headArmorUnderlayStack)
        drawable.imagePart().addDirectives(entry.maskDirectives, true);
    }
    drawable.imagePart().addDirectives(getHelmetMaskDirectives(), true);
    if (!m_headArmorStack.empty()) {
      for (ArmorEntry entry : m_headArmorStack)
        drawable.imagePart().addDirectives(entry.maskDirectives, true);
    }
    addDrawable(std::move(drawable));
  }

  if (!m_headArmorUnderlayFrameset.empty()) {
    auto directives = getHeadUnderlayDirectives();
    String image = strf("{}:normal{}", m_headArmorUnderlayFrameset, directives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    if (aimAngleToUse) {
      drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
      drawable.translate(headRotationOffset);
    }
    drawable.imagePart().addDirectives(directives, true);
    if (!m_headArmorUnderlayStack.empty()) {
      for (ArmorEntry entry : m_headArmorUnderlayStack)
        drawable.imagePart().addDirectives(entry.maskDirectives, true);
    }
    drawable.imagePart().addDirectives(getHelmetMaskDirectives(), true);
    if (!m_headArmorStack.empty()) {
      for (ArmorEntry entry : m_headArmorStack) {
        if (!entry.maskHumanoidBaseOnly)
          drawable.imagePart().addDirectives(entry.maskDirectives, true);
      }
    }
    addDrawable(std::move(drawable));
  }

  if (!m_headArmorUnderlayStack.empty()) {
    size_t stackSize = m_headArmorUnderlayStack.size();
    for (size_t i = 0; i < stackSize; i++) {
      String image = strf("{}:normal{}", m_headArmorUnderlayStack[i].frameset, m_headArmorUnderlayStack[i].directives.prefix());
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
      if (aimAngleToUse) {
        drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
        drawable.translate(headRotationOffset);
      }
      drawable.imagePart().addDirectives(m_headArmorUnderlayStack[i].directives, true);
      if (i != stackSize) {
        for (size_t j = i + 1; j < stackSize; j++)
          drawable.imagePart().addDirectives(m_headArmorUnderlayStack[j].maskDirectives, true);
      }
      drawable.imagePart().addDirectives(getHelmetMaskDirectives(), true);
      if (!m_headArmorStack.empty()) {
        for (ArmorEntry entry : m_headArmorStack) {
          if (!entry.maskHumanoidBaseOnly)
            drawable.imagePart().addDirectives(entry.maskDirectives, true);
        }
      }
      addDrawable(std::move(drawable));
    }
  }

  if (!m_headArmorFrameset.empty()) {
    auto directives = getHeadDirectives();
    String image = strf("{}:normal{}", m_headArmorFrameset, directives.prefix());
    auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
    if (aimAngleToUse) {
      drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
      drawable.translate(headRotationOffset);
    }
    drawable.imagePart().addDirectives(directives, true);
    if (!m_headArmorStack.empty()) {
      for (ArmorEntry entry : m_headArmorStack) {
        if (!entry.maskHumanoidBaseOnly)
          drawable.imagePart().addDirectives(entry.maskDirectives, true);
      }
    }
    addDrawable(std::move(drawable));
  }

  if (!m_headArmorStack.empty()) {
    size_t stackSize = m_headArmorStack.size();
    for (size_t i = 0; i < stackSize; i++) {
      String image = strf("{}:normal{}", m_headArmorStack[i].frameset, m_headArmorStack[i].directives.prefix());
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, headPosition);
      if (aimAngleToUse) {
        drawable.rotate(*aimAngleToUse * m_headRotationMultiplier, headPosition + m_headCenterPosition);
        drawable.translate(headRotationOffset);
      }
      drawable.imagePart().addDirectives(m_headArmorStack[i].directives, true);
      if (i != stackSize) {
        for (size_t j = i + 1; j < stackSize; j++) {
          if (!m_headArmorStack[j].maskHumanoidBaseOnly)
            drawable.imagePart().addDirectives(m_headArmorStack[j].maskDirectives, true);
        }
      }
      addDrawable(std::move(drawable));
    }
  }

  auto frontArmDrawable = [&](String const& frameSet, Directives const& directives) -> Drawable {
    String image = strf("{}:{}{}", frameSet, frontHand.frontFrame, directives.prefix());
    Drawable frontArm = Drawable::makeImage(image, 1.0f / TilePixels, true, frontArmFrameOffset);
    if (frontHand.skipFrontSplice) frontArm.imagePart().addDirectives(frontHand.frontDirectives, true);
    frontArm.imagePart().addDirectives(directives, true);
    if (!frontHand.skipFrontSplice) frontArm.imagePart().addDirectives(frontHand.frontDirectives, true);
    frontArm.rotate(frontHand.angle, frontArmFrameOffset + m_frontArmRotationCenter);
    return frontArm;
  };

  if (frontHand.holdingItem && !dance.isValid() && withItems) {
    auto drawItem = [&]() {
      for (auto& frontHandItem : frontHand.itemDrawables) {
        frontHandItem.translate(m_frontHandPosition + frontArmFrameOffset);
        frontHandItem.rotate(frontHand.itemAngle, frontArmFrameOffset + m_frontArmRotationCenter);
        addDrawable(frontHandItem);
      }
    };
    if (!frontHand.outsideOfHand)
      drawItem();
    if (!m_frontArmFrameset.empty() && !m_bodyHidden)
      addDrawable(frontArmDrawable(m_frontArmFrameset, getBodyDirectives()), m_bodyFullbright);
    if (!m_frontSleeveUnderlayFrameset.empty())
      addDrawable(frontArmDrawable(m_frontSleeveUnderlayFrameset, getChestUnderlayDirectives()));
    if (!m_frontSleeveUnderlayStack.empty()) {
      for (ArmorEntry entry : m_frontSleeveUnderlayStack) {
        addDrawable(frontArmDrawable(entry.frameset, entry.directives));
      }
    }
    if (!m_frontSleeveFrameset.empty())
      addDrawable(frontArmDrawable(m_frontSleeveFrameset, getChestDirectives()));
    if (!m_frontSleeveStack.empty()) {
      for (ArmorEntry entry : m_frontSleeveStack) {
        addDrawable(frontArmDrawable(entry.frameset, entry.directives));
      }
    }
    if (frontHand.outsideOfHand)
      drawItem();
  } else {
    if (!m_frontArmFrameset.empty() && !m_bodyHidden) {
      String image;
      Vec2F position;
      auto directives = getBodyDirectives();
      if (dance.isValid() && danceStep->frontArmFrame) {
        image = strf("{}:{}{}", m_frontArmFrameset, *danceStep->frontArmFrame, directives.prefix());
        position = danceStep->frontArmOffset / TilePixels;
      } else if (m_state == Idle) {
        image = strf("{}:{}{}", m_frontArmFrameset, m_identity.personality.armIdle, directives.prefix());
        position = m_identity.personality.armOffset / TilePixels;
      } else
        image = strf("{}:{}.{}{}", m_frontArmFrameset, frameBase(m_state), armStateSeq, directives.prefix());
      auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
      drawable.imagePart().addDirectives(directives, true);
      if (dance.isValid())
        drawable.rotate(danceStep->frontArmRotation);
      addDrawable(drawable, m_bodyFullbright);
    }

    if (!m_frontSleeveUnderlayFrameset.empty()) {
      String image;
      Vec2F position;
      auto directives = getChestUnderlayDirectives();
      if (dance.isValid() && danceStep->frontArmFrame) {
        image = strf("{}:{}{}", m_frontSleeveUnderlayFrameset, *danceStep->frontArmFrame, directives.prefix());
        position = danceStep->frontArmOffset / TilePixels;
      } else if (m_state == Idle) {
        image = strf("{}:{}{}", m_frontSleeveUnderlayFrameset, m_identity.personality.armIdle, directives.prefix());
        position = m_identity.personality.armOffset / TilePixels;
      } else
        image = strf("{}:{}.{}{}", m_frontSleeveUnderlayFrameset, frameBase(m_state), armStateSeq, directives.prefix());
      auto drawable = Drawable::makeImage(image, 1.0f / TilePixels, true, position);
      drawable.imagePart().addDirectives(directives, true);
      if (dance.isValid())
        drawable.rotate(danceStep->frontArmRotation);
      addDrawable(drawable);
    }

    if (!m_frontSleeveUnderlayStack.empty()) {
      for (ArmorEntry entry : m_frontSleeveUnderlayStack) {
        String image;
        Vec2F position;
        if (dance.isValid() && danceStep->frontArmFrame) {
          image = strf("{}:{}{}", entry.frameset, *danceStep->frontArmFrame, entry.directives.prefix());
          position = danceStep->frontArmOffset / TilePixels;
        } else if (m_state == Idle) {
          image = strf("{}:{}{}", entry.frameset, m_identity.personality.armIdle, entry.directives.prefix());
          position = m_identity.personality.armOffset / TilePixels;
        } else
          image = strf("{}:{}.{}{}", entry.frameset, frameBase(m_state), armStateSeq, entry.directives.prefix());
        auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
        drawable.imagePart().addDirectives(entry.directives, true);
        if (dance.isValid())
          drawable.rotate(danceStep->frontArmRotation);
        addDrawable(std::move(drawable));
      }
    }

    if (!m_frontSleeveFrameset.empty()) {
      String image;
      Vec2F position;
      auto directives = getChestDirectives();
      if (dance.isValid() && danceStep->frontArmFrame) {
        image = strf("{}:{}{}", m_frontSleeveFrameset, *danceStep->frontArmFrame, directives.prefix());
        position = danceStep->frontArmOffset / TilePixels;
      } else if (m_state == Idle) {
        image = strf("{}:{}{}", m_frontSleeveFrameset, m_identity.personality.armIdle, directives.prefix());
        position = m_identity.personality.armOffset / TilePixels;
      } else
        image = strf("{}:{}.{}{}", m_frontSleeveFrameset, frameBase(m_state), armStateSeq, directives.prefix());
      auto drawable = Drawable::makeImage(image, 1.0f / TilePixels, true, position);
      drawable.imagePart().addDirectives(getChestDirectives(), true);
      if (dance.isValid())
        drawable.rotate(danceStep->frontArmRotation);
      addDrawable(drawable);
    }

    if (!m_frontSleeveStack.empty()) {
      for (ArmorEntry entry : m_frontSleeveStack) {
        String image;
        Vec2F position;
        if (dance.isValid() && danceStep->frontArmFrame) {
          image = strf("{}:{}{}", entry.frameset, *danceStep->frontArmFrame, entry.directives.prefix());
          position = danceStep->frontArmOffset / TilePixels;
        } else if (m_state == Idle) {
          image = strf("{}:{}{}", entry.frameset, m_identity.personality.armIdle, entry.directives.prefix());
          position = m_identity.personality.armOffset / TilePixels;
        } else
          image = strf("{}:{}.{}{}", entry.frameset, frameBase(m_state), armStateSeq, entry.directives.prefix());
        auto drawable = Drawable::makeImage(std::move(image), 1.0f / TilePixels, true, position);
        drawable.imagePart().addDirectives(entry.directives, true);
        if (dance.isValid())
          drawable.rotate(danceStep->frontArmRotation);
        addDrawable(std::move(drawable));
      }
    }
  }

  if (m_drawVaporTrail) {
    auto image = strf("{}:{}",
        m_vaporTrailFrameset,
        m_timing.genericSeq(m_animationTimer, m_vaporTrailCycle, m_vaporTrailFrames, true));
    addDrawable(Drawable::makeImage(AssetPath::split(image), 1.0f / TilePixels, true, {}));
  }

  if (withItems) {
    if (m_primaryHand.nonRotatedDrawables.size())
      drawables.insertAllAt(0, m_primaryHand.nonRotatedDrawables);
    if (m_altHand.nonRotatedDrawables.size())
      drawables.insertAllAt(0, m_altHand.nonRotatedDrawables);
  }

  if (withRotation)
    Drawable::rotateAll(drawables, m_rotation);

  Drawable::translateAll(drawables, m_globalOffset);
  Drawable::rebaseAll(drawables);

  return drawables;
}

List<Drawable> Humanoid::renderPortrait(PortraitMode mode) const {
  List<Drawable> drawables;
  int emoteStateSeq = m_timing.emoteStateSeq(m_emoteAnimationTimer, m_emoteState);

  auto addDrawable = [&](Drawable&& drawable) -> Drawable& {
    if (mode != PortraitMode::Full && mode != PortraitMode::FullNeutral && mode != PortraitMode::FullNude && mode != PortraitMode::FullNeutralNude) {
      // TODO: make this configurable
      drawable.imagePart().addDirectives(String("addmask=/humanoid/portraitMask.png;0;0"), false);
    }
    drawables.append(std::move(drawable));
    return drawables.back();
  };

  bool dressed = !(mode == PortraitMode::FullNude || mode == PortraitMode::FullNeutralNude);

  Directives helmetUnderlayMaskDirective = dressed ? getHelmetMaskUnderlayDirectives() : Directives();
  Directives helmetMaskDirective = dressed ? getHelmetMaskDirectives() : Directives();

  auto personality = m_identity.personality;
  if (mode == PortraitMode::FullNeutral || mode == PortraitMode::FullNeutralNude)
    personality = Root::singleton().speciesDatabase()->species(m_identity.species)->personalities()[0];

  if (mode != PortraitMode::Head) {
    // FezzedOne: Why was this in the wrong order to begin with?
    if (mode != PortraitMode::Bust) {
      if (dressed) {
        if (!m_backArmorUnderlayFrameset.empty()) {
          String image = strf("{}:{}", m_backArmorUnderlayFrameset, personality.idle);
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
          drawable.imagePart().addDirectives(getBackUnderlayDirectives(), true);
          addDrawable(std::move(drawable));
        }
        if (!m_backArmorUnderlayStack.empty()) {
          for (BackEntry entry : m_backArmorUnderlayStack) {
            String image = strf("{}:{}", entry.frameset, personality.idle);
            Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
            drawable.imagePart().addDirectives(entry.directives, true);
            addDrawable(std::move(drawable));
          }
        }
        if (!m_backArmorFrameset.empty()) {
          String image = strf("{}:{}", m_backArmorFrameset, personality.idle);
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
          drawable.imagePart().addDirectives(getBackDirectives(), true);
          addDrawable(std::move(drawable));
        }
        if (!m_backArmorStack.empty()) {
          for (BackEntry entry : m_backArmorStack) {
            String image = strf("{}:{}", entry.frameset, personality.idle);
            Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
            drawable.imagePart().addDirectives(entry.directives, true);
            addDrawable(std::move(drawable));
          }
        }
      }
    }

    if (!m_backArmFrameset.empty()) {
      String image = strf("{}:{}", m_backArmFrameset, personality.armIdle);
      Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
      drawable.imagePart().addDirectives(getBodyDirectives(), true);
      addDrawable(std::move(drawable));
    }

    if (dressed) {
      if (!m_backSleeveUnderlayFrameset.empty()) {
        String image = strf("{}:{}", m_backSleeveUnderlayFrameset, personality.armIdle);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
        drawable.imagePart().addDirectives(getChestUnderlayDirectives(), true);
        addDrawable(std::move(drawable));
      }

      if (!m_backSleeveUnderlayStack.empty()) {
        for (ArmorEntry entry : m_backSleeveUnderlayStack) {
          String image = strf("{}:{}", entry.frameset, personality.armIdle);
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
          drawable.imagePart().addDirectives(entry.directives, true);
          addDrawable(std::move(drawable));
        }
      }

      if (!m_backSleeveFrameset.empty()) {
        String image = strf("{}:{}", m_backSleeveFrameset, personality.armIdle);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
        drawable.imagePart().addDirectives(getChestDirectives(), true);
        addDrawable(std::move(drawable));
      }

      if (!m_backSleeveStack.empty()) {
        for (ArmorEntry entry : m_backSleeveStack) {
          String image = strf("{}:{}", entry.frameset, personality.armIdle);
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
          drawable.imagePart().addDirectives(entry.directives, true);
          addDrawable(std::move(drawable));
        }
      }
    }
  }

  if (!m_headFrameset.empty()) {
    String image = strf("{}:normal", m_headFrameset);
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(getBodyDirectives(), true);
    addDrawable(std::move(drawable));
  }

  if (!m_emoteFrameset.empty()) {
    String image = strf("{}:{}.{}", m_emoteFrameset, emoteFrameBase(m_emoteState), emoteStateSeq);
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(getEmoteDirectives(), true);
    addDrawable(std::move(drawable));
  }

  if (!m_hairFrameset.empty()) {
    String image = strf("{}:normal", m_hairFrameset);
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(getHairDirectives(), true);
    if (dressed) {
      drawable.imagePart().addDirectives(helmetUnderlayMaskDirective, true);
      if (!m_headArmorUnderlayStack.empty()) {
        for (ArmorEntry entry : m_headArmorUnderlayStack)
          drawable.imagePart().addDirectives(entry.maskDirectives, true);
      }
      drawable.imagePart().addDirectives(helmetMaskDirective, true);
      if (!m_headArmorStack.empty()) {
        for (ArmorEntry entry : m_headArmorStack)
          drawable.imagePart().addDirectives(entry.maskDirectives, true);
      }
    }
    addDrawable(std::move(drawable));
  }

  if (!m_bodyFrameset.empty()) {
    String image = strf("{}:{}", m_bodyFrameset, personality.idle);
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
    drawable.imagePart().addDirectives(getBodyDirectives(), true);
    addDrawable(std::move(drawable));
  }

  if (mode != PortraitMode::Head) {
    if (dressed) {
      if (!m_legsArmorUnderlayFrameset.empty()) {
        String image = strf("{}:{}", m_legsArmorUnderlayFrameset, personality.idle);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
        drawable.imagePart().addDirectives(getLegsUnderlayDirectives(), true);
        addDrawable(std::move(drawable));
      }

      if (!m_chestArmorUnderlayFrameset.empty()) {
        String image = strf("{}:{}", m_chestArmorUnderlayFrameset, personality.idle);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
        drawable.imagePart().addDirectives(getChestUnderlayDirectives(), true);
        addDrawable(std::move(drawable));
      }

      if ((!m_chestArmorUnderlayStack.empty() || !m_legsArmorUnderlayStack.empty())) {
        size_t legsUnderlayStackSize = m_legsArmorUnderlayStack.size(), chestUnderlayStackSize = m_chestArmorUnderlayStack.size();
        size_t largerStackSize = std::max<size_t>(legsUnderlayStackSize, chestUnderlayStackSize);
        for (size_t i = 0; i < largerStackSize; i++) {
          if (i < legsUnderlayStackSize) {
            String image = strf("{}:{}", m_legsArmorUnderlayStack[i].frameset, personality.idle);
            Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
            drawable.imagePart().addDirectives(m_legsArmorUnderlayStack[i].directives, true);
            addDrawable(std::move(drawable));
          }

          if (i < chestUnderlayStackSize) {
            String image = strf("{}:{}", m_chestArmorUnderlayStack[i].frameset, personality.idle);
            Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
            drawable.imagePart().addDirectives(m_chestArmorUnderlayStack[i].directives, true);
            addDrawable(std::move(drawable));
          }
        }
      }

      if (!m_legsArmorFrameset.empty()) {
        String image = strf("{}:{}", m_legsArmorFrameset, personality.idle);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
        drawable.imagePart().addDirectives(getLegsDirectives(), true);
        addDrawable(std::move(drawable));
      }

      if (!m_chestArmorFrameset.empty()) {
        String image = strf("{}:{}", m_chestArmorFrameset, personality.idle);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
        drawable.imagePart().addDirectives(getChestDirectives(), true);
        addDrawable(std::move(drawable));
      }

      if ((!m_chestArmorStack.empty() || !m_legsArmorStack.empty())) {
        size_t legsStackSize = m_legsArmorStack.size(), chestStackSize = m_chestArmorStack.size();
        size_t largerStackSize = std::max<size_t>(legsStackSize, chestStackSize);
        for (size_t i = 0; i < largerStackSize; i++) {
          if (i < legsStackSize) {
            String image = strf("{}:{}", m_legsArmorStack[i].frameset, personality.idle);
            Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
            drawable.imagePart().addDirectives(m_legsArmorStack[i].directives, true);
            addDrawable(std::move(drawable));
          }

          if (i < chestStackSize) {
            String image = strf("{}:{}", m_chestArmorStack[i].frameset, personality.idle);
            Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, {});
            drawable.imagePart().addDirectives(m_chestArmorStack[i].directives, true);
            addDrawable(std::move(drawable));
          }
        }
      }
    }
  }

  if (!m_facialHairFrameset.empty()) {
    String image = strf("{}:normal", m_facialHairFrameset);
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(getFacialHairDirectives(), true);
    if (dressed) {
      drawable.imagePart().addDirectives(helmetUnderlayMaskDirective, true);
      if (!m_headArmorUnderlayStack.empty()) {
        for (ArmorEntry entry : m_headArmorUnderlayStack)
          drawable.imagePart().addDirectives(entry.maskDirectives, true);
      }
      drawable.imagePart().addDirectives(helmetMaskDirective, true);
      if (!m_headArmorStack.empty()) {
        for (ArmorEntry entry : m_headArmorStack)
          drawable.imagePart().addDirectives(entry.maskDirectives, true);
      }
    }
    addDrawable(std::move(drawable));
  }

  if (!m_facialMaskFrameset.empty()) {
    String image = strf("{}:normal", m_facialMaskFrameset);
    Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
    drawable.imagePart().addDirectives(getFacialMaskDirectives(), true);
    if (dressed) {
      drawable.imagePart().addDirectives(helmetUnderlayMaskDirective, true);
      if (!m_headArmorUnderlayStack.empty()) {
        for (ArmorEntry entry : m_headArmorUnderlayStack)
          drawable.imagePart().addDirectives(entry.maskDirectives, true);
      }
      drawable.imagePart().addDirectives(helmetMaskDirective, true);
      if (!m_headArmorStack.empty()) {
        for (ArmorEntry entry : m_headArmorStack)
          drawable.imagePart().addDirectives(entry.maskDirectives, true);
      }
    }
    addDrawable(std::move(drawable));
  }

  if (dressed) {
    if (!m_headArmorUnderlayFrameset.empty()) {
      String image = strf("{}:normal", m_headArmorUnderlayFrameset);
      Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
      drawable.imagePart().addDirectives(getHeadUnderlayDirectives(), true);
      if (!m_headArmorUnderlayStack.empty()) {
        for (ArmorEntry entry : m_headArmorUnderlayStack)
          drawable.imagePart().addDirectives(entry.maskDirectives, true);
      }
      drawable.imagePart().addDirectives(getHelmetMaskDirectives(), true);
      if (!m_headArmorStack.empty()) {
        for (ArmorEntry entry : m_headArmorStack) {
          if (!entry.maskHumanoidBaseOnly)
            drawable.imagePart().addDirectives(entry.maskDirectives, true);
        }
      }
      addDrawable(std::move(drawable));
    }

    if (!m_headArmorUnderlayStack.empty()) {
      size_t stackSize = m_headArmorUnderlayStack.size();
      for (size_t i = 0; i < stackSize; i++) {
        String image = strf("{}:normal", m_headArmorUnderlayStack[i].frameset);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
        drawable.imagePart().addDirectives(m_headArmorUnderlayStack[i].directives, true);
        if (i != stackSize) {
          for (size_t j = i + 1; j < stackSize; j++)
            drawable.imagePart().addDirectives(m_headArmorUnderlayStack[j].maskDirectives, true);
        }
        drawable.imagePart().addDirectives(getHelmetMaskDirectives(), true);
        if (!m_headArmorStack.empty()) {
          for (ArmorEntry entry : m_headArmorStack) {
            if (!entry.maskHumanoidBaseOnly)
              drawable.imagePart().addDirectives(entry.maskDirectives, true);
          }
        }
        addDrawable(std::move(drawable));
      }
    }

    if (!m_headArmorFrameset.empty()) {
      String image = strf("{}:normal", m_headArmorFrameset);
      Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
      drawable.imagePart().addDirectives(getHeadDirectives(), true);
      if (!m_headArmorStack.empty()) {
        for (ArmorEntry entry : m_headArmorStack) {
          if (!entry.maskHumanoidBaseOnly)
            drawable.imagePart().addDirectives(entry.maskDirectives, true);
        }
      }
      addDrawable(std::move(drawable));
    }

    if (!m_headArmorStack.empty()) {
      size_t stackSize = m_headArmorStack.size();
      for (size_t i = 0; i < stackSize; i++) {
        String image = strf("{}:normal", m_headArmorStack[i].frameset);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.headOffset);
        drawable.imagePart().addDirectives(m_headArmorStack[i].directives, true);
        if (i != stackSize) {
          for (size_t j = i + 1; j < stackSize; j++) {
            if (!m_headArmorStack[j].maskHumanoidBaseOnly)
              drawable.imagePart().addDirectives(m_headArmorStack[j].maskDirectives, true);
          }
        }
        addDrawable(std::move(drawable));
      }
    }
  }

  if (mode != PortraitMode::Head) {
    if (!m_frontArmFrameset.empty()) {
      String image = strf("{}:{}", m_frontArmFrameset, personality.armIdle);
      Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
      drawable.imagePart().addDirectives(getBodyDirectives(), true);
      addDrawable(std::move(drawable));
    }

    if (dressed) {
      if (!m_frontSleeveUnderlayFrameset.empty()) {
        String image = strf("{}:{}", m_frontSleeveUnderlayFrameset, personality.armIdle);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
        drawable.imagePart().addDirectives(getChestUnderlayDirectives(), true);
        addDrawable(std::move(drawable));
      }

      if (!m_frontSleeveUnderlayStack.empty()) {
        for (ArmorEntry entry : m_frontSleeveUnderlayStack) {
          String image = strf("{}:{}", entry.frameset, personality.armIdle);
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
          drawable.imagePart().addDirectives(entry.directives, true);
          addDrawable(std::move(drawable));
        }
      }

      if (!m_frontSleeveFrameset.empty()) {
        String image = strf("{}:{}", m_frontSleeveFrameset, personality.armIdle);
        Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
        drawable.imagePart().addDirectives(getChestDirectives(), true);
        addDrawable(std::move(drawable));
      }

      if (!m_frontSleeveStack.empty()) {
        for (ArmorEntry entry : m_frontSleeveStack) {
          String image = strf("{}:{}", entry.frameset, personality.armIdle);
          Drawable drawable = Drawable::makeImage(std::move(image), 1.0f, true, personality.armOffset);
          drawable.imagePart().addDirectives(entry.directives, true);
          addDrawable(std::move(drawable));
        }
      }
    }
  }

  return drawables;
}

List<Drawable> Humanoid::renderSkull() const {
  return {Drawable::makeImage(
      Root::singleton().speciesDatabase()->species(m_identity.species)->skull(), 1.0f, true, Vec2F())};
}

Humanoid Humanoid::makeDummy(Gender gender) {
  auto assets = Root::singleton().assets();
  Humanoid humanoid(assets->json("/humanoid.config"));

  humanoid.m_headFrameset = assets->json("/humanoid/any/dummy.config:head").toString();
  humanoid.m_bodyFrameset = assets->json("/humanoid/any/dummy.config:body").toString();
  humanoid.m_frontArmFrameset = assets->json("/humanoid/any/dummy.config:frontArm").toString();
  humanoid.m_backArmFrameset = assets->json("/humanoid/any/dummy.config:backArm").toString();
  humanoid.setFacingDirection(DirectionNames.getLeft(assets->json("/humanoid/any/dummy.config:direction").toString()));

  return humanoid;
}

List<Drawable> Humanoid::renderDummy(Gender gender, Maybe<HeadArmor const*> head, Maybe<ChestArmor const*> chest, Maybe<LegsArmor const*> legs, Maybe<BackArmor const*> back) {
  if (head) {
    setHeadArmorUnderlayFrameset("");
    setHeadArmorUnderlayDirectives("");
    setHelmetMaskUnderlayDirectives("");
    setHeadArmorUnderlayStack({});
    if (auto headPtr = *head) {
      setHeadArmorFrameset(headPtr->frameset(gender));
      setHeadArmorDirectives(headPtr->directives());
      setHelmetMaskDirectives(headPtr->maskDirectives());
      List<ArmorEntry> headArmorStack = {};
      for (auto& item : headPtr->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<HeadArmor>(item)) {
            headArmorStack.emplaceAppend(ArmorEntry{
                armourItem->frameset(gender),
                armourItem->directives(),
                armourItem->maskDirectives()});
          }
        }
      }
      setHeadArmorStack(headArmorStack);
    } else {
      setHeadArmorFrameset("");
      setHeadArmorDirectives("");
      setHelmetMaskDirectives("");
      setHeadArmorStack({});
    }
  }

  if (chest) {
    setBackSleeveUnderlayFrameset("");
    setFrontSleeveUnderlayFrameset("");
    setChestArmorUnderlayFrameset("");
    setChestArmorUnderlayDirectives("");
    setChestArmorUnderlayStack({});
    if (auto chestPtr = *chest) {
      setBackSleeveFrameset(chestPtr->backSleeveFrameset(gender));
      setFrontSleeveFrameset(chestPtr->frontSleeveFrameset(gender));
      setChestArmorFrameset(chestPtr->bodyFrameset(gender));
      setChestArmorDirectives(chestPtr->directives());
      List<ArmorEntry> chestArmorStack = {}, frontSleeveStack = {}, backSleeveStack = {};
      for (auto& item : chestPtr->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<ChestArmor>(item)) {
            chestArmorStack.emplaceAppend(ArmorEntry{
                armourItem->bodyFrameset(gender),
                armourItem->directives(),
                Directives()});
            frontSleeveStack.emplaceAppend(ArmorEntry{
                armourItem->frontSleeveFrameset(gender),
                armourItem->directives(),
                Directives()});
            backSleeveStack.emplaceAppend(ArmorEntry{
                armourItem->backSleeveFrameset(gender),
                armourItem->directives(),
                Directives()});
          }
        }
      }
      setChestArmorStack(chestArmorStack);
      setFrontSleeveStack(frontSleeveStack);
      setBackSleeveStack(backSleeveStack);
    } else {
      setBackSleeveFrameset("");
      setFrontSleeveFrameset("");
      setChestArmorFrameset("");
      setChestArmorDirectives("");
      setChestArmorStack({});
      setFrontSleeveStack({});
      setBackSleeveStack({});
    }
  }

  if (legs) {
    setLegsArmorUnderlayFrameset("");
    setLegsArmorUnderlayDirectives("");
    setLegsArmorUnderlayStack({});
    if (auto legsPtr = *legs) {
      setLegsArmorFrameset(legsPtr->frameset(gender));
      setLegsArmorDirectives(legsPtr->directives());
      List<ArmorEntry> legsArmorStack = {};
      for (auto& item : legsPtr->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<LegsArmor>(item)) {
            legsArmorStack.emplaceAppend(ArmorEntry{
                armourItem->frameset(gender),
                armourItem->directives(),
                Directives()});
          }
        }
      }
      setLegsArmorStack(legsArmorStack);
    } else {
      setLegsArmorFrameset("");
      setLegsArmorDirectives("");
      setLegsArmorStack({});
    }
  }

  if (back) {
    setBackArmorUnderlayFrameset("");
    setBackArmorUnderlayDirectives("");
    setBackArmorUnderlayStack({});
    if (auto backPtr = *back) {
      setBackArmorFrameset(backPtr->frameset(gender));
      setBackArmorDirectives(backPtr->directives());
      List<BackEntry> backArmorStack = {};
      for (auto& item : backPtr->getStackedCosmetics()) {
        if (item) {
          if (auto armourItem = as<BackArmor>(item)) {
            backArmorStack.emplaceAppend(BackEntry{
                armourItem->frameset(gender),
                armourItem->directives(),
                Directives()});
          }
        }
      }
      setBackArmorStack(backArmorStack);
    } else {
      setBackArmorFrameset("");
      setBackArmorDirectives("");
      setBackArmorStack({});
    }
  }

  auto drawables = render();
  Drawable::scaleAll(drawables, TilePixels);

  return drawables;
}

Vec2F Humanoid::primaryHandPosition(Vec2F const& offset) const {
  return primaryArmPosition(m_facingDirection, m_primaryHand.angle, primaryHandOffset(m_facingDirection) + offset);
}

Vec2F Humanoid::altHandPosition(Vec2F const& offset) const {
  return altArmPosition(m_facingDirection, m_altHand.angle, altHandOffset(m_facingDirection) + offset);
}

Vec2F Humanoid::primaryArmPosition(Direction facingDirection, float armAngle, Vec2F const& offset) const {
  float bobYOffset = getBobYOffset();

  if (m_primaryHand.holdingItem) {
    Vec2F rotationCenter;
    if (facingDirection == Direction::Left || m_twoHanded)
      rotationCenter = m_frontArmRotationCenter + Vec2F(0, bobYOffset);
    else
      rotationCenter = m_backArmRotationCenter + Vec2F(0, bobYOffset) + m_backArmOffset;

    Vec2F position = offset.rotate(armAngle) + rotationCenter;

    if (facingDirection == Direction::Left)
      position[0] *= -1;

    return position;
  } else {
    // TODO: non-aiming item holding not supported yet
    return Vec2F();
  }
}

Vec2F Humanoid::altArmPosition(Direction facingDirection, float armAngle, Vec2F const& offset) const {
  float bobYOffset = getBobYOffset();

  if (m_altHand.holdingItem) {
    Vec2F rotationCenter;
    if (facingDirection == Direction::Right)
      rotationCenter = m_frontArmRotationCenter + Vec2F(0, bobYOffset);
    else
      rotationCenter = m_backArmRotationCenter + Vec2F(0, bobYOffset) + m_backArmOffset;

    Vec2F position = Vec2F(offset).rotate(armAngle) + rotationCenter;

    if (facingDirection == Direction::Left)
      position[0] *= -1;

    return position;
  } else {
    // TODO: non-aiming item holding not supported yet
    return Vec2F();
  }
}

Vec2F Humanoid::primaryHandOffset(Direction facingDirection) const {
  if (facingDirection == Direction::Left || m_twoHanded)
    return m_frontHandPosition - m_frontArmRotationCenter;
  else
    return m_frontHandPosition - m_backArmRotationCenter;
}

Vec2F Humanoid::altHandOffset(Direction facingDirection) const {
  if (facingDirection == Direction::Left || m_twoHanded)
    return m_frontHandPosition - m_backArmRotationCenter;
  else
    return m_frontHandPosition - m_frontArmRotationCenter;
}

String Humanoid::frameBase(State state) const {
  switch (state) {
    case State::Idle:
      return "idle";
    case State::Walk:
      return "walk";
    case State::Run:
      return "run";
    case State::Jump:
      return "jump";
    case State::Swim:
      return "swim";
    case State::SwimIdle:
      return "swimIdle";
    case State::Duck:
      return "duck";
    case State::Fall:
      return "fall";
    case State::Sit:
      return "sit";
    case State::Lay:
      return "lay";

    default:
      throw StarException(strf("No such state '{}'", StateNames.getRight(state)));
  }
}

String Humanoid::emoteFrameBase(HumanoidEmote state) const {
  switch (state) {
    case HumanoidEmote::Idle:
      return "idle";
    case HumanoidEmote::Blabbering:
      return "blabber";
    case HumanoidEmote::Shouting:
      return "shout";
    case HumanoidEmote::Happy:
      return "happy";
    case HumanoidEmote::Sad:
      return "sad";
    case HumanoidEmote::NEUTRAL:
      return "neutral";
    case HumanoidEmote::Laugh:
      return "laugh";
    case HumanoidEmote::Annoyed:
      return "annoyed";
    case HumanoidEmote::Oh:
      return "oh";
    case HumanoidEmote::OOOH:
      return "oooh";
    case HumanoidEmote::Blink:
      return "blink";
    case HumanoidEmote::Wink:
      return "wink";
    case HumanoidEmote::Eat:
      return "eat";
    case HumanoidEmote::Sleep:
      return "sleep";

    default:
      throw StarException(strf("No such emote state '{}'", HumanoidEmoteNames.getRight(state)));
  }
}

String Humanoid::getHeadFromIdentity() const {
  return strf("/humanoid/{}/{}head.png",
      m_visualIdentity.imagePath ? *m_visualIdentity.imagePath : m_visualIdentity.species,
      GenderNames.getRight(m_visualIdentity.gender));
}

String Humanoid::getBodyFromIdentity() const {
  return strf("/humanoid/{}/{}body.png",
      m_visualIdentity.imagePath ? *m_visualIdentity.imagePath : m_visualIdentity.species,
      GenderNames.getRight(m_visualIdentity.gender));
}

String Humanoid::getFacialEmotesFromIdentity() const {
  return strf("/humanoid/{}/emote.png", m_visualIdentity.imagePath ? *m_visualIdentity.imagePath : m_visualIdentity.species);
}

String Humanoid::getHairFromIdentity() const {
  if (m_visualIdentity.hairType.empty())
    return "";
  return strf("/humanoid/{}/{}/{}.png",
      m_visualIdentity.imagePath ? *m_visualIdentity.imagePath : m_visualIdentity.species,
      m_visualIdentity.hairGroup,
      m_visualIdentity.hairType);
}

String Humanoid::getFacialHairFromIdentity() const {
  if (m_visualIdentity.facialHairType.empty())
    return "";
  return strf("/humanoid/{}/{}/{}.png",
      m_visualIdentity.imagePath ? *m_visualIdentity.imagePath : m_visualIdentity.species,
      m_visualIdentity.facialHairGroup,
      m_visualIdentity.facialHairType);
}

String Humanoid::getFacialMaskFromIdentity() const {
  if (m_visualIdentity.facialMaskType.empty())
    return "";
  return strf("/humanoid/{}/{}/{}.png",
      m_visualIdentity.imagePath ? *m_visualIdentity.imagePath : m_visualIdentity.species,
      m_visualIdentity.facialMaskGroup,
      m_visualIdentity.facialMaskType);
}

String Humanoid::getBackArmFromIdentity() const {
  return strf("/humanoid/{}/backarm.png", m_visualIdentity.imagePath ? *m_visualIdentity.imagePath : m_visualIdentity.species);
}

String Humanoid::getFrontArmFromIdentity() const {
  return strf("/humanoid/{}/frontarm.png", m_visualIdentity.imagePath ? *m_visualIdentity.imagePath : m_visualIdentity.species);
}

String Humanoid::getVaporTrailFrameset() const {
  return "/humanoid/any/flames.png";
}

Directives Humanoid::getBodyDirectives() const {
  return m_visualIdentity.bodyDirectives;
}

Directives Humanoid::getHairDirectives() const {
  return m_visualIdentity.hairDirectives;
}

Directives Humanoid::getEmoteDirectives() const {
  return m_visualIdentity.emoteDirectives;
}

Directives Humanoid::getFacialHairDirectives() const {
  return m_visualIdentity.facialHairDirectives;
}

Directives Humanoid::getFacialMaskDirectives() const {
  return m_visualIdentity.facialMaskDirectives;
}

Directives Humanoid::getHelmetMaskDirectives() const {
  return m_helmetMaskDirectives;
}

Directives Humanoid::getHelmetMaskUnderlayDirectives() const {
  return m_helmetMaskUnderlayDirectives;
}

Directives Humanoid::getHeadDirectives() const {
  return m_headArmorDirectives;
}

Directives Humanoid::getHeadUnderlayDirectives() const {
  return m_headArmorUnderlayDirectives;
}

Directives Humanoid::getChestDirectives() const {
  return m_chestArmorDirectives;
}

Directives Humanoid::getChestUnderlayDirectives() const {
  return m_chestArmorUnderlayDirectives;
}

Directives Humanoid::getLegsDirectives() const {
  return m_legsArmorDirectives;
}

Directives Humanoid::getLegsUnderlayDirectives() const {
  return m_legsArmorUnderlayDirectives;
}

Directives Humanoid::getBackDirectives() const {
  return m_backArmorDirectives;
}

Directives Humanoid::getBackUnderlayDirectives() const {
  return m_backArmorUnderlayDirectives;
}

void Humanoid::updateHumanoidConfigOverrides(Json overrides, bool force) {
  auto checkSpecies = [](String const& species) -> bool { // FezzedOne: Check whether a species exists in the loaded assets.
    bool speciesFound = false;
    for (auto const& nameDefPair : Root::singleton().speciesDatabase()->allSpecies()) {
      if (species == nameDefPair.second->options().species) {
        speciesFound = true;
        break;
      }
    }
    return speciesFound;
  };

  auto getString = [](Json const& json, String const& key) -> String {
    Json jValue = json.opt(key).value(Json());
    return jValue.isType(Json::Type::String) ? jValue.toString() : "";
  };

  auto checkString = [](Json const& json, String const& key) -> bool {
    Json jValue = json.opt(key).value(Json());
    return jValue.isType(Json::Type::String);
  };

  auto replaceBaseTag = [this](String const& base, String const& merger) -> String {
    // FezzedOne: Need to perform one final tag replacement in the humanoid renderer to substitute the base identity directives in that «slot» for any remaining `<base>` tags.
    StringMap<String> tags{{"base", merger}};
    return base.replaceTags(tags, false);
  };

  auto mergeOverrides = [this, getString, checkString, replaceBaseTag](Json& base, String const& key, Directives const& merger) {
    if (checkString(base, key)) {
      auto detaggedString = replaceBaseTag(getString(base, key), merger.string());
      base = base.set(key, detaggedString);
    }
  };

  if (overrides == m_previousOverrides && !force) return;
  if (!overrides.isType(Json::Type::Object)) overrides = JsonObject{};
  auto baseConfig = m_baseHumanoidConfig;
  if (auto jIdentityOverrides = overrides.get("identity", Json()); jIdentityOverrides.isType(Json::Type::Object)) {
    Json jBroadcastToStock = jIdentityOverrides.opt("broadcast").value(Json());
    m_broadcastToStock = jBroadcastToStock.isType(Json::Type::Bool) ? jBroadcastToStock.toBool() : false;
    auto baseSpecies = m_identity.imagePath ? *m_identity.imagePath : m_identity.species;
    mergeOverrides(jIdentityOverrides, "bodyDirectives", m_identity.bodyDirectives);
    mergeOverrides(jIdentityOverrides, "hairDirectives", m_identity.hairDirectives);
    mergeOverrides(jIdentityOverrides, "emoteDirectives", m_identity.emoteDirectives);
    mergeOverrides(jIdentityOverrides, "facialHairDirectives", m_identity.facialHairDirectives);
    mergeOverrides(jIdentityOverrides, "facialMaskDirectives", m_identity.facialMaskDirectives);
    auto newIdentity = HumanoidIdentity(jsonMerge(m_identity.toJson(), jIdentityOverrides));
    auto speciesToCheck = newIdentity.imagePath ? *newIdentity.imagePath : newIdentity.species;
    auto speciesToUse = checkSpecies(speciesToCheck) ? speciesToCheck : baseSpecies;
    newIdentity.species = m_identity.species;
    newIdentity.imagePath = speciesToCheck;
    setIdentity(m_identity, newIdentity);
    baseConfig = Root::singleton().speciesDatabase()->species(speciesToUse)->humanoidConfig();
  } else {
    setIdentity(m_identity);
    m_broadcastToStock = false;
  }
  Json config = jsonMerge(baseConfig, overrides);
  m_previousOverrides = overrides;

  m_timing = HumanoidTiming(config.getObject("humanoidTiming"));
  m_globalOffset = jsonToVec2F(config.get("globalOffset")) / TilePixels;
  m_headRunOffset = jsonToVec2F(config.get("headRunOffset")) / TilePixels;
  m_headSwimOffset = jsonToVec2F(config.get("headSwimOffset")) / TilePixels;
  m_runFallOffset = config.get("runFallOffset").toDouble() / TilePixels;
  m_duckOffset = config.get("duckOffset").toDouble() / TilePixels;
  m_headDuckOffset = jsonToVec2F(config.get("headDuckOffset")) / TilePixels;
  m_sitOffset = config.get("sitOffset").toDouble() / TilePixels;
  m_layOffset = config.get("layOffset").toDouble() / TilePixels;
  m_headSitOffset = jsonToVec2F(config.get("headSitOffset")) / TilePixels;
  m_headLayOffset = jsonToVec2F(config.get("headLayOffset")) / TilePixels;
  m_recoilOffset = jsonToVec2F(config.get("recoilOffset")) / TilePixels;
  m_mouthOffset = jsonToVec2F(config.get("mouthOffset")) / TilePixels;
  m_feetOffset = jsonToVec2F(config.get("feetOffset")) / TilePixels;

  m_bodyFullbright = config.getBool("bodyFullbright", false);

  m_headArmorOffset = jsonToVec2F(config.get("headArmorOffset")) / TilePixels;
  m_chestArmorOffset = jsonToVec2F(config.get("chestArmorOffset")) / TilePixels;
  m_legsArmorOffset = jsonToVec2F(config.get("legsArmorOffset")) / TilePixels;
  m_backArmorOffset = jsonToVec2F(config.get("backArmorOffset")) / TilePixels;
  m_headRotationMultiplier = config.getFloat("headRotationMultiplier", 0.375f);
  Json maximumHeadRotationOffset = config.get("maximumHeadRotationOffset", JsonArray{-2.0f, -1.0f});
  m_maximumHeadRotationOffset = jsonToVec2F(maximumHeadRotationOffset) / TilePixels;
  Json headCenterPosition = config.get("headRotationCenter", JsonArray{0.0f, 2.0f});
  m_headCenterPosition = jsonToVec2F(headCenterPosition) / TilePixels;

  m_bodyHidden = config.getBool("bodyHidden", false);

  m_armWalkSeq = jsonToIntList(config.get("armWalkSeq"));
  m_armRunSeq = jsonToIntList(config.get("armRunSeq"));

  for (auto const& v : config.get("walkBob").toArray())
    m_walkBob.append(v.toDouble() / TilePixels);
  for (auto const& v : config.get("runBob").toArray())
    m_runBob.append(v.toDouble() / TilePixels);
  for (auto const& v : config.get("swimBob").toArray())
    m_swimBob.append(v.toDouble() / TilePixels);

  m_jumpBob = config.get("jumpBob").toDouble() / TilePixels;
  m_frontArmRotationCenter = jsonToVec2F(config.get("frontArmRotationCenter")) / TilePixels;
  m_backArmRotationCenter = jsonToVec2F(config.get("backArmRotationCenter")) / TilePixels;
  m_frontHandPosition = jsonToVec2F(config.get("frontHandPosition")) / TilePixels;
  m_backArmOffset = jsonToVec2F(config.get("backArmOffset")) / TilePixels;
  m_vaporTrailFrames = config.get("vaporTrailFrames").toInt();
  m_vaporTrailCycle = config.get("vaporTrailCycle").toFloat();

  m_defaultDeathParticles = config.getString("deathParticles");
  Json jParticleEmitters = config.get("particleEmitters");
  m_particleEmitters = jParticleEmitters.isType(Json::Type::Object) ? jParticleEmitters : JsonObject{};

  m_defaultMovementParameters = config.get("movementParameters");
}

int Humanoid::getEmoteStateSequence() const {
  return m_timing.emoteStateSeq(m_emoteAnimationTimer, m_emoteState);
}

int Humanoid::getArmStateSequence() const {
  int stateSeq = m_timing.stateSeq(m_animationTimer, m_state);

  int armStateSeq;
  if (m_state == Walk) {
    armStateSeq = m_movingBackwards ? m_armWalkSeq.at(m_armWalkSeq.size() - stateSeq) : m_armWalkSeq.at(stateSeq - 1);
  } else if (m_state == Run) {
    armStateSeq = m_movingBackwards ? m_armRunSeq.at(m_armRunSeq.size() - stateSeq) : m_armRunSeq.at(stateSeq - 1);
  } else {
    armStateSeq = stateSeq;
  }

  return armStateSeq;
}

int Humanoid::getBodyStateSequence() const {
  int stateSeq = m_timing.stateSeq(m_animationTimer, m_state);

  int bodyStateSeq;
  if (m_movingBackwards && (m_state == Walk || m_state == Run))
    bodyStateSeq = m_timing.stateFrames[m_state] - stateSeq + 1;
  else
    bodyStateSeq = stateSeq;

  return std::max(bodyStateSeq, 0);
}

Maybe<DancePtr> Humanoid::getDance() const {
  if (m_dance.isNothing())
    return {};

  auto danceDatabase = Root::singleton().danceDatabase();
  return danceDatabase->getDance(*m_dance);
}

float Humanoid::getBobYOffset() const {
  int bodyStateSeq = getBodyStateSequence();
  float bobYOffset = 0.0f;
  // FezzedOne: Removed potential for an out-of-range exception to get thrown. Instead, no bob offset gets applied.
  if (m_state == Run) {
    bobYOffset = m_runFallOffset + ((m_runBob.size() > (size_t)(bodyStateSeq - 1)) ? m_runBob[bodyStateSeq - 1] : 0.0f);
  } else if (m_state == Fall) {
    bobYOffset = m_runFallOffset;
  } else if (m_state == Jump) {
    bobYOffset = m_jumpBob;
  } else if (m_state == Walk) {
    bobYOffset = ((m_walkBob.size() > (size_t)(bodyStateSeq - 1)) ? m_walkBob[bodyStateSeq - 1] : 0.0f);
  } else if (m_state == Swim) {
    bobYOffset = ((m_swimBob.size() > (size_t)(bodyStateSeq - 1)) ? m_swimBob[bodyStateSeq - 1] : 0.0f);
  } else if (m_state == Duck) {
    bobYOffset = m_duckOffset;
  } else if (m_state == Sit) {
    bobYOffset = m_sitOffset;
  } else if (m_state == Lay) {
    bobYOffset = m_layOffset;
  }
  return bobYOffset;
}

Vec2F Humanoid::armAdjustment() const {
  return Vec2F(0, getBobYOffset());
}

Vec2F Humanoid::mouthOffset(bool ignoreAdjustments) const {
  if (ignoreAdjustments) {
    return (m_mouthOffset).rotate(m_rotation);
  } else {
    Vec2F headPosition(0, getBobYOffset());
    if (m_state == Idle)
      headPosition += m_identity.personality.headOffset / TilePixels;
    else if (m_state == Run)
      headPosition += m_headRunOffset;
    else if (m_state == Swim || m_state == SwimIdle)
      headPosition += m_headSwimOffset;
    else if (m_state == Duck)
      headPosition += m_headDuckOffset;
    else if (m_state == Sit)
      headPosition += m_headSitOffset;
    else if (m_state == Lay)
      headPosition += m_headLayOffset;

    return (m_mouthOffset + headPosition).rotate(m_rotation);
  }
}

Vec2F Humanoid::feetOffset() const {
  return m_feetOffset.rotate(m_rotation);
}

Vec2F Humanoid::headArmorOffset() const {
  Vec2F headPosition(0, getBobYOffset());
  if (m_state == Idle)
    headPosition += m_identity.personality.headOffset / TilePixels;
  else if (m_state == Run)
    headPosition += m_headRunOffset;
  else if (m_state == Swim || m_state == SwimIdle)
    headPosition += m_headSwimOffset;
  else if (m_state == Duck)
    headPosition += m_headDuckOffset;
  else if (m_state == Sit)
    headPosition += m_headSitOffset;
  else if (m_state == Lay)
    headPosition += m_headLayOffset;

  return (m_headArmorOffset + headPosition).rotate(m_rotation);
}

Vec2F Humanoid::chestArmorOffset() const {
  Vec2F position(0, getBobYOffset());
  return (m_chestArmorOffset + position).rotate(m_rotation);
}

Vec2F Humanoid::legsArmorOffset() const {
  return m_legsArmorOffset.rotate(m_rotation);
}

Vec2F Humanoid::backArmorOffset() const {
  Vec2F position(0, getBobYOffset());
  return (m_backArmorOffset + position).rotate(m_rotation);
}

String Humanoid::defaultDeathParticles() const {
  return m_defaultDeathParticles;
}

List<Particle> Humanoid::particles(String const& name) const {
  auto particleDatabase = Root::singleton().particleDatabase();
  List<Particle> res;
  // FezzedOne: Null or missing humanoid particle entries are now ignored instead of throwing an exception.
  auto particleEmitter = m_particleEmitters.opt(name);
  if (!particleEmitter || particleEmitter->isNull()) return res;
  Json particles = particleEmitter->get("particles", {});
  for (auto& particle : particles.toArray()) {
    auto particleSpec = particle.get("particle", {});
    res.push_back(particleDatabase->particle(particleSpec));
  }

  return res;
}

Json const& Humanoid::defaultMovementParameters() const {
  return m_defaultMovementParameters;
}

} // namespace Star
