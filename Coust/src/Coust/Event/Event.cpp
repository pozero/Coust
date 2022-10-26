#include "pch.h"

#include "Event.h"

#include "ApplicationEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"

#include "Coust/Application.h"

namespace Coust
{
    /* Initialize EventManager & EventBus Static Member */
    std::vector<int> EventManager::EventCategory{ 0 };
    std::vector<std::string> EventManager::CategoryName{};

    std::function<void(Event&)> EventBus::mainCallback{};
    /* ************************************* */

    /* Register Built-in Events */
	COUST_REGISTER_EVENT(KeyPressedEvent,           Input | Keyboard)
	COUST_REGISTER_EVENT(KeyReleasedEvent,          Input | Keyboard)

	COUST_REGISTER_EVENT(WindowClosedEvent,         Application)
	COUST_REGISTER_EVENT(WindowResizedEvent,        Application)

	COUST_REGISTER_EVENT(MouseMovedEvent,           Input | Mouse)
	COUST_REGISTER_EVENT(MouseScrolledEvent,        Input | Mouse)
	COUST_REGISTER_EVENT(MouseButtonPressedEvent,   Input | Mouse)
	COUST_REGISTER_EVENT(MouseButtonReleasedEvent,  Input | Mouse)
    /* ************************ */


    bool EventManager::IsInCategory(int type, const std::string& category)
    {
        int categoryIndex = 0;
        for (;;++categoryIndex)
        {
            if (categoryIndex >= CategoryName.size())
            {
                COUST_CORE_ERROR("\"{0}\" Is Not A Valid Event Category Name", category);
                return false;
            }
            if (CategoryName[categoryIndex] == category)
                break;
        }
        int categoryMask = 1 << categoryIndex;
        int eventCategoryMask = EventCategory[type];
    
        return categoryMask & eventCategoryMask;
    }
    
    EventManager::EventRegisterar::EventRegisterar(const std::string& name, const char* categories)
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
        for (const auto& category : parsedCategories)
        {
            int categoryIndex = 0;
            for (;; ++categoryIndex)
            {
                if (categoryIndex == CategoryName.size())
                    CategoryName.push_back(category);
                if (CategoryName[categoryIndex] == category)
                    break;
            }
            COUST_CORE_ASSERT(categoryIndex < 8 * sizeof(mask), "Too Much Event Category Defined");
            eventCategory |= 1 << categoryIndex;
        }

        EventCategory.push_back(eventCategory);
    }

}


