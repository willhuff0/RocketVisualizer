#pragma once

#include "IRocketTimeline.hpp"

#include <vector>

class VectorRocketTimeline : public IRocketTimeline {
public:
    explicit VectorRocketTimeline(std::vector<TimedRocketState> samples);

    std::optional<TimeRange> GetRange() const override;
    std::optional<RocketState> GetStateAt(double time, bool interpolate) const override;

    bool IsCapableOfLivePlayback() const override;

private:
    std::vector<TimedRocketState> _samples;
};