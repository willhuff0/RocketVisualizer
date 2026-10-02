#pragma once

#include "IRocketTimeline.hpp"
#include "LiveRocketTimeline.hpp"
#include "SerialLineParser.hpp"
#include "CoordinateSystemImportUI.hpp"

#include <serial_cpp/serial.h>

#include <atomic>
#include <thread>
#include <optional>
#include <string>

class SerialRocketTimeline : public IRocketTimeline {
public:
    explicit SerialRocketTimeline(
        CoordinateSystemImportConfig coordinateSystem,
        SerialLineParser&& parser,
        const std::string& port,
        uint32_t buadRate);
    ~SerialRocketTimeline() override;

    std::optional<TimeRange> GetRange() const override;
    std::optional<RocketState> GetStateAt(double time, bool interpolate) const override;

    void Clear() override;

    bool IsCapableOfLivePlayback() const override;

    void _readSerialLoop();

private:
    CoordinateSystemImportConfig _coordinateSystem;
    SerialLineParser _parser;
    serial_cpp::Serial _serial;
    LiveRocketTimeline _timeline;

    std::atomic_bool _running = true;
    std::thread _serialReadThread;
};
