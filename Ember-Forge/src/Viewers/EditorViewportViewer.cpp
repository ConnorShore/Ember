#include "efpch.h"
#include "EditorViewportViewer.h"

namespace Ember {
	EditorViewportViewer::EditorViewportViewer(Type type, SharedPtr<Scene> scene, const std::string& filePath, const std::string& title)
		: m_Type(type), m_Scene(scene), m_FilePath(filePath), m_Title(title)
	{
	}
}