#include <windows.h>
#include <NFSPluginSDK/Game.MW05/Types/PVehicle.h>
#include <cstddef>
#include <cstdio>
#include <type_traits>

// Never instantiate SDK engine objects: their constructors/virtual methods may
// call the game. These are compiler layout checks against the uploaded target.
using namespace NFSPluginSDK::MW05;
static_assert(sizeof(void*) == 4);
static_assert(sizeof(Sim::Param) == 0x10);
static_assert(sizeof(VehicleParams) == 0x30);
static_assert(std::is_trivially_destructible_v<VehicleParams>);
static_assert(offsetof(VehicleParams, mData) == 0x08);
static_assert(offsetof(VehicleParams, mDriverClass) == 0x10);
static_assert(offsetof(VehicleParams, mVehicleKey) == 0x14);
static_assert(offsetof(VehicleParams, mDirection) == 0x18);
static_assert(offsetof(VehicleParams, mPosition) == 0x1C);
static_assert(offsetof(VehicleParams, mCustomization) == 0x20);
static_assert(offsetof(VehicleParams, mVehicleCache) == 0x24);
static_assert(offsetof(VehicleParams, mPerformanceMatch) == 0x28);
static_assert(offsetof(VehicleParams, mFlags) == 0x2C);

// MSVC supports offsetof on these non-standard-layout classes. The generated
// class-layout report is also reviewed for ISimable +0x2C and IVehicle +0xAC;
// member checks alone do not prove every base-interface offset.
static_assert(offsetof(PVehicle, mDriverClass) == 0x140);
static_assert(offsetof(PVehicle, mAI) == 0x100);
// The pinned SDK models only the known prefix (0x160 bytes). The native
// factory allocates 0x1AC; NEVER allocate/copy a game object using SDK sizeof.
static_assert(sizeof(PVehicle) == 0x160);
static_assert(sizeof(PVehicle) < 0x1AC);

int main() {
    std::printf("Native factory ABI: VehicleParams=%zu PVehiclePrefix=%zu nativeAllocation=428 driver=%zu AI=%zu; no engine calls\n",
        sizeof(VehicleParams), sizeof(PVehicle),
        offsetof(PVehicle, mDriverClass), offsetof(PVehicle, mAI));
    return 0;
}
