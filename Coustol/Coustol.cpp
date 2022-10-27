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
