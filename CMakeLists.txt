cmake_minimum_required( VERSION 3.15 )

# Project details
project(
    LibTokaMap
    VERSION 0.1.1
    LANGUAGES CXX
)

if( "${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}" )
    message( SEND_ERROR "In-source builds are not allowed." )
endif()

# Extra cmake modules to find libraries
list(
    APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/Modules
)

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
    ${CMAKE_CURRENT_LIST_DIR}/setup.py.in
    ${CMAKE_CURRENT_LIST_DIR}/python/setup.py
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/pyproject.toml.in
    ${CMAKE_CURRENT_LIST_DIR}/python/pyproject.toml
    @ONLY
)

add_library( libtokamap ${SOURCES} )
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
target_compile_options( libtokamap PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall -Werror -Wpedantic>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror -Wpedantic>
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
