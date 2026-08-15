cmake_minimum_required(VERSION 3.20)
project(LanLink LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

set(SOURCES
    src/main.cpp
    src/WinFrameGrabber.cpp
    src/stb_image_implementation.cpp
    src/WinFrameConverter.cpp
    src/UnifiedPacketizer.cpp
    src/WinFrameEncoder.cpp
    src/WinPacketSender.cpp 
    src/LnxPacketReceiver.cpp
    src/UnifiedDepacketizer.cpp
    src/LnxFrameDecoder.cpp
)

set(HEADERS
    src/VideoFrame.hpp
    src/EncodedFrame.hpp
    src/IFrameGrabber.hpp
    src/IFrameConverter.hpp
    src/IFrameEncoder.hpp
    src/IPacketizer.hpp
    src/IPacketSender.hpp
    src/IPacketReceiver.hpp
    src/IFrameDecoder.hpp
    src/IFrameRenderer.hpp
    src/IDepacketizer.hpp
    src/WinFrameConverter.hpp
    src/WinFrameGrabber.hpp
    src/WinPacketSender.hpp
    src/LnxPacketReceiver.hpp
    src/LnxFrameDecoder.hpp
    src/UnifiedPacketizer.hpp
    src/UnifiedDepacketizer.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

if(WIN32)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        d3d11
        dxgi
        d3dcompiler
        ws2_32

        mfplat
        mf
        mfreadwrite
        mfuuid
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )

    # copy shader file next to compiled executable
    # add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #         "${CMAKE_CURRENT_SOURCE_DIR}/shaders/NV12Converter.hlsl"
    #         "$<TARGET_FILE_DIR:${PROJECT_NAME}>/NV12Converter.hlsl"
    # )

endif()