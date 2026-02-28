#pragma once

#include <utility>

#include "Logger.h"

namespace Ember {

	template <typename T>
	class ScopedPtr
	{
	public:
		ScopedPtr() = default;
		ScopedPtr(const ScopedPtr& ptr) = delete;
		explicit ScopedPtr(T* ptr) : m_Ptr(ptr) {}

		~ScopedPtr()
		{
			delete m_Ptr;
			EB_CORE_INFO("Scoped pointer destroyed!");
		}

		template <typename... Args>
		static ScopedPtr<T> Create(Args&&... args)
		{
			return ScopedPtr<T>(new T(std::forward<Args>(args)...));
		}

		T* Ptr() const { return m_Ptr; }

		ScopedPtr& operator=(const ScopedPtr& ptr) = delete;
		ScopedPtr& operator=(ScopedPtr&& ptr) noexcept
		{
			if (this != &ptr)
			{
				delete m_Ptr;
				m_Ptr = ptr.m_Ptr;
				ptr.m_Ptr = nullptr;
			}
			return *this;
		}

		T& operator*() const { return *m_Ptr; }
		T* operator->() const { return m_Ptr; }

		bool operator==(const ScopedPtr& other) const { return m_Ptr == other.m_Ptr; }
		bool operator!=(const ScopedPtr& other) const { return m_Ptr != other.m_Ptr; }
		bool operator!() const { return m_Ptr == nullptr; }

	private:
		T* m_Ptr = nullptr;
	};

}