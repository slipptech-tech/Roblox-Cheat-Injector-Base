#pragma once
#include <Windows.h>
#include <string>
#include <cstdint>

namespace ManualMap {

    // Маппит DLL в целевой процесс вручную, без LoadLibraryW.
    // pid         — PID процесса-цели
    // dllPath     — путь к DLL на диске
    // wipeHeaders — обнулить PE-заголовки в памяти после маппинга
    // unlinkPEB   — зарезервировано (полный unlink — отдельный модуль)
    bool Map(DWORD pid, const std::wstring& dllPath,
             bool wipeHeaders = true, bool unlinkPEB = false);

    uintptr_t GetRemoteModuleBase(HANDLE hProc, const std::wstring& moduleName);
    uintptr_t GetRemoteProcAddress(HANDLE hProc, const std::wstring& moduleName,
                                   const std::string& procName);
}