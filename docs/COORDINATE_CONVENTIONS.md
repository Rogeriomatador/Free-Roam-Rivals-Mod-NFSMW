# MW05 coordinate convention

Free Roam Rivals domain code uses one coordinate convention:

- X = MW simulation X
- Y = MW simulation vertical/up
- Z = MW simulation Z

The pinned NFSPluginSDK UVector3 wrapper declares members in source order
y, z, x, while reconstructed MW05 PC UMath::Vector3 is x, y, z in memory.
Engine writes therefore appear through the wrapper as value.y/value.z/value.x.
NfsPluginCoordinateAdapter.h converts that wrapper view at the probe boundary.

Do not apply the adapter to MWSDK camera matrices or raw engine ABI structures.
Those use their own documented native layouts.

The v0.0.28 target capture was decisive: the old log's third component stayed
near road altitude while the first two varied over hundreds of world units, and
one nominal ground hit produced an implausible height delta and grade.
v0.0.29 canonicalizes the representation instead of weakening evidence gates.
