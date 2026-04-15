#include "ebpch.h"
#include "ScriptBindAssets.h"

#include "Ember/Core/Application.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Texture2D.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Material.h"
#include "Ember/Asset/Animation.h"
#include "Ember/Asset/Model.h"
#include "Ember/Asset/Skeleton.h"

namespace Ember {

	static sol::object GetAssetFromTypeString(const std::string& assetTypeStr, const std::string& assetName, sol::state& state)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		AssetType type = GetAssetTypeFromString(assetTypeStr);
		switch (type)
		{
			case AssetType::Texture:
				return sol::make_object(state, assetManager.GetAsset<Texture>(assetName).Ptr());
			case AssetType::Mesh:
				return sol::make_object(state, assetManager.GetAsset<Mesh>(assetName).Ptr());
			case AssetType::Model:
				return sol::make_object(state, assetManager.GetAsset<Model>(assetName).Ptr());
			case AssetType::Skeleton:
				return sol::make_object(state, assetManager.GetAsset<Skeleton>(assetName).Ptr());
			case AssetType::Animation:
				return sol::make_object(state, assetManager.GetAsset<Animation>(assetName).Ptr());
			case AssetType::Shader:
				return sol::make_object(state, assetManager.GetAsset<Shader>(assetName).Ptr());
			case AssetType::Material:
				return sol::make_object(state, assetManager.GetAsset<Material>(assetName).Ptr());
			case AssetType::Script:
				EB_CORE_ASSERT(false, "Cannot get script assets from Lua!");
				return sol::lua_nil;
			default:
				EB_CORE_ASSERT(false, "Unknown asset type: {}", assetTypeStr);
				return sol::lua_nil;
		}
	}

	void BindAssets(sol::state& state)
	{
		state.new_usertype<Animation>("Animation",
			"GetUUID", [](Animation& anim) { return anim.GetUUID(); },
			"GetName", [](Animation& anim) { return anim.GetName(); },
			"GetDuration", [](Animation& anim) { return anim.GetDuration(); }
		);

		sol::table assetManager = state.create_named_table("AssetManager");
		assetManager.set_function("GetAsset", [&state](const std::string& typeName, const std::string& assetName) -> sol::object {
			return GetAssetFromTypeString(typeName, assetName, state);
		});
	}

}
