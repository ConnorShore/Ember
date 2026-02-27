#pragma once

namespace Ember {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		bool IsRunning() const { return m_Running; }

	private:
		static Application* s_Instance;
		bool m_Running = false;
	};

	Application* CreateApplication();
}

