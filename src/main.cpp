#include "PollingStateProducer.hpp"
#include "RocketVisualizer.hpp"

#include <iostream>
#include <string>

// Example for live connection:
//
// auto timeline = std::make_shared<LiveRocketTimeline>();
// PollingStateProducer stateProducer(timeline, []() {
//     return TimedRocketState {
//         .time = GetTime(),
//         .state = RocketState {
//             .position = {0.0, 0.0, GetTime()},
//             .orientation = {1.0, 0.0, 0.0, 0.0},
//         },
//     };
// });

int main() {
    // auto parser = SerialLineParser(R"d(${t},${px},${py},${pz},${qw},${qx},${qy},${qz})d");
    //
    // std::string line;
    // while (true) {
    //     std::getline(std::cin, line);
    //
    //     auto maybeSample = parser.Parse(line);
    //     if (!maybeSample.has_value()) continue;
    //     auto sample = maybeSample.value();
    //
    //     std::cout << std::endl;
    //     std::cout << "Time        : " << sample.time << std::endl;
    //     std::cout << "Orientation : " << sample.state.orientation << std::endl;
    //     std::cout << "Position    : " << sample.state.position.x() << ", " << sample.state.position.y() << ", " << sample.state.position.z() << std::endl;
    //     std::cout << std::endl << std::endl;
    // }

    RocketVisualizer visualizer;
    visualizer.Run();
}
