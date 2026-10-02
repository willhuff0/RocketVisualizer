#include "AhrsTelemetryParser.hpp"
#include "CoordinateConverter.hpp"
#include <cmath>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

std::optional<TimedRocketState> ParseAhrsTelemetry(std::string_view line) {
    if (!line.starts_with("AHRS1,")) return std::nullopt;
    std::istringstream input(std::string(line.substr(6)));
    input.imbue(std::locale::classic());
    double values[8];
    for (std::size_t i = 0; i < 8; ++i) {
        if (!(input >> values[i]) || !std::isfinite(values[i])) return std::nullopt;
        if (i != 7) {
            char separator;
            if (!input.get(separator) || separator != ',') return std::nullopt;
        }
    }
    input >> std::ws;
    if (!input.eof() || values[0] < 0) return std::nullopt;
    const Eigen::Quaterniond q(values[4], values[5], values[6], values[7]);
    if (!std::isfinite(q.norm()) || q.norm() < 1e-12) return std::nullopt;
    TimedRocketState sample;
    sample.time = values[0];
    sample.state.position = {0.0, 0.0, 0.0};
    // sample.state.position = CoordinateConverter::ImportWorldVector(
    // Eigen::Vector3d(values[1], values[2], values[3] - 2070.0), WorldFrame::NED);
    sample.state.orientation = CoordinateConverter::ImportQuaternion(
        q, WorldFrame::NED, BodyFrame::FRD, RotationDirection::BodyToWorld);
    return sample;
}
