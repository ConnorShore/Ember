#pragma once

namespace Ember {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL
		};

	public:
		static API GetApi() { return s_Api; }

	private:
		static API s_Api;
	};

}