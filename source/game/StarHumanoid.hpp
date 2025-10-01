#ifndef STAR_HUMANOID_HPP
#define STAR_HUMANOID_HPP

#include "StarDataStream.hpp"
#include "StarDrawable.hpp"
#include "StarGameTypes.hpp"
#include "StarParticle.hpp"

namespace Star {

// Required for renderDummy
STAR_CLASS(HeadArmor);
STAR_CLASS(ChestArmor);
STAR_CLASS(LegsArmor);
STAR_CLASS(BackArmor);

STAR_CLASS(Humanoid);

STAR_STRUCT(Dance);

constexpr uint8_t BaseCosmeticOrdering = 255;

enum class HumanoidEmote {
  Idle,
  Blabbering,
  Shouting,
  Happy,
  Sad,
  NEUTRAL,
  Laugh,
  Annoyed,
  Oh,
  OOOH,
  Blink,
  Wink,
  Eat,
  Sleep
};

enum HumanoidRotationSettings : uint32_t {
  Null = 0,
  UseBodyMask = 1 << 0,
  UseBodyHeadMask = 1 << 1
};

extern EnumMap<HumanoidEmote> const HumanoidEmoteNames;
size_t const EmoteSize = 14;

struct Personality {
  String idle = "idle.1";
  String armIdle = "idle.1";
  Vec2F headOffset = Vec2F();
  Vec2F armOffset = Vec2F();
};

Personality parsePersonalityArray(Json const& config);

Personality& parsePersonality(Personality& personality, Json const& config);
Personality parsePersonality(Json const& config);

Json jsonFromPersonality(Personality const& personality);

struct HumanoidIdentity {
  explicit HumanoidIdentity(Json config = Json());

  Json toJson() const;

  String name;
  // Must have :idle[1-5], :sit, :duck, :walk[1-8], :run[1-8], :jump[1-4], and
  // :fall[1-4]
  String species;
  Gender gender;

  String hairGroup;
  // Must have :normal and :climb
  String hairType;
  Directives hairDirectives;
  Directives bodyDirectives;
  Directives emoteDirectives;
  String facialHairGroup;
  String facialHairType;
  Directives facialHairDirectives;
  String facialMaskGroup;
  String facialMaskType;
  Directives facialMaskDirectives;

  Personality personality;
  Vec4B color;

  Maybe<String> imagePath;
};

DataStream& operator>>(DataStream& ds, HumanoidIdentity& identity);
DataStream& operator<<(DataStream& ds, HumanoidIdentity const& identity);

class Humanoid {
public:
  enum State {
    Idle,     // 1 idle frame
    Walk,     // 8 walking frames
    Run,      // 8 run frames
    Jump,     // 4 jump frames
    Fall,     // 4 fall frames
    Swim,     // 7 swim frames
    SwimIdle, // 2 swim idle frame
    Duck,     // 1 ducking frame
    Sit,      // 1 sitting frame
    Lay,      // 1 laying frame
    STATESIZE
  };
  static EnumMap<State> const StateNames;

  Humanoid(Json const& config);
  Humanoid(HumanoidIdentity const& identity);
  Humanoid(Humanoid const&) = default;

  struct HumanoidTiming {
    explicit HumanoidTiming(Json config = Json());

    static bool cyclicState(State state);
    static bool cyclicEmoteState(HumanoidEmote state);

    int stateSeq(float timer, State state) const;
    int emoteStateSeq(float timer, HumanoidEmote state) const;
    int danceSeq(float timer, DancePtr dance) const;
    int genericSeq(float timer, float cycle, unsigned frames, bool cyclic) const;

    Array<float, STATESIZE> stateCycle;
    Array<unsigned, STATESIZE> stateFrames;

    Array<float, EmoteSize> emoteCycle;
    Array<unsigned, EmoteSize> emoteFrames;
  };

  struct ArmorEntry {
    String frameset = "";
    Directives directives = Directives();
    Directives maskDirectives = Directives();
    uint8_t ordering = BaseCosmeticOrdering; // FezzedOne: For oSB cosmetic ordering.
    bool maskHumanoidBaseOnly = false;
  };

  struct BackEntry {
    String frameset = "";
    Directives directives = Directives();
    Directives maskDirectives = Directives();
    bool rotateWithHead = false;
  };

  void setIdentity(HumanoidIdentity const& identity);
  HumanoidIdentity const& identity() const;

  void updateHumanoidConfigOverrides(Json overrides = Json());

