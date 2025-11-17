#pragma once

#include "fbxLib.h"

#include "..\..\core\mesh\mesh3d.h"

#ifdef USE_FBX
namespace FBX
{
	class Importer;

	class Scene
	{
	public:
		static Scene* create_new(FbxScene* _scene);

		FbxScene* get_scene() { return scene; }
		FbxNode* get_node(String const & _node = String::empty()); // empty for root node

		void remove_all_animations();

	private: friend class Importer;
		Scene(FbxScene* _scene);
		~Scene();

	private:
		FbxScene* scene;
	};
};
#endif
