#include "ManualMap.h"
#include "../Common/Logger.hpp"
#include <vector>
#include <fstream>
#include <TlHelp32.h>

namespace ManualMap {

    static std::vector<uint8_t> ReadFileBytes(const std::wstring& path) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open()) return {};
        std::streamsize size = f.tellg();
        f.seekg(0, std::ios::beg);
        std::vector<uint8_t> buf(static_cast<size_t>(size));
        f.read(reinterpret_cast<char*>(buf.data()), size);
        return buf;
    }

    uintptr_t GetRemoteModuleBase(HANDLE hProc, const std::wstring& moduleName) {
        HANDLE snap = CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
            GetProcessId(hProc));
        if (snap == INVALID_HANDLE_VALUE) return 0;

        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        uintptr_t base = 0;
        if (Module32FirstW(snap, &me)) {
            do {
                if (_wcsicmp(me.szModule, moduleName.c_str()) == 0) {
                    base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                    break;
                }
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
        return base;
    }

    uintptr_t GetRemoteProcAddress(HANDLE hProc, const std::wstring& moduleName,
                                   const std::string& procName) {
        HMODULE local = GetModuleHandleW(moduleName.c_str());
        if (!local) {
            local = LoadLibraryW(moduleName.c_str());
            if (!local) return 0;
        }
        uintptr_t localAddr = reinterpret_cast<uintptr_t>(
            GetProcAddress(local, procName.c_str()));
        if (!localAddr) return 0;
        uintptr_t localBase = reinterpret_cast<uintptr_t>(local);
        uintptr_t rva = localAddr - localBase;

        uintptr_t remoteBase = GetRemoteModuleBase(hProc, moduleName);
        if (!remoteBase) return 0;
        return remoteBase + rva;
    }

    bool Map(DWORD pid, const std::wstring& dllPath,
             bool wipeHeaders, bool /*unlinkPEB*/) {

        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!hProc) {
            Logger::Error("ManualMap: OpenProcess failed " + std::to_string(GetLastError()));
            return false;
        }

        auto file = ReadFileBytes(dllPath);
        if (file.empty()) {
            Logger::Error("ManualMap: cannot read DLL file");
            CloseHandle(hProc);
            return false;
        }

        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(file.data());
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            Logger::Error("ManualMap: bad DOS signature");
            CloseHandle(hProc);
            return false;
        }
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(file.data() + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) {
            Logger::Error("ManualMap: bad NT signature");
            CloseHandle(hProc);
            return false;
        }

        SIZE_T imageSize = nt->OptionalHeader.SizeOfImage;
        uintptr_t preferredBase = nt->OptionalHeader.ImageBase;

        LPVOID remoteBase = VirtualAllocEx(
            hProc,
            reinterpret_cast<LPVOID>(preferredBase),
            imageSize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE);

        bool relocated = false;
        if (!remoteBase) {
            remoteBase = VirtualAllocEx(hProc, nullptr, imageSize,
                                        MEM_COMMIT | MEM_RESERVE,
                                        PAGE_EXECUTE_READWRITE);
            if (!remoteBase) {
                Logger::Error("ManualMap: VirtualAllocEx failed " + std::to_string(GetLastError()));
                CloseHandle(hProc);
                return false;
            }
            relocated = true;
        }

        uintptr_t delta = reinterpret_cast<uintptr_t>(remoteBase) - preferredBase;

        // Заголовки
        if (!WriteProcessMemory(hProc, remoteBase, file.data(),
                                nt->OptionalHeader.SizeOfHeaders, nullptr)) {
            Logger::Error("ManualMap: write headers failed");
            VirtualFreeEx(hProc, remoteBase, 0, MEM_RELEASE);
            CloseHandle(hProc);
            return false;
        }

        // Секции
        auto section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
            if (section->SizeOfRawData == 0) continue;
            LPVOID dest = reinterpret_cast<LPVOID>(
                reinterpret_cast<uintptr_t>(remoteBase) + section->VirtualAddress);
            LPCVOID src = file.data() + section->PointerToRawData;
            WriteProcessMemory(hProc, dest, src, section->SizeOfRawData, nullptr);
        }

        // Релокации
        if (relocated && (nt->OptionalHeader.DllCharacteristics
                          & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)) {
            auto relocDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
            if (relocDir.Size > 0) {
                auto reloc = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
                    file.data() + relocDir.VirtualAddress);
                auto relocEnd = reinterpret_cast<uintptr_t>(reloc) + relocDir.Size;

                while (reinterpret_cast<uintptr_t>(reloc) < relocEnd
                       && reloc->SizeOfBlock > 0) {
                    DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                    WORD* list = reinterpret_cast<WORD*>(reloc + 1);
                    for (DWORD k = 0; k < count; ++k) {
                        WORD type = list[k] >> 12;
                        WORD offset = list[k] & 0x0FFF;
                        if (type == IMAGE_REL_BASED_DIR64) {
                            uintptr_t patchAddr = reinterpret_cast<uintptr_t>(remoteBase)
                                                + reloc->VirtualAddress + offset;
                            uint64_t orig = 0;
                            ReadProcessMemory(hProc,
                                reinterpret_cast<LPCVOID>(patchAddr),
                                &orig, sizeof(orig), nullptr);
                            orig += delta;
                            WriteProcessMemory(hProc,
                                reinterpret_cast<LPVOID>(patchAddr),
                                &orig, sizeof(orig), nullptr);
                        }
                    }
                    reloc = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
                        reinterpret_cast<uintptr_t>(reloc) + reloc->SizeOfBlock);
                }
            }
        }

        // Импорты
        auto impDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (impDir.Size > 0) {
            auto import = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
                file.data() + impDir.VirtualAddress);
            while (import->Name) {
                std::string modName = reinterpret_cast<char*>(
                    file.data() + import->Name);
                std::wstring wMod(modName.begin(), modName.end());

                uintptr_t remoteModBase = GetRemoteModuleBase(hProc, wMod);

                if (!remoteModBase) {
                    SIZE_T sz = (wMod.size() + 1) * sizeof(wchar_t);
                    LPVOID arg = VirtualAllocEx(hProc, nullptr, sz,
                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                    WriteProcessMemory(hProc, arg, wMod.c_str(), sz, nullptr);
                    HANDLE th = CreateRemoteThread(hProc, nullptr, 0,
                        reinterpret_cast<LPTHREAD_START_ROUTINE>(
                            GetProcAddress(GetModuleHandleW(L"kernel32.dll"),
                                           "LoadLibraryW")),
                        arg, 0, nullptr);
                    if (th) {
                        WaitForSingleObject(th, 10000);
                        CloseHandle(th);
                    }
                    VirtualFreeEx(hProc, arg, 0, MEM_RELEASE);
                    remoteModBase = GetRemoteModuleBase(hProc, wMod);
                    if (!remoteModBase) {
                        Logger::Error("ManualMap: cannot resolve module " + modName);
                        VirtualFreeEx(hProc, remoteBase, 0, MEM_RELEASE);
                        CloseHandle(hProc);
                        return false;
                    }
                }

                auto thunkOrig = reinterpret_cast<IMAGE_THUNK_DATA*>(
                    file.data() + import->OriginalFirstThunk);
                auto thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(
                    file.data() + import->FirstThunk);

                while (thunkOrig->u1.AddressOfData) {
                    uintptr_t funcAddr = 0;

                    HMODULE localMod = GetModuleHandleW(wMod.c_str());
                    if (!localMod) localMod = LoadLibraryW(wMod.c_str());

                    if (thunkOrig->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
                        WORD ord = static_cast<WORD>(thunkOrig->u1.Ordinal & 0xFFFF);
                        funcAddr = remoteModBase + (reinterpret_cast<uintptr_t>(
                            GetProcAddress(localMod, MAKEINTRESOURCEA(ord)))
                            - reinterpret_cast<uintptr_t>(localMod));
                    } else {
                        auto ibn = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
                            file.data() + thunkOrig->u1.AddressOfData);
                        funcAddr = remoteModBase + (reinterpret_cast<uintptr_t>(
                            GetProcAddress(localMod, ibn->Name))
                            - reinterpret_cast<uintptr_t>(localMod));
                    }

                    WriteProcessMemory(hProc,
                        reinterpret_cast<LPVOID>(
                            reinterpret_cast<uintptr_t>(thunk)
                            - reinterpret_cast<uintptr_t>(file.data())),
                        &funcAddr, sizeof(funcAddr), nullptr);

                    ++thunkOrig;
                    ++thunk;
                }
                ++import;
            }
        }

        // Затирание заголовков
        if (wipeHeaders) {
            std::vector<uint8_t> zeros(nt->OptionalHeader.SizeOfHeaders, 0);
            WriteProcessMemory(hProc, remoteBase, zeros.data(),
                               zeros.size(), nullptr);
        }

        // Корректный вызов DllMain через shellcode
        struct ShellParams {
            uintptr_t imageBase;
            uintptr_t entryPoint;
            DWORD     reason;
        } params;
        params.imageBase  = reinterpret_cast<uintptr_t>(remoteBase);
        params.entryPoint = reinterpret_cast<uintptr_t>(remoteBase)
                          + nt->OptionalHeader.AddressOfEntryPoint;
        params.reason     = DLL_PROCESS_ATTACH;

        const uint8_t shell[] = {
            0x49, 0x89, 0xCA,             // mov r10, rcx
            0x49, 0x8B, 0x0A,             // mov rcx, [r10]
            0x41, 0x8B, 0x52, 0x10,       // mov edx, [r10+0x10]
            0x4D, 0x31, 0xC0,             // xor r8, r8
            0x49, 0x8B, 0x42, 0x08,       // mov rax, [r10+0x08]
            0x48, 0x83, 0xEC, 0x28,       // sub rsp, 0x28
            0xFF, 0xD0,                   // call rax
            0x48, 0x83, 0xC4, 0x28,       // add rsp, 0x28
            0xC3                          // ret
        };

        LPVOID remoteParams = VirtualAllocEx(hProc, nullptr, sizeof(ShellParams),
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        WriteProcessMemory(hProc, remoteParams, &params, sizeof(params), nullptr);

        LPVOID remoteShell = VirtualAllocEx(hProc, nullptr, sizeof(shell),
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        WriteProcessMemory(hProc, remoteShell, shell, sizeof(shell), nullptr);

        HANDLE hThread = CreateRemoteThread(
            hProc, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteShell),
            remoteParams,
            0, nullptr);

        if (!hThread) {
            Logger::Error("ManualMap: CreateRemoteThread failed " + std::to_string(GetLastError()));
            VirtualFreeEx(hProc, remoteParams, 0, MEM_RELEASE);
            VirtualFreeEx(hProc, remoteShell, 0, MEM_RELEASE);
            VirtualFreeEx(hProc, remoteBase, 0, MEM_RELEASE);
            CloseHandle(hProc);
            return false;
        }

        WaitForSingleObject(hThread, 10000);
        CloseHandle(hThread);
        VirtualFreeEx(hProc, remoteShell, 0, MEM_RELEASE);
        VirtualFreeEx(hProc, remoteParams, 0, MEM_RELEASE);

        Logger::Info("ManualMap: DLL mapped at 0x" +
                     std::to_string(reinterpret_cast<uintptr_t>(remoteBase)));

        CloseHandle(hProc);
        return true;
    }
}