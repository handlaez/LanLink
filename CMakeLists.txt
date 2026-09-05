cmake_minimum_required(VERSION 3.20)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build types" FORCE)

project(LanLink LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt MOC/RCC build tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)
find_package(zfec CONFIG REQUIRED)
find_package(sdl2 CONFIG REQUIRED)

if(WIN32)
    # --- Windows FFmpeg (via vcpkg) ---
    find_path(FFMPEG_INCLUDE_DIR NAMES libavcodec/avcodec.h)
    find_library(AVCODEC_LIBRARY NAMES avcodec)
    find_library(AVFORMAT_LIBRARY NAMES avformat)
    find_library(AVUTIL_LIBRARY NAMES avutil)
    
    set(FFMPEG_LIBS ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY})
    
    # Windows system libraries
    set(PLATFORM_LIBS 
        d3d11
        dxgi
        d3dcompiler
        ws2_32
        mfplat
        mf
        mfreadwrite
        mfuuid
        SDL2::SDL2
    )
else()
    # --- Linux Dependencies (via pkg-config) ---
    find_package(PkgConfig REQUIRED)
    find_package(X11 REQUIRED)
    
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil)
    
    set(FFMPEG_LIBS PkgConfig::FFMPEG)
    set(PLATFORM_LIBS 
        SDL2::SDL2
        X11:X11
    )
endif()

# --- Common Sources ---
set(COMMON_SOURCES
    src/common/UnifiedPacketizer.cpp
    src/common/UnifiedDepacketizer.cpp
    src/application/Logger.cpp
    src/application/UnifiedArgParser.cpp
    src/common/UnifiedFecCodec.cpp
    src/common/UnifiedFecPacketizer.cpp
    src/common/UnifiedFecDepacketizer.cpp
    src/common/UnifiedFrameDecoder.cpp
    src/common/UnifiedFrameRenderer.cpp
    src/application/Consumer.cpp
)

set(COMMON_HEADERS
    src/common/UnifiedPacketizer.hpp
    src/common/UnifiedDepacketizer.hpp
    src/common/UDPFrameHeader.hpp
    src/common/EncodedFrame.hpp
    src/common/VideoFrame.hpp
    src/common/IPacketizer.hpp
    src/common/IDepacketizer.hpp
    src/common/IPacketReceiver.hpp
    src/common/IFrameDecoder.hpp
    src/common/IFrameRenderer.hpp
    src/application/Logger.hpp
    src/application/UnifiedArgParser.hpp
    src/common/UnifiedFecCodec.hpp
    src/common/UnifiedFecPacketizer.hpp
    src/common/UnifiedFecDepacketizer.hpp
    src/common/UnifiedFrameDecoder.hpp
    src/common/UnifiedFrameRenderer.hpp
    src/application/Consumer.hpp
)

# --- Platform Specific Configurations ---
if(WIN32)
    set(PLATFORM_SOURCES
        src/platform/windows/WinFrameGrabber.cpp
        src/platform/windows/WinFrameConverter.cpp
        src/platform/windows/WinFrameEncoder.cpp
        src/platform/windows/WinPacketSender.cpp
        src/application/Producer.cpp
        src/platform/windows/WinPacketReceiver.cpp
    )

    set(PLATFORM_HEADERS
        src/platform/windows/WinFrameGrabber.hpp
        src/platform/windows/WinFrameConverter.hpp
        src/platform/windows/WinFrameEncoder.hpp
        src/platform/windows/WinPacketSender.hpp
        src/application/Producer.hpp
        src/platform/windows/WinPacketReceiver.hpp
    )
else()
    set(PLATFORM_SOURCES
        src/platform/linux/LnxPacketReceiver.cpp
        src/platform/linux/LnxPacketSender.cpp
        src/platform/linux/LnxFrameConverter.cpp
        src/platform/linux/LnxFrameGrabber.cpp
        src/platform/linux/LnxFrameEncoder.cpp
    )

    set(PLATFORM_HEADERS
        src/platform/linux/LnxPacketReceiver.hpp
        src/platform/linux/LnxPacketSender.hpp
        src/platform/linux/LnxFrameConverter.hpp
        src/platform/linux/LnxFrameGrabber.hpp
        src/platform/linux/LnxFrame.hpp
        src/platform/linux/LnxFrameEncoder.hpp
    )
endif()

# --- Executable Setup ---
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
    ${PLATFORM_SOURCES}
    ${PLATFORM_HEADERS}
)

# Qt Quick Module
qt_add_qml_module(${PROJECT_NAME}
    URI LanLinkApp
    VERSION 1.0
    RESOURCE_PREFIX "qt/qml/"
    QML_FILES 
        Main.qml
    SOURCES
        src/application/AppController.hpp
        src/application/AppController.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
        ${CMAKE_CURRENT_SOURCE_DIR}/src/application
)

if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIR})
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        zfec::zfec
        ${FFMPEG_LIBS}
        ${PLATFORM_LIBS}
)

# Post Build

if(WIN32)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE)

    # Qt .dll copy
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env PATH="${Qt6_DIR}/../../../bin"
        windeployqt --qmldir ${CMAKE_CURRENT_SOURCE_DIR} $<TARGET_FILE:${PROJECT_NAME}>
    )
endif()