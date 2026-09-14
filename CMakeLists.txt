cmake_minimum_required(VERSION 3.21)

project(WhatTheGroundKeeps VERSION 1.0 LANGUAGES C CXX RC)

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

set(WTGK_SOURCES
    src/main.cpp
    src/audio/audio_synthesis.cpp
    src/systems/shop_atmosphere.cpp
    src/systems/ui_helpers.cpp
    src/systems/texture_factories.cpp
    src/systems/receipt_printer.cpp
    src/systems/footprint_decals.cpp
    src/systems/gas_station_system.cpp
    src/systems/crashed_sedan_site.cpp
    src/systems/hound_npc.cpp
    src/systems/skeleton_cow_npc.cpp
    src/systems/shovel_system.cpp
    src/systems/blackwood_college.cpp
    src/systems/shop_lighting.cpp
    src/systems/atmospheric_particles.cpp
    src/systems/procedural_shop_assets.cpp
)

if(WIN32)
    list(APPEND WTGK_SOURCES src/resources/WhatTheGroundKeeps.rc)
endif()

add_executable(WhatTheGroundKeeps ${WTGK_SOURCES})

target_include_directories(WhatTheGroundKeeps PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/audio"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/systems"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/resources"
)

target_compile_features(WhatTheGroundKeeps PRIVATE cxx_std_17)
target_link_libraries(WhatTheGroundKeeps PRIVATE raylib)

if(MSVC)
    target_compile_options(WhatTheGroundKeeps PRIVATE /W4 /wd4576)
else()
    target_compile_options(WhatTheGroundKeeps PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET WhatTheGroundKeeps POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:WhatTheGroundKeeps>/assets"
    COMMENT "Copying runtime assets"
)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT WhatTheGroundKeeps)