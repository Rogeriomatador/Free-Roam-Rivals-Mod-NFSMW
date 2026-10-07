#pragma once

namespace frr::game {

// NFSPluginSDK's MW05 UVector3 wrapper declares its members in the physical
// order y,z,x, while the reconstructed PC game UMath::Vector3 ABI is x,y,z.
// Engine calls therefore write game X into value.y, game Y into value.z and
// game Z into value.x. Canonicalize immediately at the SDK boundary so all FRR
// domain code uses the game's native simulation convention: X, Y-up, Z.
struct CanonicalMwVector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

template <typename NfsPluginVector3>
constexpr CanonicalMwVector3 canonicalMwVector(
    const NfsPluginVector3& value
) {
    return {
        value.y,
        value.z,
        value.x
    };
}

} // namespace frr::game
