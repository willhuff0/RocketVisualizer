#pragma once

#include "IRocketTimeline.hpp"

#include <mutex>
#include <deque>

class LiveRocketTimeline : public IRocketTimeline {
public:
    void Publish(TimedRocketState sample);
    void Clear();

    std::optional<TimeRange> GetRange() const override;
    std::optional<RocketState> GetStateAt(double time, bool interpolate) const override;

    bool IsCapableOfLivePlayback() const override;

private:
    mutable std::mutex _mutex;
    std::deque<TimedRocketState> _samples;
};
