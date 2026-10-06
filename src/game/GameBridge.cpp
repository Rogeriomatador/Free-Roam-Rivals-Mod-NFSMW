#include "GameBridge.h"

#include <mwsdk/game/mw05.hpp>
#include <mwsdk/game/mw05_research.hpp>

#include <NFSPluginSDK/Game.MW05/MW05.h>

#include <windows.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace frr::game {
namespace {

constexpr std::uintptr_t kIVehicleSubobjectOffset = 0xACu;
constexpr std::uintptr_t kPlayerPVehicleVtable = 0x008AC06Cu;
constexpr std::uintptr_t kAIPVehicleVtable = 0x008AC0FCu;
constexpr std::uint32_t kVehicleCountHardLimit = 512u;

bool isReadable(std::uintptr_t address, std::size_t size) {
    if (address == 0 || size == 0) {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(
            reinterpret_cast<const void*>(address),
            &mbi,
            sizeof(mbi)) == 0) {
        return false;
    }

    if (mbi.State != MEM_COMMIT) {
        return false;
    }

    if ((mbi.Protect & PAGE_GUARD) != 0 ||
        (mbi.Protect & PAGE_NOACCESS) != 0) {
        return false;
    }

    const auto begin =
        reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
    const auto end = begin + mbi.RegionSize;

    if (address < begin || address > end) {
        return false;
    }

    return size <= (end - address);
}

template <typename T>
bool readAbsolute(std::uintptr_t address, T& value) {
    if (!isReadable(address, sizeof(T))) {
        return false;
    }

    std::memcpy(
        &value,
        reinterpret_cast<const void*>(address),
        sizeof(T)
    );

    return true;
}

VehicleProbe probeVehicles() {
    VehicleProbe out{};

#if defined(_MSC_VER)
    __try {
#endif
        const std::uint32_t count =
            mwsdk::mw05::vehicle_count();

        if (count > kVehicleCountHardLimit) {
            return out;
        }

        out.registryReadable = true;
        out.totalVehicles = count;

        for (std::uint32_t i = 0; i < count; ++i) {
            void* iv = mwsdk::mw05::vehicle_at(i);

            if (!iv) {
                ++out.unknownVehicles;
                continue;
            }

            const auto driver =
                mwsdk::mw05::driver_class(iv);

            switch (driver) {
                case mwsdk::mw05::DriverClass::Human:
                    ++out.humanVehicles;
                    if (out.playerIVehicle == 0) {
                        out.playerIVehicle =
                            reinterpret_cast<std::uintptr_t>(iv);
                    }
                    break;
                case mwsdk::mw05::DriverClass::Traffic:
                    ++out.trafficVehicles;
                    break;
                case mwsdk::mw05::DriverClass::Cop:
                    ++out.copVehicles;
                    break;
                case mwsdk::mw05::DriverClass::Racer:
                    ++out.racerVehicles;
                    break;
                case mwsdk::mw05::DriverClass::None:
                    ++out.noneVehicles;
                    break;
                case mwsdk::mw05::DriverClass::NIS:
                    ++out.nisVehicles;
                    break;
                case mwsdk::mw05::DriverClass::Remote:
                    ++out.remoteVehicles;
                    break;
                default:
                    ++out.unknownVehicles;
                    break;
            }
        }

        if (out.playerIVehicle != 0 &&
            out.playerIVehicle >= kIVehicleSubobjectOffset) {
            const auto pvehicle =
                out.playerIVehicle - kIVehicleSubobjectOffset;

            std::uint32_t vtable = 0;
            if (readAbsolute(pvehicle, vtable)) {
                out.playerPVehicleCandidate = pvehicle;
                out.playerPVehicleVtableVerified =
                    vtable == kPlayerPVehicleVtable;

                // If this ever reports the AI vtable for the Human vehicle,
                // keep it visible in diagnostics rather than silently trusting
                // the cross-SDK conversion.
                if (vtable == kAIPVehicleVtable) {
                    out.playerPVehicleVtableVerified = false;
                }
            }
        }
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out = VehicleProbe{};
    }
#endif

    return out;
}

void probeRaceStatus(RuntimeSnapshot& out) {
#if defined(_MSC_VER)
    __try {
#endif
        auto* race =
            NFSPluginSDK::MW05::GRaceStatus::Get();

        if (!race) {
            return;
        }

        out.raceStatus =
            reinterpret_cast<std::uintptr_t>(race);
        out.raceStatusLoading = race->mIsLoading;

        switch (race->mPlayMode) {
            case NFSPluginSDK::MW05::GRaceStatus::PlayMode::Roaming:
                out.racePlayMode = RacePlayMode::Roaming;
                break;
            case NFSPluginSDK::MW05::GRaceStatus::PlayMode::Racing:
                out.racePlayMode = RacePlayMode::Racing;
                break;
            default:
                out.racePlayMode = RacePlayMode::Unknown;
                break;
        }
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out.raceStatus = 0;
        out.raceStatusLoading = false;
        out.racePlayMode = RacePlayMode::Unknown;
    }
#endif
}

CareerProbe probeCareer() {
    CareerProbe out{};

#if defined(_MSC_VER)
    __try {
#endif
        auto* db =
            NFSPluginSDK::MW05::cFrontEndDatabase::Get();

        if (!db) {
            return out;
        }

        auto* profile = db->GetUserProfile();
        if (!profile) {
            return out;
        }

        out.available = true;
        out.cash = profile->mTheCareerSettings.CurrentCash;
        out.currentCarHandle =
            profile->mTheCareerSettings.CurrentCar;
        out.careerCompletedAtLeastOnce =
            profile->mCareerModeHasBeenCompletedAtLeastOnce;

        const std::size_t carCount =
            profile->mPlayersCarStable.GetNumCareerCars();

        out.careerCars = static_cast<std::uint32_t>(
            std::min<std::size_t>(carCount, 0xFFFFFFFFu)
        );
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out = CareerProbe{};
    }
#endif

    return out;
}

} // namespace

