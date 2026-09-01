#include "ebpch.h"
#include "ProjectSerializer.h"

#include "Ember/Utils/SerializationUtils.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/Input/InputCodeNames.h"
#include "Ember/Input/Input.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

namespace Ember {

	namespace {

		void WriteStickSettings(ryml::NodeRef parent, const char* key, const StickSettings& settings)
		{
			auto node = parent[ryml::to_csubstr(key)];
			node |= ryml::MAP;
			node["Deadzone"] << settings.Deadzone;
			node["Saturation"] << settings.Saturation;
			node["Exponent"] << settings.Exponent;
			node["Actuation"] << settings.Actuation;
			node["InvertX"] << settings.InvertX;
			node["InvertY"] << settings.InvertY;
		}

		void WriteTriggerSettings(ryml::NodeRef parent, const char* key, const TriggerSettings& settings)
		{
			auto node = parent[ryml::to_csubstr(key)];
			node |= ryml::MAP;
			node["Deadzone"] << settings.Deadzone;
			node["Saturation"] << settings.Saturation;
			node["Exponent"] << settings.Exponent;
			node["Actuation"] << settings.Actuation;
		}

		// A missing block keeps the struct defaults, which are the neutral pre-conditioning values.
		template<typename NodeType>
		void ReadStickSettings(const NodeType& parent, const char* key, StickSettings& settings)
		{
			if (!parent.has_child(ryml::to_csubstr(key)))
				return;

			auto node = parent[ryml::to_csubstr(key)];
			Util::ReadField(node, "Deadzone", settings.Deadzone);
			Util::ReadField(node, "Saturation", settings.Saturation);
			Util::ReadField(node, "Exponent", settings.Exponent);
			Util::ReadField(node, "Actuation", settings.Actuation);
			Util::ReadField(node, "InvertX", settings.InvertX);
			Util::ReadField(node, "InvertY", settings.InvertY);
		}

		template<typename NodeType>
		void ReadTriggerSettings(const NodeType& parent, const char* key, TriggerSettings& settings)
		{
			if (!parent.has_child(ryml::to_csubstr(key)))
				return;

			auto node = parent[ryml::to_csubstr(key)];
			Util::ReadField(node, "Deadzone", settings.Deadzone);
			Util::ReadField(node, "Saturation", settings.Saturation);
			Util::ReadField(node, "Exponent", settings.Exponent);
			Util::ReadField(node, "Actuation", settings.Actuation);
		}

	}

	bool ProjectSerializer::Serialize(const std::string& filePath)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["Project"] << m_Project->GetConfig().ProjectName;
		root["EngineVersion"] << m_Project->GetConfig().EngineVersion;
		root["SceneDirectory"] << m_Project->GetConfig().SceneDirectory;
		root["AssetDirectory"] << m_Project->GetConfig().AssetDirectory;
		root["StartScene"] << m_Project->GetConfig().StartScene;
		root["AssetFile"] << m_Project->GetConfig().AssetFile;

		// Settings
		auto settingsNode = root["Settings"];
		settingsNode |= ryml::MAP;

		// General settings
		auto generalNode = settingsNode["General"];
		generalNode |= ryml::MAP;

		auto scenesInBuildNode = generalNode["ScenesInBuild"];
		scenesInBuildNode |= ryml::SEQ;
		auto& scenesInBuild = m_Project->GetScenesInBuild();
		for (const auto& scene : scenesInBuild)
		{
			auto sceneEntryNode = scenesInBuildNode.append_child();
			sceneEntryNode |= ryml::MAP;
			sceneEntryNode["UUID"] << scene;
		}

		// Physics Settings
		auto& physicsSettings = Application::Instance().GetSystem<PhysicsSystem>()->GetSettings();
		auto physicsNode = root["Settings"]["Physics"];
		physicsNode |= ryml::MAP;

		physicsNode["GravityStrength"] << physicsSettings.GravityStrength;
		ryml::NodeRef gravityVectorNode = physicsNode["GravityVector"];
		Util::SerializeVector3f(gravityVectorNode, physicsSettings.GravityVector);
		physicsNode["UpdateRate"] << physicsSettings.UpdateRate;
		physicsNode["PositionSolverIterations"] << physicsSettings.PositionSolverIterations;
		physicsNode["VelocitySolverIterations"] << physicsSettings.VelocitySolverIterations;

