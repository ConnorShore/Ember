#pragma once

#include "Texture2D.h"
#include "CubeMap.h"
#include "Framebuffer.h"
#include "Mesh.h"

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"

namespace Ember {

	class Skybox : public SharedResource
	{
	public:
		Skybox();
		Skybox(UUID equirectangularMapAssetUUID);

		~Skybox() = default;

		void Initialize(UUID equirectangularMapAssetUUID);

		inline uint32_t GetSkyboxBufferID() const { return m_SkyboxBuffer ? m_SkyboxBuffer->GetID() : 0; }
		inline uint32_t GetEnvironmentCubeMapID() const { return m_EnvironmentCubeMap ? m_EnvironmentCubeMap->GetID() : 0; }
		inline UUID GetSkyboxTextureHandle() const { return m_SkyboxTextureHandle; }

		inline void SetSkyboxResolution(uint32_t resolution) { m_Resolution = resolution; Initialize(GetSkyboxTextureHandle()); }
		inline uint32_t GetSkyboxResolution() const { return m_Resolution; }

		inline void SetEnabled(bool enabled) { m_Enabled = enabled; };
		inline bool Enabled() const { return m_Enabled; }

	private:
		bool m_Enabled = false;
		uint32_t m_Resolution = 1024;
		UUID m_SkyboxTextureHandle;

		SharedPtr<Framebuffer> m_SkyboxBuffer;
		SharedPtr<CubeMap> m_EnvironmentCubeMap;

		Matrix4f m_CaptureProjection;
		std::array<Matrix4f, 6> m_CaptureViewMats;
	};

}