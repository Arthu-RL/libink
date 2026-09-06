include(CMakePackageConfigHelpers)

# One shared prefix for every platform: headers install once (identical across
# targets), the library carries its ABI tag, and each build drops its own
# targets file beside a config that selects the caller's.
install(TARGETS ink
    EXPORT ink-targets
    FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ARCH_INDEPENDENT: one version file serves every platform here, and the default
# stamps the building machine's word size into it -- a wasm32 install would then
# be rejected by a 64-bit consumer. ABI matching is the targets suffix's job.
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ink-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/ink-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ink-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ink
)

# Install generated layout configurations for downstream system mapping
install(EXPORT ink-targets
    FILE ink-targets-${INK_PLATFORM_SUFFIX}.cmake
    NAMESPACE ink::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ink
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ink-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ink-config-version.cmake"
    "${PROJECT_SOURCE_DIR}/cmake/PlatformSuffix.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ink
)