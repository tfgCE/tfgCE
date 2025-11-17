#include "moduleAppearance.h"

#include "modules.h"
#include "moduleDataImpl.inl"
#include "modulePresence.h"
#include "registeredModule.h"

#include "..\advance\advanceContext.h"
#include "..\appearance\appearanceController.h"
#include "..\appearance\appearanceControllerData.h"
#include "..\appearance\appearanceControllerPoseContext.h"
#include "..\appearance\mesh.h"
#include "..\appearance\meshDepot.h"
#include "..\appearance\skeleton.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\meshGen\meshGenerator.h"
#include "..\render\renderContext.h"
#include "..\world\room.h"
#include "..\world\world.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\mesh\pose.h"

#ifdef AN_ASSERT
#include "..\debug\previewGame.h"
#endif

#ifdef AN_ALLOW_BULLSHOTS
#include "..\game\bullshotSystem.h"
#include "..\object\temporaryObject.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// module parameters
DEFINE_STATIC_NAME(useMovementCollisionBoundingBoxForFrustumCheck);
DEFINE_STATIC_NAME(usePreciseCollisionBoundingBoxForFrustumCheck);
DEFINE_STATIC_NAME(useBonesBoundingBoxForFrustumCheck);
DEFINE_STATIC_NAME(expandBonesBoundingBox);
DEFINE_STATIC_NAME(visible);
DEFINE_STATIC_NAME(visibleToOwnerOnly);
DEFINE_STATIC_NAME(renderingBuffer);
DEFINE_STATIC_NAME(autoMeshGenerate);
DEFINE_STATIC_NAME(dontAutoMeshGenerate);
DEFINE_STATIC_NAME(noLODs);
DEFINE_STATIC_NAME(aggressiveLOD);
DEFINE_STATIC_NAME(aggressiveLODStart);
DEFINE_STATIC_NAME(ignoreRenderingBeyondDistance);
DEFINE_STATIC_NAME(sharedMeshLibraryName);
DEFINE_STATIC_NAME(sharedMeshLibraryNameSuffix);

//

void prepare_random_generator_for_pois(Random::Generator& rg)
{
	rg.advance_seed(239754, 2934);
}

//

REGISTER_FOR_FAST_CAST(ModuleAppearance);

static ModuleAppearance* create_module(IModulesOwner* _owner)
{
	return new ModuleAppearance(_owner);
}

RegisteredModule<ModuleAppearance> & ModuleAppearance::register_itself()
{
	return Modules::appearance.register_module(String(TXT("base")), create_module);
}

ModuleAppearance::ModuleAppearance(IModulesOwner* _owner)
: Module(_owner)
, preliminaryPoseLS(nullptr)
, workingFinalPoseLS(nullptr)
, finalPoseLS(nullptr)
, prevFinalPoseLS(nullptr)
, finalPoseMS(nullptr)
, prevFinalPoseMS(nullptr)
{
}

ModuleAppearance::~ModuleAppearance()
{
	pois.clear();
	clear_controllers();
	clear_mesh();
	clear_skeleton();
	clear_animation_sets();
}

void ModuleAppearance::be_main_appearance()
{
	an_assert(get_owner());
	for_every_ptr(a, get_owner()->get_appearances())
	{
		a->mainAppearance = a == this;
	}
}

void ModuleAppearance::reset()
{
	tags = Tags::none;
	name = Name::invalid();
	syncTo = Name::invalid();
	useControllers = true;
	pois.clear();
	meshPOIsProcessed = false;
	clear_controllers();
	clear_mesh();
	clear_skeleton();
	clear_animation_sets();
	Module::reset();
}

void ModuleAppearance::update_mesh_instance_individual_offset()
{
	Random::Generator rg = get_owner()->get_individual_random_generator();
	rg.advance_seed(86785, 23947);
	meshInstance.set_individual_offset(rg.get_float(-1000.0f, 1000.0f));

	for_every(lmi, lodMeshInstances)
	{
		lmi->meshInstance.use_individual_offset_of(meshInstance);
	}
}

void ModuleAppearance::setup_with(ModuleData const * _moduleData)
{
	Module::setup_with(_moduleData);
	moduleAppearanceData = fast_cast<ModuleAppearanceData>(_moduleData);
	if (moduleAppearanceData)
	{
		useLODs = ! _moduleData->get_parameter<float>(this, NAME(noLODs), ! useLODs);
#ifdef AN_DEVELOPMENT
		if (!Framework::is_preview_game())
		{
			useLODs = false; // faster generation
		}
#endif
 
		aggressiveLOD = _moduleData->get_parameter<float>(this, NAME(aggressiveLOD), aggressiveLOD);
		aggressiveLODStart = _moduleData->get_parameter<float>(this, NAME(aggressiveLODStart), aggressiveLODStart);

		visible = _moduleData->get_parameter<bool>(this, NAME(visible), visible);
		visibleToOwnerOnly = _moduleData->get_parameter<bool>(this, NAME(visibleToOwnerOnly), visibleToOwnerOnly);

		useMovementCollisionBoundingBoxForFrustumCheck = _moduleData->get_parameter<bool>(this, NAME(useMovementCollisionBoundingBoxForFrustumCheck), useMovementCollisionBoundingBoxForFrustumCheck);
		usePreciseCollisionBoundingBoxForFrustumCheck = _moduleData->get_parameter<bool>(this, NAME(usePreciseCollisionBoundingBoxForFrustumCheck), usePreciseCollisionBoundingBoxForFrustumCheck);
		useBonesBoundingBoxForFrustumCheck = _moduleData->get_parameter<bool>(this, NAME(useBonesBoundingBoxForFrustumCheck), useBonesBoundingBoxForFrustumCheck);
		expandBonesBoundingBox = _moduleData->get_parameter<float>(this, NAME(expandBonesBoundingBox), expandBonesBoundingBox);

		sharedMeshLibraryName = _moduleData->get_parameter<LibraryName>(this, NAME(sharedMeshLibraryName), sharedMeshLibraryName);
		sharedMeshLibraryNameSuffix = _moduleData->get_parameter<RangeInt>(this, NAME(sharedMeshLibraryNameSuffix), sharedMeshLibraryNameSuffix);

		bool autoMeshGenerate = true;
		autoMeshGenerate = _moduleData->get_parameter<bool>(this, NAME(autoMeshGenerate), autoMeshGenerate);
		autoMeshGenerate = ! _moduleData->get_parameter<bool>(this, NAME(dontAutoMeshGenerate), ! autoMeshGenerate);
		
		{
			float v = _moduleData->get_parameter<float>(this, NAME(ignoreRenderingBeyondDistance), -1.0f);
			if (v >= 0.0f)
			{
				ignoreRenderingBeyondDistance = v;
			}
			else
			{
				ignoreRenderingBeyondDistance.clear();
			}
		}

		tags = moduleAppearanceData->tags;
		name = moduleAppearanceData->name;
		syncTo = moduleAppearanceData->syncTo;
		useControllers = moduleAppearanceData->useControllers;

		meshInstance.set_preferred_rendering_buffer(_moduleData->get_parameter<Name>(this, NAME(renderingBuffer), meshInstance.get_preferred_rendering_buffer()));
		update_mesh_instance_individual_offset();

		animationSets.add_from(moduleAppearanceData->animationSets);

		if (autoMeshGenerate)
		{
			if (moduleAppearanceData->skelGenerator.is_set() &&
				sharedMeshLibraryName.is_valid())
			{
				error(TXT("can't use shared mesh library name with skeleton generator. why do you even use skel generator? use mesh generator instead!"));
			}
			// get skeleton first
			if (moduleAppearanceData->skelGenerator.get())
			{
				if (auto* to = get_owner()->get_as_temporary_object())
				{
					warn(TXT("please do not use skel generator for temporary objects! (check %S)"), to->get_object_type()->get_name().to_string().to_char());
				}
				SimpleVariableStorage variables = moduleAppearanceData->generatorVariables;
				variables.set_from(get_owner()->get_variables());
				MeshGeneratorRequest request;
				request.for_wmp_owner(this)
					.using_parameters(variables)
					.using_random_regenerator(get_owner()->get_individual_random_generator())
					.no_lods();
				if (setup_mesh_generator_request)
				{
					setup_mesh_generator_request(request);
				}
				if (auto* skel = moduleAppearanceData->skelGenerator->generate_skel(request))

				{
					UsedLibraryStored<Skeleton> ownSkel;
					ownSkel = Library::get_current()->get_skeletons().create_temporary();
					ownSkel->replace_skeleton(skel);
					use_skeleton(ownSkel.get());
				}
			}
			if (!skeleton.is_set() && moduleAppearanceData->skeleton.get())
			{
				use_skeleton(moduleAppearanceData->skeleton.get());
			}

			provideSkeletonToMeshGenerator = skeleton.is_set();

			if (moduleAppearanceData->meshGenerator.get() || meshGenerator.get())
			{
				if (auto* to = get_owner()->get_as_temporary_object())
				{
					warn(TXT("please do not use mesh generator for temporary objects! (check %S)"), to->get_object_type()->get_name().to_string().to_char());
				}
				create_mesh_with_mesh_generator_internal(false);
			}
			else if (auto* md = moduleAppearanceData->meshDepot.get())
			{
				md->load_on_demand_if_required();
				Random::Generator rg = get_owner()->get_individual_random_generator();
				rg.advance_seed(23975, 97240);
				use_mesh_internal(md->get_mesh_for(get_owner()));
				create_mesh_with_mesh_generator_internal(false);
			}
			else if (!moduleAppearanceData->meshes.is_empty())
			{
				Random::Generator rg = get_owner()->get_individual_random_generator();
				rg.advance_seed(23975, 97240);
				int idx = rg.get_int(moduleAppearanceData->meshes.get_size());
				use_mesh_internal(moduleAppearanceData->meshes[idx].get());
			}

			if (!mesh.is_set() && !pendingMesh.is_set())
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (! Framework::Library::may_ignore_missing_references())
#endif
				error(TXT("no mesh provided, even empty"));
			}

			add_controllers_from_data();
		}
		else
		{
			if (!moduleAppearanceData->get_controllers().is_empty())
			{
				error(TXT("can't have controllers via data and skip autoMeshGenerate \"%S\""), get_owner()->ai_get_name().to_char());
			}
		}
		get_owner()->update_appearance_cached();
	}
}

