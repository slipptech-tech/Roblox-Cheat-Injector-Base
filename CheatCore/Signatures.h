#pragma once
#include <Windows.h>
#include <optional>
#include <cstdint>

namespace Signatures {
    bool Init();                     // сканирует все сигнатуры один раз при загрузке
    void Refresh();                  // повторный скан (по кнопке)

    std::optional<uintptr_t> ResolveRip(uintptr_t addr, int offsetIdx, int instrLen);

    // Найденные адреса
    extern uintptr_t ScriptContext;
    extern uintptr_t LuaState;
    extern uintptr_t luaL_loadstring;
    extern uintptr_t lua_pcall;
    extern uintptr_t luaD_throw;
    extern uintptr_t Camera;
    extern uintptr_t CameraMatrix;
    extern uintptr_t Players;
    extern uintptr_t LocalPlayer;
    extern uintptr_t HumanoidRootPart;
    extern uintptr_t DataModel;
}