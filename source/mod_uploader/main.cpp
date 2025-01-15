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

#if defined STAR_SYSTEM_LINUX
  #define STEAM_LIBRARY_FILE "libsteam_api.so"
#elif defined STAR_SYSTEM_WINDOWS
  #define STEAM_LIBRARY_FILE "steam_api64.dll"
#elif defined STAR_SYSTEM_MACOS
  #define STEAM_LIBRARY_FILE "libsteam_api.dylib"
#else
  #define STEAM_LIBRARY_FILE "<n/a>"
#endif

  if (!SteamAPI_Init()) {
    QMessageBox::critical(nullptr, "Error", "Could not initialize Steam API. Ensure that the " STEAM_LIBRARY_FILE 
      " that comes with xStarbound is present in the same folder as this mod uploader; the Steam API library in your stock Starbound installation will NOT work.");
#ifdef STAR_USE_RPMALLOC
    ::rpmalloc_finalize();
#endif
    return 1;
  }

  auto parsedArguments = ModUploader::parseArguments(argc, argv).value(ModUploader::ParsedArgs{});

  if (parsedArguments.shownHelp) {
#ifdef STAR_USE_RPMALLOC
    ::rpmalloc_finalize();
#endif
    return 0;
  }

  ModUploader modUploader(parsedArguments);
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
