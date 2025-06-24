cmake_minimum_required(VERSION 3.16)
project(danSOT CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Find necessary packages using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(CAIRO REQUIRED cairo)
find_package(Threads REQUIRED)

# Find the wayland-scanner executable, which generates C code from XML protocols
find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner REQUIRED)

# --- Protocol Code Generation ---
# This section automatically generates the necessary C headers and sources from the XML files.

# List the XML protocol files in your project directory
set(WAYLAND_PROTOCOL_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/overlay/protocols/wlr-layer-shell-unstable-v1.xml"
        #"${CMAKE_CURRENT_SOURCE_DIR}/overlay/protocols/ext-foreign-toplevel-list-v1.xml"
        "${CMAKE_CURRENT_SOURCE_DIR}/overlay/protocols/xdg-shell.xml"
)

# Directory to store the generated files
set(PROTOCOL_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen_protocols")
file(MAKE_DIRECTORY ${PROTOCOL_GENERATED_DIR})

# Loop over each protocol and create rules to generate the code
foreach(protocol_xml ${WAYLAND_PROTOCOL_FILES})
    get_filename_component(protocol_name ${protocol_xml} NAME_WE)

    set(client_header_file "${PROTOCOL_GENERATED_DIR}/${protocol_name}-client-protocol.h")
    set(private_code_file "${PROTOCOL_GENERATED_DIR}/${protocol_name}-protocol.c")

    # Rule to generate the private C source file
    add_custom_command(
            OUTPUT ${private_code_file}
            COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code < ${protocol_xml} > ${private_code_file}
            DEPENDS ${protocol_xml}
            COMMENT "Generating Wayland C source for ${protocol_name}"
    )

    # Rule to generate the client C header file
    add_custom_command(
            OUTPUT ${client_header_file}
            COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header < ${protocol_xml} > ${client_header_file}
            DEPENDS ${protocol_xml}
            COMMENT "Generating Wayland client header for ${protocol_name}"
    )

    # Add the generated files to lists that we will use later
    list(APPEND PROTOCOL_GENERATED_SOURCES ${private_code_file})
    list(APPEND PROTOCOL_GENERATED_HEADERS ${client_header_file})
endforeach()



#real project: danSOT
#cmake_minimum_required(VERSION 3.31)
#project(danSOT)
#
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(danSOT main.cpp
        overlay/Overlay.cpp
        overlay/Overlay.h
        memory/Memory.cpp
        memory/Memory.h
        utils/ProcessUtils.cpp
        utils/ProcessUtils.h
        utils/LocalStructs.h
        offsets.h
        utils/GameStructs.h

        ${PROTOCOL_GENERATED_SOURCES}
        overlay/overlay_wayland_c_bridge.h
        overlay/overlay_wayland_c_bridge.c)

target_sources(danSOT PRIVATE ${PROTOCOL_GENERATED_HEADERS})

#target_link_libraries(danSOT PRIVATE ${X11_LIBRARIES})
#target_link_libraries(danSOT PRIVATE Xrandr Xfixes)

#find_package(X11 REQUIRED)
#find_package(Threads REQUIRED)
#find_library(XFIXES_LIBRARIES NAMES Xfixes)
#find_library(XEXT_LIBRARIES NAMES Xext)

# Add the X11 include directory to the include path
target_include_directories(danSOT PRIVATE
        include
        ${CMAKE_CURRENT_SOURCE_DIR}
        #${X11_INCLUDE_DIR}

        ${WAYLAND_CLIENT_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${PROTOCOL_GENERATED_DIR}
)

# Link all the found libraries to your executable
target_link_libraries(danSOT PRIVATE
        ${WAYLAND_CLIENT_LIBRARIES}
        ${CAIRO_LIBRARIES}

#        ${X11_LIBRARIES}
#        ${X11_Xext_LIB}
#        ${X11_Xcomposite_LIB}
#        ${X11_Xfixes_LIB}
#        ${X11_XTest_LIB}
#        ${X11_Xshape_LIB}
        Threads::Threads
)

#sudo apt-get install libxext-dev
#sudo apt-get install libx11-dev libxext-dev libxcomposite-dev libxfixes-dev libxshape-dev
#sudo apt-get install libx11-dev xserver-xorg-dev xorg-dev