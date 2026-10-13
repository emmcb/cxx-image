# Compile-only regression checks: each probe produces optimizer diagnostics rather than an executable.
include("${CONFIG_FILE}")

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
separate_arguments(compiler_flags NATIVE_COMMAND "${COMPILER_FLAGS}")
set(arguments ${compiler_flags} ${COMPILE_OPTIONS} -std=c++17 -O3 -DNDEBUG -ffast-math)
list(FILTER INCLUDE_DIRECTORIES EXCLUDE REGEX "^$")
list(FILTER COMPILE_DEFINITIONS EXCLUDE REGEX "^$")
foreach(directory IN LISTS INCLUDE_DIRECTORIES)
    list(APPEND arguments "-I${directory}")
endforeach()
foreach(definition IN LISTS COMPILE_DEFINITIONS)
    list(APPEND arguments "-D${definition}")
endforeach()

# Require loop-vectorizer diagnostics; basic-block SLP remarks do not count as a vectorized loop.
if(COMPILER_ID STREQUAL "GNU")
    set(diagnostic_flags -fopt-info-vec-all)
    set(success_pattern "optimized: (epilogue )?loop vectorized")
    set(diagnostic_pattern "vectorized [0-9]+ loops in function")
else()
    set(diagnostic_flags -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize)
    set(success_pattern "remark: vectorized loop")
    set(diagnostic_pattern "\\[-Rpass[^]]*loop-vectorize\\]")
endif()

set(case_names channel-affine image-broadcast pixel-constants matrix-pixels whole-pixels opaque-callable)
string(JOIN " " displayed_flags ${arguments})
set(report "Compiler: ${COMPILER_ID} ${COMPILER_VERSION} (${COMPILER})\nFlags: ${displayed_flags}\n\n")
string(APPEND report "Case | Value | Vectorized loop | Requirement\n")
set(failures "")

foreach(value IN ITEMS float uint8_t)
    set(case_number 0)
    foreach(case IN LISTS case_names)
        math(EXPR case_number "${case_number} + 1")
        set(probe "${case}-${value}")

        # Floating-point paths must keep vectorizing; byte paths report improvements without enforcing scalar code.
        set(requirement "observe")
        if(case_number EQUAL 6)
            set(requirement "scalar")
        elseif(value STREQUAL "float")
            set(requirement "vectorized")
        endif()

        execute_process(
            COMMAND "${COMPILER}" ${arguments} ${diagnostic_flags} "-DCXXIMG_VECTORIZE_CASE=${case_number}"
                    "-DCXXIMG_VECTORIZE_VALUE=${value}" -c "${SOURCE_FILE}" -o "${OUTPUT_DIR}/${probe}.o"
            RESULT_VARIABLE result
            OUTPUT_VARIABLE stdout
            ERROR_VARIABLE diagnostics
        )
        file(WRITE "${OUTPUT_DIR}/${probe}.txt" "${stdout}${diagnostics}")
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Compilation failed for ${probe}:\n${stdout}${diagnostics}")
        endif()
        if(NOT diagnostics MATCHES "${diagnostic_pattern}")
            message(FATAL_ERROR "No recognized loop-vectorizer diagnostics for ${probe}:\n${diagnostics}")
        endif()

        set(vectorized "no")
        if(diagnostics MATCHES "${success_pattern}")
            set(vectorized "yes")
        endif()
        string(APPEND report "${case} | ${value} | ${vectorized} | ${requirement}\n")
        if(requirement STREQUAL "vectorized" AND vectorized STREQUAL "no")
            string(APPEND failures "${probe} no longer vectorizes.\n")
        elseif(requirement STREQUAL "scalar" AND vectorized STREQUAL "yes")
            string(APPEND failures "${probe} unexpectedly vectorizes an opaque call.\n")
        endif()
    endforeach()
endforeach()

# Keep a compact status table and the complete diagnostics for every case, including missed optimizations.
file(WRITE "${OUTPUT_DIR}/summary.txt" "${report}")
message("${report}\nDetailed diagnostics: ${OUTPUT_DIR}")
if(failures)
    message(FATAL_ERROR "${failures}")
endif()
