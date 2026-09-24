cmake_minimum_required(VERSION 3.21)

project(WhatTheGroundKeeps VERSION 2.0 LANGUAGES C CXX)
if(WIN32)
    enable_language(RC)
endif()

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_EXAMPLES OFF CACHE BOOL "Disable Raylib examples" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "Disable Raylib games" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable Raylib tests" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Disable GLFW Wayland backend" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE WTGK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(WIN32)
    list(APPEND WTGK_SOURCES src/resources/WhatTheGroundKeeps.rc)
endif()

add_executable(WhatTheGroundKeeps ${WTGK_SOURCES})

target_include_directories(WhatTheGroundKeeps PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(WhatTheGroundKeeps PRIVATE cxx_std_17)
target_link_libraries(WhatTheGroundKeeps PRIVATE raylib)

find_package(Threads REQUIRED)
target_link_libraries(WhatTheGroundKeeps PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(WhatTheGroundKeeps PRIVATE /W4 /wd4576 /wd4244 /wd4305)
else()
    target_compile_options(WhatTheGroundKeeps PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
endif()

add_custom_command(TARGET WhatTheGroundKeeps POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:WhatTheGroundKeeps>/assets"
    COMMENT "Copying runtime assets"
)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT WhatTheGroundKeeps)
