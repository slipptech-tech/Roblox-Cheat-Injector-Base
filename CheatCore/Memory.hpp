#pragma once
#include <Windows.h>
#include <cstdint>
#include <optional>

namespace Memory {

    template <typename T>
    inline T Read(uintptr_t addr) {
        T buf{};
        __try {
            memcpy(&buf, reinterpret_cast<void*>(addr), sizeof(T));
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return buf;
    }

    template <typename T>
    inline bool Write(uintptr_t addr, const T& value) {
        __try {
            memcpy(reinterpret_cast<void*>(addr), &value, sizeof(T));
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline std::optional<uintptr_t> ReadPtr(uintptr_t addr) {
        uintptr_t v = Read<uintptr_t>(addr);
        if (!v || v < 0x10000 || v > 0x7FFFFFFFFFFF) return std::nullopt;
        return v;
    }

    template <typename T>
    inline std::optional<T> ReadSafe(uintptr_t addr) {
        if (!addr) return std::nullopt;
        T v = Read<T>(addr);
        return v;
    }
}