#include "LiveRocketTimeline.hpp"

void LiveRocketTimeline::Publish(TimedRocketState sample) {
    std::lock_guard lock(_mutex);

    // first sample
    if (_samples.empty()) {
        _samples.push_back(sample);
        return;
    }

    // get first sample where sample.time >= time
    auto iter = std::lower_bound(_samples.begin(), _samples.end(), sample.time,
        [] (const TimedRocketState& s, double value) {
            return s.time < value;
        });

    // duplicate sample, replace
    if (iter != _samples.end() && iter->time == sample.time) {
        iter->state = sample.state;
        return;
    }

    _samples.insert(iter, sample);
}

std::optional<TimeRange> LiveRocketTimeline::GetRange() const {
    std::lock_guard lock(_mutex);

    if (_samples.empty()) return std::nullopt;

    return TimeRange {
        .begin = _samples.front().time,
        .end = _samples.back().time,
    };
}

std::optional<RocketState> LiveRocketTimeline::GetStateAt(double time, bool interpolate) const {
    std::lock_guard lock(_mutex);

    if (_samples.empty()) return std::nullopt;

    // get first sample where sample.time >= time
    auto iterRight = std::lower_bound(_samples.begin(), _samples.end(), time,
        [] (const TimedRocketState& s, double value) {
            return s.time < value;
        });

    // time is before first sample, clamp
    if (iterRight == _samples.begin()) {
        return iterRight->state;
    }
    // time is after last sample, clamp
    if (iterRight == _samples.end()) {
        return _samples.back().state;
    }

    auto iterLeft = std::prev(iterRight);

    if (interpolate) {
        double timeDiff = iterRight->time - iterLeft->time;
        // avoid division by zero
        if (timeDiff == 0.0) {
            return iterLeft->state;
        }
        double frac = (time - iterLeft->time) / timeDiff;

        return RocketState::Interpolate(iterLeft->state, iterRight->state, frac);
    } else {
        return iterLeft->state;
    }
}

bool LiveRocketTimeline::IsCapableOfLivePlayback() const {
    return true;
}
