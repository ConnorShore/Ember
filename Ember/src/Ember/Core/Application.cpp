#include "ebpch.h"
#include "Application.h"
#include "Core.h"

#include <GLFW/glfw3.h>

namespace Ember {

	Ember::Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		EB_CORE_INFO("Application created!");
		s_Instance = this;
	}

	Application::~Application()
	{
		EB_CORE_INFO("Application destroyed!");
	}

	void Application::Run()
	{
		EB_CORE_INFO("Application running!");
		m_Running = true;

		GLFWwindow* window;

		/* Initialize the library */
		EB_CORE_ASSERT(glfwInit(), "Failed to initalize GLFW!");

		/* Create a windowed mode window and its OpenGL context */
		window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
		if (!window)
		{
			glfwTerminate();
			return;
		}

		/* Make the window's context current */
		glfwMakeContextCurrent(window);

		/* Loop until the user closes the window */
		while (!glfwWindowShouldClose(window))
		{
			/* Render here */
			glClear(GL_COLOR_BUFFER_BIT);

			/* Swap front and back buffers */
			glfwSwapBuffers(window);

			/* Poll for and process events */
			glfwPollEvents();
		}

		glfwTerminate();
		EB_CORE_INFO("Application stopped!");
	}
}