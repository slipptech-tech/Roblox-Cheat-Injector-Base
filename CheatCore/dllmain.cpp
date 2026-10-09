#include <Windows.h>
#include "Hooks.h"
#include "../Common/Logger.hpp"

static HMODULE g_self = nullptr;

DWORD WINAPI MainThread(LPVOID) {
    Logger::Init("CheatCore");
    Logger::Info("CheatCore loaded");

    Hooks::Init();

    while (!(GetAsyncKeyState(VK_END) & 1)) {
        Sleep(10);
    }

    Hooks::Shutdown();
    Logger::Info("CheatCore unloaded");

    FreeLibraryAndExitThread(g_self, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = hModule;
        DisableThreadLibraryCalls(hModule);
        HANDLE h = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
        if (h) CloseHandle(h);
    }
    return TRUE;
}