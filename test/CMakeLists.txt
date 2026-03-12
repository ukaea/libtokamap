# cmake_minimum_required( VERSION 3.15 )

# Project details
# project(
#     libtokamap_tests
#     LANGUAGES CXX
# )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( TEST_SOURCES
    src/data_source_mapping_test.cpp
    src/dim_mapping_test.cpp
    src/indices_test.cpp
    src/mapping_locator_test.cpp
    src/parse_slices_test.cpp
    src/render_test.cpp
    src/subset_test.cpp
    # src/subset_edge_cases_test.cpp
    src/syntax_parser_test.cpp
    src/typed_data_array_test.cpp
    src/value_mapping_test.cpp
)

include( FetchContent )
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)
FetchContent_MakeAvailable( Catch2 )

# if( CMAKE_VERSION VERSION_GREATER_EQUAL 3.20 )
#     include( CodeCoverage )
#     append_coverage_compiler_flags()
# endif()

# Create test executable
add_executable( libtokamap_tests ${TEST_SOURCES} )

# include( Utils )
# enable_coverage( libtokamap_tests )

# INCLUDE
target_include_directories( libtokamap_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# LIBS
target_link_libraries( libtokamap_tests
    PRIVATE
    Catch2::Catch2WithMain
    LibTokaMap::libtokamap
)

if( ENABLE_COVERAGE )
    include( Coverage )
    add_llvm_coverage( libtokamap_tests ${CMAKE_CURRENT_BINARY_DIR}/libtokamap_tests )
    # target_compile_options( libtokamap_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping )
    # target_link_options( libtokamap_tests PRIVATE -fprofile-instr-generate )
endif()

# CTest integration
list( APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras )

include( Catch )
catch_discover_tests( libtokamap_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

# if( CMAKE_VERSION VERSION_GREATER_EQUAL 3.20 )
#     setup_target_for_coverage_lcov(
#         NAME coverage
#         EXECUTABLE libtokamap_tests
#         DEPENDENCIES libtokamap_tests
#     )
# endif()
