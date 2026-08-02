#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace Ember {

	using SaveValue = std::variant<int, float, bool, std::string>;

	// Transparent hashing so a lookup driven by a Lua string never materialises a std::string key.
	// The standard guarantees hash<string> and hash<string_view> agree for equal content.
	struct SaveKeyHash
	{
		using is_transparent = void;
		std::size_t operator()(std::string_view key) const { return std::hash<std::string_view>{}(key); }
	};

	using SaveValueMap = std::unordered_map<std::string, SaveValue, SaveKeyHash, std::equal_to<>>;

	// One save file's in-memory contents. Pure container - SaveGameManager owns the disk I/O.
	class SaveGameFile
	{
	public:
		explicit SaveGameFile(std::string name) : m_Name(std::move(name)) {}

		void SetInt(std::string_view key, int value) { Slot(key) = value; }
		void SetFloat(std::string_view key, float value) { Slot(key) = value; }
		void SetBool(std::string_view key, bool value) { Slot(key) = value; }
		void SetString(std::string_view key, std::string value) { Slot(key) = std::move(value); }

		int GetInt(std::string_view key, int defaultValue = 0) const;
		float GetFloat(std::string_view key, float defaultValue = 0.0f) const;
		bool GetBool(std::string_view key, bool defaultValue = false) const;
		std::string GetString(std::string_view key, const std::string& defaultValue = "") const;

		bool Has(std::string_view key) const { return m_Values.find(key) != m_Values.end(); }
		bool Remove(std::string_view key);
		void Clear() { m_Values.clear(); }

		std::size_t Size() const { return m_Values.size(); }

		const std::string& GetName() const { return m_Name; }
		const SaveValueMap& GetValues() const { return m_Values; }

		// Used by the deserializer, which already knows the concrete alternative it parsed.
		void SetValue(std::string key, SaveValue value) { m_Values[std::move(key)] = std::move(value); }

	private:
		// The only path that allocates a key - unordered_map has no heterogeneous insert.
		SaveValue& Slot(std::string_view key);

	private:
		std::string m_Name;
		SaveValueMap m_Values;
	};

}
