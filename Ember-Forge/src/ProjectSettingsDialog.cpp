#include "efpch.h"
#include "ProjectSettingsDialog.h"

#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <Ember/Core/ProjectManager.h>
#include <Ember/ECS/System/PhysicsSystem.h>
#include <Ember/Input/Input.h>
#include <Ember/Input/InputAction.h>
#include <Ember/Input/InputActionManager.h>
#include <Ember/Input/InputCodeNames.h>
#include <imgui/imgui.h>

#include <cctype>

namespace Ember {

	namespace {

		constexpr const char* s_TriggerConfigPopupName = "Configure Trigger";
		constexpr const char* s_RemoveActionPopupName = "Remove Input Action";

		// Every key the picker offers, generated from the one list in KeyCodes.inl so a new key
		// shows up here without the editor having to know about it.
		constexpr KeyCode s_PickerKeys[] =
		{
#define EB_KEY(name, value) KeyCode::name,
#include <Ember/Input/KeyCodes.inl>
#undef EB_KEY
		};

		// Same idea for the mouse, read from MouseControls.inl.
		constexpr MouseControl s_PickerMouseControls[] =
		{
#define EB_MOUSE_CONTROL(name, value) MouseControl::name,
#include <Ember/Input/MouseControls.inl>
#undef EB_MOUSE_CONTROL
		};

		constexpr GamepadButton s_PickerGamepadButtons[] =
		{
#define EB_GAMEPAD_BUTTON(name, value) GamepadButton::name,
#include <Ember/Input/GamepadButton.inl>
#undef EB_GAMEPAD_BUTTON
		};

		struct GamepadAxisEntry
		{
			GamepadAxis Axis;
			AxisDirection Direction;
		};

		// One row per pickable axis binding, so a new axis needs a row here. A stick can be bound whole
		// for a signed read or as either half; a trigger only ever moves one way.
		constexpr GamepadAxisEntry s_PickerGamepadAxes[] =
		{
			{ GamepadAxis::LeftX,  AxisDirection::Full },
			{ GamepadAxis::LeftX,  AxisDirection::Negative },
			{ GamepadAxis::LeftX,  AxisDirection::Positive },
			{ GamepadAxis::LeftY,  AxisDirection::Full },
			{ GamepadAxis::LeftY,  AxisDirection::Negative },
			{ GamepadAxis::LeftY,  AxisDirection::Positive },
			{ GamepadAxis::RightX, AxisDirection::Full },
			{ GamepadAxis::RightX, AxisDirection::Negative },
			{ GamepadAxis::RightX, AxisDirection::Positive },
			{ GamepadAxis::RightY, AxisDirection::Full },
			{ GamepadAxis::RightY, AxisDirection::Negative },
			{ GamepadAxis::RightY, AxisDirection::Positive },
			{ GamepadAxis::LeftTrigger,  AxisDirection::Full },
			{ GamepadAxis::RightTrigger, AxisDirection::Full },
		};

		constexpr std::pair<KeyModifier, const char*> s_ModifierToggles[] =
		{
			{ KeyModifier::Control, "Ctrl" },
			{ KeyModifier::Shift, "Shift" },
			{ KeyModifier::Alt, "Alt" },
			{ KeyModifier::Super, "Super" },
		};

		// A blank name would make an action nothing can look up, so the UI refuses to add one.
		bool IsBlank(std::string_view text)
		{
			return text.find_first_not_of(" \t") == std::string_view::npos;
		}

		bool ActionNameExists(const std::vector<InputAction>& actions, std::string_view name)
		{
			return std::any_of(actions.begin(), actions.end(), [name](const InputAction& action) { return action.Name == name; });
		}

		// Case-insensitive substring match so the picker search is forgiving about capitals.
		bool MatchesSearch(std::string_view name, std::string_view search)
		{
			if (search.empty())
				return true;

			auto equalsIgnoreCase = [](char a, char b) {
				return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
				};

			return std::search(name.begin(), name.end(), search.begin(), search.end(), equalsIgnoreCase) != name.end();
		}

