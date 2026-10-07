#pragma once

namespace frr::domain {

class ChallengeInputEdge {
public:
    bool update(bool down);
    void reset();

    bool previousDown() const;

private:
    bool previousDown_ = false;
};

} // namespace frr::domain
