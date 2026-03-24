#include "AssetManagerPanel.h"
#include <filesystem>

namespace Ember {

	std::string AssetManagerPanel::m_AssetDirectory = "Ember-Forge/assets";
	std::string AssetManagerPanel::m_CurrentDirectory = "Ember-Forge/assets";

	AssetManagerPanel::AssetManagerPanel(EditorContext* context)
		: Panel("Asset Manager", context)
	{
	}

	AssetManagerPanel::~AssetManagerPanel()
	{
	}

	void AssetManagerPanel::OnAttach()
	{
		// Loop through all assets in the asset directory and load them into the AssetManager
		//auto& assetManager = Application::Instance().GetAssetManager();
		//std::filesystem::path assetDir(m_AssetDirectory);
		//for (const auto& entry : std::filesystem::recursive_directory_iterator(assetDir))
		//{
		//	if (entry.is_regular_file())
		//	{
		//		std::string filePath = entry.path().string();
		//		std::string extension = entry.path().extension().string();
		//		std::string fileName = entry.path().stem().string();
		//		// Load assets based on their file extension
		//		if (extension == ".obj" || extension == ".fbx")
		//		{
		//			assetManager.Load<Model>(fileName, filePath);
		//		}
		//		else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
		//		{
		//			assetManager.Load<Texture>(fileName, filePath);
		//		}
		//		else if (extension == ".mat")
		//		{
		//			assetManager.Load<Material>(fileName, filePath);
		//		}
		//	}
		//}
	}

	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		ImGui::End();
	}

}