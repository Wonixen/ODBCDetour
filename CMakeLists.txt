cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_STANDARD 23)


# set the project name and version
project(OdbcDetour VERSION 0.1.0.0)

#option (CMAKE_COMPILE_WARNING_AS_ERROR "Warnings as error!" ON)


# global option, always assume source code is utf-8
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# setup some defaults: warning level 3, utf-8 for source code
add_library(JadaOdbc_compiler_flags INTERFACE)
target_compile_features(JadaOdbc_compiler_flags INTERFACE cxx_std_23)

# add compiler warning flags just when building this project via
# the BUILD_INTERFACE genex
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(JadaOdbc_compiler_flags INTERFACE   "$<${msvc_cxx}:$<BUILD_INTERFACE:-W4>>")

add_subdirectory(src)

