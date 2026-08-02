#pragma once
#include "Ember/Core/SaveGameFile.h"
#include "Ember/Core/ScopedPointer.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Ember {

	class SaveGameManager;

	// A cacheable handle to an open save file. Same idea as ComponentRef (see
	// Script/Bindings/ScriptComponentRef.h): resolving is a bounds check plus a generation compare,
	// so a script can hold one across frames without re-hashing the file name on every access.
	struct SaveFileHandle
	{
		static constexpr uint32_t InvalidIndex = ~0u;

		SaveGameManager* Owner = nullptr;
		uint32_t Index = InvalidIndex;
		uint32_t Generation = 0;

		bool IsValid() const;

		// Throws once the file has been closed. Scripts are invoked through sol::protected_function,
		// so that surfaces as a clean logged Lua error instead of a dangling reference.
		SaveGameFile& Resolve() const;
	};

	// Registry of open save files plus all save-game disk I/O. Multiple files can be open at once,
	// each with its own key space; scripts address them through SaveFileHandle.
	class SaveGameManager
	{
	public:
		// Fallback name so an empty string still resolves to a real file rather than ".sav".
		static constexpr const char* DefaultSlotName = "SaveGame";
		static constexpr const char* SaveFileExtension = ".sav";

		SaveGameManager() = default;
		~SaveGameManager() = default;

		// Returns the already-open file, otherwise opens it - reading from disk if the file exists.
		SaveFileHandle Open(std::string_view name);
		// Re-reads from disk, discarding unsaved in-memory changes to that file.
		SaveFileHandle Reload(std::string_view name);

		bool Save(std::string_view name);
		bool SaveAll();

		// Drops the file from memory without writing it. Any outstanding handle goes stale.
		void Close(std::string_view name);
		void CloseAll();

		bool IsOpen(std::string_view name) const;
		SaveGameFile* Find(std::string_view name);

		bool ExistsOnDisk(std::string_view name) const;
		bool DeleteFromDisk(std::string_view name);

		// i.e. C:\Users\Username\AppData\Local\ProjectName\SavedGames
		std::filesystem::path GetOSSaveDirectory() const;
		std::filesystem::path GetSaveFilePath(std::string_view name) const;

	private:
		friend struct SaveFileHandle;

		struct Slot
		{
			ScopedPtr<SaveGameFile> File;
			uint32_t Generation = 0;
		};

		// Strips a trailing ".sav" so Open("Settings") and Open("Settings.sav") share one slot.
		static std::string_view NormalizeName(std::string_view name);

		// Finds or creates the slot for a name. Never touches the disk.
		uint32_t AcquireSlot(std::string_view name, bool* outCreated = nullptr);

		std::vector<Slot> m_Slots;
		std::unordered_map<std::string, uint32_t, SaveKeyHash, std::equal_to<>> m_SlotsByName;
	};

}
