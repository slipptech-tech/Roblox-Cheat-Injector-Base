#include "LuaExecutor.h"
#include "Memory.hpp"
#include "Offsets.hpp"
#include "Signature.hpp"
#include "Signatures.h"
#include "../Common/Logger.hpp"
#include <Windows.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>

namespace LuaExecutor {

    using lua_State = void;
    using luaL_loadstring_t = int(__cdecl*)(lua_State*, const char*);
    using luaL_loadbuffer_t = int(__cdecl*)(lua_State*, const char*, size_t, const char*);
    using lua_pcall_t = int(__cdecl*)(lua_State*, int, int, int);

    static uintptr_t g_luaState = 0;
    static luaL_loadstring_t g_loadstring = nullptr;
    static luaL_loadbuffer_t g_loadbuffer = nullptr;
    static lua_pcall_t g_pcall = nullptr;
    static std::mutex g_mtx;

    static bool ResolveLuaState() {
        if (!Signatures::ScriptContext) return false;
        uintptr_t candidate = Memory::Read<uintptr_t>(Signatures::LuaState);
        if (candidate && candidate > 0x10000) {
            g_luaState = candidate;
            return true;
        }
        g_luaState = Memory::Read<uintptr_t>(Signatures::ScriptContext);
        return g_luaState != 0;
    }

    bool Init() {
        HMODULE roblox = GetModuleHandleW(nullptr);
        if (!roblox) {
            Logger::Error("LuaExecutor: roblox module missing");
            return false;
        }

        if (!Signatures::ScriptContext || !Signatures::luaL_loadstring || !Signatures::lua_pcall) {
            Logger::Warn("LuaExecutor: signatures not resolved, skipping");
            return false;
        }

        if (!ResolveLuaState()) {
            Logger::Error("LuaExecutor: failed to resolve lua_State");
            return false;
        }

        g_loadstring = reinterpret_cast<luaL_loadstring_t>(Signatures::luaL_loadstring);
        g_pcall      = reinterpret_cast<lua_pcall_t>(Signatures::lua_pcall);

        if (auto hit = Sig::Find(roblox,
                "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 49 8B E8")) {
            g_loadbuffer = reinterpret_cast<luaL_loadbuffer_t>(*hit);
        }

        Logger::Info("LuaExecutor ready. lua_State = 0x" + std::to_string(g_luaState));
        return true;
    }

    bool RefreshState() {
        std::lock_guard<std::mutex> lk(g_mtx);
        return ResolveLuaState();
    }

    uintptr_t GetLuaState() { return g_luaState; }

    bool Execute(const std::string& script) {
        std::lock_guard<std::mutex> lk(g_mtx);

        if (!g_luaState || !g_loadstring || !g_pcall) {
            Logger::Error("LuaExecutor::Execute: not initialized");
            return false;
        }

        auto L = reinterpret_cast<lua_State*>(g_luaState);

        if (g_loadstring(L, script.c_str()) != 0) {
            Logger::Error("luaL_loadstring failed (syntax error?)");
            return false;
        }

        if (g_pcall(L, 0, 0, 0) != 0) {
            Logger::Error("lua_pcall failed (runtime error)");
            return false;
        }

        return true;
    }

    bool ExecuteFile(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) {
            Logger::Error("ExecuteFile: cannot open " + path);
            return false;
        }

        std::stringstream ss;
        ss << f.rdbuf();
        std::string code = ss.str();

        if (g_loadbuffer) {
            std::lock_guard<std::mutex> lk(g_mtx);
            auto L = reinterpret_cast<lua_State*>(g_luaState);
            if (g_loadbuffer(L, code.data(), code.size(), path.c_str()) != 0) {
                Logger::Error("luaL_loadbuffer failed");
                return false;
            }
            if (g_pcall(L, 0, 0, 0) != 0) {
                Logger::Error("lua_pcall failed");
                return false;
            }
            return true;
        }

        return Execute(code);
    }

    void ExecuteAsync(const std::string& script) {
        std::thread([script]() { Execute(script); }).detach();
    }
}