		// Physics Collider Filters
		auto filterManager = m_Project->GetCollisionFilterManager();
		auto filterNode = physicsNode["CollisionFilters"];
		filterNode |= ryml::SEQ;
		{
			const auto& collisionSlots = filterManager.GetAllSlots();
			for (uint32_t i = 0; i < FilterManager::MaxSlots; i++)
			{
				if (collisionSlots[i].empty())
					continue;

				auto filterEntryNode = filterNode.append_child();
				filterEntryNode |= ryml::MAP;
				filterEntryNode["Index"] << i;
				filterEntryNode["Name"] << collisionSlots[i];
			}
		}

		// Render settings
		auto renderLayerManager = m_Project->GetRenderFilterManager();
		auto renderNode = settingsNode["Render"];
		renderNode |= ryml::MAP;
		
		auto renderLayerNode = renderNode["RenderLayers"];
		renderLayerNode |= ryml::SEQ;
		{
			const auto& renderSlots = renderLayerManager.GetAllSlots();
			for (uint32_t i = 0; i < FilterManager::MaxSlots; i++)
			{
				if (renderSlots[i].empty())
					continue;

				auto layerEntryNode = renderLayerNode.append_child();
				layerEntryNode |= ryml::MAP;
				layerEntryNode["Index"] << i;
				layerEntryNode["Name"] << renderSlots[i];
			}
		}

		// Input Settings
		auto& inputActionManager = Application::Instance().GetInputActionManager();
		auto inputNode = settingsNode["Input"];
		inputNode |= ryml::MAP;

		WriteStickSettings(inputNode, "LeftStick", Input::GetStickSettings(GamepadStick::Left));
		WriteStickSettings(inputNode, "RightStick", Input::GetStickSettings(GamepadStick::Right));
		WriteTriggerSettings(inputNode, "LeftTrigger", Input::GetTriggerSettings(GamepadTrigger::Left));
		WriteTriggerSettings(inputNode, "RightTrigger", Input::GetTriggerSettings(GamepadTrigger::Right));

		auto mouseNode = inputNode["Mouse"];
		mouseNode |= ryml::MAP;
		mouseNode["InvertX"] << Input::GetMouseSettings().InvertX;
		mouseNode["InvertY"] << Input::GetMouseSettings().InvertY;

		auto actionsNode = inputNode["Actions"];
		actionsNode |= ryml::SEQ;
		for (const auto& action : inputActionManager.GetActions())
		{
			auto actionNode = actionsNode.append_child();
			actionNode |= ryml::MAP;
			actionNode["Name"] << action.Name;

			auto triggersNode = actionNode["Triggers"];
			triggersNode |= ryml::SEQ;
			for (const auto& trigger : action.Triggers)
			{
				// The trigger string carries the device, the control type and the modifiers together;
				// a bare control id cannot, since a gamepad button and axis share the same integers.
				const std::string triggerText = InputCodeNames::TriggerToString(trigger);
				if (triggerText.empty())
				{
					EB_CORE_WARN("ProjectSerializer::Serialize: Dropping unserializable trigger on action '{0}'", action.Name);
					continue;
				}

				auto triggerNode = triggersNode.append_child();
				triggerNode |= ryml::MAP;
				triggerNode["Trigger"] << triggerText;
			}
		}

