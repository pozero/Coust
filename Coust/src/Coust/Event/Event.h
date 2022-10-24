#pragma once

// #include <vector>
// #include <iostream>

#include "pch.h"

namespace Coust
{
    class EventManager
    {
        friend class Event;
    public:
        using mask = uint32_t;

        struct EventRegisterar
        {
            EventRegisterar(const std::string& name, const char* categories);
        };
        static int GetNewestID() { return int(EventCategory.size()) - 1; }
        
    private:
        static bool IsInCategory(int type, const std::string& category);

    private:
        static std::vector<int> EventCategory;
        static std::vector<std::string> CategoryName;
    };


/* Only Register Final Event Class
   Please Register Event in a translation unit */
#define COUST_REGISTER_EVENT(name, categories)      static EventManager::EventRegisterar Registerar##name(#name, #categories); \
                                                    int name::StaticType = EventManager::GetNewestID();

// Declare Type Inside Event Class Declaration
#define COUST_EVENT_TYPE(name)        public: \
                                          static int GetStaticType() { return StaticType; } \
                                          int GetType() const override { return StaticType; } \
                                      private: \
                                          static int StaticType;


    class Event
    {
    public:
        bool IsInCategory(const std::string& category)
        {
            return EventManager::IsInCategory(GetType(), category);
        }

        virtual std::string ToString() const = 0;

        virtual int GetType() const { return 0; }

    public:
        bool Handled = false;
    };

    inline std::ostream& operator<<(std::ostream& os, const Event& e)
    {
        return os << e.ToString();
    }

    class EventBus
    {
        friend class Application;
    public:
        static void Publish(Event& e)
        {
            mainCallback(e);
        }

        template<typename T>
        static bool Dispatch(Event& e, const std::function<bool(T&)>& f)
        {
            if (e.GetType() == T::GetStaticType())
            {
                e.Handled = f(*(T*)&e);
                return true;
            }
            return false;
        }

    private:
        static void Subscribe(const std::function<void(Event&)>& f)
        {
            mainCallback = f;
        }

    private:
        static std::function<void(Event&)> mainCallback;
    };
}