#include "xfileLoader.h"
#include "exceptions.h"
#include <fstream>
#include <map>
using namespace mini;
using namespace std;
using namespace DirectX;

void XFileLoader::Load(string filename)
{	
	m_nodes.clear();
	m_meshes.clear();
	m_materials.clear();
	m_materialLookup.clear();
	ifstream s;
	s.open(filename);
	if (!s)
		throw "Could not open file!";
	CheckHeader(s);
	auto prevSybling = -1;
	while (true)
	{
		auto token = XFileToken::NextToken(s);
		if (token.m_type == XFileToken::Identifier)
		{
			if (token.m_content == "Frame")
				prevSybling = ReadSceneNode(s, prevSybling);
			else if (token.m_content == "Mesh")
				ReadMesh(s);
			else
				SkipDataObject(s);
		}
		else if (token.m_type == XFileToken::LeftBrace)
			SkipDataReference(s);
		else if (token.m_type == XFileToken::None)
			break;
		else
			THROW(L"XFile Parsing Error!");
	}
	s.close();
}

int XFileLoader::ReadSceneNode(istream& s, int prevSybling)
{
	int nodeIdx = static_cast<int>(m_nodes.size());
	m_nodes.push_back(SceneNode());
	if (prevSybling != -1)
		m_nodes[prevSybling].m_nextSibling = nodeIdx;
	XFileToken t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::Identifier)
	{
		m_nodes[nodeIdx].m_name = move(t.m_content);
		t = XFileToken::NextToken(s);
	}
	if (t.m_type != XFileToken::LeftBrace)
		THROW(L"XFile Parsing Error!");
	auto prevChild = -1;
	while (true)
	{
		t = XFileToken::NextToken(s);
		if (t.m_type == XFileToken::Identifier)
		{
			if (t.m_content == "Frame")
			{
				auto child = ReadSceneNode(s, prevChild);
				if (prevChild == -1)
					m_nodes[nodeIdx].m_firstChild = child;
				m_nodes[child].m_parent = nodeIdx;
				prevChild = child;
			}
			else if (t.m_content == "Mesh")
				m_nodes[nodeIdx].m_mesh = ReadMesh(s);
			else if (t.m_content == "FrameTransformMatrix")
				m_nodes[nodeIdx].m_localTransform = ReadFaceTransform(s);
			else
				SkipDataObject(s);
		}
		else if (t.m_type == XFileToken::UUID)
		// ReSharper disable once CppRedundantControlFlowJump
			continue;
		else if (t.m_type == XFileToken::LeftBrace)
			SkipDataReference(s);
		else if (t.m_type == XFileToken::RightBrace)
			break;
		else
			THROW(L"XFile Parsing Error!");
	}
	return nodeIdx;
}

template<>
void XFileLoader::ReadData<XMFLOAT4X4>(istream& s, XMFLOAT4X4& d)
{
	float elems[16];
	ReadArray(s, elems, elems + 16);
	d = XMFLOAT4X4(elems);
	SkipToken(s, XFileToken::Semicolon);
}

XMFLOAT4X4 XFileLoader::ReadFaceTransform(istream& s)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::Identifier)
	{
		t = XFileToken::NextToken(s);
	}
	if (t.m_type != XFileToken::LeftBrace)
		THROW(L"XFile Parsing Error!");
	XMFLOAT4X4 mtx;
	ReadData(s, mtx);
	SkipToken(s, XFileToken::RightBrace);
	return mtx;
}

void XFileLoader::SkipToken(istream& s, XFileToken::Type type)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type != type)
		THROW(L"XFile Parsing Error!");
}

template<>
void XFileLoader::ReadData<int>(istream& s, int& d)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type != XFileToken::Integer)
		THROW(L"XFile Parsing Error!");
	d = stoi(t.m_content);
}

template<>
void XFileLoader::ReadData<unsigned short>(istream& s, unsigned short& d)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type != XFileToken::Integer)
		THROW(L"XFile Parsing Error!");
	d = stoi(t.m_content);
}

