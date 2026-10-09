#pragma once
#include <Windows.h>
#include <cstdio>
#include <string>

namespace Logger {
    inline void Init(const char* title) {
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
        SetConsoleTitleA(title);
    }

    inline void Info(const std::string& msg) {
        printf("[+] %s\n", msg.c_str());
    }

    inline void Warn(const std::string& msg) {
        printf("[!] %s\n", msg.c_str());
    }

    inline void Error(const std::string& msg) {
        printf("[-] %s\n", msg.c_str());
    }
}