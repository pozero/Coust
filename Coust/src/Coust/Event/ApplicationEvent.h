#pragma once

#include "Event.h"

namespace Coust
{
    class WindowResizedEvent : public Event
    {
        COUST_EVENT_TYPE(WindowResizedEvent)

    public:
        WindowResizedEvent(unsigned int width, unsigned int height)
            : m_Width(width), m_Height(height) {}
        WindowResizedEvent() = delete;

        unsigned int GetWidth() const { return m_Width; }
        unsigned int GetHeight() const { return m_Height; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizedEvent[ " << "Width: " << m_Width << ", Height: " << m_Height << " ]";
            return ss.str();
        }
    private:
        unsigned int m_Width, m_Height;
    };

    class WindowClosedEvent : public Event
    {
        COUST_EVENT_TYPE(WindowClosedEvent)

    public:
        WindowClosedEvent() = default;
    };
}