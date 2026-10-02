#pragma once

#include "LiveRocketTimeline.hpp"
#include <memory>
#include <string>

// Poll from the render loop. Reads only available bytes, so waiting for data
// and shutting down the window never require joining a blocked reader thread.
class StdinAhrsProducer {
public:
    explicit StdinAhrsProducer(std::shared_ptr<LiveRocketTimeline> timeline);
    void Poll();
    bool IsFinished() const { return _finished; }
    const std::string& Error() const { return _error; }
    std::size_t SampleCount() const { return _sampleCount; }

private:
    void Consume(const char* data, std::size_t size);
    std::shared_ptr<LiveRocketTimeline> _timeline;
    std::string _line;
    std::string _error;
    bool _discardLine = false;
    bool _finished = false;
    bool _hasSample = false;
    double _lastTime = 0;
    std::size_t _sampleCount = 0;
};
