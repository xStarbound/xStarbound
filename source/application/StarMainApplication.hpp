#ifndef STAR_MAIN_APPLICATION_HPP
#define STAR_MAIN_APPLICATION_HPP

#include "StarApplication.hpp"
#include "StarApplicationController.hpp"
#include "StarRenderer.hpp"

namespace Star {
  int runMainApplication(ApplicationUPtr application, StringList cmdLineArgs);
}

#if defined STAR_SYSTEM_WINDOWS

#include <windows.h>

#if defined STAR_USE_RPMALLOC

#include "rpmalloc.h"

#define STAR_MAIN_APPLICATION(ApplicationClass)                                          \
  int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {                              \
    ::rpmalloc_initialize();                                                             \
    int nArgs;                                                                           \
    LPWSTR* argsList = CommandLineToArgvW(GetCommandLineW(), &nArgs);                    \
    Star::StringList args;                                                               \
    for (int i = 0; i < nArgs; ++i) args.append(Star::String(argsList[i]));              \
    int returnVal Star::runMainApplication(Star::make_unique<ApplicationClass>(), args); \
    ::rpmalloc_finalize();                                                               \
    return returnVal;                                                                    \
  }


#else

#define STAR_MAIN_APPLICATION(ApplicationClass)                                   \
  int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {                       \
    int nArgs;                                                                    \
    LPWSTR* argsList = CommandLineToArgvW(GetCommandLineW(), &nArgs);             \
    Star::StringList args;                                                        \
    for (int i = 0; i < nArgs; ++i) args.append(Star::String(argsList[i]));       \
    return Star::runMainApplication(Star::make_unique<ApplicationClass>(), args); \
  }

#endif

#else

#if defined STAR_USE_RPMALLOC

#include "rpmalloc.h"

#define STAR_MAIN_APPLICATION(ApplicationClass)                                                                    \
  int main(int argc, char** argv) {                                                                                \
    ::rpmalloc_initialize();                                                                                       \
    int returnVal = Star::runMainApplication(Star::make_unique<ApplicationClass>(), Star::StringList(argc, argv)); \
    ::rpmalloc_finalize();                                                                                         \
    return returnVal;                                                                                              \
  }

#else

#define STAR_MAIN_APPLICATION(ApplicationClass)                                                           \
  int main(int argc, char** argv) {                                                                       \
    return Star::runMainApplication(Star::make_unique<ApplicationClass>(), Star::StringList(argc, argv)); \
  }

#endif

#endif

#endif
