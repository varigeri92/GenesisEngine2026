#pragma once
#include <cstdint>
#include "API.h"

namespace gns{
	struct Handle {
		static constexpr uint64_t Invalid = static_cast<uint64_t>(-1);
		GNS_API static Handle New();
		GNS_API static Handle Create(uint64_t handle);

		constexpr Handle() noexcept = default;
		constexpr Handle(const Handle&) noexcept = default;
		constexpr Handle& operator=(const Handle&) noexcept = default;
		constexpr Handle(Handle&& other) noexcept = default;
		constexpr Handle& operator=(Handle&& other) noexcept = default;
		~Handle() = default;

		const uint64_t Get()  const noexcept { return m_handle; };

		bool IsValid() const
		{
			return m_handle != Handle::Invalid;
		}

		inline bool operator==(const Handle& other) const noexcept
		{
			return m_handle == other.m_handle;
		}

		inline bool operator!=(const Handle& other) const noexcept
		{
			return m_handle != other.m_handle;
		}

	private:
		uint64_t m_handle = Invalid;
	private:
		Handle(uint64_t handle);
	};
}

namespace std
{
	template<>
	struct hash<gns::Handle>
	{
		size_t operator()(const gns::Handle& h) const noexcept
		{
			return h.Get();
		}
	};
}


namespace gns {

	template<typename T>
	struct Reference
	{
		Handle m_handle;
		size_t typeID;
		
		Reference(Handle handle):m_handle(handle) {
			typeID = typeid(T).hash_code();
		}
	};
}