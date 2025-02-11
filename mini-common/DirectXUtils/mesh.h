#pragma once

#include "dxptr.h"
#include <vector>
#include <DirectXMath.h>
#include <D3D11.h>
#include "dxArray.h"

namespace mini
{
	class Mesh
	{
	public:
		Mesh();
		Mesh(dx_ptr_vector<ID3D11Buffer>&& vbuffers,
			 std::vector<unsigned int>&& vstrides,
			 std::vector<unsigned int>&& voffsets,
			 dx_ptr<ID3D11Buffer>&& indices,
			 unsigned int indexCount);

		Mesh(Mesh&& right);
		Mesh(const Mesh& right) = delete;
		void Release();
		~Mesh();

		Mesh& operator=(const Mesh& right) = delete;
		Mesh& operator=(Mesh&& right);
		void Render(const dx_ptr<ID3D11DeviceContext>& context) const;

	private:

		dx_ptr<ID3D11Buffer> m_indexBuffer;
		dx_ptr_vector<ID3D11Buffer> m_vertexBuffers;
		std::vector<unsigned int> m_strides;
		std::vector<unsigned int> m_offsets;
		unsigned int m_buffersCount;
		unsigned int m_indexCount;
	};
}
