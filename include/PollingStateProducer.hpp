#pragma once

#include "LiveRocketTimeline.hpp"

#include <memory>
#include <thread>
#include <functional>
#include <atomic>

class PollingStateProducer {
public:
    PollingStateProducer(
        std::shared_ptr<LiveRocketTimeline> timeline,
        std::function<TimedRocketState()> getRocketState,
        double pollInterval = 1.0 / 100.0);
    ~PollingStateProducer();

    void _pollSensorLoop();

private:
    std::shared_ptr<LiveRocketTimeline> _timeline;
    std::function<TimedRocketState()> _getRocketState;
    double _pollInterval;

    std::atomic_bool _running = true;
    std::thread _sensorPollThread;
};
