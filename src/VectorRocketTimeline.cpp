#include <VectorRocketTimeline.hpp>

#include <ranges>

VectorRocketTimeline::VectorRocketTimeline(std::vector<TimedRocketState> samples)
    : _samples(std::move(samples)) { }

std::optional<TimeRange> VectorRocketTimeline::GetRange() const {
    if (_samples.empty()) return std::nullopt;

    return TimeRange {
        .begin = _samples.front().time,
        .end = _samples.back().time,
    };
}

std::optional<RocketState> VectorRocketTimeline::GetStateAt(double time, bool interpolate) const {
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

void VectorRocketTimeline::Clear() {
    // VectorRocketTimeline should never clear
}

bool VectorRocketTimeline::IsCapableOfLivePlayback() const {
    return false;
}
