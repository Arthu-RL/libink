include(CMakePackageConfigHelpers)

# Install static library binaries and header file sets directly. ink and
# ink_threading are two separate targets (see src/CMakeLists.txt for why);
# only ink carries the public_headers FILE_SET.
install(TARGETS ink
    EXPORT ink-targets
    FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS threading
    EXPORT ink-targets
)

# Generate config configuration templates in build tree
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ink-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/ink-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ink-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ink
)

# Install generated layout configurations for downstream system mapping
install(EXPORT ink-targets
    FILE ink-targets.cmake
    NAMESPACE ink::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ink
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ink-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ink-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ink
)