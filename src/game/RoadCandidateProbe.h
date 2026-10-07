#pragma once

#include "RoadNavProbe.h"
#include "../domain/RoadCandidatePlanner.h"

#include <vector>

namespace frr::game {

class RoadCandidateProbe {
public:
    static std::vector<frr::domain::RoadCandidateObservation>
    build(
        const PlayerRoadNavigationProbe& roadNavigation
    );
};

} // namespace frr::game
