#include <SerialTimelineLoader.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

const SerialConnectConfig COMMA_SEPARATED_PROFILE = {
    .configName = "Comma Separated",
    .coordinateSystem = {
        .worldFrame = WorldFrame::ENU,
        .bodyFrame = BodyFrame::FLU,
        .rotationDirection = RotationDirection::BodyToWorld,
    },
    .format = R"d(${t},${px},${py},${pz},${qw},${qx},${qy},${qz})d",
};

const SerialConnectConfig IMPORT_PROFILES[] = {
    COMMA_SEPARATED_PROFILE,
};

static std::string getPortShortName(const serial_cpp::PortInfo& portInfo) {
    std::stringstream ss;
    ss << portInfo.port;
    if (!portInfo.description.empty()) {
        ss << " | " << portInfo.description;
    }
    if (!portInfo.hardware_id.empty()) {
        ss << " | " << portInfo.hardware_id;
    }
    return ss.str();
}

static std::shared_ptr<SerialRocketTimeline> loadTimelineFromSerial(const std::string& port, SerialConnectConfig config) {
    return std::make_shared<SerialRocketTimeline>(config.coordinateSystem, SerialLineParser(config.format), port, config.buadRate);
}

SerialTimelineLoader::SerialTimelineLoader(OnConnectCallback onConnect)
    : _onConnect(std::move(onConnect)) { }

void SerialTimelineLoader::ShowConnectDialog() {
    _refreshPorts();
    ImGui::OpenPopup(_popupId);
}

void SerialTimelineLoader::DrawUI() {
    _popupId = ImGui::GetID("Connect to Serial Device");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool isModalOpen = true;
    if (ImGui::BeginPopupModal("Connect to Serial Device", &isModalOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::Separator();
        std::string selectedPortText = _port.has_value() ? getPortShortName(_port.value()) : _ports.empty() ? "No devices found" : "Select device";
        if (ImGui::BeginCombo("Device", selectedPortText.c_str())) {
            for (const auto& port : _ports) {
                std::string name = getPortShortName(port);
                if (ImGui::Selectable(name.c_str(), _port.has_value() && _port.value().port == port.port)) {
                    _port = port;
                }
            }
            ImGui::EndCombo();
        }

        std::string buadRateBefore = std::to_string(_config.buadRate);
        std::string buadRate = buadRateBefore;
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputText("Buad Rate", &buadRate);
        if (buadRate != buadRateBefore) {
            try {
                _config.buadRate = std::stoul(buadRate);
            } catch (const std::exception& _) {}
        }

        ImGui::Separator();
        if (ImGui::BeginCombo("##PresetsCombo", "Presets")) {
            for (const auto& profile : IMPORT_PROFILES) {
                if (ImGui::Selectable(profile.configName.c_str())) {
                    _config = profile;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();
        CoordinateSystemImportUI::Draw(_config.coordinateSystem);

        ImGui::Separator();
        ImGui::InputTextMultiline("Format", &_config.format, ImVec2(0.0f, 0.0f), ImGuiInputTextFlags_WordWrap);

        ImGui::Separator();
        ImGui::BeginDisabled(!_port.has_value());
        if (ImGui::Button("Connect")) {
            auto result = loadTimelineFromSerial(_port.value().port, _config);
            if (result != nullptr) {
                _onConnect(RocketConfig {}, result);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::EndPopup();
    }
}

void SerialTimelineLoader::_refreshPorts() {
    _ports = serial_cpp::list_ports();
    if (_port.has_value()) {
        for (const auto& port : _ports) {
            if (port.port == _port.value().port) {
                _port = port;
                break;
            }
        }

        _port = std::nullopt;
    }
}
