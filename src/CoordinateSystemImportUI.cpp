#include <CoordinateSystemImportUI.hpp>

#include <imgui.h>

void CoordinateSystemImportUI::Draw(CoordinateSystemImportConfig& config) {
    ImGui::Text("Coordinate System");

    if (ImGui::BeginCombo("World", GetWorldFrameLabel(config.worldFrame))) {
        for (size_t i = 0; i < std::size(AllWorldFrames); i++) {
            WorldFrame worldFrame = AllWorldFrames[i];

            const char* label = GetWorldFrameLabel(worldFrame);
            bool isSelected = config.worldFrame == worldFrame;

            if (ImGui::Selectable(label, isSelected)) {
                config.worldFrame = worldFrame;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Body", GetBodyFrameLabel(config.bodyFrame))) {
        for (size_t i = 0; i < std::size(AllBodyFrames); i++) {
            BodyFrame bodyFrame = AllBodyFrames[i];

            const char* label = GetBodyFrameLabel(bodyFrame);
            bool isSelected = config.bodyFrame == bodyFrame;

            if (ImGui::Selectable(label, isSelected)) {
                config.bodyFrame = bodyFrame;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Direction", GetRotationDirectionLabel(config.rotationDirection))) {
        for (size_t i = 0; i < std::size(AllRotationDirections); i++) {
            RotationDirection rotationDirection = AllRotationDirections[i];

            const char* label = GetRotationDirectionLabel(rotationDirection);
            bool isSelected = config.rotationDirection == rotationDirection;

            if (ImGui::Selectable(label, isSelected)) {
                config.rotationDirection = rotationDirection;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}