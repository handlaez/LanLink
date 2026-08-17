cmake_minimum_required(VERSION 3.20)

project(LanLink LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMON_SOURCES
    src/common/UnifiedPacketizer.cpp
    src/common/UnifiedDepacketizer.cpp
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
)

if(WIN32)

    set(PLATFORM_SOURCES
        src/platform/windows/WinFrameGrabber.cpp
        src/platform/windows/WinFrameConverter.cpp
        src/platform/windows/WinFrameEncoder.cpp
        src/platform/windows/WinPacketSender.cpp
        src/application/Producer.cpp
    )

    set(PLATFORM_HEADERS
        src/platform/windows/WinFrameGrabber.hpp
        src/platform/windows/WinFrameConverter.hpp
        src/platform/windows/WinFrameEncoder.hpp
        src/platform/windows/WinPacketSender.hpp
        src/application/Producer.hpp
    )

    add_executable(${PROJECT_NAME}
        src/main.cpp
        ${COMMON_SOURCES}
        ${COMMON_HEADERS}
        ${PLATFORM_SOURCES}
        ${PLATFORM_HEADERS}
    )

    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )

    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            d3d11
            dxgi
            d3dcompiler
            ws2_32
            mfplat
            mf
            mfreadwrite
            mfuuid
    )

else()

    set(PLATFORM_SOURCES
        src/platform/linux/LnxPacketReceiver.cpp
        src/platform/linux/LnxFrameDecoder.cpp
        src/platform/linux/LnxFrameRenderer.cpp
        src/application/Consumer.cpp
    )

    set(PLATFORM_HEADERS
        src/platform/linux/LnxPacketReceiver.hpp
        src/platform/linux/LnxFrameDecoder.hpp
        src/platform/linux/LnxFrameRenderer.hpp
        src/application/Consumer.hpp
    )

    find_package(PkgConfig REQUIRED)

    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavutil
    )

    add_executable(${PROJECT_NAME}
        src/main.cpp
        ${COMMON_SOURCES}
        ${COMMON_HEADERS}
        ${PLATFORM_SOURCES}
        ${PLATFORM_HEADERS}
    )

    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            PkgConfig::SDL2
            PkgConfig::FFMPEG
    )

endif()