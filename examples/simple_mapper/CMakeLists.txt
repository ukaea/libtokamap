cmake_minimum_required( VERSION 3.15 )

project( simple_mapper )

set( SOURCES
    src/simple_mapper.cpp
)

add_executable( simple_mapper ${SOURCES} )
target_link_libraries( simple_mapper PRIVATE LibTokaMap::libtokamap )
target_include_directories( simple_mapper PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )

add_library( custom_library SHARED custom_library/custom_library.cpp )
target_link_libraries( custom_library PRIVATE LibTokaMap::libtokamap )

add_library( json_data_source SHARED data_source_library/json_data_source.cpp )
target_link_libraries( json_data_source PRIVATE LibTokaMap::libtokamap )

set( CONFIG_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

configure_file( src/config_path.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config_path.hpp @ONLY )
configure_file( config.toml.in ${CMAKE_CURRENT_BINARY_DIR}/config.toml @ONLY )
