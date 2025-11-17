#include "vrMeshes.h"

#include "..\appearance\meshStatic.h"
#include "..\game\game.h"
#include "..\library\library.h"
#include "..\jobSystem\jobSystem.h"
#include "..\render\renderScene.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\vr\iVR.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

using namespace Framework;

//

DEFINE_STATIC_NAME(vr);
DEFINE_STATIC_NAME_STR(vrTrackingDeviceMaterial, TXT("vr tracking device material"));
DEFINE_STATIC_NAME_STR(vrTrackingDeviceLeftMaterial, TXT("vr tracking device left material"));
DEFINE_STATIC_NAME_STR(vrTrackingDeviceRightMaterial, TXT("vr tracking device right material"));
DEFINE_STATIC_NAME_STR(vrController, TXT("vr controller"));

//

VRMeshes::VRMeshes(Library* _library)
: library(_library)
{
}

bool VRMeshes::unload(LibraryLoadLevel::Type _libraryLoadLevel)
{
	Concurrency::ScopedSpinLock lock(meshesLock);
	meshes.clear();
	return true;
}

struct LoadRenderModel
: public Concurrency::AsynchronousJobData
{
	VRMeshes* vrMeshes;
	Name modelName;
	LoadRenderModel(VRMeshes* _vrMeshes, Name const & _modelName)
	: vrMeshes(_vrMeshes)
	, modelName(_modelName)
	{}

};

void VRMeshes::load_render_model(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	LoadRenderModel* data = plain_cast<LoadRenderModel>(_data);

	Meshes::Mesh3D* mesh = VR::IVR::get()->load_mesh(data->modelName);

	if (mesh)
	{
		// create library mesh and store there
		Mesh* frameworkMesh;
		LibraryName libraryName(NAME(vr), data->modelName);
		frameworkMesh = data->vrMeshes->library->get_meshes_static().find_or_create(libraryName);
		frameworkMesh->replace_mesh(mesh);

		if (auto* material = data->vrMeshes->library->get_global_references().get<Material>(NAME(vrTrackingDeviceMaterial)))
		{
			frameworkMesh->set_material(0, material);
		}

		Concurrency::ScopedSpinLock lock(data->vrMeshes->meshesLock);
		data->vrMeshes->meshes[data->modelName] = frameworkMesh;
	}
}

Mesh* VRMeshes::get_mesh_for_hand(int _handIndex)
{
	Name const & modelName = Name::invalid();// VR::IVR::get()->get_model_name_for_hand(_handIndex);
	Concurrency::ScopedSpinLock lock(meshesLock);
	if (auto * existing = meshes.get_existing(modelName))
	{
		return existing->get();
	}
	if (!modelName.is_valid())
	{
		auto* mesh = library->get_global_references().get<MeshStatic>(NAME(vrController));
		meshes[modelName] = mesh;
		return mesh;
	}
	if (auto* mesh = library->get_global_references().get<Mesh>(modelName, true))
	{
		meshes[modelName] = mesh;
		return mesh;
	}
	if (auto * game = Game::get())
	{
		// add here and load async
		meshes[modelName].clear();

		game->async_background_job(game, load_render_model, new LoadRenderModel(this, modelName));

		// ask again in a while
		return nullptr;
	}
	else
	{
		return nullptr;
	}
}

void VRMeshes::add_hands_to_render_scenes(Render::Scene** _scenes, int _sceneCount, Transform const & _vrAnchor)
{
	for_count(int, handIdx, 2)
	{
		auto * mesh = Library::get_current()->access_vr_meshes().get_mesh_for_hand(handIdx);
		if (mesh)
		{
			Transform meshPlacement(Vector3(0.0f, 0.0f, 1.0f), Quat::identity);
			auto & hand = VR::IVR::get()->get_render_pose_set().hands[handIdx == 0? VR::IVR::get()->get_left_hand() : VR::IVR::get()->get_right_hand()];
			if (hand.placement.is_set())
			{
				meshPlacement = hand.placement.get();
			}
			meshPlacement = _vrAnchor.to_world(meshPlacement);
			Meshes::Mesh3DInstance & meshInstance = instances[handIdx];
			meshInstance.set_mesh(mesh->get_mesh());
			MaterialSetup::apply_material_setups(mesh->get_material_setups(), meshInstance, ::System::MaterialSetting::IndividualInstance);
			if (auto* material = Library::get_current()->get_global_references().get<Material>(handIdx == 0? NAME(vrTrackingDeviceLeftMaterial) : NAME(vrTrackingDeviceRightMaterial)))
			{
				meshInstance.set_material(0, material->get_material());
			}
			meshInstance.fill_materials_with_missing();
			for_count(int, sceneIdx, _sceneCount)
			{
				_scenes[sceneIdx]->add_extra(&meshInstance, meshPlacement);
			}
		}
	}
}


