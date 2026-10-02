#pragma once

#include "RocketState.hpp"

#include <optional>

struct TimedRocketState {
    double time = 0.0;
    RocketState state;
};

struct TimeRange {
    double begin;
    double end;
};

class IRocketTimeline {
public:
    virtual ~IRocketTimeline() = default;

    /**
     * @return the start and end time of the available data.
     */
    virtual std::optional<TimeRange> GetRange() const = 0;

    /**
     * Gets the state at an arbitrary time. May interpolate between samples. Clamps time, see GetRange.
     */
    virtual std::optional<RocketState> GetStateAt(double time, bool interpolate) const = 0;

    virtual bool IsCapableOfLivePlayback() const = 0;
};