template<>
void XFileLoader::ReadData<float>(istream& s, float& d)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type != XFileToken::Float)
		THROW(L"XFile Parsing Error!");
	d = stof(t.m_content);
}

template<>
void XFileLoader::ReadData<string>(istream& s, string& d)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type != XFileToken::String)
		THROW(L"XFile Parsing Error!");
	d = move(t.m_content);
}

template<>
void XFileLoader::ReadData<XMFLOAT2>(istream& s, XMFLOAT2& d)
{
	ReadMember(s, d.x);
	ReadMember(s, d.y);
}

template<>
void XFileLoader::ReadData<XMFLOAT3>(istream& s, XMFLOAT3& d)
{
	ReadMember(s, d.x);
	ReadMember(s, d.y);
	ReadMember(s, d.z);
}

template<>
void XFileLoader::ReadData<XMFLOAT4>(istream& s, XMFLOAT4& d)
{
	ReadMember(s, d.x);
	ReadMember(s, d.y);
	ReadMember(s, d.z);
	ReadMember(s, d.w);
}

template<class T>
void XFileLoader::ReadMember(istream& s, T& d)
{
	ReadData(s, d);
	SkipToken(s, XFileToken::Semicolon);
}

template<class iter>
void XFileLoader::ReadArray(istream& s, iter b, iter e)
{
	for (iter i = b; i != e; ++i)
	{
		if (i != b)
			SkipToken(s, XFileToken::Comma);
		ReadData(s, *i);
	}
	SkipToken(s, XFileToken::Semicolon);
}

void XFileLoader::ReadFaceArray(istream& s, vector<unsigned short>& indices)
{
	int nFaces;
	ReadMember(s, nFaces);
	indices.reserve(nFaces * 3);
	for (auto i = 0; i < nFaces; ++i)
	{
		if (i != 0)
			SkipToken(s, XFileToken::Comma);
		int nIndices;
		ReadMember(s, nIndices);
		assert(nIndices == 3);
		vector<int> faceIndices(nIndices);
		ReadArray(s, faceIndices.begin(), faceIndices.end());
		for (auto j = 1; j < nIndices - 1; ++j)
		{
			indices.push_back(faceIndices[0]);
			indices.push_back(faceIndices[j]);
			indices.push_back(faceIndices[j + 1]);
		}
	}
	SkipToken(s, XFileToken::Semicolon);
}

void XFileLoader::ReadMeshNormals(istream&s, vector<XMFLOAT3>& normals, vector<unsigned short>& nindices)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::Identifier)
	{
		//m_meshes[meshIdx].m_name = t.m_content;
		t = XFileToken::NextToken(s);
	}
	if (t.m_type != XFileToken::LeftBrace)
		THROW(L"XFile Parsing Error!");
	int nElems;
	ReadMember(s, nElems);
	normals.resize(nElems);
	ReadArray(s, normals.begin(), normals.end());
	ReadFaceArray(s, nindices);
	SkipToken(s, XFileToken::RightBrace);
}

string XFileLoader::ReadTextureFilename(istream& s)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::Identifier)
		t = XFileToken::NextToken(s);
	if (t.m_type != XFileToken::LeftBrace)
		THROW(L"XFile Parsing Error!");
	string fileName;
	ReadMember(s, fileName);
	SkipToken(s, XFileToken::RightBrace);
	return fileName;
}

int XFileLoader::CreateMaterial(const string& materialName, XMFLOAT4 diffuse, XMFLOAT4 specular, const string& textureFilename)
{
	if (m_materialLookup.find(materialName) != m_materialLookup.end())
		return m_materialLookup[materialName];
	int materialIdx = static_cast<int>(m_materials.size());
	m_materials.push_back(Material());
	Material::MaterialData data = { diffuse, specular };
	m_materials[materialIdx].setMaterialData(data);
	m_materialLookup.insert(make_pair(move(materialName), materialIdx));
	if (!textureFilename.empty())
	{
		wstring fname(textureFilename.begin(), textureFilename.end());
		m_materials[materialIdx].setDiffuseTexture(move(m_device.CreateShaderResourceView(fname)));
		auto ext = fname.rfind(L'.');
		if (ext == fname.npos)
			THROW(L"XFile Parsing Error!");
		auto specName = fname.substr(0, ext) + L"_Specular" + fname.substr(ext);
		m_materials[materialIdx].setSpecularTexture(move(m_device.CreateShaderResourceView(specName)));
	}
	return materialIdx;
}

