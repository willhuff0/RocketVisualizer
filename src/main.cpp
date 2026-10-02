#include "PollingStateProducer.hpp"
#include "RocketVisualizer.hpp"
#include "StdinAhrsProducer.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    bool liveStdin = false;
    for (int i = 1; i < argc; ++i) {
        const std::string option = argv[i];
        if (option == "--help" || option == "-h") {
            std::cout << "Usage: RocketVisualizer [--live-stdin]\n"
                         "Read AHRS1 telemetry from a controller or USB serial pipe.\n";
            return 0;
        }
        if (option == "--live-stdin") liveStdin = true;
        else {
            std::cerr << "Unknown option: " << option << '\n';
            return 1;
        }
    }
    RocketVisualizer visualizer;
    auto timeline = std::make_shared<LiveRocketTimeline>();
    visualizer.SetConfigAndTimeline({}, timeline);
    if (liveStdin) {
        SetWindowTitle("Rocket Visualizer - Live AHRS");
        StdinAhrsProducer producer(timeline);
        bool reportedError = false;
        visualizer.Run([&] {
            producer.Poll();
            if (!reportedError && !producer.Error().empty()) {
                std::cerr << "Live AHRS: " << producer.Error() << '\n';
                reportedError = true;
            }
        });
    } else {
        PollingStateProducer producer(timeline, [] {
            return TimedRocketState{
                .time = GetTime(),
                .state = RocketState{
                    .position = {0.0, 0.0, GetTime()},
                    .orientation = {1.0, 0.0, 0.0, 0.0},
                },
            };
        });
        visualizer.Run();
    }
}