void ModuleAppearance::add_controllers_from_data()
{
	Array<AppearanceControllerDataPtr> const& controllersData = moduleAppearanceData->get_controllers();
	if (!controllersData.is_empty() && useControllers)
	{
		for_every_ref(controllerData, controllersData)
		{
			controllers.push_back(AppearanceControllerPtr(controllerData->create_controller()));
			requireAutoProcessingOrderForControllers = true;
		}
		get_owner()->update_appearance_cached();
	}
}

void ModuleAppearance::recreate_mesh_with_mesh_generator(bool _changeAsSoonAsPossible)
{
	create_mesh_with_mesh_generator_internal(true, _changeAsSoonAsPossible);
}

void ModuleAppearance::create_mesh_with_mesh_generator_internal(bool _force, bool _changeAsSoonAsPossible)
{
	if (!_force && mesh.is_set())
	{
		return;
	}

	/**
	 *	Please note, that everything may change and other stuff has to be redone (looking up sockets etc)
	 *	Use with extreme care!
	 */
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	auto* asObject = get_owner_as_object();
	auto* asTemp = get_owner_as_temporary_object();
	bool isWorldActive = (asObject && asObject->is_world_active()) || (asTemp && asTemp->is_active());

	if (asObject && isWorldActive)
	{
		if (Game::get()->is_on_short_pause())
		{
			clear_controllers();
			pois.clear();
			meshPOIsProcessed = false;
		}
		else
		{
			Game::get()->perform_sync_world_job(TXT("clean up gun bits"), [this]()
			{
				clear_controllers();
				pois.clear();
				meshPOIsProcessed = false;
			});
		}
		
	}

	MeshGenerator* useMeshGenerator = meshGenerator.get();
	if (!useMeshGenerator)
	{
		useMeshGenerator = moduleAppearanceData->meshGenerator.get();
	}
	if (!useMeshGenerator)
	{
		error(TXT("no mesh generator provided for \"%S\""), get_owner()->ai_get_name().to_char());
		return;
	}

	//

	bool generateNow = true;
	LibraryName actualSharedMeshLibraryName = sharedMeshLibraryName;
	if (actualSharedMeshLibraryName.is_valid())
	{
		if (!sharedMeshLibraryNameSuffix.is_empty())
		{
			auto rg = get_owner()->get_individual_random_generator();
			actualSharedMeshLibraryName = LibraryName::from_string(actualSharedMeshLibraryName.to_string() + String::printf(TXT(" %i"), rg.get(sharedMeshLibraryNameSuffix)));
		}
		if (auto* lib = Library::get_current())
		{
			UsedLibraryStored<Mesh> useExistingMesh;
			useExistingMesh = lib->get_meshes().find(actualSharedMeshLibraryName);
			if (useExistingMesh.get())
			{
				useExistingMesh->be_shared();
				pendingMesh = useExistingMesh;
				generateNow = false;
			}
		}
	}
	if (generateNow)
	{
		SimpleVariableStorage variables = moduleAppearanceData->generatorVariables;
		variables.set_from(get_owner()->get_variables());
		MeshGeneratorRequest request;
		request.for_wmp_owner(this)
			   .use_library_name_for_new(actualSharedMeshLibraryName)
			   .using_parameters(variables)
			   .using_random_regenerator(get_owner()->get_individual_random_generator())
			   .using_skeleton(provideSkeletonToMeshGenerator ? skeleton.get() : nullptr);
		if (!useLODs)
		{
			request.no_lods();
		}
		if (setup_mesh_generator_request)
		{
			setup_mesh_generator_request(request);
		}
		UsedLibraryStored<Mesh> ownMesh = useMeshGenerator->generate_temporary_library_mesh(Library::get_current(), request);
		if (ownMesh.is_set())
		{
			// this is the moment when we replace mesh
			// for an object that is being created, we're free to do anything we want at any time
			// for an object that is active, we should do it in sync
			if (sharedMeshLibraryName.is_valid())
			{
				ownMesh->be_shared();
			}
			else
			{
				ownMesh->be_unique_instance();
			}
			pendingMesh = ownMesh;
		}
	}

	if (_changeAsSoonAsPossible)
	{
		setup_with_pending_mesh();
	}
}

void ModuleAppearance::setup_with_pending_mesh()
{
	if (!pendingMesh.is_set())
	{
		error(TXT("no pending mesh"));
		return;
	}

	// store existing mesh and skeleton in case we replace them. if so, remove them from library
	UsedLibraryStored<Mesh> existingMesh = mesh;
	UsedLibraryStored<Skeleton> existingSkeleton = skeleton;

	if (Framework::Game::get()->is_performing_async_job_on_this_thread())
	{
		if ((get_owner_as_object() && get_owner_as_object()->is_world_active()) ||
			(get_owner_as_temporary_object() && get_owner_as_temporary_object()->is_active()))
		{
			Game::get()->perform_sync_world_job(TXT("replace mesh"),
				[this]
				{
					setup_with_just_generated_mesh(pendingMesh.get());
				});
		}
		else
		{
			setup_with_just_generated_mesh(pendingMesh.get());
		}

		if (existingMesh.get() &&
			existingMesh != mesh &&
			existingMesh->is_temporary())
		{
			Library::get_current()->unload_and_remove(existingMesh.get());
		}
		if (existingSkeleton.get() &&
			existingSkeleton != skeleton &&
			existingSkeleton->is_temporary())
		{
			Library::get_current()->unload_and_remove(existingSkeleton.get());
		}
	}
	else
	{
		Game::get()->add_immediate_sync_world_job(TXT("replace mesh"),
			[this, existingMesh, existingSkeleton]
			{
				if (pendingMesh.is_set())
				{
					setup_with_just_generated_mesh(pendingMesh.get());
					pendingMesh.clear();
					if (existingMesh.get() &&
						existingMesh != mesh &&
						existingMesh->is_temporary())
					{
						Library::get_current()->unload_and_remove(existingMesh.get());
					}
					if (existingSkeleton.get() &&
						existingSkeleton != skeleton &&
						existingSkeleton->is_temporary())
					{
						Library::get_current()->unload_and_remove(existingSkeleton.get());
					}
				}
			});
	}
}

