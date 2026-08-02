#include "ebpch.h"
#include "SaveGameFile.h"

namespace Ember {

	SaveValue& SaveGameFile::Slot(std::string_view key)
	{
		auto it = m_Values.find(key);
		if (it != m_Values.end())
			return it->second;

		return m_Values.emplace(std::string(key), SaveValue{}).first->second;
	}

	// Getters coerce between int and float rather than falling back to the default. YAML does not
	// reliably preserve 1 vs 1.0, and silently handing back the default loses player progress.
	int SaveGameFile::GetInt(std::string_view key, int defaultValue) const
	{
		auto it = m_Values.find(key);
		if (it == m_Values.end())
			return defaultValue;

		if (const int* value = std::get_if<int>(&it->second))
			return *value;
		if (const float* value = std::get_if<float>(&it->second))
			return static_cast<int>(*value);
		if (const bool* value = std::get_if<bool>(&it->second))
			return *value ? 1 : 0;

		return defaultValue;
	}

	float SaveGameFile::GetFloat(std::string_view key, float defaultValue) const
	{
		auto it = m_Values.find(key);
		if (it == m_Values.end())
			return defaultValue;

		if (const float* value = std::get_if<float>(&it->second))
			return *value;
		if (const int* value = std::get_if<int>(&it->second))
			return static_cast<float>(*value);

		return defaultValue;
	}

	bool SaveGameFile::GetBool(std::string_view key, bool defaultValue) const
	{
		auto it = m_Values.find(key);
		if (it == m_Values.end())
			return defaultValue;

		if (const bool* value = std::get_if<bool>(&it->second))
			return *value;
		if (const int* value = std::get_if<int>(&it->second))
			return *value != 0;

		return defaultValue;
	}

	std::string SaveGameFile::GetString(std::string_view key, const std::string& defaultValue) const
	{
		auto it = m_Values.find(key);
		if (it == m_Values.end())
			return defaultValue;

		if (const std::string* value = std::get_if<std::string>(&it->second))
			return *value;

		return defaultValue;
	}

	bool SaveGameFile::Remove(std::string_view key)
	{
		auto it = m_Values.find(key);
		if (it == m_Values.end())
			return false;

		m_Values.erase(it);
		return true;
	}

}
