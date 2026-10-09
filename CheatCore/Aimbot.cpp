#include "Aimbot.h"
#include "../Common/Config.hpp"
#include <Windows.h>
#include <cmath>

namespace Aimbot {

    void Update() {
        if (!Config::g_cfg.aimbotEnabled) return;
        if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) return;

        // Заглушка. Реальная логика:
        // 1. Найти Camera CFrame через DataModel.
        // 2. Найти ближайшего игрока в FOV.
        // 3. Рассчитать дельту углов.
        // 4. Записать новый CFrame в Camera (или mouse move).
        //
        // Ниже — демонстрационное движение мыши по кругу,
        // чтобы показать, что поток живой.

        static float t = 0.f;
        t += 0.05f;
        int dx = static_cast<int>(std::cos(t) * Config::g_cfg.aimbotSmooth);
        int dy = static_cast<int>(std::sin(t) * Config::g_cfg.aimbotSmooth);
        mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);
    }
}