# Optional embedded-Python data source bridge.
#
# Builds two libraries:
#   tokamap_python_bridge       the data source factory loaded via `data_source_factories`. Does not depend on Python.
#   tokamap_python_bridge_impl  the Python-dependent implementation, loaded by the shim once libpython is visible.
#                               Built like a Python extension module, i.e. not linked against libpython.

if( WIN32 )
  message( FATAL_ERROR "LIBTOKAMAP_BUILD_PYTHON_BRIDGE is not yet supported on Windows" )
endif()

find_package( Python3 3.11 REQUIRED COMPONENTS Interpreter Development.Module )

# Locate the shared libpython the shim loads into C++ host processes by default
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c [=[
import os, sysconfig
v = sysconfig.get_config_var
if v("PYTHONFRAMEWORK"):
    candidates = [os.path.join(v("PYTHONFRAMEWORKPREFIX"), v("LDLIBRARY"))]
else:
    # Not gated on Py_ENABLE_SHARED: e.g. Debian builds python3 with libpython built in, but still ships libpython.so
    name = "libpython" + v("LDVERSION")
    dirs = [v("LIBDIR"), os.path.join(v("LIBDIR") or "", v("MULTIARCH") or ""), v("LIBPL")]
    candidates = [os.path.join(d, name + suffix) for d in dirs if d for suffix in (".so.1.0", ".so", ".dylib")]
print(next((c for c in candidates if os.path.isfile(c)), ""), end="")
]=]
    OUTPUT_VARIABLE LIBTOKAMAP_DETECTED_PYTHON_LIBRARY
    RESULT_VARIABLE _python_library_result
)
if( NOT _python_library_result EQUAL 0 )
  set( LIBTOKAMAP_DETECTED_PYTHON_LIBRARY "" )
endif()
set( LIBTOKAMAP_PYTHON_LIBRARY "${LIBTOKAMAP_DETECTED_PYTHON_LIBRARY}" CACHE FILEPATH
     "Shared libpython loaded by the Python bridge in C++ host processes (overridable at runtime with LIBTOKAMAP_PYTHON_LIBRARY)" )
if( LIBTOKAMAP_PYTHON_LIBRARY STREQUAL "" )
  message( WARNING "No shared libpython found for ${Python3_EXECUTABLE}. C++ applications using the Python bridge "
                   "must set LIBTOKAMAP_PYTHON_LIBRARY at runtime." )
endif()

message( STATUS "Python bridge: Python ${Python3_VERSION} (${Python3_EXECUTABLE})" )
message( STATUS "Python bridge: default libpython '${LIBTOKAMAP_PYTHON_LIBRARY}'" )

## Implementation library

add_library( tokamap_python_bridge_impl SHARED
    src/common/python_conversions.cpp
    src/common/python_utils.cpp
    src/impl/python_bridge_impl.cpp
    src/impl/python_data_source.cpp
    src/impl/python_runtime.cpp
)
target_include_directories( tokamap_python_bridge_impl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Python3_INCLUDE_DIRS}
)
target_link_libraries( tokamap_python_bridge_impl PRIVATE LibTokaMap::libtokamap )
if( APPLE )
  # Resolve Python symbols from the process at load time, as extension modules do
  target_link_options( tokamap_python_bridge_impl PRIVATE "LINKER:-undefined,dynamic_lookup" )
endif()

## Shim library (the configured data source factory)

add_library( tokamap_python_bridge SHARED
    src/shim/python_bridge_shim.cpp
)
target_link_libraries( tokamap_python_bridge PRIVATE LibTokaMap::libtokamap ${CMAKE_DL_LIBS} )
target_compile_definitions( tokamap_python_bridge PRIVATE
    LIBTOKAMAP_PYTHON_VERSION="${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}"
    LIBTOKAMAP_DEFAULT_PYTHON_LIBRARY="${LIBTOKAMAP_PYTHON_LIBRARY}"
    LIBTOKAMAP_PYTHON_BRIDGE_IMPL_NAME="$<TARGET_FILE_NAME:tokamap_python_bridge_impl>"
)
add_dependencies( tokamap_python_bridge tokamap_python_bridge_impl )

foreach( _target tokamap_python_bridge tokamap_python_bridge_impl )
  if( MSVC )
    target_compile_options( ${_target} PRIVATE /W4 /WX )
  else()
    target_compile_options( ${_target} PRIVATE -Wall -Wextra -Wpedantic -Werror )
  endif()
endforeach()

set_target_properties( tokamap_python_bridge PROPERTIES EXPORT_NAME python_bridge )
add_library( LibTokaMap::python_bridge ALIAS tokamap_python_bridge )

## Install: both libraries must be installed to the same directory

install( TARGETS tokamap_python_bridge
    EXPORT tokamapTargets
    LIBRARY DESTINATION lib
)
install( TARGETS tokamap_python_bridge_impl
    LIBRARY DESTINATION lib
)

if( ENABLE_TESTING )
  add_subdirectory( test )
endif()
