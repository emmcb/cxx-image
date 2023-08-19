add_compile_options("$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall;-Wextra;-Werror>")

add_subdirectory(convert-image)