void ModuleAppearance::setup_with_just_generated_mesh(Mesh const* _mesh)
{
	if (_mesh->get_skeleton() && get_skeleton() != _mesh->get_skeleton())
	{
		use_skeleton(_mesh->get_skeleton());
	}
	use_mesh_internal(_mesh);

	if ((get_owner_as_object() && get_owner_as_object()->is_world_active()) ||
		(get_owner_as_temporary_object() && get_owner_as_temporary_object()->is_active())) 
	{
		process_mesh_pois(true);
		add_controllers_from_data();
		initialise_not_initialised_controllers();
		activate_inactive_controllers();
		update_collision_module_if_initialised();
		auto_processing_order_for_controllers_if_required();

		get_owner_as_object()->get_presence()->refresh_attachment();
	}
}

void ModuleAppearance::initialise()
{
	base::initialise();

	initialise_not_initialised_controllers();

	auto_processing_order_for_controllers();

	// setup collision model
	update_collision_module_if_initialised();
}

void ModuleAppearance::initialise_not_initialised_controllers()
{
	if (is_initialised())
	{
		for_every_ref(controller, controllers)
		{
			if (!controller->is_initialised())
			{
				controller->initialise(this);
			}
		}
	}
}

void ModuleAppearance::ready_to_activate()
{
	process_mesh_pois();

	auto_processing_order_for_controllers_if_required();

	base::ready_to_activate();
}

void ModuleAppearance::activate()
{
	base::activate();

	activate_inactive_controllers();

	if (moduleAppearanceData)
	{
		copy_material_parameters();
	}
}

void ModuleAppearance::activate_inactive_controllers()
{
	if (was_activated())
	{
		for_every_ref(controller, controllers)
		{
			controller->activate();
		}
	}
}

void ModuleAppearance::update_collision_module_if_initialised()
{
	if (!is_initialised())
	{
		return;
	}
	ModuleCollision * mc = get_owner()->get_collision();
	if (mainAppearance && mc)
	{
		bool updateBoundingBox = false;
		if (mesh.is_set() && mesh->get_movement_collision_model())
		{
			todo_note(TXT("maybe only if collision model or collision module allows that? TN#11"));
			if (auto* ci = mc->use_movement_collision(mesh->get_movement_collision_model()->get_model()))
			{
				usingMeshMovementCollisionId = ci->get_id_within_set();
			}
			else
			{
				usingMeshMovementCollisionId = NONE;
			}
			updateBoundingBox = true;
		}
		if (mesh.is_set() && mesh->get_precise_collision_model())
		{
			todo_note(TXT("maybe only if collision model or collision module allows that? TN#11"));
			auto * ci = mc->use_precise_collision(mesh->get_precise_collision_model()->get_model());
			usingMeshPreciseCollisionId = ci->get_id_within_set();
			updateBoundingBox = true;
		}

		if (updateBoundingBox)
		{
			mc->update_bounding_box();
		}
	}
}

void ModuleAppearance::clear_controllers()
{
	controllers.clear();
}

void ModuleAppearance::clear_animation_sets()
{
	animationSets.clear();
}

void ModuleAppearance::clear_mesh()
{
	// clear movement collision (if available, if set)
	ModuleCollision * mc = get_owner()->get_collision();
	if (mainAppearance && mc)
	{
		if (usingMeshMovementCollisionId != NONE)
		{
			if (auto const * mcm = mesh->get_movement_collision_model())
			{
				mc->clear_movement_collision();
			}
		}
		if (usingMeshPreciseCollisionId != NONE)
		{
			if (auto const * mcm = mesh->get_precise_collision_model())
			{
				mc->clear_precise_collision();
			}
		}
	}
	usingMeshMovementCollisionId = NONE;
	usingMeshPreciseCollisionId = NONE;

	if (mesh.is_set())
	{
		for (int i = 0; i < controllers.get_size(); ++i)
		{
			if (auto* data = controllers[i]->get_data())
			{
				bool used = false;
				for_every_ref(controllerData, mesh->get_controllers())
				{
					if (data == controllerData)
					{
						used = true;
						break;
					}
				}
				if (used)
				{
					controllers.remove_at(i);
					--i;
				}
			}
		}
	}

	mesh.clear();
	meshInstance.set_mesh(nullptr, nullptr);
}

void ModuleAppearance::clear_skeleton()
{
	if (skeleton.get())
	{
		an_assert(preliminaryPoseLS);
		preliminaryPoseLS->release();
		an_assert(workingFinalPoseLS);
		workingFinalPoseLS->release();
		an_assert(finalPoseLS);
		finalPoseLS->release();
		an_assert(finalPoseMS);
		finalPoseMS->release();
		an_assert(prevFinalPoseLS);
		prevFinalPoseLS->release();
		an_assert(prevFinalPoseMS);
		prevFinalPoseMS->release();
		for (int i = 0; i < controllers.get_size(); ++i)
		{
			if (auto* data = controllers[i]->get_data())
			{
				bool used = false;
				for_every_ref(controllerData, skeleton->get_controllers())
				{
					if (data == controllerData)
					{
						used = true;
						break;
					}
				}
				if (used)
				{
					controllers.remove_at(i);
					--i;
				}
			}
		}
	}
	skeleton.clear();
	preliminaryPoseLS = nullptr;
	workingFinalPoseLS = nullptr;
	finalPoseLS = nullptr;
	finalPoseMS = nullptr;
	prevFinalPoseLS = nullptr;
	prevFinalPoseMS = nullptr;
}

void ModuleAppearance::use_another_mesh()
{
	if (meshFromAppearanceDataMeshes && moduleAppearanceData->meshes.get_size() > 1)
	{
		++meshFromAppearanceDataMeshesIdx;
		Random::Generator rg = get_owner()->get_individual_random_generator();
		rg.advance_seed(23975 + meshFromAppearanceDataMeshesIdx, 97240);
		use_mesh(moduleAppearanceData->meshes[rg.get_int(moduleAppearanceData->meshes.get_size())].get());
	}
}

void ModuleAppearance::use_mesh_generator_on_setup(MeshGenerator const* _meshGenerator)
{
	an_assert(!is_ready_to_activate() || fast_cast<TemporaryObject>(get_owner()), TXT("atm cannot use mesh after it was readied to activate, there is no POI processing, think about changing meshes on the fly; temporary objects are fine because they reuse mesh"));
	meshGenerator = _meshGenerator;
}

void ModuleAppearance::use_mesh(Mesh const* _mesh)
{
	an_assert(!is_ready_to_activate() || fast_cast<TemporaryObject>(get_owner()), TXT("atm cannot use mesh after it was readied to activate, there is no POI processing, think about changing meshes on the fly; temporary objects are fine because they reuse mesh"));
	use_mesh_internal(_mesh);
}