  // All of the image identifiers here are meant to be image *base* names, with
  // a collection of frames specific to each piece.  If an image is set to
  // empty string, it is disabled.

  // Asset directives for the head armor.
  void setHeadArmorDirectives(Directives directives);
  // Must have :normal, climb
  void setHeadArmorFrameset(String headFrameset);
  // Asset directives for the chest, back and front arms armor.
  void setChestArmorDirectives(Directives directives);
  // Will have :run, :normal, and :duck
  void setChestArmorFrameset(String chest);
  // Same as back arm image frames
  void setBackSleeveFrameset(String backSleeveFrameset);
  // Same as front arm image frames
  void setFrontSleeveFrameset(String frontSleeveFrameset);

  // Asset directives for the legs armor.
  void setLegsArmorDirectives(Directives directives);
  // Must have :idle, :duck, :walk[1-8], :run[1-8], :jump[1-4], :fall[1-4]
  void setLegsArmorFrameset(String legsFrameset);

  // Asset directives for the back armor.
  void setBackArmorDirectives(Directives directives);
  // Must have :idle, :duck, :walk[1-8], :run[1-8], :jump[1-4], :fall[1-4]
  void setBackArmorFrameset(String backFrameset);
  void setBackArmorHeadRotation(bool rotation);

  void setHelmetMaskDirectives(Directives helmetMaskDirectives);

  void setHeadArmorUnderlayDirectives(Directives directives);
  void setHeadArmorUnderlayFrameset(String headFrameset);
  void setChestArmorUnderlayDirectives(Directives directives);
  void setChestArmorUnderlayFrameset(String chest);
  void setBackSleeveUnderlayFrameset(String backSleeveFrameset);
  void setFrontSleeveUnderlayFrameset(String frontSleeveFrameset);

  void setLegsArmorUnderlayDirectives(Directives directives);
  void setLegsArmorUnderlayFrameset(String legsFrameset);

  void setBackArmorUnderlayDirectives(Directives directives);
  void setBackArmorUnderlayFrameset(String backFrameset);
  void setBackArmorUnderlayHeadRotation(bool rotation);

  void setHelmetMaskUnderlayDirectives(Directives helmetMaskDirectives);

  void setBackArmorStack(List<BackEntry> const& newStack);
  void setBackSleeveStack(List<ArmorEntry> const& newStack);
  void setLegsArmorStack(List<ArmorEntry> const& newStack);
  void setChestArmorStack(List<ArmorEntry> const& newStack);
  void setHeadArmorStack(List<ArmorEntry> const& newStack);
  void setFrontSleeveStack(List<ArmorEntry> const& newStack);

  void setBackArmorUnderlayStack(List<BackEntry> const& newStack);
  void setBackSleeveUnderlayStack(List<ArmorEntry> const& newStack);
  void setLegsArmorUnderlayStack(List<ArmorEntry> const& newStack);
  void setChestArmorUnderlayStack(List<ArmorEntry> const& newStack);
  void setHeadArmorUnderlayStack(List<ArmorEntry> const& newStack);
  void setFrontSleeveUnderlayStack(List<ArmorEntry> const& newStack);

  void setBodyHidden(bool hidden);

  void setState(State state);
  void setEmoteState(HumanoidEmote state);
  void setDance(Maybe<String> const& dance);
  void setFacingDirection(Direction facingDirection);
  void setMovingBackwards(bool movingBackwards);
  void setRotation(float rotation);

  void setVaporTrail(bool enabled);

  State state() const;
  HumanoidEmote emoteState() const;
  Maybe<String> dance() const;
  bool danceCyclicOrEnded() const;
  Direction facingDirection() const;
  bool movingBackwards() const;

  // If not rotating, then the arms follow normal movement animation.  The
  // angle parameter should be in the range [-pi/2, pi/2] (the facing direction
  // should not be included in the angle).
  void setPrimaryHandParameters(bool holdingItem, float angle, float itemAngle, bool twoHanded,
      bool recoil, bool outsideOfHand);
  void setPrimaryHandFrameOverrides(String backFrameOverride, String frontFrameOverride);
  void setPrimaryHandDrawables(List<Drawable> drawables);
  void setPrimaryHandNonRotatedDrawables(List<Drawable> drawables);

  // Same as primary hand.
  void setAltHandParameters(bool holdingItem, float angle, float itemAngle, bool recoil,
      bool outsideOfHand);
  void setAltHandFrameOverrides(String backFrameOverride, String frontFrameOverride);
  void setAltHandDrawables(List<Drawable> drawables);
  void setAltHandNonRotatedDrawables(List<Drawable> drawables);

