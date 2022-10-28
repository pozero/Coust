#pragma once

#include "Coust/Layer.h"

#include <imgui.h>

namespace Coust
{
    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer()
            : Layer("ImGuiLayer") {}
        ~ImGuiLayer() {}

        void OnAttach() override;
        void OnDetach() override;

        void OnUIRender() override;

        void Begin();
        void End();
    };
}