void ModuleAppearance::use_mesh_internal(Mesh const* _mesh)
{
	// this might be also used to reinitialise mesh
	UsedLibraryStored<Mesh> __mesh(_mesh);
	clear_mesh();
	mesh = __mesh;
	if (mesh.is_set())
	{
		mesh->load_on_demand_if_required();
		mesh->generated_on_demand_if_required();

		meshFromAppearanceDataMeshes = false;
		if (moduleAppearanceData) // as it is possible to call use_mesh pre setup
		{
			for_every(m, moduleAppearanceData->meshes)
			{
				if (mesh.get() == m->get())
				{
					meshFromAppearanceDataMeshes = true;
				}
			}
		}
		if (Skeleton const * skeleton = mesh->get_skeleton())
		{
			meshInstance.set_mesh(mesh->get_mesh(), skeleton->get_skeleton());
			use_skeleton(skeleton);
		}
		else
		{
			meshInstance.set_mesh(mesh->get_mesh());
		}
		if (auto* m3d = mesh->get_mesh())
		{
			// mark parts to load to gpu when possible
			m3d->mark_to_load_to_gpu();
		}

		MaterialSetup::apply_material_setups(mesh->get_material_setups(), meshInstance, System::MaterialSetting::NotIndividualInstance);
		if (moduleAppearanceData)
		{
			MaterialSetup::apply_material_setups(moduleAppearanceData->materialSetups, meshInstance, System::MaterialSetting::NotIndividualInstance); // override with modules setup
		}

		{
			lodMeshInstances.set_size(mesh->get_lod_count() - 1);
			for_every(lmi, lodMeshInstances)
			{
				auto* useMesh = mesh->get_lod(for_everys_index(lmi) + 1);
				if (auto* m3d = useMesh->get_mesh())
				{
					// mark parts to load to gpu when possible
					m3d->mark_to_load_to_gpu();
				}
				if (Skeleton const* skeleton = useMesh->get_skeleton())
				{
					lmi->meshInstance.set_mesh(useMesh->get_mesh(), skeleton->get_skeleton());
				}
				else
				{
					lmi->meshInstance.set_mesh(useMesh->get_mesh());
				}
				lmi->remapBones = mesh->get_remap_bones_for_lod(for_everys_index(lmi) + 1);
				MaterialSetup::apply_material_setups(useMesh->get_material_setups(), lmi->meshInstance, System::MaterialSetting::NotIndividualInstance);
				if (moduleAppearanceData)
				{
					MaterialSetup::apply_material_setups(moduleAppearanceData->materialSetups, lmi->meshInstance, System::MaterialSetting::NotIndividualInstance); // override with modules setup
				}
			}
		}

		if (useControllers)
		{
			for_every_ref(controllerData, mesh->get_controllers())
			{
				controllers.push_back(AppearanceControllerPtr(controllerData->create_controller()));
				requireAutoProcessingOrderForControllers = true;
			}
		}

		update_mesh_instance_individual_offset(); // set using owner's params + set along lods

		initialise_not_initialised_controllers();
		activate_inactive_controllers();

		update_collision_module_if_initialised();

		if (moduleAppearanceData)
		{
			copy_material_parameters();
		}

		get_owner()->update_appearance_cached();
	}
}

void ModuleAppearance::use_skeleton(Skeleton const * _skeleton)
{
	if (skeleton.get() == _skeleton &&
		(!skeleton.get() || !_skeleton ||
		 skeleton->get_skeleton()->get_num_bones() == _skeleton->get_skeleton()->get_num_bones())) // we might be using fused skeleton
	{
		return;
	}
	clear_skeleton();
	skeleton = _skeleton;
	if (skeleton.get())
	{
		preliminaryPoseLS = Meshes::Pose::get_one(skeleton->get_skeleton());
		workingFinalPoseLS = Meshes::Pose::get_one(skeleton->get_skeleton());
		finalPoseLS = Meshes::Pose::get_one(skeleton->get_skeleton());
		finalPoseMS = Meshes::Pose::get_one(skeleton->get_skeleton());
		prevFinalPoseLS = Meshes::Pose::get_one(skeleton->get_skeleton());
		prevFinalPoseMS = Meshes::Pose::get_one(skeleton->get_skeleton());
		finalPoseMatMS.set_size(finalPoseLS->get_num_bones());
		prevFinalPoseMatMS.set_size(finalPoseLS->get_num_bones());
		// fill with defaults
		preliminaryPoseLS->fill_with(skeleton->get_skeleton()->get_bones_default_placement_LS());
		workingFinalPoseLS->fill_with(skeleton->get_skeleton()->get_bones_default_placement_LS());
		finalPoseLS->fill_with(skeleton->get_skeleton()->get_bones_default_placement_LS());
		finalPoseMS->fill_with(skeleton->get_skeleton()->get_bones_default_placement_MS());
		prevFinalPoseLS->fill_with(skeleton->get_skeleton()->get_bones_default_placement_LS());
		prevFinalPoseMS->fill_with(skeleton->get_skeleton()->get_bones_default_placement_MS());
		skeleton->get_skeleton()->change_bones_ls_to_ms(finalPoseLS->get_bones(), OUT_ finalPoseMatMS);
		skeleton->get_skeleton()->change_bones_ls_to_ms(prevFinalPoseLS->get_bones(), OUT_ prevFinalPoseMatMS);
		if (useControllers)
		{
			for_every_ref(controllerData, skeleton->get_controllers())
			{
				controllers.push_back(AppearanceControllerPtr(controllerData->create_controller()));
				requireAutoProcessingOrderForControllers = true;
			}
		}
		initialise_not_initialised_controllers();
		activate_inactive_controllers();
	}
	get_owner()->update_appearance_cached();
}

void ModuleAppearance::advance_vr_important__controllers_and_attachments(IModulesOwner* _modulesOwner, float _deltaTime)
{
	if (!_modulesOwner->get_presence()->is_attached())
	{
		calculate_preliminary_pose(_modulesOwner);
#ifndef AN_ADVANCE_VR_IMPORTANT_ONCE
		_deltaTime = 0.0f;
#endif
#ifdef AN_ADVANCE_VR_IMPORTANT_ONCE
		// if we don't do for attached here, it won't be done at all
		// as it has to be done in this order and when we calculate final pose, it is calculated and done
		advance_controllers_and_adjust_preliminary_pose_and_attachments(_modulesOwner, _deltaTime, PoseAndAttachmentsReason::VRImportant);
#else
		advance_controllers_and_adjust_preliminary_pose(_modulesOwner, _deltaTime, PoseAndAttachmentsReason::VRImportant);
#endif
		calculate_final_pose_and_attachments_0(_modulesOwner, _deltaTime, PoseAndAttachmentsReason::VRImportant);
	}
}

void ModuleAppearance::advance__calculate_preliminary_pose(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(calculatePreliminaryPose);
		{
			// store prev pose
			for_every_ptr(self, modulesOwner->get_appearances())
			{
				self->prevFinalPoseLS->fill_with(self->finalPoseLS);
			}
			calculate_preliminary_pose(modulesOwner);
		}
	}
}

void ModuleAppearance::calculate_preliminary_pose(IModulesOwner * _modulesOwner)
{
	MEASURE_PERFORMANCE(calculatePreliminaryPoseDo);
	for_every_ptr(self, _modulesOwner->get_appearances())
	{
		if (::Framework::Skeleton const* skeleton = self->skeleton.get())
		{
			self->preliminaryPoseLS->fill_with(skeleton->get_skeleton()->get_bones_default_placement_LS()); // test line - this should be output taken from animation system
		}
	}
}

void ModuleAppearance::advance__advance_controllers_and_adjust_preliminary_pose(IMMEDIATE_JOB_PARAMS)
{
	Framework::AdvanceContext const* ac = plain_cast<Framework::AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(advanceControllersAndAdjustPreliminaryPose);
		advance_controllers_and_adjust_preliminary_pose(modulesOwner, ac->get_delta_time(), PoseAndAttachmentsReason::Normal);
	}
}

void ModuleAppearance::advance_controllers_and_adjust_preliminary_pose(IModulesOwner* _modulesOwner, float _deltaTime, PoseAndAttachmentsReason::Type _reason)
{
	MEASURE_PERFORMANCE(advanceControllersAndAdjustPreliminaryPoseDo);
	for_every_ptr(self, _modulesOwner->get_appearances())
	{
#ifdef AN_ADVANCE_VR_IMPORTANT_ONCE
		if (_reason == PoseAndAttachmentsReason::VRImportant ||
			! self->doneAsVRImportant)
#endif
		{
			self->advance_controllers_and_adjust_preliminary_pose(_modulesOwner->get_pose_advance_delta_time());
		}
	}
}

void ModuleAppearance::advance_controllers_and_adjust_preliminary_pose_and_attachments(IModulesOwner* _modulesOwner, float _deltaTime, PoseAndAttachmentsReason::Type _reason)
{
	an_assert(_reason == PoseAndAttachmentsReason::VRImportant);
	advance_controllers_and_adjust_preliminary_pose(_modulesOwner, _deltaTime, _reason);

	// now work through the attachments
	auto* presence = _modulesOwner->get_presence();
	an_assert(presence);
	{
		if (presence->has_attachments())
		{
			MEASURE_PERFORMANCE(attachements);
			Concurrency::ScopedSpinLock lock(presence->attachedLock);

			scoped_call_stack_info(TXT("attachements"));
			for_every_ptr(at, presence->attached)
			{
				scoped_call_stack_info(at->ai_get_name().to_char());
				if (auto* atP = at->get_presence())
				{
					scoped_call_stack_info(TXT("attached with presence"));
					if (at->should_do_calculate_final_pose_attachment_with(_modulesOwner))
					{
						ModuleAppearance::advance_controllers_and_adjust_preliminary_pose_and_attachments(at, _deltaTime, _reason);
					}
				}
			}
		}
	}
}