		// Moves the cursor so the widgets that follow end flush with the right edge of the row.
		void RightAlignCursor(float width)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Math::Max(0.0f, ImGui::GetContentRegionAvail().x - width));
		}

	}

	void ProjectSettingsDialog::OnImGuiRender()
	{
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		// Set a fixed window size so the split layout has room to breathe
		ImGui::SetNextWindowSize(ImVec2(700, 450), ImGuiCond_Appearing);

		bool isOpen = true;
		if (ImGui::BeginPopupModal(m_PopupName.c_str(), &isOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize))
		{
			if (!isOpen)
			{
				ImGui::CloseCurrentPopup();
				return;
			}

			ImVec2 contentRegion = ImGui::GetContentRegionAvail();
			float splitHeight = contentRegion.y;

			// Left pane (Category List)
			ImGui::BeginChild("CategoryPane", ImVec2(150, splitHeight), true);
			RenderLeftPane();
			ImGui::EndChild();

			ImGui::SameLine();

			// Right pane (Content)
			ImGui::BeginChild("ContentPane", ImVec2(0, splitHeight), true);
			RenderRightPane();
			ImGui::EndChild();

			// Nested modals are drawn out here so they are never parented to a child window
			RenderTriggerConfigPopup();
			RenderRemoveActionPopup();

			ImGui::EndPopup();
		}
	}

	void ProjectSettingsDialog::RenderLeftPane()
	{
		if (ImGui::Selectable("General", m_SelectedCategory == Category::General))
			m_SelectedCategory = Category::General;

		if (ImGui::Selectable("Input", m_SelectedCategory == Category::Input))
			m_SelectedCategory = Category::Input;

		if (ImGui::Selectable("Physics", m_SelectedCategory == Category::Physics))
			m_SelectedCategory = Category::Physics;

		if (ImGui::Selectable("Rendering", m_SelectedCategory == Category::Rendering))
			m_SelectedCategory = Category::Rendering;
	}

	void ProjectSettingsDialog::RenderRightPane()
	{
		switch (m_SelectedCategory)
		{
		case Category::General: RenderGeneralSettings(); break;
		case Category::Input: RenderInputSettings(); break;
		case Category::Physics: RenderPhysicsSettings(); break;
		case Category::Rendering: RenderRenderingSettings(); break;
		}
	}

	void ProjectSettingsDialog::RenderGeneralSettings()
	{
		ImGui::Text("General Settings");
		ImGui::Separator();
		ImGui::Spacing();

		std::string projName = ProjectManager::GetActive()->GetConfig().ProjectName;
		static char projectName[128] = "";

		// Set projectName to projName's value
		for (size_t i = 0; i < IM_ARRAYSIZE(projectName); i++)
			projectName[i] = i < projName.size() ? projName[i] : '\0';

		if (ImGui::InputText("Project Name", projectName, IM_ARRAYSIZE(projectName)))
		{
			// TODO: Handle project renaming (do we need to delete old project file and save a new one? Or just update the name in the config and save on next project save?)
			ProjectManager::GetActive()->GetConfig().ProjectName = std::string(projectName);
		}

		// List order of scenes (like waypoints UI style) (so can call SceneManager.LoadNextScene() in lua)
		ImGui::Spacing();
		ImGui::Text("Scenes In Build");
		ImGui::Separator();

		if (UI::PropertyGrid::Begin("BuildScenesGrid"))
		{
			// 1. Tell the widget how to get a name
			auto nameResolver = [](UUID uuid) -> std::string {
				auto asset = Application::Instance().GetAssetManager().GetAssetBase(uuid);
				return asset ? asset->GetName() : "Invalid Scene";
				};

			// 2. Gather all available scenes so the dropdown has items to display!
			// (Assuming you have a GetAssetsOfType method or similar)
			auto scenes = Application::Instance().GetAssetManager().GetAssetsOfType<Scene>();
			std::vector<UUID> availableScenes;
			for (auto& scene : scenes)
			{
				availableScenes.push_back(scene->GetUUID());
			}

			// 3. Get the project config list to modify
			auto project = ProjectManager::GetActive();
			auto& buildScenes = project->GetScenesInBuild();

			// 4. Draw the ComboBox array
			UI::PropertyGrid::DynamicUUIDArrayComboBox("Scenes", "Index", buildScenes, availableScenes, nameResolver);

			// Set first build scene as the default scene in the project (TODO)
			if (!buildScenes.empty())
			{
				project->SetStartScene(buildScenes[0]);
			}

			UI::PropertyGrid::End();
		}

	}

	void ProjectSettingsDialog::RenderPhysicsSettings()
	{
		ImGui::Text("Physics 3D");
		ImGui::Separator();
		ImGui::Spacing();

		auto& physicsSettings = Application::Instance().GetSystem<PhysicsSystem>()->GetSettings();

		ImGui::TextDisabled("Simulation");
		if (UI::PropertyGrid::Begin("Physics Simulation Settings"))
		{
			UI::PropertyGrid::Float("Default Gravity Strength", physicsSettings.GravityStrength, 0.01f, 0.0f, 1000.0f);
			UI::PropertyGrid::Float3("Default Gravity Vector", physicsSettings.GravityVector, 0.01f, -1.0f, 1.0f);

			UI::PropertyGrid::End();
		}

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::TextDisabled("Solver");
		if (UI::PropertyGrid::Begin("Physics Solver Settings"))
		{
			UI::PropertyGrid::UInt("Position Iterations", physicsSettings.PositionSolverIterations, 1, 1, 32);
			UI::PropertyGrid::UInt("Velocity Iterations", physicsSettings.VelocitySolverIterations, 1, 1, 32);

			UI::PropertyGrid::End();
		}

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::TextDisabled("Collision Categories");

		// Use a child window with a border so it looks clean inside the right pane.
		// ImVec2(0, 0) tells ImGui to fill the remaining width and height perfectly.
		ImGui::BeginChild("CollisionCategoriesSection", ImVec2(0, 0), true);

		auto& filterManager = ProjectManager::GetActive()->GetCollisionFilterManager();

		// ReactPhysics3D uses a 16-bit integer for masks, so we iterate exactly 16 times.
		for (uint32_t i = 0; i < 16; i++)
		{
			ImGui::PushID(i);

			// Fixed width for the label so the input boxes align perfectly into a neat column
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Layer %2d", i+1);
			ImGui::SameLine(80.0f);

			// Render an input box for the filter name, pre-filled with the current name from the filter manager
			std::string currentName = filterManager.GetFilterNameBySlot(i);
			char buffer[64];
			strncpy_s(buffer, sizeof(buffer), currentName.c_str(), _TRUNCATE);

			// Dynamically size the input box to stretch, but leave 60px for the Clear button
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);

			// Update the slot name instantly as the user types
			if (ImGui::InputText("##LayerName", buffer, sizeof(buffer)))
			{
				filterManager.SetFilterNameAtSlot(i, std::string(buffer));
			}

			// The Clear button quickly wipes the slot back to an empty string
			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				filterManager.SetFilterNameAtSlot(i, "");
			}

			ImGui::PopID();
		}

		ImGui::EndChild();

	}

	void ProjectSettingsDialog::RenderRenderingSettings()
	{
		ImGui::Text("Rendering");
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextDisabled("Render Layers");

		// Use a child window with a border so it looks clean inside the right pane.
		// ImVec2(0, 0) tells ImGui to fill the remaining width and height perfectly.
		ImGui::BeginChild("RenderLayersSection", ImVec2(0, 0), true);

		auto& filterManager = ProjectManager::GetActive()->GetRenderFilterManager();

		// ReactPhysics3D uses a 16-bit integer for masks, so we iterate exactly 16 times.
		for (uint32_t i = 0; i < 16; i++)
		{
			ImGui::PushID(i);

			// Fixed width for the label so the input boxes align perfectly into a neat column
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Layer %2d", i+1);
			ImGui::SameLine(80.0f);

			// Render textbox for the layer name, with a Clear button next to it
			std::string currentName = filterManager.GetFilterNameBySlot(i);
			char buffer[64];
			strncpy_s(buffer, sizeof(buffer), currentName.c_str(), _TRUNCATE);

			// Dynamically size the input box to stretch, but leave 60px for the Clear button
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);

			// Update the slot name instantly as the user types
			if (ImGui::InputText("##LayerName", buffer, sizeof(buffer)))
			{
				filterManager.SetFilterNameAtSlot(i, std::string(buffer));
			}

			// The Clear button quickly wipes the slot back to an empty string
			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				filterManager.SetFilterNameAtSlot(i, "");
			}

			ImGui::PopID();
		}

		ImGui::EndChild();
	}

	void ProjectSettingsDialog::RenderInputSettings()
	{
		ImGui::Text("Input");
		ImGui::Separator();
		ImGui::Spacing();

		RenderInputDeviceSettings();

		ImGui::Spacing();

		// Open by default, unlike Devices - the action list is what this page is mostly for.
		ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
		if (!ImGui::CollapsingHeader("Actions"))
			return;

		ImGui::Spacing();

		RenderAddActionRow();

		const auto& actions = m_InputActionManager.GetActions();

		ImGui::BeginChild("InputSection", ImVec2(0, 0), true);

		if (actions.empty())
			ImGui::TextDisabled("No input actions yet - name one above and press Add Action.");

		for (int i = 0; i < static_cast<int>(actions.size()); i++)
			RenderActionRow(i, actions[i]);

		ImGui::EndChild();
	}

	void ProjectSettingsDialog::RenderInputDeviceSettings()
	{
		if (!ImGui::CollapsingHeader("Devices"))
			return;

		ImGui::TextDisabled("Applied to every read of the control, before actions are evaluated.");
		ImGui::Spacing();

		RenderStickSettings("Left Stick", GamepadStick::Left, GamepadAxis::LeftX, GamepadAxis::LeftY);
		RenderStickSettings("Right Stick", GamepadStick::Right, GamepadAxis::RightX, GamepadAxis::RightY);
		RenderTriggerSettings("Left Trigger", GamepadTrigger::Left, GamepadAxis::LeftTrigger);
		RenderTriggerSettings("Right Trigger", GamepadTrigger::Right, GamepadAxis::RightTrigger);

		if (ImGui::TreeNodeEx("Mouse", ImGuiTreeNodeFlags_SpanFullWidth))
		{
			MouseSettings& settings = Input::GetMouseSettings();
			ImGui::Checkbox("Invert X##Mouse", &settings.InvertX);
			ImGui::Checkbox("Invert Y##Mouse", &settings.InvertY);
			ImGui::TextDisabled("Sensitivity belongs to the game - the mouse delta has several readers.");
			ImGui::TreePop();
		}
	}

	void ProjectSettingsDialog::RenderStickSettings(const char* label, GamepadStick stick, GamepadAxis xAxis, GamepadAxis yAxis)
	{
		ImGui::PushID(label);

		if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_SpanFullWidth))
		{
			StickSettings& settings = Input::GetStickSettings(stick);

			// Clamped on typed input too: a negative deadzone or a saturation under the deadzone
			// would collapse the span the curve is normalised against.
			ImGui::DragFloat("Deadzone", &settings.Deadzone, 0.005f, 0.0f, 0.9f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SetItemTooltip("Deflection below this reads as centred, measured on the pair rather than per axis.");

			ImGui::DragFloat("Saturation", &settings.Saturation, 0.005f, 0.1f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SetItemTooltip("Deflection that already counts as fully pushed - lower it for a stick that cannot reach 1.");

			ImGui::DragFloat("Exponent", &settings.Exponent, 0.01f, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SetItemTooltip("1 is linear. Above 1 gives finer control near centre, which is what FPS look wants.");

			ImGui::DragFloat("Actuation", &settings.Actuation, 0.005f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SetItemTooltip("How far the stick must physically travel before a digital read calls it pressed.");

			ImGui::Checkbox("Invert X", &settings.InvertX);
			ImGui::SameLine();
			ImGui::Checkbox("Invert Y", &settings.InvertY);

			RenderResponsePreview(settings, xAxis, yAxis);

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void ProjectSettingsDialog::RenderTriggerSettings(const char* label, GamepadTrigger trigger, GamepadAxis axis)
	{
		ImGui::PushID(label);

		if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_SpanFullWidth))
		{
			TriggerSettings& settings = Input::GetTriggerSettings(trigger);

			ImGui::DragFloat("Deadzone", &settings.Deadzone, 0.005f, 0.0f, 0.9f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("Saturation", &settings.Saturation, 0.005f, 0.1f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("Exponent", &settings.Exponent, 0.01f, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("Actuation", &settings.Actuation, 0.005f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

			ImGui::Text("Pull: %.3f", Input::GetGamepadAxis(0, axis));

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void ProjectSettingsDialog::RenderResponsePreview(const StickSettings& settings, GamepadAxis xAxis, GamepadAxis yAxis)
	{
		constexpr int sampleCount = 64;
		float curve[sampleCount];
		for (int i = 0; i < sampleCount; i++)
		{
			const float deflection = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
			curve[i] = Input::ShapeMagnitude(deflection, settings.Deadzone, settings.Saturation, settings.Exponent);
		}

		const float previewSize = ImGui::GetFontSize() * 6.0f;
		ImGui::PlotLines("##Curve", curve, sampleCount, 0, "Response", 0.0f, 1.0f, ImVec2(previewSize, previewSize));

		ImGui::SameLine();

		// The dots make the radial shaping legible: a circular sweep of the stick has to stay
		// circular, which a per-axis curve would not manage.
		const GamepadState& pad = Input::GetGamepadState(0);
		const Vector2f raw = { pad.RawAxis[static_cast<size_t>(xAxis)], pad.RawAxis[static_cast<size_t>(yAxis)] };
		const Vector2f conditioned = { pad.Axis[static_cast<size_t>(xAxis)], pad.Axis[static_cast<size_t>(yAxis)] };

		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const float radius = previewSize * 0.5f;
		const ImVec2 centre(origin.x + radius, origin.y + radius);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(origin, ImVec2(origin.x + previewSize, origin.y + previewSize), IM_COL32(120, 120, 120, 255));
		drawList->AddCircle(centre, radius * settings.Deadzone, IM_COL32(120, 120, 120, 160));
		drawList->AddCircleFilled(ImVec2(centre.x + raw.x * radius, centre.y - raw.y * radius), 3.0f, IM_COL32(150, 150, 150, 255));
		drawList->AddCircleFilled(ImVec2(centre.x + conditioned.x * radius, centre.y - conditioned.y * radius), 3.0f, IM_COL32(255, 170, 60, 255));

		ImGui::Dummy(ImVec2(previewSize, previewSize));
		ImGui::TextDisabled("Grey is raw, orange is conditioned.");
	}

	void ProjectSettingsDialog::RenderAddActionRow()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const auto& actions = m_InputActionManager.GetActions();

		// The button hugs the right edge and the name field takes whatever is left of the row.
		float buttonWidth = ImGui::CalcTextSize("Add Action").x + style.FramePadding.x * 2.0f;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - buttonWidth - style.ItemSpacing.x);

		// Keeps the caret in the field after a name is submitted so actions can be typed in a row.
		if (m_FocusNewActionField)
		{
			ImGui::SetKeyboardFocusHere();
			m_FocusNewActionField = false;
		}

		bool submitted = ImGui::InputTextWithHint("##NewActionName", "New action name", m_NewActionName, sizeof(m_NewActionName), ImGuiInputTextFlags_EnterReturnsTrue);

		bool isDuplicate = !IsBlank(m_NewActionName) && ActionNameExists(actions, m_NewActionName);
		bool canAdd = !IsBlank(m_NewActionName) && !isDuplicate;

		ImGui::SameLine();
		ImGui::BeginDisabled(!canAdd);
		bool pressed = ImGui::Button("Add Action", ImVec2(buttonWidth, 0.0f));
		ImGui::EndDisabled();

		if (canAdd && (pressed || submitted))
		{
			AddInputAction(m_NewActionName);
			m_NewActionName[0] = '\0';
			m_FocusNewActionField = true;
		}

		// The line is drawn either way so the list below does not jump as the warning appears.
		if (isDuplicate)
			ImGui::TextColored(ImVec4(0.90f, 0.65f, 0.20f, 1.0f), "\"%s\" is already an action name.", m_NewActionName);
		else
			ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
	}

	void ProjectSettingsDialog::RenderActionRow(int actionIndex, const InputAction& action)
	{
		const ImGuiStyle& style = ImGui::GetStyle();

		ImGui::PushID(actionIndex);

		// Square buttons sized to the row so both columns line up all the way down the list.
		float buttonSize = ImGui::GetFrameHeight();
		float buttonsWidth = buttonSize * 2.0f + style.ItemSpacing.x;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;

		// The node spans the row, so the buttons that follow have to be allowed to sit on top of it.
		ImGui::SetNextItemAllowOverlap();
		bool isOpen = ImGui::TreeNodeEx("##Action", flags, "%s", action.Name.c_str());

		// Trigger count next to the name, the way Godot summarises an action's events.
		ImGui::SameLine();
		ImGui::TextDisabled("(%d)", static_cast<int>(action.Triggers.size()));

		ImGui::SameLine();
		RightAlignCursor(buttonsWidth);

		if (ImGui::Button("+", ImVec2(buttonSize, buttonSize)))
			OpenTriggerConfigPopup(actionIndex, -1);
		ImGui::SetItemTooltip("Add a trigger to this action");

		ImGui::SameLine();
		if (ImGui::Button("x", ImVec2(buttonSize, buttonSize)))
		{
			m_ActionPendingRemoval = actionIndex;
			m_RemovePopupRequested = true;
		}
		ImGui::SetItemTooltip("Remove this action");

		if (isOpen)
		{
			if (action.Triggers.empty())
				ImGui::TextDisabled("No triggers bound - use + to add one.");

			for (int i = 0; i < static_cast<int>(action.Triggers.size()); i++)
				RenderTriggerRow(actionIndex, i, action.Triggers[i]);

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void ProjectSettingsDialog::RenderTriggerRow(int actionIndex, int triggerIndex, const InputTrigger& trigger)
	{
		const ImGuiStyle& style = ImGui::GetStyle();

		ImGui::PushID(triggerIndex);

		float buttonSize = ImGui::GetFrameHeight();
		float editWidth = ImGui::CalcTextSize("Edit").x + style.FramePadding.x * 2.0f;
		float buttonsWidth = editWidth + buttonSize + style.ItemSpacing.x;

		// InputCodeNames owns how a binding reads, so "Ctrl + W" looks the same everywhere.
		std::string displayName = InputCodeNames::TriggerToDisplayName(trigger);

		// Row height matches the buttons, and the label is centred against them.
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
		bool rowClicked = ImGui::Selectable(displayName.c_str(), false,
			ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowOverlap, ImVec2(0.0f, buttonSize));
		ImGui::PopStyleVar();

		// Double-clicking the row is the quick path to the same dialog the Edit button opens.
		if (rowClicked && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			OpenTriggerConfigPopup(actionIndex, triggerIndex);

		ImGui::SameLine();
		RightAlignCursor(buttonsWidth);

		if (ImGui::Button("Edit", ImVec2(editWidth, buttonSize)))
			OpenTriggerConfigPopup(actionIndex, triggerIndex);

		ImGui::SameLine();
		if (ImGui::Button("x", ImVec2(buttonSize, buttonSize)))
			RemoveInputTrigger(actionIndex, triggerIndex);
		ImGui::SetItemTooltip("Remove this trigger");

		ImGui::PopID();
	}

	void ProjectSettingsDialog::OpenTriggerConfigPopup(int actionIndex, int triggerIndex)
	{
		m_TriggerActionIndex = actionIndex;
		m_TriggerEditIndex = triggerIndex;
		m_TriggerSearch[0] = '\0';
		m_TriggerPopupRequested = true;

		const auto& actions = m_InputActionManager.GetActions();

		// Editing starts from the binding that is already there; adding starts from a blank one.
		bool isEditing = actionIndex >= 0 && actionIndex < static_cast<int>(actions.size()) &&
			triggerIndex >= 0 && triggerIndex < static_cast<int>(actions[actionIndex].Triggers.size());

		m_PendingTrigger = isEditing ? actions[actionIndex].Triggers[triggerIndex] : InputTrigger{};
	}

	void ProjectSettingsDialog::RenderTriggerConfigPopup()
	{
		if (m_TriggerPopupRequested)
		{
			ImGui::OpenPopup(s_TriggerConfigPopupName);
			m_TriggerPopupRequested = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(420, 480), ImGuiCond_Appearing);

		if (!ImGui::BeginPopupModal(s_TriggerConfigPopupName, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize))
			return;

		const ImGuiStyle& style = ImGui::GetStyle();
		const auto& actions = m_InputActionManager.GetActions();

		// The action can go away while the popup is up, so never index past the end of the list.
		if (m_TriggerActionIndex < 0 || m_TriggerActionIndex >= static_cast<int>(actions.size()))
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		bool isEditing = m_TriggerEditIndex >= 0;
		ImGui::Text(isEditing ? "Edit trigger for \"%s\"" : "New trigger for \"%s\"", actions[m_TriggerActionIndex].Name.c_str());
		ImGui::Separator();
		ImGui::Spacing();

		// Live preview of the binding, in the same wording the action list uses.
		ImGui::TextDisabled("Binding");
		ImGui::SameLine(90.0f);
		if (m_PendingTrigger.Device == InputDevice::None)
			ImGui::TextDisabled("Pick a key or button below");
		else
			ImGui::TextUnformatted(InputCodeNames::TriggerToDisplayName(m_PendingTrigger).c_str());

		ImGui::Spacing();

		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputTextWithHint("##TriggerSearch", "Search keys and buttons", m_TriggerSearch, sizeof(m_TriggerSearch));

		// The modifiers row and the confirm buttons both sit under the picker, so it has to leave
		// room for two framed rows and the spacer between them rather than scrolling them away.
		float footerHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f + style.ItemSpacing.y;
		ImGui::BeginChild("TriggerPicker", ImVec2(0.0f, -footerHeight), true);

		bool committed = RenderKeyboardSection();
		committed |= RenderMouseSection();
		committed |= RenderGamepadButtonSection();
		committed |= RenderGamepadAxisSection();

		ImGui::EndChild();

		// Modifiers are part of the trigger for mouse controls too, so they are never device-gated.
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Modifiers");
		ImGui::SameLine(90.0f);
		for (const auto& [modifier, label] : s_ModifierToggles)
		{
			bool active = (m_PendingTrigger.RequiredModifiers & modifier) != 0;
			if (ImGui::Checkbox(label, &active))
			{
				if (active)
					m_PendingTrigger.RequiredModifiers |= modifier;
				else
					m_PendingTrigger.RequiredModifiers = static_cast<KeyModifierType>(m_PendingTrigger.RequiredModifiers & ~static_cast<KeyModifierType>(modifier));
			}

			ImGui::SameLine();
		}
		ImGui::NewLine();

		ImGui::Spacing();

		bool hasSelection = m_PendingTrigger.Device != InputDevice::None;
		float footerButtonWidth = 110.0f;
		RightAlignCursor(footerButtonWidth * 2.0f + style.ItemSpacing.x);

		ImGui::BeginDisabled(!hasSelection);
		if (ImGui::Button(isEditing ? "Save Trigger" : "Add Trigger", ImVec2(footerButtonWidth, 0.0f)))
			committed = true;
		ImGui::EndDisabled();

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(footerButtonWidth, 0.0f)))
			ImGui::CloseCurrentPopup();

		if (committed && hasSelection)
		{
			CommitPendingTrigger();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	bool ProjectSettingsDialog::RenderKeyboardSection()
	{
		std::string_view search = m_TriggerSearch;

		// A search should reveal what it matched, so the sections force open while one is typed.
		if (!search.empty())
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);

		if (!ImGui::CollapsingHeader("Keyboard"))
			return false;

		KeyCode selectedKey = KeyCode::Unknown;
		if (m_PendingTrigger.Device == InputDevice::Keyboard)
		{
			if (const KeyCode* pendingKey = std::get_if<KeyCode>(&m_PendingTrigger.ControlId))
				selectedKey = *pendingKey;
		}

		bool committed = false;
		bool anyMatches = false;

		for (KeyCode key : s_PickerKeys)
		{
			std::string name(InputCodeNames::KeyCodeDisplayName(key));
			if (!MatchesSearch(name, search))
				continue;

			anyMatches = true;

			ImGui::PushID(static_cast<int>(key));

			// Double-clicking a control picks and confirms in one go.
			if (ImGui::Selectable(name.c_str(), key == selectedKey, ImGuiSelectableFlags_AllowDoubleClick))
			{
				m_PendingTrigger.Device = InputDevice::Keyboard;
				m_PendingTrigger.ControlId = key;
				m_PendingTrigger.Direction = AxisDirection::Full;
				committed = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
			}

			ImGui::PopID();
		}

		if (!anyMatches)
			ImGui::TextDisabled("No keys match the search.");

		return committed;
	}

	bool ProjectSettingsDialog::RenderMouseSection()
	{
		std::string_view search = m_TriggerSearch;

		if (!search.empty())
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);

		if (!ImGui::CollapsingHeader("Mouse"))
			return false;

		// Last is never a real control, so it stands in for "nothing picked yet".
		MouseControl selectedControl = MouseControl::Last;
		if (m_PendingTrigger.Device == InputDevice::Mouse)
		{
			if (const MouseControl* pendingControl = std::get_if<MouseControl>(&m_PendingTrigger.ControlId))
				selectedControl = *pendingControl;
		}

		bool committed = false;
		bool anyMatches = false;

		for (MouseControl control : s_PickerMouseControls)
		{
			std::string name(InputCodeNames::MouseControlDisplayName(control));
			if (!MatchesSearch(name, search))
				continue;

			anyMatches = true;

			ImGui::PushID(static_cast<int>(control));

			if (ImGui::Selectable(name.c_str(), control == selectedControl, ImGuiSelectableFlags_AllowDoubleClick))
			{
				m_PendingTrigger.Device = InputDevice::Mouse;
				m_PendingTrigger.ControlId = control;
				m_PendingTrigger.Direction = AxisDirection::Full;
				committed = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
			}

			ImGui::PopID();
		}

		if (!anyMatches)
			ImGui::TextDisabled("No mouse controls match the search.");

		return committed;
	}

	bool ProjectSettingsDialog::RenderGamepadButtonSection()
	{
		std::string_view search = m_TriggerSearch;

		if (!search.empty())
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);

		if (!ImGui::CollapsingHeader("Gamepad Buttons"))
			return false;

		// Last is never a real control, so it stands in for "nothing picked yet".
		GamepadButton selectedControl = GamepadButton::Last;
		if (m_PendingTrigger.Device == InputDevice::Gamepad)
		{
			if (const GamepadButton* pendingControl = std::get_if<GamepadButton>(&m_PendingTrigger.ControlId))
				selectedControl = *pendingControl;
		}

		bool committed = false;
		bool anyMatches = false;

		// Control ids restart at zero per device, so each section needs its own ID scope.
		ImGui::PushID("GamepadButtons");

		for (GamepadButton control : s_PickerGamepadButtons)
		{
			std::string name(InputCodeNames::GamepadButtonDisplayName(control));
			if (!MatchesSearch(name, search))
				continue;

			anyMatches = true;

			ImGui::PushID(static_cast<int>(control));

			if (ImGui::Selectable(name.c_str(), control == selectedControl, ImGuiSelectableFlags_AllowDoubleClick))
			{
				m_PendingTrigger.Device = InputDevice::Gamepad;
				m_PendingTrigger.ControlId = control;
				m_PendingTrigger.Direction = AxisDirection::Full;
				committed = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
			}

			ImGui::PopID();
		}

		ImGui::PopID();

		if (!anyMatches)
			ImGui::TextDisabled("No gamepad buttons match the search.");

		return committed;
	}

	bool ProjectSettingsDialog::RenderGamepadAxisSection()
	{
		std::string_view search = m_TriggerSearch;

		if (!search.empty())
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);

		if (!ImGui::CollapsingHeader("Gamepad Axis"))
			return false;

		// Last is never a real control, so it stands in for "nothing picked yet".
		GamepadAxis selectedControl = GamepadAxis::Last;
		AxisDirection selectedDirection = AxisDirection::Full;
		if (m_PendingTrigger.Device == InputDevice::Gamepad)
		{
			if (const GamepadAxis* pendingControl = std::get_if<GamepadAxis>(&m_PendingTrigger.ControlId))
			{
				selectedControl = *pendingControl;
				selectedDirection = m_PendingTrigger.Direction;
			}
		}

		bool committed = false;
		bool anyMatches = false;

		ImGui::PushID("GamepadAxes");

		for (int i = 0; i < static_cast<int>(std::size(s_PickerGamepadAxes)); i++)
		{
			const GamepadAxisEntry& entry = s_PickerGamepadAxes[i];

			std::string name(InputCodeNames::GamepadAxisDisplayName(entry.Axis, entry.Direction));
			if (!MatchesSearch(name, search))
				continue;

			anyMatches = true;

			// Indexed by row, since one axis appears under several directions.
			ImGui::PushID(i);

			const bool selected = entry.Axis == selectedControl && entry.Direction == selectedDirection;
			if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick))
			{
				m_PendingTrigger.Device = InputDevice::Gamepad;
				m_PendingTrigger.ControlId = entry.Axis;
				m_PendingTrigger.Direction = entry.Direction;
				committed = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
			}

			ImGui::PopID();
		}

		ImGui::PopID();

		if (!anyMatches)
			ImGui::TextDisabled("No gamepad axis match the search.");

		return committed;
	}

	void ProjectSettingsDialog::RenderRemoveActionPopup()
	{
		if (m_RemovePopupRequested)
		{
			ImGui::OpenPopup(s_RemoveActionPopupName);
			m_RemovePopupRequested = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (!ImGui::BeginPopupModal(s_RemoveActionPopupName, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
			return;

		const auto& actions = m_InputActionManager.GetActions();
		if (m_ActionPendingRemoval < 0 || m_ActionPendingRemoval >= static_cast<int>(actions.size()))
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		const InputAction& action = actions[m_ActionPendingRemoval];
		size_t triggerCount = action.Triggers.size();

		ImGui::Text("Remove \"%s\" and its %d %s?", action.Name.c_str(), static_cast<int>(triggerCount), triggerCount == 1 ? "trigger" : "triggers");
		ImGui::TextDisabled("Anything looking the action up by name will stop finding it.");
		ImGui::Spacing();

		if (ImGui::Button("Remove", ImVec2(120, 0)))
		{
			RemoveInputAction(m_ActionPendingRemoval);
			m_ActionPendingRemoval = -1;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			m_ActionPendingRemoval = -1;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	void ProjectSettingsDialog::AddInputAction(const std::string& name)
	{
		m_InputActionManager.AddAction({ name, {} });
	}

	void ProjectSettingsDialog::RemoveInputAction(int index)
	{
		m_InputActionManager.RemoveAction(index);
	}

	void ProjectSettingsDialog::CommitPendingTrigger()
	{
		bool isUpdate = m_TriggerEditIndex >= 0;
		if (isUpdate)
			m_InputActionManager.UpdateTrigger(m_TriggerActionIndex, m_TriggerEditIndex, m_PendingTrigger);
		else
			m_InputActionManager.AddTrigger(m_TriggerActionIndex, m_PendingTrigger);
	}

	void ProjectSettingsDialog::RemoveInputTrigger(int actionIndex, int triggerIndex)
	{
		m_InputActionManager.RemoveTrigger(actionIndex, triggerIndex);
	}

}
