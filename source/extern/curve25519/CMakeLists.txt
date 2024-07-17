add_library(curve25519 STATIC
        include/curve25519_dh.h
        include/ed25519_signature.h
        include/external_calls.h
        source/base_folding8.h
        source/BaseTypes.h
        source/curve25519_dh.c
        source/curve25519_mehdi.c
        source/curve25519_mehdi.h
        source/curve25519_order.c
        source/curve25519_utils.c
        source/custom_blind.c
        source/custom_blind.h
        source/ed25519_sign.c
        source/ed25519_verify.c
        source/sha512.c
        source/sha512.h
)

target_include_directories(curve25519
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

