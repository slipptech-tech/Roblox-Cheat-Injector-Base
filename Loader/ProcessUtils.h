#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>

namespace ProcessUtils {
    DWORD FindProcessId(const std::wstring& name);
    uintptr_t GetModuleBase(DWORD pid, const std::wstring& moduleName);
    bool EnableDebugPrivilege();
}