RuntimeSnapshot GameBridge::sample() {
    RuntimeSnapshot out{};

#if defined(_MSC_VER)
    __try {
#endif
        out.gameFlowState =
            mwsdk::mw05::research::game_flow_state();
        out.inWorld =
            mwsdk::mw05::research::in_world();

        out.roadNetwork =
            reinterpret_cast<std::uintptr_t>(
                mwsdk::mw05::road_network()
            );
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out.gameFlowState = 0;
        out.inWorld = false;
        out.roadNetwork = 0;
    }
#endif

    bool inNIS = false;
    if (readAbsolute(0x0091606Cu, inNIS)) {
        out.inNIS = inNIS;
    }

    bool fade = false;
    if (readAbsolute(0x0091CAE4u, fade)) {
        out.fadeScreen = fade;
    }

    probeRaceStatus(out);
    out.vehicles = probeVehicles();
    out.career = probeCareer();

    if (!out.inWorld) {
        out.mode = WorldProbeMode::NoWorld;
    } else if (out.vehicles.playerIVehicle == 0) {
        out.mode = WorldProbeMode::NoPlayerVehicle;
    } else if (out.inNIS) {
        out.mode = WorldProbeMode::NIS;
    } else if (out.fadeScreen || out.raceStatusLoading) {
        out.mode = WorldProbeMode::Transition;
    } else if (out.racePlayMode == RacePlayMode::Racing) {
        out.mode = WorldProbeMode::StockRace;
    } else if (out.racePlayMode == RacePlayMode::Roaming) {
        out.mode = WorldProbeMode::FreeRoamCandidate;
    } else {
        out.mode = WorldProbeMode::NoPlayerVehicle;
    }

    return out;
}

const char* worldProbeModeName(WorldProbeMode mode) {
    switch (mode) {
        case WorldProbeMode::NoWorld:
            return "NoWorld";
        case WorldProbeMode::NoPlayerVehicle:
            return "NoPlayerVehicle";
        case WorldProbeMode::Transition:
            return "Transition";
        case WorldProbeMode::NIS:
            return "NIS";
        case WorldProbeMode::StockRace:
            return "StockRace";
        case WorldProbeMode::FreeRoamCandidate:
            return "FreeRoamCandidate";
    }

    return "Unknown";
}

const char* racePlayModeName(RacePlayMode mode) {
    switch (mode) {
        case RacePlayMode::Unknown:
            return "Unknown";
        case RacePlayMode::Roaming:
            return "Roaming";
        case RacePlayMode::Racing:
            return "Racing";
    }

    return "Unknown";
}

} // namespace frr::game
