#pragma once
#include <unordered_map>
#include <string>
#include <filesystem>

namespace Ember {

	class SaveGameManager
	{
	public:
		SaveGameManager() = default;
		~SaveGameManager() = default;

		void SetInt(const std::string& key, int value) { m_IntData[key] = value; }
		void SetFloat(const std::string& key, float value) { m_FloatData[key] = value; }
		void SetString(const std::string& key, const std::string& value) { m_StringData[key] = value; }

		int GetInt(const std::string& key, int defaultValue = 0);
		float GetFloat(const std::string& key, float defaultValue = 0.0f);
		std::string GetString(const std::string& key, const std::string& defaultValue = "");

		bool SaveToFile(const std::string& filename);
		bool LoadFromFile(const std::string& filename);

	private:
		// i.e. C:\Users\Username\AppData\Local\ProjectName\SavedGames
		std::filesystem::path GetOSSaveDirectory() const;

	private:
		std::unordered_map<std::string, int> m_IntData;
		std::unordered_map<std::string, float> m_FloatData;
		std::unordered_map<std::string, std::string> m_StringData;
	};

}