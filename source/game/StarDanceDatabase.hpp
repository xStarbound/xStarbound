#ifndef STAR_DANCE_DATABASE_HPP
#define STAR_DANCE_DATABASE_HPP

#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

STAR_STRUCT(DanceStep);
STAR_STRUCT(Dance);
STAR_CLASS(DanceDatabase);

// FezzedOne: Set defaults.
struct DanceStep {
  Maybe<String> bodyFrame = {};
  Maybe<String> frontArmFrame = {};
  Maybe<String> backArmFrame = {};
  Vec2F headOffset = Vec2F(0.0f, 0.0f);
  Vec2F frontArmOffset = Vec2F(0.0f, 0.0f);
  Vec2F backArmOffset = Vec2F(0.0f, 0.0f);
  float frontArmRotation = 0.0f;
  float backArmRotation = 0.0f;
};

struct Dance {
  String name = "assetmissing";
  List<String> states = {};
  float cycle = 0.0f;
  bool cyclic = false;
  float duration = 0.0f;
  List<DanceStep> steps = {};
};

class DanceDatabase {
public:
  DanceDatabase();

  DancePtr getDance(String const& name) const;

private:
  static DancePtr readDance(String const& path);

  StringMap<DancePtr> m_dances;
};

} // namespace Star

#endif
