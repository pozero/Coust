#include <Coust.h>

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
            COUST_INFO("Delta Milisecond: {0}", ts.ToMiliSecond());
            if (Input::IsKeyDown(KeyCode::A))
                COUST_INFO("Key A Is Pressed");
        }
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
