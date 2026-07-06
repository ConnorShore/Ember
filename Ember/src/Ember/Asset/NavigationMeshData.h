#pragma once
#include "Asset.h"
#include "Ember/AI/NavigationMeshBakeSettings.h"
#include <DetourNavMesh.h>
#include <vector>

namespace Ember {

	class NavigationMeshData : public Asset
	{
	public:
		NavigationMeshData(const std::string& name, const std::string& filePath)
			: Asset(name, filePath, AssetType::NavMeshData)
		{
		}
		NavigationMeshData(UUID uuid, const std::string& name, const std::string& filePath)
			: Asset(uuid, name, filePath, AssetType::NavMeshData)
		{
		}

		~NavigationMeshData()
		{
			if (m_NavMesh)
			{
				dtFreeNavMesh(m_NavMesh);
				m_NavMesh = nullptr;
			}
		}

		// -------------------------------------------------------------------
		// 1. RUNTIME ACCESS
		// -------------------------------------------------------------------
		inline dtNavMesh* GetNavMesh() const { return m_NavMesh; }

		// Called by your Asset Loader after reading the binary file
		bool InitializeFromRawData()
		{
			if (m_NavMeshDataBlob.empty())
				return false;

			if (m_NavMesh)
			{
				dtFreeNavMesh(m_NavMesh);
				m_NavMesh = nullptr;
			}

			m_NavMesh = dtAllocNavMesh();
			if (!m_NavMesh)
				return false;

			// Keep ownership of m_NavMeshDataBlob in this asset (flags = 0).
			dtStatus status = m_NavMesh->init(m_NavMeshDataBlob.data(), static_cast<int>(m_NavMeshDataBlob.size()), 0);
			if (dtStatusFailed(status))
			{
				dtFreeNavMesh(m_NavMesh);
				m_NavMesh = nullptr;
				return false;
			}

			return dtStatusSucceed(status);
		}

		// -------------------------------------------------------------------
		// 2. EDITOR / BAKING ACCESS
		// -------------------------------------------------------------------
		inline NavigationMeshBakeSettings& GetBakeSettings() { return m_BakeSettings; }
		inline const NavigationMeshBakeSettings& GetBakeSettings() const { return m_BakeSettings; }

		void SetBakeSettings(const NavigationMeshBakeSettings& settings) { m_BakeSettings = settings; }

		// Allows your Serializer to write the blob to disk
		inline const std::vector<uint8_t>& GetRawDataBlob() const { return m_NavMeshDataBlob; }
		inline std::vector<uint8_t>& GetRawDataBlob() { return m_NavMeshDataBlob; }
		inline void SetRawDataBlob(const std::vector<uint8_t>& blob) { m_NavMeshDataBlob = blob; }
		inline void SetRawDataBlob(std::vector<uint8_t>&& blob) { m_NavMeshDataBlob = std::move(blob); }

		static AssetType GetStaticType() { return AssetType::NavMeshData; }

	private:
		dtNavMesh* m_NavMesh = nullptr;
		std::vector<uint8_t> m_NavMeshDataBlob;

		NavigationMeshBakeSettings m_BakeSettings;
	};

}