cmake_minimum_required(VERSION 3.20)

project(FreeRoamRivals
    VERSION 0.0.4
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

set(NFSMW_SDK_INCLUDE_DIR  "${nfsmw_sdk_SOURCE_DIR}/include")
set(NFSMW_SDK_ENTRY_SOURCE "${nfsmw_sdk_SOURCE_DIR}/src/entry.c")
set(NFSMW_SDK_MINHOOK_DIR  "${nfsmw_sdk_SOURCE_DIR}/extern/minhook")
set(NFSMW_HOOKS_BACKEND "minhook")

set(FRR_DOMAIN_SOURCES
    src/domain/EncounterStateMachine.cpp
    src/domain/Progression.cpp
    src/domain/StakeRules.cpp
)

nfsmw_add_plugin(FreeRoamRivals
    SOURCES
        src/main.cpp
        src/core/Config.cpp
        src/core/Log.cpp
        src/core/VersionGuard.cpp
        src/game/GameBridge.cpp
        src/game/RuntimeProbe.cpp
        ${FRR_DOMAIN_SOURCES}
)

target_include_directories(FreeRoamRivals PRIVATE src)

target_compile_definitions(FreeRoamRivals PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(FreeRoamRivals PRIVATE advapi32)

if(MSVC)
    target_compile_options(FreeRoamRivals PRIVATE /W4)
else()
    target_compile_options(FreeRoamRivals PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(FreeRoamRivalsDomainTests
    tests/domain_tests.cpp
    ${FRR_DOMAIN_SOURCES}
)

target_include_directories(FreeRoamRivalsDomainTests PRIVATE src)

if(MSVC)
    target_compile_options(FreeRoamRivalsDomainTests PRIVATE /W4)
endif()

add_test(
    NAME FreeRoamRivalsDomainTests
    COMMAND FreeRoamRivalsDomainTests
)
