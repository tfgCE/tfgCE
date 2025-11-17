#pragma once

#include "fbxLib.h"

#include "..\..\core\math\math.h"
#include "..\..\core\types\string.h"

namespace Meshes
{
	class Mesh3D;
	class Skeleton;
	struct Mesh3DImportOptions;
	struct Mesh3DPart;
};

#ifdef USE_FBX
namespace FBX
{
	class Scene;

	class Mesh3DExtractor
	{
	public:
		static void initialise_static();
		static void close_static();

		static Meshes::Mesh3D* extract_from(Scene* _scene, Meshes::Mesh3DImportOptions const & _options);

	protected:
		Mesh3DExtractor(Scene* _scene, Meshes::Mesh3DImportOptions const & _options);
		~Mesh3DExtractor();

		void extract_mesh(String const & _rootNode = String::empty());

	private:
		struct MeshNode
		{
			Matrix44 globalMatrix; // to mesh
			Matrix44 localMatrix; // to parent
			FbxNode* meshNode;
		};

		Meshes::Mesh3DImportOptions const & options;

		Scene* scene = nullptr;
		Meshes::Skeleton* skeleton = nullptr;

		Meshes::Mesh3D* mesh = nullptr;
		Array<MeshNode> meshNodes;

		void find_nodes_containing_mesh(FbxNode * _inNode);
		Meshes::Mesh3DPart* extract_node_to_mesh(MeshNode* _meshNode, MeshNode* _rootNode);
	};

};
#endif
