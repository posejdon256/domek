#pragma once
#include "mesh.h"

namespace mini
{
	class NodeMesh : public Mesh
	{
	public:
		NodeMesh()
			: m_materialIdx(UINT_MAX)
		{ }
		NodeMesh(dx_ptr_vector<ID3D11Buffer>&& vbuffers,
			std::vector<unsigned int>&& vstrides,
			std::vector<unsigned int>&& voffsets,
			dx_ptr<ID3D11Buffer>&& indices,
			unsigned int indexCount,
			unsigned int materialIdx)
			: Mesh(std::move(vbuffers), std::move(vstrides),
				std::move(voffsets), std::move(indices), indexCount),
			m_materialIdx(materialIdx)
		{ }

		NodeMesh(Mesh&& right, unsigned int materialIdx)
			: Mesh(std::move(right)), m_materialIdx(materialIdx)
		{ }

		NodeMesh(NodeMesh&& right)
			: Mesh(std::move(right)), m_materialIdx(right.m_materialIdx),
			m_transform(right.m_transform)
		{ }

		NodeMesh(const NodeMesh& right) = delete;

		NodeMesh& operator=(NodeMesh&& right);

		NodeMesh& operator= (const NodeMesh& right) = delete;

		unsigned int getMaterialIdx() const { return m_materialIdx; }
		void setMaterialIdx(unsigned int idx) { m_materialIdx = idx; }
		const DirectX::XMFLOAT4X4& getTransform() const { return m_transform; }
		void setTransform(const DirectX::XMFLOAT4X4& transform) { m_transform = transform; }

	private:
		unsigned int m_materialIdx;
		DirectX::XMFLOAT4X4 m_transform;
	};
}
