#include <Coust.h>

#include <imgui.h>

namespace Coust
{
    class HoorayEvent : public Event
    {
        COUST_EVENT_TYPE(HoorayEvent)
    public:
        HoorayEvent() = default;
        ~HoorayEvent() = default;

        std::string ToString() const override
        {
            return "Hooray!!!";
        }
    };

    COUST_REGISTER_EVENT(HoorayEvent, Dull)

    class ExampleLayer : public Layer
    {
    public:
        void OnAttach() override
        {
            EventBus::Publish(HoorayEvent{});
        }

        void OnEvent(Event& e) override
        {
            if (e.IsInCategory("Dull"))
                COUST_INFO(e);
        }

        void OnUpdate(const TimeStep& ts) override
        {
            m_DeltaTime = ts.ToMiliSecond();
        }

        void OnUIRender() override
        {
            // ImGui::Begin("Delta Time");
            // ImGui::Text("MiliSecond Per Frame: %f", m_DeltaTime);
            // ImGui::End();
        }

    private:
        float m_DeltaTime= 0;
    };

    class Coustol : public Application
    {
    public:
        Coustol() {}
        ~Coustol() {}
    };
    
    Application* CreateApplication()
    {
        Application* app = new Coustol();
        Layer* exampleLayer = new ExampleLayer();
        app->PushLayer(exampleLayer);
        return app;
    }
}
