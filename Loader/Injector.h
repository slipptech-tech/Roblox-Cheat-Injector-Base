#pragma once
#include <Windows.h>
#include <string>

namespace Injector {
    bool InjectDll(DWORD pid, const std::wstring& dllPath);
}