cmake_minimum_required(VERSION 3.16)

# set the project name
project(tiny-telnet)

# lots of warnings and all warnings as errors
# add_compile_options(-Wall -Wextra )

# define location for header files
add_library(tiny-telnet INTERFACE)
target_include_directories(tiny-telnet INTERFACE src)


add_subdirectory("test")