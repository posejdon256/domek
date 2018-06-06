#include "xfileToken.h"
#include "sceneNode.h"
#include "dxDevice.h"
#include "material.h"
#include <map>
#include <DirectXMath.h>
#include "nodeMesh.h"

namespace mini
{
	class XFileLoader
	{
	public:
		explicit XFileLoader(const DxDevice& d) : m_device(d) { }

		std::vector<NodeMesh> m_meshes;
		std::vector<SceneNode> m_nodes;
		std::vector<Material> m_materials;

		void Load(std::string filename);
	private:
		static void CheckHeader(std::istream& s);
		int ReadSceneNode(std::istream& s, int prevSybling);
		int ReadMesh(std::istream& s);
		void ReadMeshNormals(std::istream&s, std::vector<DirectX::XMFLOAT3>& normals, std::vector<unsigned short>& nindices);
		void ReadTexCoords(std::istream& s, std::vector<DirectX::XMFLOAT2>& texCoords);
		int ReadMeshMaterials(std::istream& s);
		int ReadMaterial(std::istream& s);
		std::string ReadTextureFilename(std::istream&s);
		int CreateMesh(std::vector<DirectX::XMFLOAT3>& positions, std::vector<DirectX::XMFLOAT2>& texCoords, std::vector<unsigned short>& indices, std::vector<DirectX::XMFLOAT3>& normals, std::vector<unsigned short>& nindices, unsigned int materiaIdx);
		int CreateMaterial(const std::string& materialName, DirectX::XMFLOAT4 diffuse, DirectX::XMFLOAT4 specular, const std::string& textureFilename);
		DirectX::XMFLOAT4X4 ReadFaceTransform(std::istream& s);
		static void SkipDataObject(std::istream& s);
		static void SkipDataReference(std::istream& s);

		template<class T>
		// ReSharper disable once CppFunctionIsNotImplemented
		void ReadData(std::istream& s, T& d);

		template<class T>
		void ReadMember(std::istream& s, T& d);

		template<class iter>
		void ReadArray(std::istream& s, iter begin, iter end);

		void ReadFaceArray(std::istream& s, std::vector<unsigned short>& indices);

		static void SkipToken(std::istream& s, XFileToken::Type type);

		const DxDevice& m_device;
		std::map<std::string, unsigned int> m_materialLookup;
	};
}
