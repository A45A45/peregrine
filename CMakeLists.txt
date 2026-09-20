cmake_minimum_required(VERSION 3.20)
project(peregrine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(WEBKIT REQUIRED IMPORTED_TARGET webkitgtk-6.0)

add_executable(peregrine src/main.c)

target_link_libraries(peregrine PRIVATE PkgConfig::GTK4 PkgConfig::WEBKIT)
