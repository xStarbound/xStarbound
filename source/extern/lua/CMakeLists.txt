# libsoup
add_subdirectory(vendor/Soup)

add_library(pluto
  lapi.h
  lauxlib.h
  lcode.h
  lctype.h
  ldebug.h
  ldo.h
  lfunc.h
  lgc.h
  ljumptabgcc.h
  ljumptab.h
  llex.h
  llimits.h
  lmem.h
  lobject.h
  lopcodes.h
  lopnames.h
  lparser.h
  lprefix.h
  lstate.h
  lstring.h
  ltable.h
  ltm.h
  luaconf.h
  lua.h
  lualib.h
  lundump.h
  lvm.h
  lzio.h
  lapi.cpp
  lassertlib.cpp
  lauxlib.cpp
  lbase32.cpp
  lbase64.cpp
  lbaselib.cpp
  lbigint.cpp
  lbufferlib.cpp
  lcanvas.cpp
  lcatlib.cpp
  lcode.cpp
  lcorolib.cpp
  lcryptolib.cpp
  lctype.cpp
  ldblib.cpp
  ldebug.cpp
  ldo.cpp
  ldump.cpp
  lffi.cpp
  lfunc.cpp
  lgc.cpp
  lhttplib.cpp
  linit.cpp
  liolib.cpp
  ljson.cpp
  llex.cpp
  lmathlib.cpp
  lmem.cpp
  loadlib.cpp
  lobject.cpp
  lopcodes.cpp
  loslib.cpp
  lparser.cpp
  lregex.cpp
  lschedulerlib.cpp
  lsocketlib.cpp
  lstarlib.cpp
  lstate.cpp
  lstring.cpp
  lstrlib.cpp
  ltable.cpp
  ltablib.cpp
  ltm.cpp
  luac.cpp
  lua.cpp
  lundump.cpp
  lurllib.cpp
  lutf8lib.cpp
  lvector3lib.cpp
  lvm.cpp
  lxml.cpp
  lzio.cpp
)

target_include_directories(pluto
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pluto
        PUBLIC
        soup
)
