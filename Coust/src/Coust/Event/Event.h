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
        static int GetNewestID() { return int(EventType.size()) - 1; }
        
    private:
        static const std::string& GetEventType(int type)
        {
            return EventType[type];
        }

        static bool IsInCategory(int type, const std::string& category);

    private:
        static std::vector<std::string> EventType;
        static std::vector<int> EventCategory;
        static std::vector<std::string> CategoryName;
    };


/* Only Register Final Event Class
   Please Register Event in a translation unit */
#define COUST_REGISTER_EVENT(name, categories)      static EventManager::EventRegisterar Registerar##name(#name, #categories); \
                                                    int name::Type = EventManager::GetNewestID();

// Declare Type Inside Event Class Declaration
#define COUST_EVENT_TYPE(name)        private: \
                                          static int Type; \
                                          int GetType() const override { return Type; }


    class Event
    {
    public:
        const std::string& GetName() const
        {
            return EventManager::GetEventType(GetType());
        }

        bool IsInCategory(const std::string& category)
        {
            return EventManager::IsInCategory(GetType(), category);
        }

        virtual std::string ToString() const 
        {
            return GetName();
        }

    private:
        virtual int GetType() const { return 0; }

    public:
        bool Handled = false;
    };

    inline std::ostream& operator<<(std::ostream& os, const Event& e)
    {
        return os << e.ToString();
    }
}