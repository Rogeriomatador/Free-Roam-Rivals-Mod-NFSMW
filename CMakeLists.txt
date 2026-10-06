cmake_minimum_required(VERSION 3.20)

project(FreeRoamRivals
    VERSION 0.0.1
    LANGUAGES C CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "Free Roam Rivals targets the Win32 NFSMW executable.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "NFSMW is 32-bit. Configure with -A Win32.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(NFSMW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nfsmw_sdk
    GIT_REPOSITORY https://github.com/s-b-repo/nfsmw-2005-sdk.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(nfsmw_sdk)

nfsmw_add_plugin(FreeRoamRivals
    SOURCES
        src/main.cpp
)

target_compile_definitions(FreeRoamRivals PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(FreeRoamRivals PRIVATE /W4)
else()
    target_compile_options(FreeRoamRivals PRIVATE -Wall -Wextra -Wpedantic)
endif()