void ModuleAppearance::advance_controllers_and_adjust_preliminary_pose(float _deltaTime)
{
	if (does_require_syncing_to_other_apperance())
	{
		MEASURE_PERFORMANCE(syncingToOtherAppearance);
		update_sync_to();
		// if has to be synced to other appearance, sync preliminary pose here
		if (syncToAppearance)
		{
			sync_from(preliminaryPoseLS, syncToAppearance->preliminaryPoseLS);
		}
	}

	MEASURE_PERFORMANCE(advanceAndAdjustPreliminaryPose);

	if (auto* controller = get_owner()->get_controller())
	{
		controller->reset_appearace_allowed_gait();
	}

	AppearanceControllerPoseContext poseContext;
	poseContext.with_delta_time(_deltaTime);
	poseContext.with_pose_LS(preliminaryPoseLS);
	poseContext.with_prev_pose_LS(prevFinalPoseLS);
	poseContext.reset_processing();

	an_assert(!requireAutoProcessingOrderForControllers);

#ifdef AN_ALLOW_BULLSHOTS
	if (auto* asTo = fast_cast<TemporaryObject>(get_owner()))
	{
		auto& allowFor = asTo->access_bullshot_allow_advance_for();
		if (allowFor.is_set())
		{
			allowFor = max(0.0f, allowFor.get() - _deltaTime);
			if (allowFor.get() == 0.0f)
			{
				_deltaTime = 0.0f;
			}
		}
	}
#endif

	if (!controllers.is_empty())
	{
		while (poseContext.is_more_processing_required())
		{
			poseContext.start_next_processing();
			for_every_ref(controller, controllers)
			{
				if (controller->check_processing_order(poseContext))
				{
					controller->advance_and_adjust_preliminary_pose(poseContext);
				}
			}
		}
	}
}

void ModuleAppearance::advance__calculate_final_pose_and_attachments(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(calculateFinalPoseAndAttachments);
		bool skipDueToAttachment = modulesOwner->get_presence()->is_attached();
		if (skipDueToAttachment)
		{
			auto* attachedTo = modulesOwner->get_presence()->get_attached_to();
			if (!modulesOwner->should_do_calculate_final_pose_attachment_with(attachedTo))
			{
				// we need to do it separately
				skipDueToAttachment = false;
			}
			else
			{
				if (! attachedTo->is_advanceable_continuously() ||
					! attachedTo->does_require_pose_advance() ||
					! attachedTo->does_appearance_require(ModuleAppearance::advance__calculate_final_pose_and_attachments))
				{
					// we will do it with the object we're attached to
					skipDueToAttachment = false;
				}
			}
		}
		if (!skipDueToAttachment)
		{
			float deltaTime = modulesOwner->get_pose_advance_delta_time();
			calculate_final_pose_and_attachments_0(modulesOwner, deltaTime, PoseAndAttachmentsReason::Normal);
		}
	}
}

/**
 *	calculate_final_pose_and_attachments ping pongs between appearance (0) and presence (1)
 *	although that ping pong might be broken when going from object to temporary object
 *	temporary objects are advanced on their own and should get those updates in their own part of the frame
 */
void ModuleAppearance::calculate_final_pose_and_attachments_0(IModulesOwner * _modulesOwner, float _deltaTime, PoseAndAttachmentsReason::Type _reason)
{
	MEASURE_PERFORMANCE(calculate_final_pose_and_attachments_0);
	for_every_ptr(self, _modulesOwner->get_appearances())
	{
#ifdef AN_ADVANCE_VR_IMPORTANT_ONCE
		if (_reason == PoseAndAttachmentsReason::VRImportant ||
			! self->doneAsVRImportant)
#endif
		{
			self->calculate_final_pose(_deltaTime, _reason);
		}
	}
	if (_modulesOwner->get_presence()->has_attachments() ||
		_modulesOwner->get_presence()->does_require_update_of_presence_link_setup())
	{
		_modulesOwner->get_presence()->calculate_final_pose_and_attachments_1(_deltaTime, _reason);
	}
#ifdef AN_ADVANCE_VR_IMPORTANT_ONCE
	for_every_ptr(self, _modulesOwner->get_appearances())
	{
		if (_reason == PoseAndAttachmentsReason::VRImportant)
		{
			self->doneAsVRImportant = true;
		}
		else
		{
			self->doneAsVRImportant = false;
		}
	}
#endif
}

void ModuleAppearance::calculate_final_pose(float _deltaTime, PoseAndAttachmentsReason::Type _reason)
{
	MEASURE_PERFORMANCE(calculate_final_pose);
#ifdef AN_ALLOW_BULLSHOTS
	if (auto* asTo = fast_cast<TemporaryObject>(get_owner()))
	{
		auto& allowFor = asTo->access_bullshot_allow_advance_for();
		if (allowFor.is_set())
		{
			if (allowFor.get() == 0.0f)
			{
				_deltaTime = 0.0f;
			}
		}
	}
#endif

	prevFinalPoseMS->fill_with(finalPoseMS);
	prevFinalPoseMatMS = finalPoseMatMS;
	finalPoseMSUsable = false;

#ifndef AN_ADVANCE_VR_IMPORTANT_ONCE
	if (_reason == PoseAndAttachmentsReason::VRImportant)
	{
		// do not advance controllers on vr important!
		_deltaTime = 0.0f;
	}
#endif

	// do everything using working final pose
	workingFinalPoseLS->fill_with(preliminaryPoseLS);

	if (does_require_syncing_to_other_apperance())
	{
		MEASURE_PERFORMANCE(calculateFinalPose_syncToOther);
		update_sync_to();
		// if has to be synced to other appearance, sync final pose here
		if (syncToAppearance)
		{
			sync_from(workingFinalPoseLS, syncToAppearance->finalPoseLS);
		}
	}

	// do in passes as some controllers may want to be placed on end, some may change their processing level during run-time, some may require to always process
	AppearanceControllerPoseContext poseContext;
	poseContext.with_delta_time(_deltaTime);
	poseContext.with_pose_LS(workingFinalPoseLS);
	poseContext.with_prev_pose_LS(prevFinalPoseLS);
	poseContext.reset_processing();

	if (!controllers.is_empty())
	{
		MEASURE_PERFORMANCE(calculateFinalPose_controllers);
		while (poseContext.is_more_processing_required())
		{
			poseContext.start_next_processing();
			for_every_ref(controller, controllers)
			{
				if (controller->check_processing_order(REF_ poseContext))
				{
					controller->calculate_final_pose(REF_ poseContext);
				}
			}
		}
	}

	// swap poses now, we won't be touching final pose LS
	swap(workingFinalPoseLS, finalPoseLS);

#ifdef AN_ALLOW_BULLSHOTS
	if (useBullshotFinalPose)
	{
		for_every(bullshotBone, bullshotFinalPoseLS)
		{
			int idx = for_everys_index(bullshotBone);
			if (idx >= 0 && idx < finalPoseLS->get_num_bones())
			{
				Transform boneLS = finalPoseLS->get_bone(idx);
				if (bullshotBone->translationLS.is_set())
				{
					boneLS.set_translation(bullshotBone->translationLS.get());
				}
				if (bullshotBone->rotationLS.is_set())
				{
					boneLS.set_orientation(bullshotBone->rotationLS.get().to_quat());
				}
				if (bullshotBone->scaleLS.is_set())
				{
					boneLS.set_scale(bullshotBone->scaleLS.get());
				}
				finalPoseLS->set_bone(idx, boneLS);
			}
		}
	}
#endif

	// useful both for rendering and collisions
	if (skeleton.get())
	{
		bool doCalculate = *finalPoseLS != *prevFinalPoseLS || forceCalculateFinalPose;
		if (!doCalculate)
		{
			bool hasAttachments = false;
			if (auto* imo = get_owner())
			{
				if (auto* p = imo->get_presence())
				{
					hasAttachments = p->has_attachments();
				}
			}
			doCalculate = hasAttachments;
		}
		// update only if differ
		if (doCalculate)
		{
			forceCalculateFinalPose = false;

			{
				MEASURE_PERFORMANCE(calculateFinalPose_skeletal_pose);
				finalPoseMS->ls_to_ms_from(finalPoseLS, skeleton->get_skeleton());
				finalPoseMS->to_matrix_array(finalPoseMatMS);
			}

			if (auto* collision = get_owner()->get_collision())
			{
				MEASURE_PERFORMANCE(calculateFinalPose_skeletal_collisions);
				collision->update_on_final_pose(finalPoseMS);
			}

			if (useBonesBoundingBoxForFrustumCheck || get_owner()->get_presence()->does_require_appearance_bones_bounding_box())
			{
				bonesBoundingBox = Range3::empty;
				Transform ms2os = get_ms_to_os_transform();
				for_every(bone, finalPoseMS->get_bones())
				{
					bonesBoundingBox.include(ms2os.location_to_world(bone->get_translation()));
				}
				bonesBoundingBox.expand_by(Vector3(expandBonesBoundingBox, expandBonesBoundingBox, expandBonesBoundingBox));
			}
		}
	}
	else
	{
		an_assert(Library::may_ignore_missing_references() || !useBonesBoundingBoxForFrustumCheck);
	}

	finalPoseMSUsable = true;
}