  // Updates the animation based on whatever the current animation state is,
  // wrapping or clamping animation time as appropriate.
  void animate(float dt);

  // Reset animation time to 0.0f
  void resetAnimation();

  // Renders to centered drawables (centered on the normal image center for the
  // player graphics), (in world space, not pixels)
  List<Drawable> render(bool withItems = true, bool withRotation = true, Maybe<float> aimAngle = {});

  // Renders to centered drawables (centered on the normal image center for the
  // player graphics), (in pixels, not world space)
  List<Drawable> renderPortrait(PortraitMode mode) const;

  List<Drawable> renderSkull() const;

  static Humanoid makeDummy(Gender gender);
  // Renders to centered drawables (centered on the normal image center for the
  // player graphics), (in pixels, not world space)
  List<Drawable> renderDummy(Gender gender, Maybe<HeadArmor const*> head = {}, Maybe<ChestArmor const*> chest = {},
      Maybe<LegsArmor const*> legs = {}, Maybe<BackArmor const*> back = {});

  Vec2F primaryHandPosition(Vec2F const& offset) const;
  Vec2F altHandPosition(Vec2F const& offset) const;

  // Finds the arm position in world space if the humanoid was facing the given
  // direction and applying the given arm angle.  The offset given is from the
  // rotation center of the arm.
  Vec2F primaryArmPosition(Direction facingDirection, float armAngle, Vec2F const& offset) const;
  Vec2F altArmPosition(Direction facingDirection, float armAngle, Vec2F const& offset) const;

  // Gives the offset of the hand from the arm rotation center
  Vec2F primaryHandOffset(Direction facingDirection) const;
  Vec2F altHandOffset(Direction facingDirection) const;

  Vec2F armAdjustment() const;

  Vec2F mouthOffset(bool ignoreAdjustments = false) const;
  float getBobYOffset() const;
  Vec2F feetOffset() const;

  Vec2F headArmorOffset() const;
  Vec2F chestArmorOffset() const;
  Vec2F legsArmorOffset() const;
  Vec2F backArmorOffset() const;

  String defaultDeathParticles() const;
  List<Particle> particles(String const& name) const;

  Json const& defaultMovementParameters() const;

private:
  struct HandDrawingInfo {
    List<Drawable> itemDrawables;
    List<Drawable> nonRotatedDrawables;
    bool holdingItem = false;
    bool skipFrontSplice = false;
    bool skipBackSplice = false;
    float angle = 0.0f;
    float itemAngle = 0.0f;
    String backFrame;
    String frontFrame;
    Directives backDirectives = Directives();
    Directives frontDirectives = Directives();
    float frameAngleAdjust = 0.0f;
    bool recoil = false;
    bool outsideOfHand = false;
  };

  String frameBase(State state) const;
  String emoteFrameBase(HumanoidEmote state) const;

  String getHeadFromIdentity() const;
  String getBodyFromIdentity() const;
  String getFacialEmotesFromIdentity() const;
  String getHairFromIdentity() const;
  String getFacialHairFromIdentity() const;
  String getFacialMaskFromIdentity() const;
  String getBackArmFromIdentity() const;
  String getFrontArmFromIdentity() const;
  String getVaporTrailFrameset() const;

  Directives getBodyDirectives() const;
  Directives getHairDirectives() const;
  Directives getEmoteDirectives() const;
  Directives getFacialHairDirectives() const;
  Directives getFacialMaskDirectives() const;
  Directives getHelmetMaskDirectives() const;
  Directives getHelmetMaskUnderlayDirectives() const;
  Directives getHeadDirectives() const;
  Directives getHeadUnderlayDirectives() const;
  Directives getChestDirectives() const;
  Directives getChestUnderlayDirectives() const;
  Directives getLegsDirectives() const;
  Directives getLegsUnderlayDirectives() const;
  Directives getBackDirectives() const;
  Directives getBackUnderlayDirectives() const;

  int getEmoteStateSequence() const;
  int getArmStateSequence() const;
  int getBodyStateSequence() const;

  Maybe<DancePtr> getDance() const;

  Vec2F m_globalOffset;
  Vec2F m_headRunOffset;
  Vec2F m_headSwimOffset;
  Vec2F m_headDuckOffset;
  Vec2F m_headSitOffset;
  Vec2F m_headLayOffset;
  float m_runFallOffset;
  float m_duckOffset;
  float m_sitOffset;
  float m_layOffset;
  Vec2F m_recoilOffset;
  Vec2F m_mouthOffset;
  Vec2F m_feetOffset;

