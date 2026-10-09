#pragma once
#include <string>
#include <cstdint>

namespace LuaExecutor {
    bool Init();
    bool RefreshState();                  // перечитывает lua_State из ScriptContext
    bool Execute(const std::string& script);
    bool ExecuteFile(const std::string& path);
    uintptr_t GetLuaState();

    // Исполнение в отдельном потоке, чтобы не лагало на тяжёлых скриптах
    void ExecuteAsync(const std::string& script);
}