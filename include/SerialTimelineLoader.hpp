#pragma once

#include "SerialRocketTimeline.hpp"
#include "CoordinateSystemImportUI.hpp"

#include <imgui.h>

#include <functional>
#include <memory>
#include <vector>
#include <optional>

struct SerialConnectConfig {
    std::string configName;

    CoordinateSystemImportConfig coordinateSystem;

    uint32_t buadRate = 115200;
    std::string format;
};

class SerialTimelineLoader {
    using OnConnectCallback = std::function<void(RocketConfig, std::shared_ptr<SerialRocketTimeline>)>;
public:
    explicit SerialTimelineLoader(OnConnectCallback onConnect);

    void ShowConnectDialog();
    void DrawUI();

private:
    OnConnectCallback _onConnect;
    ImGuiID _popupId = 0;

    std::vector<serial_cpp::PortInfo> _ports;
    std::optional<serial_cpp::PortInfo> _port;
    SerialConnectConfig _config;

    void _refreshPorts();
};
