#include "ebpch.h"
#include "ScriptBindSaveGame.h"

#include "Ember/Core/Application.h"
#include "Ember/Core/SaveGameManager.h"

namespace Ember {

	void BindSaveGame(sol::state& state)
	{
		state.new_usertype<SaveGameManager>("SaveSystem",
			"SetInt", &SaveGameManager::SetInt,
			"SetFloat", &SaveGameManager::SetFloat,
			"SetString", &SaveGameManager::SetString,
			"GetInt", &SaveGameManager::GetInt,
			"GetFloat", &SaveGameManager::GetFloat,
			"GetString", &SaveGameManager::GetString,
			"Save", &SaveGameManager::SaveToFile,
			"Load", &SaveGameManager::LoadFromFile
		);

		state["GameData"] = &Application::Instance().GetSaveGameManager();
	}

}