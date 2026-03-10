#pragma once

#include <utility>

#include "Logger.h"

namespace Ember {

	template <typename T>
	class ScopedPtr
	{
	public:
		ScopedPtr() = default;
		ScopedPtr(std::nullptr_t ptr) : m_Ptr(nullptr) {}
		ScopedPtr(const ScopedPtr& ptr) = delete;
		explicit ScopedPtr(T* ptr) : m_Ptr(ptr) {}

		ScopedPtr(ScopedPtr&& ptr) noexcept : m_Ptr(ptr.m_Ptr)
		{
			ptr.m_Ptr = nullptr;
		}

		template <typename U>
		ScopedPtr(ScopedPtr<U>&& ptr) noexcept : m_Ptr(ptr.m_Ptr)
		{
			ptr.m_Ptr = nullptr;
		}

		~ScopedPtr()
		{
			if (m_Ptr)
			{
				delete m_Ptr;
			}
		}

		template <typename... Args>
		static ScopedPtr<T> Create(Args&&... args)
		{
			return ScopedPtr<T>(new T(std::forward<Args>(args)...));
		}

		void Reset()
		{
			if (m_Ptr)
			{
				delete m_Ptr;
				m_Ptr = nullptr;
			}
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

		template <typename U>
		ScopedPtr& operator=(ScopedPtr<U>&& ptr) noexcept
		{
			delete m_Ptr;
			m_Ptr = ptr.m_Ptr;
			ptr.m_Ptr = nullptr;
			return *this;
		}

		T& operator*() const { return *m_Ptr; }
		T* operator->() const { return m_Ptr; }

		bool operator==(const ScopedPtr& other) const { return m_Ptr == other.m_Ptr; }
		bool operator!=(const ScopedPtr& other) const { return m_Ptr != other.m_Ptr; }
		bool operator!() const { return m_Ptr == nullptr; }

	private:
		template <typename U>
		friend class ScopedPtr;

		T* m_Ptr = nullptr;
	};

}