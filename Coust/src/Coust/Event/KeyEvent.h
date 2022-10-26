#pragma once

#include "Coust/Event/Event.h"

namespace Coust
{
	class KeyEvent : public Event
	{
	public:
		int GetKeyCode() const { return m_KeyCode; }

		KeyEvent() = delete;

	protected:
		KeyEvent(int keyCode) : m_KeyCode(keyCode) {}

	protected:
		int m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
		COUST_EVENT_TYPE(KeyPressedEvent)

	public:
		KeyPressedEvent(int keyCode, bool isRepeated)
			: KeyEvent(keyCode), m_IsRepeated(isRepeated) {}

		bool IsRepeated() const { return m_IsRepeated; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent[ KeyCode: " << m_KeyCode << ", Repeated: " << std::boolalpha << m_IsRepeated << " ]";
			return ss.str();
		}

	private:
		bool m_IsRepeated;
	};

	class KeyReleasedEvent : public KeyEvent
	{
		COUST_EVENT_TYPE(KeyReleasedEvent)

	public:
		KeyReleasedEvent(int keyCode)
			: KeyEvent(keyCode) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent[ KeyCode: " << m_KeyCode << " ]";
			return ss.str();
		}
	};
}