#pragma once

#include <Eigen/Dense>

constexpr size_t MAX_FIN_COUNT = 16;

struct RocketConfig {
    Eigen::Vector3d finRotationAxes[MAX_FIN_COUNT] = {};
    std::string finNodeNames[MAX_FIN_COUNT] = {};
};

struct RocketState {
    Eigen::Vector3d position = Eigen::Vector3d::Zero();
    Eigen::Quaterniond orientation = Eigen::Quaterniond::Identity();

    double actualFinAngles[MAX_FIN_COUNT] = {};
    double targetFinAngles[MAX_FIN_COUNT] = {};

    std::string stateMachineState;

    static RocketState Interpolate(const RocketState& a, const RocketState& b, double t);
};
