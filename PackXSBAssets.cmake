if(PACKAGE_XSB_ASSETS)
    # Check if we can run our built executables on the host to package assets
    include(CheckCXXSourceRuns)
    check_cxx_source_runs("int main() { return 0; }" CAN_RUN_BINARIES_ON_HOST)
    if(NOT CAN_RUN_BINARIES_ON_HOST)
        message(FATAL_ERROR "Packaging assets has been requested, but cannot run produced binaries on the host (most probably due to cross-compilation).\n"
                "Please disable PACKAGE_XSB_ASSETS and package xSBassets manually after the build with a host-runnable asset_packer executable.")
    endif()

    file(GLOB_RECURSE XSB_ASSET_FILES LIST_DIRECTORIES FALSE *)
    file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/xsb-assets")

    add_custom_command(OUTPUT "${PROJECT_SOURCE_DIR}/xsb-assets/packed.pak"
            COMMAND asset_packer ARGS -c "${PROJECT_SOURCE_DIR}/scripts/packing.config" "${PROJECT_SOURCE_DIR}/assets/xSBassets" "${PROJECT_SOURCE_DIR}/xsb-assets/packed.pak"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            DEPENDS asset_packer
    )

    add_custom_target(package_xsb_assets ALL
            DEPENDS "${PROJECT_SOURCE_DIR}/xsb-assets/packed.pak"
    )
endif()

# Add install script even if the packaging isn't done at build time. The packed.pak
# may be created outside the build before installing.
install(FILES ${PROJECT_SOURCE_DIR}/xsb-assets/packed.pak
        DESTINATION ${STAR_INSTALL_DATADIR}/xsb-assets/
        COMPONENT Assets
        OPTIONAL # Silently ignore if the asset pack is missing.
)
