#pragma once

#include "Ember/Core/Application.h"

/*
 *  Only include in main application file:
 * 
 *	#include "Ember/Core/EntryPoint.h" 
 * 
*/

#include "Ember/Core/Window.h"
#include "Ember/Core/Layer.h"

#include "Ember/ImGui/ImGuiLayer.h"

#include "Ember/Core/Logger.h"
#include "Ember/Core/Time.h"

#include "Ember/Core/ScopedPointer.h"
#include "Ember/Core/SharedPointer.h"

#include "Ember/Input/Input.h"
#include "Ember/Input/InputCode.h"

#include "Ember/Math/Math.h"

#include "Ember/Render/Buffer.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/Camera.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"
#include "Ember/Render/PrimitiveGenerator.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/UniformBuffer.h"

#include "Ember/Render/VFX/PostProcessPass.h"
#include "Ember/Render/VFX/BloomPass.h"
#include "Ember/Render/VFX/OutlinePass.h"

#include "Ember/Event/Event.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/Event/WindowEvent.h"

#include "Ember/ECS/Registry.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/RenderSystem.h"

#include "Ember/Scene/Entity.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Scene/Behavior.h"
#include "Ember/Scene/SceneSerializer.h"

#include "Ember/Asset/UUID.h"
#include "Ember/Asset/Asset.h"
#include "Ember/Asset/Model.h"
#include "Ember/Asset/AssetManager.h"

#include "Ember/Utils/PlatformUtil.h"

#include "Ember/Core/Project.h"
#include "Ember/Core/ProjectManager.h"

// Vendor includes
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <assimp/Importer.hpp>