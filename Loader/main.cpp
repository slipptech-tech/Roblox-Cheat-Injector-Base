#include <Windows.h>
#include <string>
#include <filesystem>
#include <exception>
#include "ProcessUtils.h"
#include "Injector.h"
#include "ManualMap.h"
#include "../Common/Logger.hpp"

static std::string W2A(const std::wstring& w) {
    if (w.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                   nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string out(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                        out.data(), size, nullptr, nullptr);
    return out;
}

int wmain(int argc, wchar_t** argv) {
    Logger::Init("RBX_Injector Loader");
    Logger::Info("Loader started");

    __try {
        if (!ProcessUtils::EnableDebugPrivilege())
            Logger::Warn("SeDebugPrivilege not enabled");

        std::wstring dllPath;
        if (argc >= 2 && argv[1] && argv[1][0]) {
            dllPath = argv[1];
        } else {
            wchar_t buf[MAX_PATH] = {};
            if (GetModuleFileNameW(nullptr, buf, MAX_PATH) == 0) {
                Logger::Error("GetModuleFileNameW failed");
                system("pause");
                return 1;
            }
            std::filesystem::path exe(buf);
            dllPath = (exe.parent_path() / "CheatCore.dll").wstring();
        }

        Logger::Info("DLL path: " + W2A(dllPath));

        std::error_code ec;
        if (!std::filesystem::exists(dllPath, ec) || ec) {
            Logger::Error("DLL not found: " + W2A(dllPath));
            system("pause");
            return 1;
        }

        Logger::Info("Waiting for RobloxPlayerBeta.exe...");

        DWORD pid = 0;
        int tries = 0;
        while (!pid && tries < 600) {   // ~5 минут ожидания
            pid = ProcessUtils::FindProcessId(L"RobloxPlayerBeta.exe");
            if (!pid) Sleep(500);
            ++tries;
        }

        if (!pid) {
            Logger::Error("Roblox process not found within timeout");
            system("pause");
            return 1;
        }

        Logger::Info("Process found. PID: " + std::to_string(pid));

        if (ManualMap::Map(pid, dllPath, /*wipeHeaders=*/true, /*unlinkPEB=*/false)) {
            Logger::Info("Injection successful (manual map)");
        } else {
            Logger::Error("Injection failed");
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Logger::Error("Unhandled SEH exception: code " +
                      std::to_string(GetExceptionCode()));
    }

    system("pause");
    return 0;
}