int XFileLoader::ReadMaterial(istream& s)
{
	auto t = XFileToken::NextToken(s);
	string materialName;
	if (t.m_type == XFileToken::Identifier)
	{
		materialName = move(t.m_content);
		t = XFileToken::NextToken(s);
	}
	if (t.m_type != XFileToken::LeftBrace)
		THROW(L"XFile Parsing Error!");
	XMFLOAT4 diffColor;
	XMFLOAT3 specColor;
	XMFLOAT3 emisColor;
	float pow;
	ReadMember(s, diffColor);
	ReadMember(s, pow);
	ReadMember(s, specColor);
	ReadMember(s, emisColor);
	string texFile;
	while (true)
	{
		t = XFileToken::NextToken(s);
		if (t.m_type == XFileToken::Identifier)
		{
			if ( t.m_content == "TextureFilename")
				texFile = ReadTextureFilename(s);
			else
				SkipDataObject(s);
		}
		else if (t.m_type == XFileToken::LeftBrace)
			SkipDataReference(s);
		else if (t.m_type == XFileToken::RightBrace)
			break;
		else
			THROW(L"XFile Parsing Error!");
	}
	return CreateMaterial(materialName, diffColor, XMFLOAT4(specColor.x, specColor.y, specColor.z, pow), texFile);
}

int XFileLoader::ReadMeshMaterials(istream& s)
{
	vector<int> materialIndices;
	int nMaterialIndices;
	int nMaterials;
	auto t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::Identifier)
	{
		//m_meshes[meshIdx].m_name = t.m_content;
		t = XFileToken::NextToken(s);
	}
	if (t.m_type != XFileToken::LeftBrace)
		THROW(L"XFile Parsing Error!");
	ReadMember(s, nMaterials);
	ReadMember(s, nMaterialIndices);
	materialIndices.resize(nMaterialIndices);
	ReadArray(s, materialIndices.begin(), materialIndices.end());
	SkipToken(s, XFileToken::Semicolon);
	unsigned int materialIdx = UINT_MAX;
	while (true)
	{
		t = XFileToken::NextToken(s);
		if (t.m_type == XFileToken::Identifier)
		{
			if (t.m_content == "Material" && materialIdx == UINT_MAX)
				materialIdx = ReadMaterial(s);
			else
				SkipDataObject(s);
		}
		else if (t.m_type == XFileToken::LeftBrace)
			SkipDataReference(s);
		else if (t.m_type == XFileToken::RightBrace)
			break;
		else
			THROW(L"XFile Parsing Error!");
	}
	return materialIdx;
}

void XFileLoader::ReadTexCoords(istream& s, vector<XMFLOAT2>& texCoords)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::Identifier)
		t = XFileToken::NextToken(s);
	if (t.m_type != XFileToken::LeftBrace)
		THROW(L"XFile Parsing Error!");
	int nElems;
	ReadMember(s, nElems);
	texCoords.resize(nElems);
	ReadArray(s, texCoords.begin(), texCoords.end());
	SkipToken(s, XFileToken::RightBrace);
}

