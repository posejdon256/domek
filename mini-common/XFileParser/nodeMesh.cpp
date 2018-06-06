#include "nodeMesh.h"

using namespace mini;
using namespace std;

NodeMesh& NodeMesh::operator=(NodeMesh&& right)
{
	m_materialIdx = right.m_materialIdx;
	m_transform = right.m_transform;
	static_cast<Mesh&>(*this) = move(right);
	return *this;
}
