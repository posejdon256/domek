#pragma once

#include <DirectXMath.h>
#include <string>

namespace mini
{
	struct SceneNode
	{
		SceneNode();

		DirectX::XMFLOAT4X4 m_localTransform;
		DirectX::XMFLOAT4X4 m_transform;
		std::string m_name;
		int m_nextSibling;
		int m_firstChild;
		int m_parent;
		int m_mesh;
	};
}