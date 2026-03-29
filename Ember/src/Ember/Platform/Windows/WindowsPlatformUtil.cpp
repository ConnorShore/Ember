#include "ebpch.h"
#include "Ember/Utils/PlatformUtil.h"

#include <windows.h>
#include <shobjidl.h>

#ifdef EB_PLATFORM_WINDOWS

namespace Ember {

	static std::string WStringToString(const std::wstring& wstr)
	{
		if (wstr.empty()) return std::string();
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	static std::wstring StringToWString(const std::string& str)
	{
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}
		
	std::string FileDialog::OpenFile(const char* initialDirectory /*= ""*/, const char* filterName /* = "All Files (*.*)"*/, const char* filterExt /*= "*.*"*/)
	{
		std::string result = "";

		// Initialize COM library
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		bool needUninit = SUCCEEDED(hr);

		// Create the Open Dialog object
		IFileOpenDialog* fileDialog = nullptr;
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&fileDialog));

		if (SUCCEEDED(hr))
		{
			// Set the filters
			std::wstring wFilterName = StringToWString(filterName);
			std::wstring wFilterExt = StringToWString(filterExt);
			COMDLG_FILTERSPEC rgSpec[] = { { wFilterName.c_str(), wFilterExt.c_str() } };
			fileDialog->SetFileTypes(1, rgSpec);

			if (initialDirectory != nullptr && initialDirectory[0] != '\0')
			{
				std::filesystem::path absolutePath = std::filesystem::absolute(initialDirectory);
				if (std::filesystem::exists(absolutePath) && std::filesystem::is_directory(absolutePath))
				{
					// std::filesystem nicely provides a native wide-string conversion
					std::wstring wInitialDir = absolutePath.wstring();
					IShellItem* pFolder = nullptr;

					if (SUCCEEDED(SHCreateItemFromParsingName(wInitialDir.c_str(), NULL, IID_IShellItem, reinterpret_cast<void**>(&pFolder))))
					{
						// SetFolder forces the dialog to this directory
						fileDialog->SetFolder(pFolder);
						pFolder->Release();
					}
				}
				else
				{
					EB_CORE_WARN("File Dialog failed to set initial directory. Path does not exist: {0}", absolutePath.string());
				}
			}

			// Force it to return actual file system paths (not abstract things like 'Control Panel')
			DWORD dwOptions;
			if (SUCCEEDED(fileDialog->GetOptions(&dwOptions)))
			{
				fileDialog->SetOptions(dwOptions | FOS_FORCEFILESYSTEM);
			}

			// Show the dialog
			if (SUCCEEDED(fileDialog->Show(NULL)))
			{
				IShellItem* pItem;
				if (SUCCEEDED(fileDialog->GetResult(&pItem)))
				{
					PWSTR pszFilePath;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
					{
						result = WStringToString(pszFilePath);
						CoTaskMemFree(pszFilePath); // Free the memory allocated by Windows
					}
					pItem->Release();
				}
			}
			fileDialog->Release();
		}

		if (needUninit)
			CoUninitialize(); // Cleanup COM

		return result;
	}

	std::string FileDialog::OpenDirectory()
	{
		std::string result = "";

		// Initialize COM library
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr)) return result;

		IFileOpenDialog* pFolderDialog;

		// Create the FileOpenDialog object
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFolderDialog));

		if (SUCCEEDED(hr))
		{
			// Get current options
			DWORD dwOptions;
			if (SUCCEEDED(pFolderDialog->GetOptions(&dwOptions)))
			{
				// CRITICAL STEP: Tell the OS to only pick folders!
				pFolderDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
			}

			// Show the dialog
			if (SUCCEEDED(pFolderDialog->Show(NULL)))
			{
				IShellItem* pItem;
				if (SUCCEEDED(pFolderDialog->GetResult(&pItem)))
				{
					PWSTR pszFilePath;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
					{
						// Convert Wide String to std::string
						std::wstring ws(pszFilePath);
						result = std::string(ws.begin(), ws.end());
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFolderDialog->Release();
		}

		CoUninitialize();
		return result;
	}

	std::string FileDialog::SaveFile(const char* initialDirectory /*= ""*/, const char* initialFileName /*= ""*/, const char* filterName /* = "All Files (*.*)"*/, const char* filterExt /*= "*.*"*/)
	{
		std::string result = "";
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		bool needUninit = SUCCEEDED(hr);

		IFileSaveDialog* fileDialog = nullptr;
		hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&fileDialog));

		if (SUCCEEDED(hr))
		{
			std::wstring wFilterName = StringToWString(filterName);
			std::wstring wFilterExt = StringToWString(filterExt);
			COMDLG_FILTERSPEC rgSpec[] = { { wFilterName.c_str(), wFilterExt.c_str() } };
			fileDialog->SetFileTypes(1, rgSpec);

			// Extract the default extension (e.g. "*.ebs" -> "ebs") so Windows auto-appends it
			std::wstring defaultExt = wFilterExt.substr(wFilterExt.find_last_of(L'.') + 1);
			fileDialog->SetDefaultExtension(defaultExt.c_str());

			if (initialFileName != nullptr && initialFileName[0] != '\0')
			{
				std::wstring wDefaultName = StringToWString(initialFileName);
				fileDialog->SetFileName(wDefaultName.c_str());
			}

			std::filesystem::path absolutePath = std::filesystem::absolute(initialDirectory);
			if (std::filesystem::exists(absolutePath) && std::filesystem::is_directory(absolutePath))
			{
				// std::filesystem nicely provides a native wide-string conversion
				std::wstring wInitialDir = absolutePath.wstring();
				IShellItem* pFolder = nullptr;

				if (SUCCEEDED(SHCreateItemFromParsingName(wInitialDir.c_str(), NULL, IID_IShellItem, reinterpret_cast<void**>(&pFolder))))
				{
					// SetFolder forces the dialog to this directory
					fileDialog->SetFolder(pFolder);
					pFolder->Release();
				}
			}
			else
			{
				EB_CORE_WARN("File Dialog failed to set initial directory. Path does not exist: {0}", absolutePath.string());
			}

			DWORD dwOptions;
			if (SUCCEEDED(fileDialog->GetOptions(&dwOptions)))
			{
				fileDialog->SetOptions(dwOptions | FOS_FORCEFILESYSTEM | FOS_OVERWRITEPROMPT);
			}

			if (SUCCEEDED(fileDialog->Show(NULL)))
			{
				IShellItem* pItem;
				if (SUCCEEDED(fileDialog->GetResult(&pItem)))
				{
					PWSTR pszFilePath;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
					{
						result = WStringToString(pszFilePath);
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			fileDialog->Release();
		}

		if (needUninit)
			CoUninitialize();

		return result;
	}
}

#endif