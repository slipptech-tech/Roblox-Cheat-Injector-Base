#include "ESP.h"
#include "../Common/Config.hpp"
#include <imgui.h>

namespace ESP {

    void Render() {
        if (!Config::g_cfg.espEnabled) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImGuiIO& io = ImGui::GetIO();

        // Заглушка. Реальные данные — через обход Players/Character
        // по оффсетам DataModel -> Players -> LocalPlayer -> Character.
        // Для демонстрации рисуем рамку по центру.

        ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        dl->AddRect(
            ImVec2(center.x - 50, center.y - 100),
            ImVec2(center.x + 50, center.y + 100),
            IM_COL32(0, 255, 0, 255), 0.f, 0, 2.f);

        dl->AddText(
            ImVec2(center.x - 50, center.y - 120),
            IM_COL32(255, 255, 0, 255),
            "ESP target");
    }
}