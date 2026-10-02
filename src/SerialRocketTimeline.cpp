#include <SerialRocketTimeline.hpp>

#include "CoordinateConverter.hpp"

SerialRocketTimeline::SerialRocketTimeline(
    CoordinateSystemImportConfig coordinateSystem,
    SerialLineParser&& parser,
    const std::string& port,
    uint32_t buadRate)
        : _coordinateSystem(std::move(coordinateSystem)),
        _parser(std::move(parser)),
        _serial(port, buadRate),
        _serialReadThread([&] { _readSerialLoop(); }) {}

SerialRocketTimeline::~SerialRocketTimeline() {
    _running = false;
    _serialReadThread.join();
}

std::optional<TimeRange> SerialRocketTimeline::GetRange() const {
    return _timeline.GetRange();
}

std::optional<RocketState> SerialRocketTimeline::GetStateAt(double time, bool interpolate) const {
    return _timeline.GetStateAt(time, interpolate);
}

void SerialRocketTimeline::Clear() {
    _running = false;
    _serialReadThread.join();

    _timeline.Clear();

    _running = true;
    _serialReadThread = std::thread([&] { _readSerialLoop(); });
}

bool SerialRocketTimeline::IsCapableOfLivePlayback() const {
    return _timeline.IsCapableOfLivePlayback();
}

void SerialRocketTimeline::_readSerialLoop() {
    using Clock = std::chrono::steady_clock;

    auto timeStart = Clock::now();
    while (_running.load(std::memory_order_relaxed) && _serial.isOpen()) {
        std::string line = _serial.readline();
        if (line.empty()) continue;

        auto maybeSample = _parser.Parse(line);
        if (!maybeSample.has_value()) continue;
        auto sample = maybeSample.value();

        // Serial device is not reporting time. Use our time as backup.
        if (sample.time < 0.0) {
            std::chrono::duration<double> time = Clock::now() - timeStart;
            sample.time = time.count();
        }

        sample.state.orientation = CoordinateConverter::ImportQuaternion(
            sample.state.orientation,
            _coordinateSystem.worldFrame,
            _coordinateSystem.bodyFrame,
            _coordinateSystem.rotationDirection
        );

        sample.state.position = CoordinateConverter::ImportWorldVector(
            sample.state.position,
            _coordinateSystem.worldFrame
        );

        _timeline.Publish(sample);
    }
}
