#include "Signatures.h"
#include "Signature.hpp"
#include "Memory.hpp"
#include "Offsets.hpp"
#include "../Common/Logger.hpp"
#include <string>

namespace Signatures {

    uintptr_t ScriptContext = 0;
    uintptr_t LuaState = 0;
    uintptr_t luaL_loadstring = 0;
    uintptr_t lua_pcall = 0;
    uintptr_t luaD_throw = 0;
    uintptr_t Camera = 0;
    uintptr_t CameraMatrix = 0;
    uintptr_t Players = 0;
    uintptr_t LocalPlayer = 0;
    uintptr_t HumanoidRootPart = 0;
    uintptr_t DataModel = 0;

    std::optional<uintptr_t> ResolveRip(uintptr_t addr, int offsetIdx, int instrLen) {
        int32_t disp = Memory::Read<int32_t>(addr + offsetIdx);
        if (disp == 0) return std::nullopt;
        return addr + instrLen + disp;
    }

    static HMODULE g_roblox = nullptr;

    bool Init() {
        g_roblox = GetModuleHandleW(nullptr);
        if (!g_roblox) {
            Logger::Error("Signatures: Roblox module not found");
            return false;
        }

        {
            auto hit = Sig::Find(g_roblox,
                "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 48 8B D8 48 85 C0 74");
            if (hit) {
                auto resolved = ResolveRip(*hit, 3, 7);
                if (resolved) ScriptContext = *resolved;
            }
        }
        {
            auto hit = Sig::Find(g_roblox,
                "48 8B 0D ? ? ? ? 48 8B 01 48 8B 80 ? ? ? ? FF D0");
            if (hit) {
                auto resolved = ResolveRip(*hit, 3, 7);
                if (resolved) LuaState = *resolved;
            }
        }
        {
            auto hit = Sig::Find(g_roblox,
                "48 89 5C 24 ? 57 48 83 EC 20 48 8B D9 48 8B FA 48 8B 0D ? ? ? ?");
            if (hit) luaL_loadstring = *hit;
        }
        {
            auto hit = Sig::Find(g_roblox,
                "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 48 8B D9 41 8B F0");
            if (hit) lua_pcall = *hit;
        }
        {
            auto hit = Sig::Find(g_roblox,
                "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 8B DA 48 8B F9 83 FA ?");
            if (hit) luaD_throw = *hit;
        }
        {
            auto hit = Sig::Find(g_roblox,
                "48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 01");
            if (hit) {
                auto resolved = ResolveRip(*hit, 3, 7);
                if (resolved) Camera = *resolved;
            }
        }
        {
            auto hit = Sig::Find(g_roblox,
                "F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 ? F3 0F 10 05 ? ? ? ?");
            if (hit) {
                auto resolved = ResolveRip(*hit, 4, 8);
                if (resolved) CameraMatrix = *resolved;
            }
        }
        {
            auto hit = Sig::Find(g_roblox,
                "48 8B 0D ? ? ? ? 48 8B 01 48 8B 80 ? ? ? ? FF D0 48 8B 0D ? ? ? ?");
            if (hit) {
                auto resolved = ResolveRip(*hit, 3, 7);
                if (resolved) Players = *resolved;
            }
        }
        {
            auto hit = Sig::Find(g_roblox,
                "48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 74 ? 48 8B 01");
            if (hit) {
                auto resolved = ResolveRip(*hit, 3, 7);
                if (resolved) LocalPlayer = *resolved;
            }
        }
        {
            auto hit = Sig::Find(g_roblox,
                "48 8B 87 ? ? ? ? 48 85 C0 74 ? 48 8B 40 ? 48 85 C0");
            if (hit) HumanoidRootPart = *hit;
        }
        {
            auto hit = Sig::Find(g_roblox,
                "48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 50");
            if (hit) {
                auto resolved = ResolveRip(*hit, 3, 7);
                if (resolved) DataModel = *resolved;
            }
        }

        Offsets::ScriptContext    = ScriptContext;
        Offsets::LuaState         = LuaState;
        Offsets::luaL_loadstring  = luaL_loadstring - reinterpret_cast<uintptr_t>(g_roblox);
        Offsets::lua_pcall        = lua_pcall - reinterpret_cast<uintptr_t>(g_roblox);
        Offsets::luaD_throw       = luaD_throw - reinterpret_cast<uintptr_t>(g_roblox);
        Offsets::Camera           = Camera;
        Offsets::CameraMatrix     = CameraMatrix;
        Offsets::Players          = Players;
        Offsets::LocalPlayer      = LocalPlayer;
        Offsets::HumanoidRootPart = HumanoidRootPart;
        Offsets::DataModel        = DataModel;

        Logger::Info("Signatures resolved:");
        Logger::Info("  ScriptContext    = 0x" + std::to_string(ScriptContext));
        Logger::Info("  LuaState         = 0x" + std::to_string(LuaState));
        Logger::Info("  luaL_loadstring  = 0x" + std::to_string(luaL_loadstring));
        Logger::Info("  lua_pcall        = 0x" + std::to_string(lua_pcall));
        Logger::Info("  Camera           = 0x" + std::to_string(Camera));
        Logger::Info("  DataModel        = 0x" + std::to_string(DataModel));

        return ScriptContext && LuaState && luaL_loadstring && lua_pcall;
    }

    void Refresh() {
        ScriptContext = LuaState = luaL_loadstring = lua_pcall = 0;
        luaD_throw = Camera = CameraMatrix = Players = 0;
        LocalPlayer = HumanoidRootPart = DataModel = 0;
        Init();
    }
}