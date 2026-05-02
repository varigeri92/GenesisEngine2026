#pragma once
#include <cstdint>

namespace gns
{
	struct ScreenSize
	{
		uint32_t width = 0;
		uint32_t height = 0;

		bool IsValid() const
		{
			return width > 0 && height > 0;
		}
	};

	struct ScreenPosition
	{
		uint32_t x = 0;
		uint32_t y = 0;
	};

	class Screen
	{
	public:
		Screen() = default;

		Screen(uint32_t width, uint32_t height)
			: m_size{ width, height }
		{
		}

		Screen(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
			: m_position{ x, y }, m_size{ width, height }
		{
		}

		void SetPosition(uint32_t x, uint32_t y)
		{
			m_position = { x, y };
		}

		void SetSize(uint32_t width, uint32_t height)
		{
			m_size = { width, height };
		}

		void SetRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			m_position = { x, y };
			m_size = { width, height };
		}

		ScreenPosition GetPosition() const
		{
			return m_position;
		}

		ScreenSize GetSize() const
		{
			return m_size;
		}

		uint32_t GetX() const
		{
			return m_position.x;
		}

		uint32_t GetY() const
		{
			return m_position.y;
		}

		uint32_t GetWidth() const
		{
			return m_size.width;
		}

		uint32_t GetHeight() const
		{
			return m_size.height;
		}

		float GetAspectRatio(float fallback = 1.0f) const
		{
			if (!m_size.IsValid())
			{
				return fallback;
			}

			return static_cast<float>(m_size.width) / static_cast<float>(m_size.height);
		}

		bool IsValid() const
		{
			return m_size.IsValid();
		}

		bool operator==(const Screen& other) const
		{
			return m_position.x == other.m_position.x &&
				m_position.y == other.m_position.y &&
				m_size.width == other.m_size.width &&
				m_size.height == other.m_size.height;
		}

		bool operator!=(const Screen& other) const
		{
			return !(*this == other);
		}

	private:
		ScreenPosition m_position = {};
		ScreenSize m_size = {};
	};
}
