#include <RocketState.hpp>

static Eigen::Vector3d lerpVector3d(const Eigen::Vector3d& a, const Eigen::Vector3d& b, double t) {
    return (1.0 - t) * a + t * b;
}

static double lerpDouble(double a, double b, double t) {
    return (1.0 - t) * a + t * b;
}

RocketState RocketState::Interpolate(const RocketState& a, const RocketState& b, double t) {
    RocketState state = {
        .position = lerpVector3d(a.position, b.position, t),
        .orientation =  a.orientation.slerp(t, b.orientation),
        .stateMachineState = a.stateMachineState,
    };
    for (size_t i = 0; i < MAX_FIN_COUNT; i++) {
        state.actualFinAngles[i] = lerpDouble(a.actualFinAngles[i], b.actualFinAngles[i], t);
        state.targetFinAngles[i] = lerpDouble(a.targetFinAngles[i], b.targetFinAngles[i], t);
    }
    return state;
}
