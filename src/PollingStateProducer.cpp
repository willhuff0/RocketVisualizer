#include <PollingStateProducer.hpp>

PollingStateProducer::PollingStateProducer(
    std::shared_ptr<LiveRocketTimeline> timeline,
    std::function<TimedRocketState()> getRocketState,
    double pollInterval)
        : _timeline(std::move(timeline)),
        _getRocketState(std::move(getRocketState)),
        _pollInterval(pollInterval),
        _sensorPollThread([&] { _pollSensorLoop(); }) {}

PollingStateProducer::~PollingStateProducer() {
    _running = false;
    _sensorPollThread.join();
}

void PollingStateProducer::_pollSensorLoop() {
    using Clock = std::chrono::steady_clock;

    const double dt = _pollInterval;

    auto nextUpdate = Clock::now();
    double simTime = 0.0;
    while (_running.load(std::memory_order_relaxed)) {
        TimedRocketState state = _getRocketState();

        _timeline->Publish(state);

        simTime += dt;
        nextUpdate += std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(dt));

        std::this_thread::sleep_until(nextUpdate);
    }
}
