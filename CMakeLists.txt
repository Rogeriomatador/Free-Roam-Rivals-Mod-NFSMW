cmake_minimum_required(VERSION 3.20)

project(FreeRoamRivals
    VERSION 0.0.25
    LANGUAGES C CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "Free Roam Rivals targets the Win32 NFSMW executable.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "NFSMW is 32-bit. Configure with -A Win32.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(NFSMW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nfsmw_sdk
    GIT_REPOSITORY https://github.com/s-b-repo/nfsmw-2005-sdk.git
    GIT_TAG 3b3d05b9194844883aa42d75dd7aae66838092ff
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(nfsmw_sdk)

FetchContent_Declare(
    mwsdk
    GIT_REPOSITORY https://github.com/TsyVM/MWSDK.git
    GIT_TAG 6db158647fe05a3d1cbbfee38f0f5d1e91b2f473
    GIT_SHALLOW FALSE
)
FetchContent_GetProperties(mwsdk)
if(NOT mwsdk_POPULATED)
    FetchContent_Populate(mwsdk)
endif()

FetchContent_Declare(
    nfsplugin_sdk
    GIT_REPOSITORY https://github.com/berkayylmao/NFSPluginSDK.git
    GIT_TAG d238bdffc648498840d17133edb256fa0310d355
    GIT_SHALLOW FALSE
)
FetchContent_GetProperties(nfsplugin_sdk)
if(NOT nfsplugin_sdk_POPULATED)
    FetchContent_Populate(nfsplugin_sdk)
endif()

set(NFSMW_SDK_INCLUDE_DIR  "${nfsmw_sdk_SOURCE_DIR}/include")
set(NFSMW_SDK_ENTRY_SOURCE "${nfsmw_sdk_SOURCE_DIR}/src/entry.c")
set(NFSMW_SDK_MINHOOK_DIR  "${nfsmw_sdk_SOURCE_DIR}/extern/minhook")
set(NFSMW_HOOKS_BACKEND "minhook")
set(NFSMW__MINHOOK_SRCS
    "${NFSMW_SDK_MINHOOK_DIR}/src/buffer.c"
    "${NFSMW_SDK_MINHOOK_DIR}/src/hook.c"
    "${NFSMW_SDK_MINHOOK_DIR}/src/trampoline.c"
    "${NFSMW_SDK_MINHOOK_DIR}/src/hde/hde32.c"
    "${NFSMW_SDK_MINHOOK_DIR}/src/hde/hde64.c"
)

set(FRR_DOMAIN_SOURCES
    src/domain/CameraFrustumEvidence.cpp
    src/domain/ChallengeInput.cpp
    src/domain/EncounterStateMachine.cpp
    src/domain/MotionScaleObserver.cpp
    src/domain/MutationReadiness.cpp
    src/domain/RenderObservation.cpp
    src/domain/OutrunRace.cpp
    src/domain/Progression.cpp
    src/domain/RoadCandidatePlanner.cpp
    src/domain/RivalGarage.cpp
    src/domain/RivalPopulation.cpp
    src/domain/RivalRuntimeHandle.cpp
    src/domain/RuntimeEvidence.cpp
    src/domain/SelectedRivalEvidence.cpp
    src/domain/RuntimeSession.cpp
    src/domain/SpawnExperiment.cpp
    src/domain/SpawnSafety.cpp
    src/domain/StakeRules.cpp
    src/domain/StagingPlanner.cpp
    src/domain/StagingStateMachine.cpp
    src/domain/UndergroundBlacklist.cpp
    src/domain/VehicleSelection.cpp
    src/domain/VehicleFootprintLearning.cpp
    src/domain/VehicleSpatialEvidence.cpp
    src/domain/WorldCollisionEvidence.cpp
    src/domain/WorldMetricCalibration.cpp
)

nfsmw_add_plugin(FreeRoamRivals
    SOURCES
        src/main.cpp
        src/core/Config.cpp
        src/core/Log.cpp
        src/core/ProfileKey.cpp
        src/core/VersionGuard.cpp
        src/game/CameraFrustumProbe.cpp
        src/game/ChallengeInputProbe.cpp
        src/game/GameBridge.cpp
        src/game/PlayerMotionProbe.cpp
        src/game/RoadCandidateProbe.cpp
        src/game/RoadNavProbe.cpp
        src/game/RuntimeProbe.cpp
        src/game/RenderObservationHook.cpp
        src/game/VehicleCatalogProbe.cpp
        src/game/VehicleSpatialProbe.cpp
        src/game/WorldCollisionProbe.cpp
        src/persistence/UndergroundBlacklistStore.cpp
        ${FRR_DOMAIN_SOURCES}
)

set_target_properties(FreeRoamRivals PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_include_directories(FreeRoamRivals PRIVATE
    src
    "${mwsdk_SOURCE_DIR}/include"
    "${nfsplugin_sdk_SOURCE_DIR}"
)

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

add_executable(FreeRoamRivalsSystemsTests
    tests/systems_tests.cpp
    src/game/RoadCandidateProbe.cpp
    ${FRR_DOMAIN_SOURCES}
)

target_include_directories(FreeRoamRivalsSystemsTests PRIVATE src)

if(MSVC)
    target_compile_options(FreeRoamRivalsSystemsTests PRIVATE /W4)
endif()

add_test(
    NAME FreeRoamRivalsSystemsTests
    COMMAND FreeRoamRivalsSystemsTests
)

add_executable(FreeRoamRivalsBlacklistTests
    tests/blacklist_tests.cpp
    ${FRR_DOMAIN_SOURCES}
)

target_include_directories(FreeRoamRivalsBlacklistTests PRIVATE src)

if(MSVC)
    target_compile_options(FreeRoamRivalsBlacklistTests PRIVATE /W4)
endif()

add_test(
    NAME FreeRoamRivalsBlacklistTests
    COMMAND FreeRoamRivalsBlacklistTests
)


add_executable(FreeRoamRivalsPersistenceTests
    tests/persistence_tests.cpp
    src/core/ProfileKey.cpp
    src/domain/UndergroundBlacklist.cpp
    src/persistence/UndergroundBlacklistStore.cpp
)

target_include_directories(FreeRoamRivalsPersistenceTests PRIVATE src)

target_compile_definitions(FreeRoamRivalsPersistenceTests PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(FreeRoamRivalsPersistenceTests PRIVATE /W4)
endif()

add_test(
    NAME FreeRoamRivalsPersistenceTests
    COMMAND FreeRoamRivalsPersistenceTests
)


add_executable(FreeRoamRivalsSpawnExperimentTests
    tests/spawn_experiment_tests.cpp
    src/domain/SpawnExperiment.cpp
)

target_include_directories(FreeRoamRivalsSpawnExperimentTests PRIVATE src)

if(MSVC)
    target_compile_options(FreeRoamRivalsSpawnExperimentTests PRIVATE /W4)
endif()

add_test(
    NAME FreeRoamRivalsSpawnExperimentTests
    COMMAND FreeRoamRivalsSpawnExperimentTests
)


add_executable(FreeRoamRivalsEvidenceTests tests/evidence_tests.cpp ${FRR_DOMAIN_SOURCES})
target_include_directories(FreeRoamRivalsEvidenceTests PRIVATE src)
add_test(NAME FreeRoamRivalsEvidenceTests COMMAND FreeRoamRivalsEvidenceTests)

add_executable(FreeRoamRivalsCameraTests tests/camera_tests.cpp src/domain/CameraFrustumEvidence.cpp src/domain/VehicleSpatialEvidence.cpp)
target_include_directories(FreeRoamRivalsCameraTests PRIVATE src)
add_test(NAME FreeRoamRivalsCameraTests COMMAND FreeRoamRivalsCameraTests)

add_executable(FreeRoamRivalsMotionTests tests/motion_tests.cpp src/domain/MotionScaleObserver.cpp src/domain/RuntimeEvidence.cpp)
target_include_directories(FreeRoamRivalsMotionTests PRIVATE src)
add_test(NAME FreeRoamRivalsMotionTests COMMAND FreeRoamRivalsMotionTests)

add_executable(FreeRoamRivalsRenderObservationTests tests/render_observation_tests.cpp src/domain/RenderObservation.cpp)
target_include_directories(FreeRoamRivalsRenderObservationTests PRIVATE src)
add_test(NAME FreeRoamRivalsRenderObservationTests COMMAND FreeRoamRivalsRenderObservationTests)

add_executable(FreeRoamRivalsRenderHookTests tests/render_hook_tests.cpp src/game/RenderObservationHook.cpp src/domain/RenderObservation.cpp src/core/Log.cpp ${NFSMW__MINHOOK_SRCS})
target_include_directories(FreeRoamRivalsRenderHookTests PRIVATE src "${NFSMW_SDK_MINHOOK_DIR}/include")
target_compile_definitions(FreeRoamRivalsRenderHookTests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX FRR_RENDER_HOOK_TESTING)
add_test(NAME FreeRoamRivalsRenderHookTests COMMAND FreeRoamRivalsRenderHookTests)
