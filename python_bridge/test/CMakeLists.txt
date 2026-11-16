execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import numpy"
    RESULT_VARIABLE _numpy_result
    OUTPUT_QUIET ERROR_QUIET
)
if( NOT _numpy_result EQUAL 0 )
  message( WARNING "NumPy is not installed for ${Python3_EXECUTABLE}; Python bridge tests are disabled" )
  return()
endif()

include( FetchContent )
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)
FetchContent_MakeAvailable( Catch2 )

set( BRIDGE_TEST_PYTHONPATH ${CMAKE_CURRENT_SOURCE_DIR}/python )
set( BRIDGE_TEST_ENVIRONMENT
    PYTHONPATH=${BRIDGE_TEST_PYTHONPATH}
    LIBTOKAMAP_PYTHON_EXECUTABLE=${Python3_EXECUTABLE}
)

# Deliberately linked only to core LibTokaMap: the bridge, Python and NumPy are loaded at runtime
add_executable( python_bridge_tests src/python_bridge_test.cpp )
target_link_libraries( python_bridge_tests PRIVATE Catch2::Catch2WithMain LibTokaMap::libtokamap )
target_compile_definitions( python_bridge_tests PRIVATE
    LIBTOKAMAP_TEST_BRIDGE_PATH="$<TARGET_FILE:tokamap_python_bridge>"
    LIBTOKAMAP_TEST_MAPPINGS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/mappings"
    LIBTOKAMAP_TEST_SCHEMAS_DIR="${PROJECT_SOURCE_DIR}/examples/simple_mapper/schemas"
)
add_dependencies( python_bridge_tests tokamap_python_bridge )

# Run as one process, so all test cases share one embedded interpreter as they would in an application
add_test( NAME python_bridge_tests COMMAND python_bridge_tests )
set_tests_properties( python_bridge_tests PROPERTIES ENVIRONMENT "${BRIDGE_TEST_ENVIRONMENT}" )

# Python-hosted process loading the bridge through LibTokaMap
add_library( python_bridge_attach_driver SHARED src/attach_driver.cpp )
target_link_libraries( python_bridge_attach_driver PRIVATE LibTokaMap::libtokamap )
add_dependencies( python_bridge_attach_driver tokamap_python_bridge )

add_test(
    NAME python_bridge_attach_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/test_attach.py
            $<TARGET_FILE:python_bridge_attach_driver> $<TARGET_FILE:tokamap_python_bridge>
)
set_tests_properties( python_bridge_attach_test PROPERTIES ENVIRONMENT "PYTHONPATH=${BRIDGE_TEST_PYTHONPATH}" )
