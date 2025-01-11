#include <QApplication>
#include <QMessageBox>

#include "steam_api.h"

#include "StarModUploader.hpp"
#include "StarStringConversion.hpp"

#ifdef STAR_USE_RPMALLOC
#include "rpmalloc/rpmalloc.h"
#endif

using namespace Star;

int main(int argc, char** argv) {
#ifdef STAR_USE_RPMALLOC
  ::rpmalloc_initialize();
#endif
  QApplication app(argc, argv);

  if (!SteamAPI_Init()) {
    QMessageBox::critical(nullptr, "Error", "Could not initialize Steam API");
#ifdef STAR_USE_RPMALLOC
    ::rpmalloc_finalize();
#endif
    return 1;
  }

  ModUploader modUploader;
  modUploader.show();

  int returnVal;
  try {
    returnVal = app.exec();
  } catch (std::exception const& e) {
    QMessageBox::critical(nullptr, "Error", toQString(strf("Exception caught: {}\n", outputException(e, true))));
    coutf("Exception caught: {}\n", outputException(e, true));
    returnVal = 1;
  }
#ifdef STAR_USE_RPMALLOC
  ::rpmalloc_finalize();
#endif
  return returnVal;
}
