#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include "../External/json/json.hpp"

struct CheatConfig {
    bool espEnabled = false;
    bool aimbotEnabled = false;
    float aimbotFov = 90.f;
    float aimbotSmooth = 5.f;
    bool menuOpen = true;
};

namespace Config {
    inline CheatConfig g_cfg;

    inline std::string Path() {
        char* appdata = nullptr;
        size_t len = 0;
        _dupenv_s(&appdata, &len, "APPDATA");
        std::string p = std::string(appdata) + "\\RBX_Injector";
        free(appdata);
        std::filesystem::create_directories(p);
        return p + "\\config.json";
    }

    inline void Save() {
        nlohmann::json j;
        j["espEnabled"] = g_cfg.espEnabled;
        j["aimbotEnabled"] = g_cfg.aimbotEnabled;
        j["aimbotFov"] = g_cfg.aimbotFov;
        j["aimbotSmooth"] = g_cfg.aimbotSmooth;
        std::ofstream f(Path());
        f << j.dump(4);
    }

    inline void Load() {
        std::ifstream f(Path());
        if (!f.is_open()) return;
        nlohmann::json j;
        f >> j;
        g_cfg.espEnabled = j.value("espEnabled", false);
        g_cfg.aimbotEnabled = j.value("aimbotEnabled", false);
        g_cfg.aimbotFov = j.value("aimbotFov", 90.f);
        g_cfg.aimbotSmooth = j.value("aimbotSmooth", 5.f);
    }
}