  Vec2F m_headArmorOffset;
  Vec2F m_chestArmorOffset;
  Vec2F m_legsArmorOffset;
  Vec2F m_backArmorOffset;

  bool m_bodyHidden;

  List<int> m_armWalkSeq;
  List<int> m_armRunSeq;
  List<float> m_walkBob;
  List<float> m_runBob;
  List<float> m_swimBob;
  float m_jumpBob;
  Vec2F m_frontArmRotationCenter;
  Vec2F m_backArmRotationCenter;
  Vec2F m_frontHandPosition;
  Vec2F m_backArmOffset;

  String m_headFrameset;
  String m_bodyFrameset;
  String m_backArmFrameset;
  String m_frontArmFrameset;
  String m_emoteFrameset;
  String m_hairFrameset;
  String m_facialHairFrameset;
  String m_facialMaskFrameset;

  bool m_bodyFullbright;

  String m_vaporTrailFrameset;
  unsigned m_vaporTrailFrames;
  float m_vaporTrailCycle;

  String m_backSleeveFrameset;
  String m_frontSleeveFrameset;
  String m_headArmorFrameset;
  Directives m_headArmorDirectives;
  String m_chestArmorFrameset;
  Directives m_chestArmorDirectives;
  String m_legsArmorFrameset;
  Directives m_legsArmorDirectives;
  String m_backArmorFrameset;
  Directives m_backArmorDirectives;
  Directives m_helmetMaskDirectives;

  String m_backSleeveUnderlayFrameset;
  String m_frontSleeveUnderlayFrameset;
  String m_headArmorUnderlayFrameset;
  Directives m_headArmorUnderlayDirectives;
  String m_chestArmorUnderlayFrameset;
  Directives m_chestArmorUnderlayDirectives;
  String m_legsArmorUnderlayFrameset;
  Directives m_legsArmorUnderlayDirectives;
  String m_backArmorUnderlayFrameset;
  Directives m_backArmorUnderlayDirectives;
  Directives m_helmetMaskUnderlayDirectives;

  List<BackEntry> m_backArmorStack;
  List<ArmorEntry> m_backSleeveStack;
  List<ArmorEntry> m_legsArmorStack;
  List<ArmorEntry> m_chestArmorStack;
  List<ArmorEntry> m_headArmorStack;
  List<ArmorEntry> m_frontSleeveStack;

  List<BackEntry> m_backArmorUnderlayStack;
  List<ArmorEntry> m_backSleeveUnderlayStack;
  List<ArmorEntry> m_legsArmorUnderlayStack;
  List<ArmorEntry> m_chestArmorUnderlayStack;
  List<ArmorEntry> m_headArmorUnderlayStack;
  List<ArmorEntry> m_frontSleeveUnderlayStack;

  Vec2F m_headCenterPosition;     // FezzedOne: Position of the centre of the humanoid's head.
  float m_headRotationMultiplier; // FezzedOne: The amount to multiply head rotation by.
  // FezzedOne: The maximum offset by which the head is shifted while it is being rotated.
  // The head rotation (before the multiplier above) is divided by `0.5f * Constants::pi`,
  // its value is used to fill the X and Y of a `Vec2F`, then that `Vec2F` is piecewise-
  // multiplied by this value (`x * x`, `y * y`) and used to offset the head.
  Vec2F m_maximumHeadRotationOffset;

  State m_state;
  HumanoidEmote m_emoteState;
  Maybe<String> m_dance;
  Direction m_facingDirection;
  bool m_movingBackwards;
  float m_rotation;
  bool m_drawVaporTrail;
  bool m_backArmorHeadRotation;
  bool m_backArmorUnderlayHeadRotation;

  HandDrawingInfo m_primaryHand;
  HandDrawingInfo m_altHand;

  bool m_twoHanded;

  HumanoidIdentity m_identity;
  HumanoidTiming m_timing;

  float m_animationTimer;
  float m_emoteAnimationTimer;
  float m_danceTimer;

  Json m_particleEmitters;
  String m_defaultDeathParticles;

  Json m_defaultMovementParameters;

  uint32_t m_humanoidRotationSettings;

  Json m_baseHumanoidConfig;
  Json m_previousOverrides;
};

} // namespace Star

#endif
