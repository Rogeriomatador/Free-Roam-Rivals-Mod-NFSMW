#include "ChallengeInput.h"

namespace frr::domain {

bool ChallengeInputEdge::update(bool down) {
    const bool pressed =
        down && !previousDown_;

    previousDown_ = down;
    return pressed;
}

void ChallengeInputEdge::reset() {
    previousDown_ = false;
}

bool ChallengeInputEdge::previousDown() const {
    return previousDown_;
}

} // namespace frr::domain
