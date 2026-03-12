function(add_llvm_coverage TARGET TEST_EXECUTABLE)
    # Add coverage flags
    target_compile_options(${TARGET} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(${TARGET} PRIVATE -fprofile-instr-generate)

    # Detect whether we need xcrun (macOS)
    if(APPLE)
        find_program(XCRUN xcrun)
        if(XCRUN)
            set(LLVM_COV "${XCRUN}" llvm-cov)
            set(LLVM_PROFDATA "${XCRUN}" llvm-profdata)
        else()
            message(WARNING "xcrun not found; falling back to llvm-cov directly")
            set(LLVM_COV llvm-cov)
            set(LLVM_PROFDATA llvm-profdata)
        endif()
    else()
        set(LLVM_COV llvm-cov)
        set(LLVM_PROFDATA llvm-profdata)
    endif()

    # Regex to exclude Catch2 headers (v2 and v3)
    # Matches:
    #   .../catch2/*
    #   .../Catch2/*
    set(CATCH2_EXCLUDE_REGEX ".*[Cc]atch2/.*")
    set(TEST_EXCLUDE_REGEX ".*test/.*")
    set(EXT_EXCLUDE_REGEX ".*ext_include/.*")

    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests with coverage instrumentation..."
        COMMAND LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage.profraw ${TEST_EXECUTABLE}

        COMMAND ${CMAKE_COMMAND} -E echo "Merging raw profile data..."
        COMMAND ${LLVM_PROFDATA} merge -sparse
                ${CMAKE_BINARY_DIR}/coverage.profraw
                -o ${CMAKE_BINARY_DIR}/coverage.profdata

        COMMAND ${CMAKE_COMMAND} -E echo "Generating text coverage report..."
        COMMAND ${LLVM_COV} show ${TEST_EXECUTABLE}
                -instr-profile=${CMAKE_BINARY_DIR}/coverage.profdata
                --ignore-filename-regex=${CATCH2_EXCLUDE_REGEX}
                --ignore-filename-regex=${TEST_EXCLUDE_REGEX}
                --ignore-filename-regex=${EXT_EXCLUDE_REGEX}
                > ${CMAKE_BINARY_DIR}/coverage.txt

        COMMAND ${CMAKE_COMMAND} -E echo "Generating HTML coverage report..."
        COMMAND ${LLVM_COV} show ${TEST_EXECUTABLE}
                -instr-profile=${CMAKE_BINARY_DIR}/coverage.profdata
                --ignore-filename-regex=${CATCH2_EXCLUDE_REGEX}
                --ignore-filename-regex=${TEST_EXCLUDE_REGEX}
                --ignore-filename-regex=${EXT_EXCLUDE_REGEX}
                -format=html
                -output-dir=${CMAKE_BINARY_DIR}/coverage_html

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${TARGET}
        COMMENT "Running LLVM code coverage"
    )
endfunction()
