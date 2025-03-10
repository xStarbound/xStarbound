if(STAR_ENABLE_DISCORD_INTEGRATION)
  add_subdirectory(discord)
endif()

if(STAR_ENABLE_STEAM_INTEGRATION)
  add_subdirectory(steam)
endif()

add_subdirectory(curve25519)
add_subdirectory(fmt)
add_subdirectory(lua)
add_subdirectory(tinyformat)
add_subdirectory(xxhash)

if(STAR_USE_RPMALLOC)
  add_subdirectory(rpmalloc)
endif()

add_library(star_extern INTERFACE)

# This target also links the (non-GUI) third-party libraries pulled in via find_package()
if(STAR_USE_RPMALLOC)
  target_link_libraries(star_extern
          INTERFACE
          curve25519
          fmt
          pluto
          tinyformat
          xxhash
          rpmalloc
          ${STAR_EXT_LIBS}
  )
else()
  target_link_libraries(star_extern
          INTERFACE
          curve25519
          fmt
          pluto
          tinyformat
          xxhash
          ${STAR_EXT_LIBS}
  )
endif()

add_library(Star::Extern ALIAS star_extern)