bool ModuleAppearance::does_require_ready_for_rendering() const
{
	return skeleton.is_set();
}

void ModuleAppearance::ready_for_rendering()
{
#ifdef AN_ALLOW_BULLSHOTS
	if (!bullshotMaterials.is_empty())
	{
		MaterialSetup::apply_material_setups(bullshotMaterials, meshInstance, System::MaterialSetting::Forced);
	}
#endif
#ifdef AN_DEVELOPMENT
	if (mesh.get() && fast_cast<MeshSkinned>(mesh.get()))
	{
		an_assert(skeleton.get(), TXT("using skinned mesh \"%S\" without skeleton"), mesh->get_name().to_string().to_char());
	}
#endif
	if (skeleton.get())
	{
		meshInstance.set_render_bone_matrices(finalPoseMatMS);
	}
	for_every(lmi, lodMeshInstances)
	{
		if (lmi->requested)
		{
			lmi->bonesUpdated = true;
			if (skeleton.get())
			{
				lmi->meshInstance.set_render_bone_matrices(finalPoseMatMS, &lmi->remapBones);
			}
		}
		else
		{
			lmi->bonesUpdated = false;
		}
	}
	if (moduleAppearanceData &&
		!moduleAppearanceData->copyMaterialParameters.is_empty() &&
		!moduleAppearanceData->copyMaterialParametersOnce)
	{
		copy_material_parameters();
	}
}

