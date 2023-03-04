#include "pch.h"

#include "Coust/Event/Event.h"

#include "Coust/Event/ApplicationEvent.h"
#include "Coust/Event/KeyEvent.h"
#include "Coust/Event/MouseEvent.h"

#include "Coust/Core/Application.h"

namespace Coust
{
    std::function<void(Event&)> EventBus::mainCallback{};

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

    EventManager& EventManager::GetInstance()
    {
        static EventManager manager{};
        return manager;
    }
}


