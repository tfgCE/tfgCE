#pragma once

#include "fbxLib.h"

#include "..\..\core\mesh\socketDefinitionSet.h"

#ifdef USE_FBX
namespace FBX
{
	class Scene;

	class SocketsExtractor
	{
	public:
		static void initialise_static();
		static void close_static();

		static Meshes::SocketDefinitionSet* extract_from(Scene* _scene, Meshes::SocketDefinitionSetImportOptions const & _options = Meshes::SocketDefinitionSetImportOptions());

	protected:
		SocketsExtractor(Scene* _scene);
		~SocketsExtractor();

		void extract_sockets(Meshes::SocketDefinitionSetImportOptions const & _options);

		void extract_sockets(FbxNode* _rootNode, FbxNode* _parentNode, FbxNode* _node, Meshes::SocketDefinitionSetImportOptions const & _options);

	private:
		Scene* scene = nullptr;

		Meshes::SocketDefinitionSet* sockets;
	};

};
#endif
