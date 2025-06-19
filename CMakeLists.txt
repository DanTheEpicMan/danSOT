cmake_minimum_required(VERSION 3.31)
project(danSOT)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(danSOT main.cpp
        overlay/Overlay.cpp
        overlay/Overlay.h
        memory/Memory.cpp
        memory/Memory.h
        utils/ProcessUtils.cpp
        utils/ProcessUtils.h
        utils/LocalStructs.h
        offsets.h
        utils/GameStructs.h)

find_package(X11 REQUIRED)
find_package(Threads REQUIRED)
find_library(XFIXES_LIBRARIES NAMES Xfixes)
find_library(XEXT_LIBRARIES NAMES Xext)

# Add the X11 include directory to the include path
target_include_directories(danSOT PRIVATE
        include
        ${X11_INCLUDE_DIR})

# Link all the found libraries to your executable
target_link_libraries(danSOT PRIVATE
        ${X11_LIBRARIES}
        ${X11_Xext_LIB}
        ${X11_Xcomposite_LIB}
        ${X11_Xfixes_LIB}
        ${X11_XTest_LIB}
        ${X11_Xshape_LIB}
        Threads::Threads
)

#sudo apt-get install libxext-dev
#sudo apt-get install libx11-dev libxext-dev libxcomposite-dev libxfixes-dev libxshape-dev
#sudo apt-get install libx11-dev xserver-xorg-dev xorg-dev