#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <optional>

namespace Sig {

    inline std::vector<int> Parse(const std::string& pattern) {
        std::vector<int> bytes;
        std::string cur;
        for (char c : pattern) {
            if (c == ' ') {
                if (!cur.empty()) {
                    bytes.push_back(cur == "?" || cur == "??" ? -1 : std::stoi(cur, nullptr, 16));
                    cur.clear();
                }
            } else {
                cur += c;
            }
        }
        if (!cur.empty())
            bytes.push_back(cur == "?" || cur == "??" ? -1 : std::stoi(cur, nullptr, 16));
        return bytes;
    }

    inline std::optional<uintptr_t> Find(HMODULE module, const std::string& pattern) {
        auto bytes = Parse(pattern);
        if (bytes.empty()) return std::nullopt;

        auto base = reinterpret_cast<uint8_t*>(module);
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        size_t size = nt->OptionalHeader.SizeOfImage;

        for (size_t i = 0; i + bytes.size() < size; ++i) {
            bool ok = true;
            for (size_t j = 0; j < bytes.size(); ++j) {
                if (bytes[j] != -1 && base[i + j] != static_cast<uint8_t>(bytes[j])) {
                    ok = false;
                    break;
                }
            }
            if (ok) return reinterpret_cast<uintptr_t>(base + i);
        }
        return std::nullopt;
    }
}