# Minimum required version of CMake
cmake_minimum_required(VERSION 3.16)

# Project name and languages
project(DanSOT CXX C)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# --- Find Packages ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(GTK_LAYER_SHELL REQUIRED gtk4-layer-shell-0)
find_package(Threads REQUIRED)

# --- Add global include directory for the "common" folder ---
# This allows #include "common/shared_data.h" from any file
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# --- Cheat Executable Target ("danSOT") ---
# This target ONLY includes main.cpp
add_executable(danSOT
        main.cpp
        utils/ProcessUtils.cpp
        memory/Memory.cpp
        overlay/drawing.h
        utils/tables.h
        hacks/Aimbot.cpp
        hacks/Aimbot.h)

target_link_libraries(danSOT PRIVATE
        # Link rt for shm_open / shm_unlink on some systems
        rt
        Threads::Threads
)

# --- Overlay Executable Target ("OverlayApp") ---
# This target includes the overlay code
add_executable(OverlayApp
        overlay/overlay_main.cpp
        overlay/Overlay.cpp
)

# Tell the compiler where to find GTK header files for the Overlay
target_include_directories(OverlayApp PRIVATE
        ${GTK4_INCLUDE_DIRS}
        ${GTK_LAYER_SHELL_INCLUDE_DIRS}
)

# Tell the linker which libraries to link for the Overlay
target_link_libraries(OverlayApp PRIVATE
        ${GTK4_LIBRARIES}
        ${GTK_LAYER_SHELL_LIBRARIES}
        Threads::Threads
)

# A little message for the user
message(STATUS "Project configured. Use 'make' to build targets: CheatApp and OverlayApp")