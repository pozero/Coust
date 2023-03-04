#pragma once

#include <utility>

namespace Coust
{
    class Input
    {
    public:
        static bool IsKeyDown(int keyCode);

        static bool IsMouseButtonDown(int button);

        static std::pair<float, float> GetCursorPos();

        static float GetCursorPosX();
        static float GetCursorPosY();
    };
}