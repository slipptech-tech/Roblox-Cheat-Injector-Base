#pragma once
#include <cstdint>

// Все оффсеты требуют обновления под текущую версию Roblox.
// Значения ниже — шаблон. Реальные берутся сканированием сигнатур
// или из дампов (например, roblox-ts / rbxoffsets).

namespace Offsets {
    // DataModel
    inline uintptr_t DataModel = 0x0;

    // ScriptContext
    inline uintptr_t ScriptContext = 0x0;

    // lua_State внутри ScriptContext
    inline uintptr_t LuaState = 0x0;

    // Luau VM функции
    inline uintptr_t luaD_throw = 0x0;
    inline uintptr_t luaL_loadstring = 0x0;
    inline uintptr_t lua_pcall = 0x0;

    // Camera
    inline uintptr_t Camera = 0x0;
    inline uintptr_t CameraMatrix = 0x0;

    // Players / LocalPlayer
    inline uintptr_t Players = 0x0;
    inline uintptr_t LocalPlayer = 0x0;

    // Humanoid / HRP
    inline uintptr_t HumanoidRootPart = 0x0;
}