int XFileLoader::ReadMesh(istream& s)
{
	vector<XMFLOAT3> positions;
	vector<XMFLOAT2> texCoords;
	vector<unsigned short> indices;
	vector<XMFLOAT3> normals;
	vector<unsigned short> nindices;
	auto t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::Identifier)
	{
		//m_meshes[meshIdx].m_name = t.m_content;
		t = XFileToken::NextToken(s);
	}
	if (t.m_type != XFileToken::LeftBrace)
		THROW(L"XFile Parsing Error!");
	int nElems;
	ReadMember(s, nElems);
	positions.resize(nElems);
	ReadArray(s, positions.begin(), positions.end());
	ReadFaceArray(s, indices);
	auto materialIdx = -1;
	while (true)
	{
		t = XFileToken::NextToken(s);
		if (t.m_type == XFileToken::Identifier)
		{
			if (t.m_content == "MeshNormals")
				ReadMeshNormals(s, normals, nindices);
			else if(t.m_content == "MeshMaterialList")
				materialIdx = ReadMeshMaterials(s);
			else if(t.m_content == "MeshTextureCoords")
				ReadTexCoords(s, texCoords);
			else
				SkipDataObject(s);
		}
		else if (t.m_type == XFileToken::LeftBrace)
			SkipDataReference(s);
		else if (t.m_type == XFileToken::RightBrace)
			break;
		else
			THROW(L"XFile Parsing Error!");
	}
	if (!texCoords.empty() && texCoords.size() != positions.size())
		THROW(L"XFile Parsing Error!");
	return CreateMesh(positions, texCoords, indices, normals, nindices, materialIdx);
}


int XFileLoader::CreateMesh(vector<XMFLOAT3>& positions, vector<XMFLOAT2>& texCoords, vector<unsigned short>& pindices, vector<XMFLOAT3>& normals, vector<unsigned short>& nindices, unsigned int materialIdx)
{
	if (pindices.size() != nindices.size())
		THROW(L"XFile Parsing Error!");
	//maps [position index, normal index] pair to vertex index
	map<pair<unsigned short, unsigned short>, unsigned short> lookup;
	vector<unsigned short> ib(pindices.size());
	for (unsigned int i = 0; i < pindices.size(); ++i)
	{
		auto key = make_pair(pindices[i], nindices[i]);
		unsigned short idx;
		if (lookup.count(key) == 0)
		{
			idx = static_cast<unsigned short>(lookup.size());
			lookup.insert(make_pair(key, idx));
		}
		else
			idx = lookup[key];
		ib[i] = idx;
	}
	vector<XMFLOAT3> vb(lookup.size()), nb(lookup.size());
	for(auto iter = lookup.begin(); iter != lookup.end(); ++iter)
	{
		vb[iter->second] = positions[iter->first.first];
		nb[iter->second] = normals[iter->first.second];
	}
	vector<XMFLOAT2> tb;
	if (!texCoords.empty())
	{
		tb.resize(lookup.size());
		for(auto iter = lookup.begin(); iter != lookup.end(); ++iter)
			tb[iter->second] = texCoords[iter->first.first];
	}

	int meshIdx = static_cast<int>(m_meshes.size());
	if (!tb.empty())
	{
		m_meshes.emplace_back(m_device.CreateMesh(ib, vb, nb, tb), materialIdx);
	}
	else
		m_meshes.emplace_back(m_device.CreateMesh(ib, vb, nb), materialIdx);
	return meshIdx;
}

void XFileLoader::SkipDataObject(istream& s)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::Identifier)
		t = XFileToken::NextToken(s);
	if (t.m_type != XFileToken::LeftBrace)
		THROW(L"XFile Parsing Error!");
	auto leftBraceCount = 1;
	while (true)
	{
		t = XFileToken::NextToken(s);
		if (t.m_type == XFileToken::LeftBrace)
			++leftBraceCount;
		else if (t.m_type == XFileToken::RightBrace)
		{
			--leftBraceCount;
			if (leftBraceCount == 0)
				return;
		}
		else if (t.m_type == XFileToken::None)
			THROW(L"XFile Parsing Error!");
	}
}

void XFileLoader::SkipDataReference(istream& s)
{
	auto t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::Identifier)
		t = XFileToken::NextToken(s);
	if (t.m_type == XFileToken::UUID)
		t = XFileToken::NextToken(s);
	if (t.m_type != XFileToken::RightBrace)
		THROW(L"Syntax error");
}

void XFileLoader::CheckHeader(istream& s)
{
	char header[17];
	if (!s.read(header, 16))
		THROW(L"Read error!");
	header[16] = '\0';
	if (strcmp(header, "xof 0303txt 0032") != 0)
		THROW(L"Unsuported file format!");
}