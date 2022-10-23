#pragma once

#include "Event.h"

namespace Coust
{
    class MouseButtonEvent : public Event
    {
    public:
        int GetButtonCode() const { return m_ButtonCode; }

        MouseButtonEvent() = delete;

    protected:
        MouseButtonEvent(int buttonCode) : m_ButtonCode(buttonCode) {}
    protected:
        int m_ButtonCode;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent
    {
        COUST_EVENT_TYPE(MouseButtonPressedEvent)
    public:
        MouseButtonPressedEvent(int buttonCode)
            : MouseButtonEvent(buttonCode) {}
        
        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonPressedEvent[ ButtonCode: " << m_ButtonCode << " ]";
            return ss.str();
        }
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
        COUST_EVENT_TYPE(MouseButtonReleasedEvent)
    public:
        MouseButtonReleasedEvent(int buttonCode)
            : MouseButtonEvent(buttonCode) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonReleasedEvent[ ButtonCode: " << m_ButtonCode << " ]";
            return ss.str();
        }

    };

    class MouseMovedEvent : public Event
    {
        COUST_EVENT_TYPE(MouseMovedEvent)

    public:
        MouseMovedEvent(float x, float y)
            : m_CursorX(x), m_CursorY(y) {}
        MouseMovedEvent() = delete;

        float GetX() const { return m_CursorX; }
        float GetY() const { return m_CursorY; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseMovedEvent[ Cursor.X: " << m_CursorX << ", Cursor.Y: " << m_CursorY << " ]";
            return ss.str();
        }

    private:
        float m_CursorX, m_CursorY;
    };

    class MouseScrolledEvent : public Event
    {
        COUST_EVENT_TYPE(MouseScrolledEvent)

    public:
        MouseScrolledEvent(float XOffset, float YOffset)
            : m_XOffset(XOffset), m_YOffset(YOffset) {}
        MouseScrolledEvent() = delete;

        float GetXOffset() const { return m_XOffset; }
        float GetYOffset() const { return m_YOffset; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseScrolledEvent[ XOffset: " << m_XOffset << ", YOffset: " << m_YOffset << " ]";
            return ss.str();
        }

    private:
        float m_XOffset, m_YOffset;
    };
}