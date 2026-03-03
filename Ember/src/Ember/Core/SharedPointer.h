#pragma once

#include <atomic>
#include <iostream>
#include <type_traits>
#include <utility>

#include "Logger.h"

namespace Ember {

	/// -------------------------------------------------------------------------------------
	/// Shared Resource Implementation
	/// -------------------------------------------------------------------------------------

	class SharedResource
	{
	public:

		SharedResource() = default;
		virtual ~SharedResource() = default;

		void IncrementRefCount() const { ++m_RefCount; }
		void DecrementRefCount() const { --m_RefCount; }

		size_t GetRefCount() const { return m_RefCount.load(); }

	private:
		mutable std::atomic<size_t> m_RefCount = 0;
	};

	/// -------------------------------------------------------------------------------------
	/// Shared Ref Implementation
	/// -------------------------------------------------------------------------------------

	template<typename T>
	class SharedPtr
	{
	public:
		SharedPtr() = default;
		SharedPtr(std::nullptr_t ptr) : m_Ptr(nullptr) {}
		SharedPtr(T* ptr) : m_Ptr(ptr)
		{
			m_Ptr->IncrementRefCount();
		}

		template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		SharedPtr(const SharedPtr<U>& other)
			: m_Ptr(static_cast<T*>(other.m_Ptr))
		{
			IncrementRefCount();
		}

		SharedPtr(const SharedPtr<T>& other)
			: m_Ptr(other.m_Ptr)
		{
			IncrementRefCount();
		}

		SharedPtr(SharedPtr<T>&& other) noexcept
			: m_Ptr(other.m_Ptr)
		{
			other.m_Ptr = nullptr;
		}

		SharedPtr<T>& operator=(const SharedPtr<T>& other)
		{
			if (this != &other)
			{
				DecrementRefCount();

				m_Ptr = other.m_Ptr;
				if (m_Ptr)
				{
					m_Ptr->IncrementRefCount();
				}
			}
			return *this;
		}

		SharedPtr<T>& operator=(SharedPtr<T>&& other) noexcept
		{
			if (this != &other)
			{
				DecrementRefCount();
				m_Ptr = other.m_Ptr;
				other.m_Ptr = nullptr;
			}
			return *this;
		}

		template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		SharedPtr<T>& operator=(const SharedPtr<U>& other)
		{
			if (m_Ptr != other.m_Ptr)
			{
				DecrementRefCount();
				m_Ptr = static_cast<T*>(other.m_Ptr);
				IncrementRefCount();
			}
			return *this;
		}

		~SharedPtr()
		{
			DecrementRefCount();
		}

		template <typename... Args>
		static SharedPtr<T> Create(Args&&... args)
		{
			return SharedPtr<T>(new T(std::forward<Args>(args)...));
		}

		T* operator->() { return m_Ptr; }
		const T& operator*() const { return *m_Ptr; }

		bool operator==(const SharedPtr<T>& other) const { return m_Ptr == other.m_Ptr; }
		bool operator!=(const SharedPtr<T>& other) const { return m_Ptr != other.m_Ptr; }
		bool operator==(std::nullptr_t) const { return m_Ptr == nullptr; }
		bool operator!=(std::nullptr_t) const { return m_Ptr != nullptr; }

	private:
		template<typename U>
		friend class SharedPtr;

		void IncrementRefCount() const
		{
			if (m_Ptr)
			{
				m_Ptr->IncrementRefCount();
			}
		}

		void DecrementRefCount() const
		{
			if (m_Ptr)
			{
				m_Ptr->DecrementRefCount();
				if (m_Ptr->GetRefCount() == 0)
				{
					delete m_Ptr;
					m_Ptr = nullptr;
				}
			}
		}

	private:
		mutable T* m_Ptr = nullptr;
	};

}