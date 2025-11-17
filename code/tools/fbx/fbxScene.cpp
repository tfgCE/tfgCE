#include "fbxScene.h"

#include "..\..\core\memory\memory.h"

#ifdef USE_FBX
#include "fbxManager.h"
#include "fbxMesh3dExtractor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace FBX;

//

Scene* Scene::create_new(FbxScene* _scene)
{
	return new Scene(_scene);
}

Scene::Scene(FbxScene* _scene)
: scene(_scene)
{
}

Scene::~Scene()
{
	if (scene)
	{
		scene->Destroy();
	}
}

FbxNode* Scene::get_node(String const & _node)
{
	if (_node.is_empty())
	{
		return get_scene()->GetRootNode();
	}
	else
	{
		FbxNode* node = get_scene()->FindNodeByName(FbxString(_node.to_char8_array().get_data()));
		if (!node)
		{
			error(TXT("node \"%S\" not found"), _node.to_char());
		}
		return node;
	}
}

void Scene::remove_all_animations()
{
	// Keep looping until all animations are deleted.
	while (true)
	{
		FbxAnimStack* animStack = FbxCast<FbxAnimStack>(scene->GetSrcObject<FbxAnimStack>(0));
		if (!animStack)
		{
			break;
		}

		scene->RemoveAnimStack(animStack->GetName());
	}
	scene->SetCurrentAnimationStack(nullptr);
	auto* s = scene->GetCurrentAnimationStack();
	AN_BREAK;
}

#endif
