#ifndef STAR_CHAR_SELECTION_HPP
#define STAR_CHAR_SELECTION_HPP

#include "StarPane.hpp"
#include "StarPlayerStorage.hpp"

namespace Star {

STAR_CLASS(PlayerStorage);

class CharSelectionPane : public Pane {
public:
  typedef function<void()> CreateCharCallback;
  typedef function<void(PlayerPtr const&)> SelectCharacterCallback;
  typedef function<void(Uuid)> DeleteCharacterCallback;
  typedef function<bool(Uuid const&)> FilterCallback;

  CharSelectionPane(PlayerStoragePtr playerStorage, CreateCharCallback createCallback,
      SelectCharacterCallback selectCallback, DeleteCharacterCallback deleteCallback,
      FilterCallback filterCallback = {});

  bool sendEvent(InputEvent const& event) override;
  void show() override;
  void update(float dt) override;
  void updateCharacterPlates();

private:
  void shiftCharacters(int movement);
  void selectCharacter(unsigned buttonIndex);

  PlayerStoragePtr m_playerStorage;
  unsigned m_downScroll;

  CreateCharCallback m_createCallback;
  SelectCharacterCallback m_selectCallback;
  DeleteCharacterCallback m_deleteCallback;
  FilterCallback m_filterCallback;

  bool m_listNeedsUpdate;
};
typedef shared_ptr<CharSelectionPane> CharSelectionPanePtr;
}

#endif
