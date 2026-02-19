cmake_minimum_required( VERSION 3.15 )

# Extra cmake modules to find libraries
list(
    APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/Modules
)

include( GitVersion )

# Project details
project(
    LibTokaMap
    VERSION ${GIT_VERSION}
    LANGUAGES CXX
)

if( "${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}" )
    message( SEND_ERROR "In-source builds are not allowed." )
endif()

# Set project options
include( StaticAnalyzers )
include( Utils )

option( ENABLE_TESTING "Enable unit tests for the projects (from the `test` subfolder)." OFF )
option( ENABLE_EXAMPLES "Enable examples for the projects (from the `examples` subfolder)." OFF )
option( ENABLE_PROFILING "Compile with profiling enabled." OFF )

add_clang_format_target()

if( ENABLE_TESTING )
    enable_testing()
endif()

# Specify C++ standard for all targets
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# Export compile commands for tools like clang-tidy
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Group source files by functionality for better organization
set( UTIL_SOURCES
    src/utils/indices.cpp
    src/utils/library_loader.cpp
    src/utils/mapping_locator.cpp
    src/utils/profiler.cpp
    src/utils/ram_cache.cpp
    src/utils/render.cpp
    src/utils/subset.cpp
    src/utils/syntax_parser.cpp
    src/utils/typed_data_array.cpp
)
if( MSVC )
    list( APPEND UTIL_SOURCES src/utils/windows_utils.cpp )
else()
    list( APPEND UTIL_SOURCES src/utils/unix_utils.cpp )
endif()

set( MAP_TYPE_SOURCES
    src/map_types/custom_mapping.cpp
    src/map_types/data_source_mapping.cpp
    src/map_types/dim_mapping.cpp
    src/map_types/expr_mapping.cpp
    src/map_types/value_mapping.cpp
)

set( HANDLER_SOURCES
    src/handlers/mapping_handler.cpp
)

set( SOURCES
    ${UTIL_SOURCES}
    ${MAP_TYPE_SOURCES}
    ${HANDLER_SOURCES}
)

set( UTIL_HEADERS
    src/utils/algorithm.hpp
    src/utils/compiler.hpp
    src/utils/indices.hpp
    src/utils/library_loader.hpp
    src/utils/mapping_locator.hpp
    src/utils/os_utils.hpp
    src/utils/profiler.hpp
    src/utils/ram_cache.hpp
    src/utils/render.hpp
    src/utils/subset.hpp
    src/utils/syntax_parser.hpp
    src/utils/typed_data_array.hpp
    src/utils/types.hpp
)

set( MAP_TYPE_HEADERS
    src/map_types/base_mapping.hpp
    src/map_types/custom_mapping.hpp
    src/map_types/data_source_mapping.hpp
    src/map_types/dim_mapping.hpp
    src/map_types/expr_mapping.hpp
    src/map_types/map_arguments.hpp
    src/map_types/value_mapping.hpp
)

set( HANDLER_HEADERS
    src/handlers/mapping_handler.hpp
)

set( EXCEPTIONS_HEADERS
    src/exceptions/exceptions.hpp
)

set( HEADERS
    ${UTIL_HEADERS}
    ${MAP_TYPE_HEADERS}
    ${HANDLER_HEADERS}
    ${EXCEPTIONS_HEADERS}
)

set( EXT_HEADERS
    ext_include/nlohmann/json.hpp
    ext_include/valijson_nlohmann_bundled.hpp
    ext_include/exprtk/exprtk.hpp
    ext_include/inja/inja.hpp
)

if( "${PROJECT_VERSION_TWEAK}" STREQUAL "" )
    set( PROJECT_VERSION_TWEAK 0 )
endif()

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/include/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/include/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/config.hpp
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/python/setup.py.in
    ${CMAKE_CURRENT_LIST_DIR}/python/setup.py
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/python/pyproject.toml.in
    ${CMAKE_CURRENT_LIST_DIR}/python/pyproject.toml
    @ONLY
)

add_library( libtokamap ${SOURCES} )

if( MSVC )
    target_compile_options( libtokamap PRIVATE /W4 /WX /bigobj )
    target_link_libraries( libtokamap PRIVATE Dbghelp )
else()
    target_compile_options( libtokamap PRIVATE -Wall -Wextra -Wpedantic -Werror )
endif()

target_include_directories( libtokamap
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/ext_include
      ${CMAKE_CURRENT_BINARY_DIR}/include
    INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext_include>
      $<INSTALL_INTERFACE:include/libtokamap>
)
if( ENABLE_PROFILING )
    target_compile_definitions( libtokamap PRIVATE LIBTOKAMAP_PROFILE_ENABLED )
