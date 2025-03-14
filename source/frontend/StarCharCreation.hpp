#ifndef _STAR_CHAR_CREATION_H_
#define _STAR_CHAR_CREATION_H_

#include "StarPane.hpp"
#include "StarImageProcessing.hpp"
#include "StarHumanoid.hpp"

namespace Star {

class Player;
typedef shared_ptr<Player> PlayerPtr;

STAR_EXCEPTION(CharCreationException, StarException);

STAR_CLASS(CharCreationPane);
class CharCreationPane : public Pane {
public:
  // The callback here is either called with null (when the user hits the
  // cancel button) or the newly created player (when the user hits the save
  // button).
  CharCreationPane(function<void(PlayerPtr)> requestCloseFunc, PlayerPtr existingPlayer = nullptr);

  void randomize();
  void randomizeName();

  virtual void tick(float dt) override;
  virtual bool sendEvent(InputEvent const& event) override;
  virtual void dismissed() override;
  virtual void displayed() override;

  virtual PanePtr createTooltip(Vec2I const&) override;

private:
  void nameBoxCallback(Widget* object);

  void changed();

  void createPlayer();

  void setShirt(String const& shirt, size_t colorIndex);
  void setPants(String const& pants, size_t colorIndex);

  PlayerPtr m_previewPlayer;

  Maybe<HumanoidIdentity> m_oldIdentity;

  StringList m_speciesList;

  size_t m_speciesChoice;
  size_t m_genderChoice;
  size_t m_modeChoice;
  size_t m_bodyColor;
  size_t m_alty;
  size_t m_hairChoice;
  size_t m_heady;
  size_t m_shirtChoice;
  size_t m_shirtColor;
  size_t m_pantsChoice;
  size_t m_pantsColor;
  size_t m_personality;

  bool m_isExistingPlayer;
  bool m_skipRandomisation;
  bool m_speciesChanged;
  bool m_genderChanged;
  bool m_coloursChanged;
  bool m_personalityChanged;
  bool m_hairChanged;
  bool m_facialHairChanged;
  bool m_facialMaskChanged;
  bool m_modeChanged;
};

}

#endif
