#pragma once

#include <Ember.h>

namespace Ember {

	class RuntimeLayer : public Layer
	{
	public:
		RuntimeLayer() : Layer("Runtime Layer") {}
		virtual ~RuntimeLayer() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta) override;
		void OnEvent(Event& event) override;

	private:
		SharedPtr<Scene> m_ActiveScene;
	};

}