endif()
target_sources( libtokamap PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
    FILES ${HEADERS}
)
target_sources( libtokamap PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext_include
    FILES ${EXT_HEADERS}
)
set( PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/libtokamap.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/config.hpp
)
set_target_properties( libtokamap PROPERTIES
    OUTPUT_NAME tokamap
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

add_library( LibTokaMap::libtokamap ALIAS libtokamap )

include( GNUInstallDirs )

install( TARGETS libtokamap
    EXPORT tokamapTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include/libtokamap
    PUBLIC_HEADER DESTINATION include/libtokamap
    FILE_SET HEADERS DESTINATION include/libtokamap
)

install( EXPORT tokamapTargets
    FILE LibTokaMapTargets.cmake
    NAMESPACE LibTokaMap::
    DESTINATION lib/cmake/libtokamap
)

include( CMakePackageConfigHelpers )

configure_package_config_file(
    cmake/LibTokaMapConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LibTokaMapConfig.cmake
    INSTALL_DESTINATION lib/cmake/libtokamap
)

install( FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LibTokaMapConfig.cmake
    DESTINATION lib/cmake/libtokamap
)

# Install documentation
install( FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    DESTINATION "${CMAKE_INSTALL_DOCDIR}/libtokamap"
    RENAME copyright
)

install( FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    DESTINATION "${CMAKE_INSTALL_DOCDIR}/libtokamap"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
    COMMAND gzip -cn9 "${CMAKE_CURRENT_SOURCE_DIR}/changelog" > "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/changelog"
    COMMENT "Compressing changelog"
)

add_custom_target( changelog ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz" )

install( FILES
    "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}/libtokamap"
)

# Unit testing setup
if( ENABLE_TESTING )
    add_subdirectory( test )
endif()

if( ENABLE_EXAMPLES )
    add_subdirectory( examples/simple_mapper )
endif()

list( TRANSFORM SOURCES PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )
list( TRANSFORM HEADERS PREPEND ${CMAKE_CURRENT_LIST_DIR}/ )

set_property( TARGET clang-format APPEND PROPERTY SOURCES ${SOURCES} ${HEADERS} )

# Packaging setup
set( CPACK_PACKAGE_NAME "libtokamap" )
set( CPACK_PACKAGE_VENDOR "UKAEA" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library for flexible data mapping and transformation with multiple data sources" )
set( CPACK_PACKAGE_VERSION "${PROJECT_VERSION}" )
set( CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}" )
set( CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}" )
set( CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}" )
set( CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}" )
set( CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/ukaea/libtokamap" )
set( CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/packages" )
set( CPACK_PACKAGE_CONTACT "jonathan.hollocombe@ukaea.uk" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_INSTALL_DOCDIR}/LICENSE" )
set( CPACK_RESOURCE_FILE_README "${CMAKE_INSTALL_DOCDIR}/README.md" )

# DEB-specific settings
set( CPACK_DEBIAN_PACKAGE_NAME "libtokamap-dev" )
set( CPACK_DEBIAN_FILE_NAME DEB-DEFAULT )
set( CPACK_DEBIAN_PACKAGE_DEPENDS "" )
set( CPACK_DEBIAN_RUNTIME_PACKAGE_MAINTAINER "Jonathan Hollocombe <${CPACK_PACKAGE_CONTACT}>" )

# RPM-specific settings
set( CPACK_RPM_PACKAGE_NAME "libtokamap-devel" )
set( CPACK_RPM_FILE_NAME RPM-DEFAULT )
set( CPACK_RPM_PACKAGE_LICENSE "MIT" )
set( CPACK_RPM_DEBUGINFO_PACKAGE ON )

# Generators
if( UNIX AND NOT APPLE )
    set( CPACK_GENERATOR "DEB;RPM;TGZ" )
elseif( APPLE )
    set( CPACK_GENERATOR "TGZ;ZIP" )
else()
    set( CPACK_GENERATOR "ZIP" )
endif()

include( CPack )

# Print packaging configuration summary
message( STATUS "=== Packaging Configuration ===" )
message( STATUS "Package name: ${CPACK_PACKAGE_NAME}" )
message( STATUS "Version: ${CPACK_PACKAGE_VERSION}" )
message( STATUS "Generators: ${CPACK_GENERATOR}" )
message( STATUS "Output directory: ${CPACK_OUTPUT_FILE_PREFIX}" )
message( STATUS "===============================" )
