#include "fbxImporter.h"

#ifdef USE_FBX
#include "fbxManager.h"
#include "fbxScene.h"

#include "..\..\core\debug\extendedDebug.h"
#include "..\..\core\memory\memory.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

Concurrency::SpinLock FBX::Importer::importerLock = Concurrency::SpinLock(TXT("FBX.Importer.importerLock"));

#ifndef USE_FBX
void FBX::Importer::finished_importing()
{
}
#else
using namespace FBX;

Scene* FBX::Importer::s_scene = nullptr;
String* FBX::Importer::s_scenePath = nullptr;

void FBX::Importer::finished_importing()
{
	delete_and_clear(s_scene);
	delete_and_clear(s_scenePath);
}

#ifdef AN_ALLOW_EXTENDED_DEBUG
void display_node_for_extended_debug(FbxNode* node, String const & incline = String(TXT("")))
{
	String nextIncline = incline + String(TXT("  "));
	for (int i = 0; i < node->GetChildCount(); ++i)
	{
		FbxNode* childNode = node->GetChild(i);
		FbxNodeAttribute* attr = childNode->GetNodeAttribute();
		output(TXT("%Schild %i : %S [type %i]"), incline.to_char(), i, String::from_char8(childNode->GetName()).to_char(), attr? attr->GetAttributeType() : -1);
		display_node_for_extended_debug(childNode, nextIncline);
	}
}
#endif

Scene* FBX::Importer::import(String const & _filepath)
{
	an_assert(!_filepath.is_empty(), TXT("trying to import no-file?"));

	if (s_scenePath && _filepath == *s_scenePath && s_scene)
	{
		return s_scene;
	}

	delete_and_clear(s_scene);

	// Create an importer.
	FbxImporter* importer = FbxImporter::Create(Manager::get_sdk_manager(), "importer");
	if (!importer->Initialize(_filepath.to_char8_array().get_data(), -1, Manager::get_sdk_manager()->GetIOSettings()))
	{
		error(TXT("could not initialise import from file \"%S\""), _filepath.to_char());
		importer->Destroy();
		return nullptr;
	}

	FbxScene* scene = FbxScene::Create(Manager::get_sdk_manager(), "imported scene");
	if (!importer->Import(scene))
	{
		error(TXT("could not import from file \"%S\""), _filepath.to_char());
		importer->Destroy();
		scene->Destroy();
		return nullptr;
	}
	importer->Destroy();

	// convert scene to axis system that is used around here (x-right y-forward z-up)
	FbxAxisSystem axisSystem(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
	axisSystem.ConvertScene(scene);

	// scale to meters
	FbxSystemUnit::m.ConvertScene(scene);

#ifdef AN_ALLOW_EXTENDED_DEBUG
	IF_EXTENDED_DEBUG(FbxHierarchy)
	{
		output(TXT("imported scene from \"%S\""), _filepath.to_char());
		display_node_for_extended_debug(scene->GetRootNode(), String(TXT("    ")));
	}
#endif

	s_scene = new Scene(scene);
	delete_and_clear(s_scenePath);
	s_scenePath = new String();
	*s_scenePath = _filepath;

	return s_scene;
}
#endif
