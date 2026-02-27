#pragma once

namespace Ember {

	template <typename T>
	class ScopedPtr
	{
	public:
		ScopedPtr() = default;
		ScopedPtr(const ScopedPtr& ptr) = delete;

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

		T& operator*() const { return *m_Ptr; }
		T* operator->() const { return m_Ptr; }

		ScopedPtr& operator=(const ScopedPtr& ptr) = delete;
		ScopedPtr& operator=(ScopedPtr&& other) = delete;
		bool operator==(const ScopedPtr& other) const { return m_Ptr == other.m_Ptr; }
		bool operator!=(const ScopedPtr& other) const { return m_Ptr != other.m_Ptr; }


	private:
		explicit ScopedPtr(T* ptr) : m_Ptr(ptr) {}
		T* m_Ptr = nullptr;
	};

}