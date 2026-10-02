#pragma once

#include "IRocketTimeline.hpp"
#include <string_view>

// Non-telemetry/debug lines and malformed samples are ignored.
std::optional<TimedRocketState> ParseAhrsTelemetry(std::string_view line);
