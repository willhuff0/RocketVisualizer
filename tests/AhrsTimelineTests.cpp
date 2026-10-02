#include "AhrsTelemetryParser.hpp"
#include "LiveRocketTimeline.hpp"
#include "StdinAhrsProducer.hpp"
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void parsingAndFrames() {
    const auto sample = ParseAhrsTelemetry("AHRS1,1.25,2,3,-4,2,0,0,0\r");
    require(sample.has_value(), "Accept a complete record and normalize its quaternion");
    require(sample->time == 1.25, "Preserve the source timestamp");
    require(sample->state.position.isApprox(Eigen::Vector3d(2, -3, 4)), "NED position must become NWU");
    const auto& q = sample->state.orientation;
    require((q * Eigen::Vector3d::UnitZ()).isApprox(Eigen::Vector3d::UnitX()), "FRD nose points north for identity attitude");
    require((q * Eigen::Vector3d::UnitX()).isApprox(-Eigen::Vector3d::UnitY()), "FRD right points east");
    require((q * Eigen::Vector3d::UnitY()).isApprox(-Eigen::Vector3d::UnitZ()), "FRD down points down");

    const auto upright = ParseAhrsTelemetry("AHRS1,2,0,0,-10,0.7071067811865476,0,0.7071067811865476,0");
    require(upright.has_value(), "Accept an upright attitude");
    require((upright->state.orientation * Eigen::Vector3d::UnitZ()).isApprox(Eigen::Vector3d::UnitZ()), "Upright rocket nose must render up");
    const auto rolled = ParseAhrsTelemetry("AHRS1,3,0,0,0,0.7071067811865476,0.7071067811865476,0,0");
    require(rolled.has_value(), "Accept a rolled attitude");
    require((rolled->state.orientation * Eigen::Vector3d::UnitX()).isApprox(-Eigen::Vector3d::UnitZ()), "Preserve roll about the nose");

    for (const char* bad : {"Flight controller ready.", "AHRS2,0,0,0,0,1,0,0,0",
            "AHRS1,-1,0,0,0,1,0,0,0", "AHRS1,0,0,0,0,0,0,0,0",
            "AHRS1,0,0,0,0,nan,0,0,0", "AHRS1,0,0,0,0,1,0,0",
            "AHRS1,0,0,0,0,1,0,0,0,1", "AHRS1,0,0,0,0,1,0,0,0junk",
            "AHRS1,0,0,inf,0,1,0,0,0", "AHRS1,0;0,0,0,1,0,0,0"}) {
        require(!ParseAhrsTelemetry(bad), "Ignore malformed records and unrelated serial output");
    }
}

void timelineLookup() {
    LiveRocketTimeline timeline;
    require(!timeline.GetRange(), "Timeline begins empty");
    TimedRocketState a{1, {}}, b{2, {}};
    a.state.position = {1, 0, 0};
    b.state.position = {3, 0, 0};
    timeline.Publish(a);
    timeline.Publish(b);
    require(timeline.GetStateAt(2, false)->position.x() == 3, "Live lookup must return the latest exact sample");
    require(timeline.GetStateAt(1.5, false)->position.x() == 1, "Hold the previous sample without interpolation");
    require(timeline.GetStateAt(1.5, true)->position.x() == 2, "Interpolate buffered samples");
    b.state.position.x() = 4;
    timeline.Publish(b);
    require(timeline.GetStateAt(2, false)->position.x() == 4, "Replace duplicate timestamps");
    timeline.Clear();
    require(!timeline.GetRange() && !timeline.GetStateAt(2, false), "Reset clears buffered history");
}

void readStdin(bool once) {
    auto timeline = std::make_shared<LiveRocketTimeline>();
    StdinAhrsProducer producer(timeline);
    const auto start = std::chrono::steady_clock::now();
    do {
        producer.Poll();
        if (once || producer.IsFinished()) break;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10))
            throw std::runtime_error("Timed out reading telemetry");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (true);
    require(producer.Error().empty(), producer.Error().c_str());
    std::cout << std::setprecision(17) << "{\"count\":" << producer.SampleCount()
              << ",\"finished\":" << (producer.IsFinished() ? "true" : "false");
    if (const auto range = timeline->GetRange()) {
        const auto state = timeline->GetStateAt(range->end, false).value();
        const Eigen::Vector3d nose = state.orientation * Eigen::Vector3d::UnitZ();
        std::cout << ",\"begin\":" << range->begin << ",\"end\":" << range->end
                  << ",\"position\":[" << state.position.x() << ',' << state.position.y() << ',' << state.position.z()
                  << "],\"nose\":[" << nose.x() << ',' << nose.y() << ',' << nose.z() << ']';
    }
    std::cout << "}\n";
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--stdin") readStdin(false);
        else if (argc == 2 && std::string(argv[1]) == "--poll-once") readStdin(true);
        else {
            parsingAndFrames();
            timelineLookup();
            std::cout << "AHRS timeline tests passed\n";
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
