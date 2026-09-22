cmake_minimum_required(VERSION 3.20)
project(peregrine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)

add_executable(peregrine
    src/main.c
    src/tab_manager.c
    src/command_bar.c
    src/vim_bindings.c
)

target_include_directories(peregrine PRIVATE ${GTK4_INCLUDE_DIRS} ${WEBKIT_INCLUDE_DIRS})
target_link_directories(peregrine PRIVATE ${GTK4_LIBRARY_DIRS} ${WEBKIT_LIBRARY_DIRS})
target_link_libraries(peregrine PRIVATE ${GTK4_LIBRARIES} ${WEBKIT_LIBRARIES})
