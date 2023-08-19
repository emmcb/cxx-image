add_compile_options("$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall;-Wextra;-Werror>")

add_subdirectory(image)
add_subdirectory(io)
add_subdirectory(math)
add_subdirectory(model)
add_subdirectory(parser)
add_subdirectory(util)
