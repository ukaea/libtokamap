find_package( Git REQUIRED )
if( GIT_EXECUTABLE )
    set( GIT_VERSION "0.0.0" )
    set( PYTHON_PROJECT_VERSION "0.0.0" )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE _GIT_DESCRIBE_OUTPUT
        RESULT_VARIABLE _GIT_DESCRIBE_ERROR_CODE
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if( _GIT_DESCRIBE_OUTPUT AND NOT _GIT_DESCRIBE_ERROR_CODE )
        if( _GIT_DESCRIBE_OUTPUT MATCHES "([0-9]+).([0-9]+).([0-9]+)(-([0-9]+))?.*" )
            if( CMAKE_MATCH_4 )
                set( GIT_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.${CMAKE_MATCH_5}" )
                set( PYTHON_PROJECT_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.dev${CMAKE_MATCH_5}" )
            else()
                set( GIT_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" )
                set( PYTHON_PROJECT_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" )
            endif()
        endif()
    endif()
endif()