		std::ofstream fout(filePath);
		fout << tree;
		fout.close();

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::string& filePath)
	{
		std::ifstream stream(filePath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open scene file: {0}", filePath);
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		if (!root.has_child("Project"))
			return false;

		auto& config = m_Project->m_Config;
		root["Project"] >> config.ProjectName;
		root["EngineVersion"] >> config.EngineVersion;
		root["StartScene"] >> config.StartScene;
		root["AssetDirectory"] >> config.AssetDirectory;

		// Settings
		if (root.has_child("Settings"))
		{
			auto settingsNode = root["Settings"];

			// General Settings
			auto generalNode = settingsNode["General"];

			// Scenes in build
			auto scenesInBuildNode = generalNode["ScenesInBuild"];
			std::vector<UUID> scenesInBuild;
			for (auto sceneNode : scenesInBuildNode.children())
			{
				uint64_t sceneUUID;
				sceneNode["UUID"] >> sceneUUID;
				scenesInBuild.push_back((UUID)sceneUUID);
			}
			m_Project->SetScenesInBuild(scenesInBuild);

			// Physics Settings
			auto& physicsSettings = Application::Instance().GetSystem<PhysicsSystem>()->GetSettings();
			auto physicsNode = settingsNode["Physics"];
			physicsNode["GravityStrength"] >> physicsSettings.GravityStrength;
			auto gravityVectorNode = physicsNode["GravityVector"];
			Util::DeserializeVector3f(gravityVectorNode, physicsSettings.GravityVector);
			physicsNode["UpdateRate"] >> physicsSettings.UpdateRate;
			physicsNode["PositionSolverIterations"] >> physicsSettings.PositionSolverIterations;
			physicsNode["VelocitySolverIterations"] >> physicsSettings.VelocitySolverIterations;

			// Physics Collider Filters
			auto& collisionFilterManager = m_Project->GetCollisionFilterManager();
			std::array<std::string, FilterManager::MaxSlots> filters{};

			uint32_t fallbackIndex = 0;
			for (auto filterNode : physicsNode["CollisionFilters"].children())
			{
				std::string filterName;
				filterNode["Name"] >> filterName;

				uint32_t slotIndex = fallbackIndex;
				Util::ReadField(filterNode, "Index", slotIndex);

				if (slotIndex < FilterManager::MaxSlots)
					filters[slotIndex] = filterName;

				fallbackIndex++;
			}

			collisionFilterManager.InitWithFilters(filters);

			// Render settings
			auto& renderLayerManager = m_Project->GetRenderFilterManager();
			std::array<std::string, FilterManager::MaxSlots> renderLayers{};

			if (settingsNode.has_child("Render"))
			{
				auto renderNode = settingsNode["Render"];

				uint32_t fallbackIndex = 0;
				for (auto renderLayerNode : renderNode["RenderLayers"].children())
				{
					std::string layerName;
					renderLayerNode["Name"] >> layerName;

					uint32_t slotIndex = fallbackIndex;
					Util::ReadField(renderLayerNode, "Index", slotIndex);

					if (slotIndex < FilterManager::MaxSlots)
						renderLayers[slotIndex] = layerName;

					fallbackIndex++;
				}
			}

			renderLayerManager.InitWithFilters(renderLayers);

			// Input Settings
			auto& inputActionManager = Application::Instance().GetInputActionManager();
			inputActionManager.ClearActions();

			// Reset first so a project that omits a block gets the defaults rather than whatever the
			// previously opened project left behind.
			Input::GetSettings() = InputSettings{};

			if (settingsNode.has_child("Input"))
			{
				auto inputNode = settingsNode["Input"];

				ReadStickSettings(inputNode, "LeftStick", Input::GetStickSettings(GamepadStick::Left));
				ReadStickSettings(inputNode, "RightStick", Input::GetStickSettings(GamepadStick::Right));
				ReadTriggerSettings(inputNode, "LeftTrigger", Input::GetTriggerSettings(GamepadTrigger::Left));
				ReadTriggerSettings(inputNode, "RightTrigger", Input::GetTriggerSettings(GamepadTrigger::Right));

				if (inputNode.has_child("Mouse"))
				{
					auto mouseNode = inputNode["Mouse"];
					Util::ReadField(mouseNode, "InvertX", Input::GetMouseSettings().InvertX);
					Util::ReadField(mouseNode, "InvertY", Input::GetMouseSettings().InvertY);
				}

				if (inputNode.has_child("Actions"))
				{
					auto actionsNode = inputNode["Actions"];
					for (auto actionNode : actionsNode.children())
					{
						InputAction action;
						actionNode["Name"] >> action.Name;
						auto triggersNode = actionNode["Triggers"];
						for (auto triggerNode : triggersNode.children())
						{
							std::string triggerText;
							triggerNode["Trigger"] >> triggerText;

							// A binding that no longer names a real control is dropped rather than kept as
							// an unbound trigger, which would silently fire on nothing.
							InputTrigger trigger;
							if (!InputCodeNames::TriggerFromString(triggerText, trigger))
							{
								EB_CORE_ERROR("ProjectSerializer::Deserialize: Unrecognized trigger '{0}' on action '{1}'", triggerText, action.Name);
								continue;
							}

							action.Triggers.push_back(trigger);
						}
						inputActionManager.AddAction(action);
					}
				}
			}
		}

		m_Project->m_ProjectDirectory = std::filesystem::path(filePath).parent_path();

		return true;
	}

}