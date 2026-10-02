#include "PollingStateProducer.hpp"
#include "RocketVisualizer.hpp"

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
    RocketVisualizer visualizer;
    visualizer.Run();
}
