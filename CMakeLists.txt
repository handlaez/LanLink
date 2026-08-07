cmake_minimum_required(VERSION 3.20)
project(LanLink LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# src
set(SOURCES
    src/main.cpp
    src/WinDuplicationGrabber.cpp "src/stb_image_implementation.cpp")

set(HEADERS
    src/IFrameGrabber.hpp
    src/IVideoEncoder.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# native WinSDK system libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
    ws2_32.lib
)

# make it lean
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