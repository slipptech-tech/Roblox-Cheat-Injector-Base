#include "Menu.h"
#include "LuaExecutor.h"
#include "Signatures.h"
#include "../Common/Config.hpp"
#include <imgui.h>
#include <Windows.h>
#include <string>

namespace Menu {

    static bool g_open = true;
    static char g_script[8192] = "";

    void Toggle() { g_open = !g_open; }
    bool IsOpen() { return g_open; }

    void Render() {
        if (GetAsyncKeyState(VK_INSERT) & 1) g_open = !g_open;
        if (!g_open) return;

        ImGui::Begin("RBX_Injector", &g_open, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::BeginTabBar("tabs")) {
            if (ImGui::BeginTabItem("Main")) {
                ImGui::Checkbox("ESP", &Config::g_cfg.espEnabled);
                ImGui::Checkbox("Aimbot", &Config::g_cfg.aimbotEnabled);
                ImGui::SliderFloat("Aimbot FOV", &Config::g_cfg.aimbotFov, 1.f, 360.f);
                ImGui::SliderFloat("Aimbot Smooth", &Config::g_cfg.aimbotSmooth, 1.f, 20.f);

                if (ImGui::Button("Save Config")) Config::Save();
                ImGui::SameLine();
                if (ImGui::Button("Load Config")) Config::Load();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Lua")) {
                ImGui::InputTextMultiline("##script", g_script, sizeof(g_script),
                                          ImVec2(600, 300));

                if (ImGui::Button("Execute")) {
                    LuaExecutor::ExecuteAsync(std::string(g_script));
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear")) g_script[0] = '\0';
                ImGui::SameLine();
                if (ImGui::Button("Refresh lua_State")) {
                    LuaExecutor::RefreshState();
                }
                ImGui::SameLine();
                if (ImGui::Button("Rescan Signatures")) {
                    Signatures::Refresh();
                    LuaExecutor::Init();
                }

                ImGui::Separator();
                ImGui::Text("Load .lua from disk:");
                static char g_path[MAX_PATH] = "C:\\cheat.lua";
                ImGui::InputText("##path", g_path, sizeof(g_path));
                if (ImGui::Button("Inject File")) {
                    LuaExecutor::ExecuteFile(std::string(g_path));
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}