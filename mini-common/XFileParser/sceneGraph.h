#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>
#include "nodeMesh.h"
#include "sceneNode.h"
#include "material.h"

namespace mini
{
	class SceneGraph
	{
	public:
		SceneGraph(std::vector<SceneNode>&& nodes, std::vector<NodeMesh>&& meshes, std::vector<Material>&& materials);
		SceneGraph(SceneGraph&& right);
		SceneGraph(const SceneGraph& right) = delete;

		SceneGraph& operator=(const SceneGraph& right) = delete;
		SceneGraph& operator=(SceneGraph&& right);

		int nodeByName(const std::string& name) const;
		DirectX::XMFLOAT4X4 getNodeTransform(unsigned nodeIndex) const;
		void setNodeTransform(unsigned int nodeIndex, const DirectX::XMFLOAT4X4& transform);
		int nodeFirstChild(unsigned int nodeIndex) const;
		int nodeNextSibling(unsigned int nodeIndex) const;
		int nodeParent(unsigned int nodeIndex) const;
		int nodeMesh(unsigned int nodeIndex) const;

		size_t meshCount() const { return m_meshes.size(); }
		NodeMesh& getMesh(unsigned int index) { return m_meshes.at(index); }
		Material& getMeshMaterial(unsigned int index) { return m_materials.at(m_meshes.at(index).getMaterialIdx()); }
		
	private:
		void UpdateChildTransforms(unsigned int childIdx, DirectX::CXMMATRIX parentTransform);

		void Clear();

		std::vector<NodeMesh> m_meshes;
		std::vector<SceneNode> m_nodes;
		std::vector<Material> m_materials;
	};
}