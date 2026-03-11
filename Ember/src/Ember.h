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
#include "Ember/Render/Camera.h"
#include "Ember/Render/Mesh.h"

#include "Ember/ImGui/ImGuiLayer.h"

#include "Ember/Event/Event.h"

#include "Ember/ECS/Registry.h"
#include "Ember/ECS/Component/Components.h"

#include "Ember/Scene/Entity.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Scene/Behavior.h"