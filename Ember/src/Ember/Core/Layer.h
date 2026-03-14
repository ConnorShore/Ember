#pragma once

#include "Core.h"
#include "Ember/Event/Event.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Texture.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"

#include <string>

namespace Ember {

	class Layer
	{
	public:
		Layer(const std::string& name)
			: m_Name(name) { }
		virtual ~Layer() { }

		virtual void OnAttach() {}
		virtual void OnDetach() {};
		virtual void OnEvent(Event& event) {}
		virtual void OnUpdate(TimeStep delta) {};
		virtual void OnImGuiRender(TimeStep delta) {};

		inline const std::string& GetName() const { return m_Name; }

	protected:
		const SharedPtr<Shader>& RegisterShader(const std::string& filePath);
		const SharedPtr<Shader>& GetShader(const std::string& name);

		const SharedPtr<Texture>& RegisterTexture(const std::string& filePath);
		const SharedPtr<Texture>& GetTexture(const std::string& name);

		const SharedPtr<Mesh>& RegisterMesh(const std::string& filePath);
		const SharedPtr<Mesh>& GetMesh(const std::string& name);

		const SharedPtr<Material>& RegisterMaterial(const std::string& name);
		const SharedPtr<Material>& RegisterMaterial(const std::string& name, SharedPtr<Shader> shader);
		const SharedPtr<Material>& RegisterMaterial(const std::string& name, SharedPtr<Shader> shader, std::initializer_list<MaterialUniform> uniforms);
		const SharedPtr<Material>& RegisterMaterial(const std::string& name, std::initializer_list<MaterialUniform> uniforms);
		const SharedPtr<MaterialInstance>& RegisterMaterial(const std::string& name, SharedPtr<Material> material);
		const SharedPtr<MaterialBase>& GetMaterial(const std::string& name);

	private:
		// TODO: Make m_Name debug only?
		std::string m_Name;
	};

}