template <typename Class>
bool ModuleAppearance::copy_material_param(Name const& _var, TypeId _varType, void const* _value)
{
	if (_varType == type_id<Class>())
	{
		for_count(int, idx, meshInstance.get_material_instance_count())
		{
			meshInstance.access_material_instance(idx)->set_uniform(_var, *((Class*)_value), System::MaterialSetting::IndividualInstance);
		}
		for_count(int, lod, lodMeshInstances.get_size())
		{
			auto& mi = lodMeshInstances[lod].meshInstance;
			for_count(int, idx, mi.get_material_instance_count())
			{
				mi.access_material_instance(idx)->set_uniform(_var, *((Class*)_value), System::MaterialSetting::IndividualInstance);
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

void ModuleAppearance::copy_material_param(Name const & _var, TypeId _varType, void const * _value)
{
	if (copy_material_param<float>(_var, _varType, _value)) { /* ok, copied */ }
	else if (copy_material_param<Vector2>(_var, _varType, _value)) { /* ok, copied */ }
	else if (copy_material_param<Matrix44>(_var, _varType, _value)) { /* ok, copied */ }
	else if (_varType == type_id<Colour>())
	{
		for_count(int, idx, meshInstance.get_material_instance_count())
		{
			meshInstance.access_material_instance(idx)->set_uniform(_var, ((Colour*)_value)->to_vector4(), System::MaterialSetting::IndividualInstance);
		}
		for_count(int, lod, lodMeshInstances.get_size())
		{
			auto& mi = lodMeshInstances[lod].meshInstance;
			for_count(int, idx, mi.get_material_instance_count())
			{
				mi.access_material_instance(idx)->set_uniform(_var, ((Colour*)_value)->to_vector4(), System::MaterialSetting::IndividualInstance);
			}
		}
	}
	else
	{
		todo_important(TXT("implement \"%S\""), RegisteredType::get_name_of(_varType));
	}
}

Meshes::Mesh3DInstance const& ModuleAppearance::get_mesh_instance_for_lod(int _lod) const
{
	_lod = clamp(_lod, 0, lodMeshInstances.get_size());
	while (_lod > 0)
	{
		auto& lmi = lodMeshInstances[_lod - 1];
		if (lmi.bonesUpdated || ! skeleton.get())
		{
			return lmi.meshInstance;
		}
		--_lod;
	}
	return meshInstance;
}

void ModuleAppearance::copy_material_parameters()
{
	for_every(var, moduleAppearanceData->copyMaterialParameters)
	{
		TypeId varType;
		void const * value;
		if (get_owner()->get_variables().get_existing_of_any_type_id(*var, varType, value))
		{
			copy_material_param(*var, varType, value);
		}
	}
}

bool ModuleAppearance::does_require_copying_material_parameters() const
{
	if (moduleAppearanceData &&
		!moduleAppearanceData->copyMaterialParameters.is_empty() &&
		!moduleAppearanceData->copyMaterialParametersOnce)
	{
		return true;
	}
	return false;
}

void ModuleAppearance::update_sync_to()
{
	if (! syncTo.is_valid())
	{
		syncToAppearance = nullptr;
		syncToSkeleton.clear();
		return;
	}

	ModuleAppearance* shouldSyncToAppearance = get_owner()->get_appearance_named(syncTo);
	if (syncToAppearance != shouldSyncToAppearance ||
		(shouldSyncToAppearance && syncToSkeleton.get() != shouldSyncToAppearance->get_skeleton()))
	{
		syncToBoneMap.clear();
		Skeleton const * shouldSyncToSkeleton = shouldSyncToAppearance->get_skeleton();
		if (skeleton.get() && shouldSyncToSkeleton)
		{
			syncToAppearance = shouldSyncToAppearance;
			syncToSkeleton.clear();
			syncToBoneMap.set_size(skeleton->get_skeleton()->get_num_bones());

			auto iBone = skeleton->get_skeleton()->get_bones().begin_const();
			for_every(syncToBoneIdx, syncToBoneMap)
			{
				*syncToBoneIdx = shouldSyncToSkeleton->get_skeleton()->find_bone_index(iBone->get_name());
				++iBone;
			}
		}
		else
		{
			syncToAppearance = nullptr;
			syncToSkeleton.clear();
		}
	}
}

void ModuleAppearance::sync_from(Meshes::Pose * _dest, Meshes::Pose const * _source)
{
	auto iDest = _dest->access_bones().begin();
	for_every(syncToBoneIdx, syncToBoneMap)
	{
		if (*syncToBoneIdx != NONE)
		{
			*iDest = _source->get_bone(*syncToBoneIdx);
		}
		++iDest;
	}
}

int ModuleAppearance::find_socket_index(Name const & _socketName) const
{
	return mesh.is_set() ? mesh->find_socket_index(_socketName) : NONE;
}

bool ModuleAppearance::has_socket(Name const & _socketName) const
{
	return mesh.is_set() ? mesh->has_socket(_socketName) : false;
}

Transform ModuleAppearance::calculate_socket_os(Name const & _socketName, Meshes::Pose const *_usingPoseLS) const
{
	return get_ms_to_os_transform().to_world(calculate_socket_ms(_socketName, _usingPoseLS));
}

Transform ModuleAppearance::calculate_socket_ms(Name const & _socketName, Meshes::Pose const *_usingPoseLS) const
{
	return mesh.is_set() ? mesh->calculate_socket_using_ls(_socketName, _usingPoseLS ? _usingPoseLS : finalPoseLS) : Transform::identity;
}

Transform ModuleAppearance::calculate_socket_os(int _socketIdx, Meshes::Pose const *_usingPoseLS) const
{
	return get_ms_to_os_transform().to_world(calculate_socket_ms(_socketIdx, _usingPoseLS));
}

Transform ModuleAppearance::calculate_socket_ms(int _socketIdx, Meshes::Pose const *_usingPoseLS) const
{
	return mesh.is_set() ? mesh->calculate_socket_using_ls(_socketIdx, _usingPoseLS ? _usingPoseLS : finalPoseLS) : Transform::identity;
}

bool ModuleAppearance::does_require(Concurrency::ImmediateJobFunc _jobFunc) const
{
	if (_jobFunc == advance__calculate_preliminary_pose ||
		_jobFunc == advance__advance_controllers_and_adjust_preliminary_pose ||
		_jobFunc == advance__calculate_final_pose_and_attachments ||
		_jobFunc == advance__pois)
	{
		return get_owner()->does_appearance_require(_jobFunc);
		an_assert_immediate(false, TXT("do not use it"));
		return (skeleton.is_set() || does_require_syncing_to_other_apperance() || !controllers.is_empty() || get_owner()->get_presence()->has_attachments());
	}
	if (_jobFunc == advance__pois)
	{
		an_assert_immediate(false, TXT("do not use it"));
		return false;
	}
	return true;
}

Meshes::PoseMatBonesRef ModuleAppearance::get_final_pose_mat_MS_ref(bool _allowPrev) const
{
	an_assert(finalPoseMSUsable || _allowPrev);
	return Meshes::PoseMatBonesRef(skeleton.get() ? skeleton.get()->get_skeleton() : nullptr, finalPoseMSUsable? finalPoseMatMS : prevFinalPoseMatMS);
}

void ModuleAppearance::debug_draw_skeleton() const
{
	debug_context(get_owner()->get_presence()->get_in_room());
	debug_subject(get_owner());
	debug_push_transform(get_owner()->get_presence()->get_placement());
	if (skeleton.get() && skeleton->get_skeleton())
	{
		skeleton->debug_draw_pose_MS(finalPoseMatMS);
	}
	debug_pop_transform();
	debug_no_subject();
	debug_no_context();
}

void ModuleAppearance::debug_draw_sockets() const
{
	debug_context(get_owner()->get_presence()->get_in_room());
	debug_subject(get_owner());
	debug_push_transform(get_owner()->get_presence()->get_placement());
	if (skeleton.get() && skeleton->get_skeleton())
	{
		skeleton->get_sockets().debug_draw_pose_MS(finalPoseMatMS, skeleton->get_skeleton());
	}
	mesh->get_sockets().debug_draw_pose_MS(finalPoseMatMS, skeleton.get() ? skeleton->get_skeleton() : nullptr);
	debug_pop_transform();
	debug_no_subject();
	debug_no_context();
}

void ModuleAppearance::debug_draw_pois(bool _setDebugContext)
{
	debug_subject(get_owner());
	for_every_ref(poi, pois)
	{
		if (!poi->is_cancelled())
		{
			poi->debug_draw(_setDebugContext);
		}
	}
	debug_no_subject();
}

void ModuleAppearance::process_mesh_pois(bool _force)
{
	if (meshPOIsProcessed && !_force)
	{
		return;
	}
	if (auto* presence = get_owner()->get_presence())
	{
		if (mesh.is_set())
		{
			Random::Generator rg = get_owner()->get_individual_random_generator();
			prepare_random_generator_for_pois(rg);
			// during POIs processing we may add more POIs
			for (int idx = 0; idx < mesh->get_pois().get_size(); ++idx)
			{
				auto* poi = mesh->get_pois()[idx].get();
				pois.push_back(PointOfInterestInstancePtr(Game::get()->get_customisation().create_point_of_interest_instance()));
				pois.get_last()->access_parameters().set_from(get_owner()->get_variables()); // this will take owner's parameters and also room's, region's etc in case they are not available (which might be when we are creating POI on an object that was not yet placed in a room)
				pois.get_last()->create_instance(rg.spawn().temp_ref(), poi, presence->get_in_room(), get_owner(), NONE, presence->get_placement(), mesh.get(), &meshInstance);
				if (pois.get_last()->does_require_advancement())
				{
					get_owner()->mark_pois_require_advancement();
				}
			}
		}
	}
	meshPOIsProcessed = true;
}

void ModuleAppearance::for_every_point_of_interest(Optional<Name> const& _poiName, OnFoundPointOfInterest _fpoi, IsPointOfInterestValid _is_valid) const
{
	ASSERT_NOT_ASYNC_OR(get_owner()->get_presence()->get_in_room()->get_in_world()->multithread_check__reading_world_is_safe());
	if (get_owner_as_object() && !get_owner_as_object()->is_world_active() && pois.is_empty() && mesh.is_set() && !mesh->get_pois().is_empty())
	{
		// sometimes during the creation we want to already have access to object's pois (to place stuff, etc). this is 
		if (auto* presence = get_owner()->get_presence())
		{
			Random::Generator rg = get_owner()->get_individual_random_generator();
			prepare_random_generator_for_pois(rg);
			// during POIs processing we may add more POIs
			for (int idx = 0; idx < mesh->get_pois().get_size(); ++idx)
			{
				auto* poi = mesh->get_pois()[idx].get();
				if ((!_poiName.is_set() || poi->name == _poiName.get()))
				{
					PointOfInterestInstancePtr poii = PointOfInterestInstancePtr(Game::get()->get_customisation().create_point_of_interest_instance());
					poii->access_parameters().set_from(get_owner()->get_variables()); // this will take owner's parameters and also room's, region's etc in case they are not available (which might be when we are creating POI on an object that was not yet placed in a room)
					poii->create_instance(rg.spawn().temp_ref(), poi, presence->get_in_room(), get_owner(), NONE, presence->get_placement(), mesh.get(), cast_to_nonconst(&meshInstance), false /* do not process! */);
					if (!_is_valid || _is_valid(poii.get())) // this rarely should be used, but just to make it consistent
					{
						_fpoi(poii.get());
					}
				}
			}
		}
	}
	else
	{
		for_every_ref(poi, pois)
		{
			if (!poi->is_cancelled() &&
				(!_poiName.is_set() || poi->get_name() == _poiName.get()) &&
				(!_is_valid || _is_valid(poi)))
			{
				_fpoi(poi);
			}
		}
	}
}


void ModuleAppearance::do_for_all_appearances(std::function<void(ModuleAppearance * _appearance)> _do)
{
	for_every_ptr(appearance, get_owner()->get_appearances())
	{
		_do(appearance);
	}
}

struct RearrangeAppearanceController
{
	::Meshes::Skeleton const * skeleton = nullptr;
	AppearanceController* controller = nullptr;

	RearrangeAppearanceController(AppearanceController* _controller = nullptr, ::Meshes::Skeleton const * _skeleton = nullptr) : skeleton(_skeleton), controller(_controller) {}

	static bool check_depends_on_bones(::Meshes::Skeleton const * skeleton, ArrayStack<int> const & dependsOnBones, ArrayStack<int> const & providesBones, bool _dependsOnParents = false)
	{
		if (dependsOnBones.is_empty() || ! skeleton)
		{
			return false;
		}
		if (_dependsOnParents)
		{
			for_every(dependsOnBone, dependsOnBones)
			{
				for_every(providesBone, providesBones)
				{
					int parent = skeleton->get_parent_of(*dependsOnBone);
					if (parent != NONE && skeleton->is_descendant(*providesBone, parent, true))
					{
						return true;
					}
				}
			}
		}
		else
		{
			for_every(dependsOnBone, dependsOnBones)
			{
				for_every(providesBone, providesBones)
				{
					if (*dependsOnBone != NONE && skeleton->is_descendant(*providesBone, *dependsOnBone, true))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	static bool check_names(ArrayStack<Name> const & dependsOn, ArrayStack<Name> const & provides)
	{
		if (dependsOn.is_empty())
		{
			return false;
		}
		for_every(d, dependsOn)
		{
			for_every(p, provides)
			{
				if (*d == *p)
				{
					return true;
				}
			}
		}
		return false;
	}

	static int sort_out_dependence(bool BbeforeA, bool AbeforeB)
	{
		if (BbeforeA && !AbeforeB)
		{
			return B_BEFORE_A;
		}
		if (AbeforeB && !BbeforeA)
		{
			return A_BEFORE_B;
		}
		return 0; // not determined
	}

	#define SORT_OUT_DEPENDENCE(BbeforeA, AbeforeB) \
	{ \
		if (int result = sort_out_dependence(BbeforeA, AbeforeB)) \
		{ \
			return result; \
		} \
	}

	static int sort_for_processing_order(void const * _a, void const * _b) // sort_for_processing_order
	{
		RearrangeAppearanceController const * a = (RearrangeAppearanceController const *)(_a);
		RearrangeAppearanceController const * b = (RearrangeAppearanceController const *)(_b);

		int aPAL = a->controller->get_process_at_level();
		int bPAL = b->controller->get_process_at_level();
		if (aPAL < bPAL) { return A_BEFORE_B; } else
		if (aPAL > bPAL) { return B_BEFORE_A; }

		an_assert(a->skeleton == b->skeleton);

		ARRAY_STACK(int, a_dependsOnBones, a->skeleton ? a->skeleton->get_num_bones() : 1);
		ARRAY_STACK(int, a_dependsOnParentBones, a->skeleton ? a->skeleton->get_num_bones() : 1);
		ARRAY_STACK(int, a_usesBones, a->skeleton ? a->skeleton->get_num_bones() : 1);
		ARRAY_STACK(int, a_providesBones, a->skeleton ? a->skeleton->get_num_bones() : 1);
		ARRAY_STACK(Name, a_dependsOnVariables, 64);
		ARRAY_STACK(Name, a_providesVariables, 64);
		ARRAY_STACK(Name, a_dependsOnControllers, 64);
		ARRAY_STACK(Name, a_providesControllers, 64);
		a->controller->get_info_for_auto_processing_order(a_dependsOnBones, a_dependsOnParentBones, a_usesBones, a_providesBones,
			a_dependsOnVariables, a_providesVariables, a_dependsOnControllers, a_providesControllers);

		ARRAY_STACK(int, b_dependsOnBones, b->skeleton ? b->skeleton->get_num_bones() : 1);
		ARRAY_STACK(int, b_dependsOnParentBones, b->skeleton ? b->skeleton->get_num_bones() : 1);
		ARRAY_STACK(int, b_usesBones, b->skeleton ? b->skeleton->get_num_bones() : 1);
		ARRAY_STACK(int, b_providesBones, b->skeleton ? b->skeleton->get_num_bones() : 1);
		ARRAY_STACK(Name, b_dependsOnVariables, 64);
		ARRAY_STACK(Name, b_providesVariables, 64);
		ARRAY_STACK(Name, b_dependsOnControllers, 64);
		ARRAY_STACK(Name, b_providesControllers, 64);
		b->controller->get_info_for_auto_processing_order(b_dependsOnBones, b_dependsOnParentBones, b_usesBones, b_providesBones,
			b_dependsOnVariables, b_providesVariables, b_dependsOnControllers, b_providesControllers);

		SORT_OUT_DEPENDENCE(check_names(a_dependsOnControllers, a_providesControllers),
							check_names(b_dependsOnControllers, b_providesControllers));

		SORT_OUT_DEPENDENCE(check_depends_on_bones(a->skeleton, a_dependsOnBones, b_providesBones),
							check_depends_on_bones(a->skeleton, b_dependsOnBones, a_providesBones));

		SORT_OUT_DEPENDENCE(check_depends_on_bones(a->skeleton, a_dependsOnParentBones, b_providesBones, true),
							check_depends_on_bones(a->skeleton, b_dependsOnParentBones, a_providesBones, true));

		SORT_OUT_DEPENDENCE(check_depends_on_bones(a->skeleton, a_usesBones, b_providesBones),
							check_depends_on_bones(a->skeleton, b_usesBones, a_providesBones));

		SORT_OUT_DEPENDENCE(check_names(a_dependsOnVariables, a_providesVariables),
							check_names(b_dependsOnVariables, b_providesVariables));

		return 0;
	}
};


void ModuleAppearance::auto_processing_order_for_controllers()
{
	requireAutoProcessingOrderForControllers = false;
	if (controllers.is_empty())
	{
		return;
	}

	// rearrange controllers
	// we will modify their processing orders using index in this array (after they are rearranged)
	ARRAY_STACK(RearrangeAppearanceController, rearrange, controllers.get_size());
	for_every_ref(controller, controllers)
	{
		rearrange.push_back(RearrangeAppearanceController(controller, skeleton.is_set()? skeleton->get_skeleton() : nullptr));
	}

	sort(rearrange, RearrangeAppearanceController::sort_for_processing_order);

	// update processing order
	for_every(re, rearrange)
	{
		re->controller->set_processing_order(for_everys_index(re));
	}
}

void ModuleAppearance::advance__pois(IMMEDIATE_JOB_PARAMS)
{
	Framework::AdvanceContext const* ac = plain_cast<Framework::AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(advancePOIs_object);

		modulesOwner->clear_pois_require_advancement();
		for_every_ptr(self, modulesOwner->get_appearances())
		{
			for_every_ref(poi, self->pois)
			{
				if (!poi->is_cancelled() &&
					poi->does_require_advancement())
				{
					poi->advance(*ac);
					if (poi->does_require_advancement())
					{
						modulesOwner->mark_pois_require_advancement();
					}
				}
			}
		}
	}
}

void ModuleAppearance::generate_and_fuse(MeshGenerator * _meshGenerator, SimpleVariableStorage const * _usingVariables)
{
	an_assert(mesh.is_set() && mesh->is_temporary());
	an_assert(moduleAppearanceData, TXT("can only be performed when module has been already initialised"));
	an_assert(is_initialised(), TXT("atm has to be initialised"));
	an_assert(!is_ready_to_activate(), TXT("atm cannot be readied to activate, there is no POI processing, think about fusing meshes on the fly (if that's really required)"));

	SimpleVariableStorage variables = moduleAppearanceData->generatorVariables;
	variables.set_from(get_owner()->get_variables());
	if (_usingVariables)
	{
		variables.set_from(*_usingVariables);
	}
	// do not use skeleton, assume it may create its new bones
	MeshGeneratorRequest request;
	request.for_wmp_owner(this).using_parameters(variables)
		   .using_random_regenerator(get_owner()->get_individual_random_generator())
		   .generate_lods(mesh->get_lod_count());
	if (!useLODs)
	{
		request.no_lods();
	}
	if (setup_mesh_generator_request)
	{
		setup_mesh_generator_request(request);
	}
	UsedLibraryStored<Mesh> newMesh = _meshGenerator->generate_temporary_library_mesh(Library::get_current(), request);
	if (newMesh.is_set())
	{
		Meshes::RemapBoneArray remapBones;
		if (newMesh->get_skeleton())
		{
			if (get_skeleton())
			{
				skeleton->fuse(newMesh->get_skeleton(), REF_ remapBones);
			}
		}
		newMesh->be_unique_instance();
		Array<Mesh::FuseMesh> meshesToFuse;
		meshesToFuse.push_back(Mesh::FuseMesh(newMesh.get()));
		Array<Meshes::RemapBoneArray const *> remapBonesArrays;
		remapBonesArrays.push_back(&remapBones);
		mesh->fuse(meshesToFuse, &remapBonesArrays);

		// reinitialise
		use_mesh(mesh.get());
	}
}

void ModuleAppearance::for_every_socket_starting_with(Name const& _socketPrefix, std::function<void(Name const& _socket)> _do) const
{
	if (auto* mesh = get_mesh())
	{
		for_every(socket, mesh->get_sockets().get_sockets())
		{
			if (socket->get_name().to_string().has_prefix(_socketPrefix.to_string()))
			{
				_do(socket->get_name());
			}
		}
	}
	if (auto* skeleton = get_skeleton())
	{
		for_every(socket, skeleton->get_sockets().get_sockets())
		{
			if (socket->get_name().to_string().has_prefix(_socketPrefix.to_string()))
			{
				_do(socket->get_name());
			}
		}
	}
}
