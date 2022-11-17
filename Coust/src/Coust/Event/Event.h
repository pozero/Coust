#pragma once

#include "pch.h"

#include "Coust/Logger.h"

namespace Coust
{
    class EventManager
    {
    public:
        using mask = uint32_t;

        template<typename T>
        struct EventRegisterar
        {
            EventRegisterar(const std::string& name, const char* categories)
            {
                std::vector<std::string> parsedCategories;
                std::string s{ categories };
                for (char *p = s.data(), *nameBegin = nullptr;; ++p)
                {
                    char c = *p;
                    if (std::isalnum(c))
                    {
                        if (!nameBegin)
                            nameBegin = p;
                    }
                    else if (c == '\0')
                    {
                        if (nameBegin)
                            parsedCategories.emplace_back(nameBegin);
                        break;
                    }
                    else if (nameBegin)
                    {
                        *p = '\0';
                        parsedCategories.emplace_back(nameBegin);
                        nameBegin = nullptr;
                    }
                }

                mask eventCategory = 0;
                std::vector<std::string>& categoryName = GetInstance().CategoryName;
                std::vector<int>& eventCategories = GetInstance().EventCategory;
                for (const auto& category : parsedCategories)
                {
                    int categoryIndex = 0;
                    for (;; ++categoryIndex)
                    {
                        if (categoryIndex == categoryName.size())
                            categoryName.push_back(category);
                        if (categoryName[categoryIndex] == category)
                            break;
                    }
                    COUST_CORE_ASSERT(categoryIndex < 8 * sizeof(mask), "Too Much Event Category Defined");
                    eventCategory |= 1 << categoryIndex;
                }

                eventCategories.push_back(eventCategory);

                T::StaticType = (int) eventCategories.size() - 1;
            }
        };
    public:
        EventManager()
            : EventCategory(), CategoryName()
        {
            EventCategory.push_back(0);
        }
        
        bool IsInCategory(int type, const std::string& category);

        static EventManager& GetInstance();

    private:
        std::vector<int> EventCategory;
        std::vector<std::string> CategoryName;
    };


/* Only Register Final Event Class
   Please Register Event in a translation unit */
#define COUST_REGISTER_EVENT(name, categories)  int name::StaticType = 0; \
                                                static EventManager::EventRegisterar<name> Registerar##name(#name, #categories); \

// Declare Type Inside Event Class Declaration
#define COUST_EVENT_TYPE(name)              friend class EventManager; \
                                        public: \
                                            static int GetStaticType() { return StaticType; } \
                                            int GetType() const override { return StaticType; } \
                                        private: \
                                            static int StaticType;


    class Event
    {
    public:
        bool IsInCategory(const std::string& category)
        {
            return EventManager::GetInstance().IsInCategory(GetType(), category);
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

        static void Publish(Event&& e)
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