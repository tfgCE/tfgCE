#include "mesh.h"

#include "..\..\core\debugConfig.h"
#include "..\..\core\collision\model.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\io\xml.h"
#include "..\..\core\memory\memory.h"
#include "..\..\core\mesh\pose.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\serialisation\importerHelper.h"
#include "..\..\core\types\names.h"
#include "..\..\core\utils\utils_loadingForSystemShader.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\debug\previewGame.h"
#include "..\game\game.h"
#include "..\game\gameConfig.h"
#include "..\jobSystem\jobSystem.h"
#include "..\meshGen\meshGenerator.h"
#include "..\world\pointOfInterest.h"

#include "appearanceControllerData.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

REGISTER_FOR_FAST_CAST(Framework::Mesh);
LIBRARY_STORED_DEFINE_TYPE(Mesh, mesh);

#ifdef AN_DEVELOPMENT
bool Mesh::debugDrawMesh = true;
bool Mesh::debugDrawSkeleton = true;
bool Mesh::debugDrawSockets = true;
bool Mesh::debugDrawMeshNodes = true;
bool Mesh::debugDrawPOIs = true;
bool Mesh::debugDrawPreciseCollision = true;
bool Mesh::debugDrawMovementCollision = true;
bool Mesh::debugDrawSpaceBlockers = true;
#endif

Mesh::Mesh(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, mesh(nullptr)
, meshUsage(Meshes::Usage::Static)
, sockets(new Meshes::SocketDefinitionSet())
{
}

struct Mesh_DeleteMesh
: public Concurrency::AsynchronousJobData
{
	Meshes::Mesh3D* mesh;
	Mesh_DeleteMesh(Meshes::Mesh3D* _mesh)
	: mesh(_mesh)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		Mesh_DeleteMesh* data = (Mesh_DeleteMesh*)_data;
		delete data->mesh;
	}
};

void Mesh::defer_mesh_delete()
{
	if (mesh)
	{
		Game::async_system_job(get_library()->get_game(), Mesh_DeleteMesh::perform, new Mesh_DeleteMesh(mesh));
		mesh = nullptr;
	}
}

Mesh::~Mesh()
{
	defer_mesh_delete();
	an_assert(sockets);
	delete_and_clear(sockets);
}

void Mesh::prepare_to_unload()
{
	base::prepare_to_unload();
	movementCollisionModel.clear();
	preciseCollisionModel.clear();
	spaceBlockers.clear();
	useMeshGenerator.clear();
}

#ifdef AN_DEVELOPMENT
void Mesh::ready_for_reload()
{
	base::ready_for_reload();

	useMeshGenerator.clear();

	defer_mesh_delete();
	an_assert(sockets);
	delete sockets;

	sockets = new Meshes::SocketDefinitionSet();
	movementCollisionModel.clear();
	preciseCollisionModel.clear();
	spaceBlockers.clear();
	materialSetups.clear();
	meshNodes.clear();
	pois.clear();
	controllerDatas.clear();
}
#endif

struct ::Framework::MeshMeshImporter
: public ImporterHelper<Meshes::Mesh3D, Meshes::Mesh3DImportOptions>
{
	typedef ImporterHelper<Meshes::Mesh3D, Meshes::Mesh3DImportOptions> base;

	MeshMeshImporter(Framework::Mesh* _mesh)
	: base(_mesh->mesh)
	, mesh(_mesh)
	{}

protected:
	override_ void create_object()
	{
		object = new Meshes::Mesh3D();
		object->set_usage(mesh->meshUsage);
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		object->set_debug_mesh_name(String(TXT("Framework::Mesh (imported) ")) + mesh->get_name().to_string());
#endif
	}

	override_ void delete_object()
	{
		mesh->defer_mesh_delete();
	}

	override_ void import_object()
	{
		object = Meshes::Mesh3D::importer().import(importFromFileName, importOptions);
		if (object)
		{
			object->set_usage(mesh->meshUsage);
		}
	}

private:
	Framework::Mesh* mesh;
};

struct ::Framework::MeshSocketsImporter
: public ImporterHelper<Meshes::SocketDefinitionSet, Meshes::SocketDefinitionSetImportOptions>
{
	typedef ImporterHelper<Meshes::SocketDefinitionSet, Meshes::SocketDefinitionSetImportOptions> base;

	MeshSocketsImporter(Framework::Mesh* _mesh)
	: base(_mesh->sockets)
	{}

protected:
	override_ void import_object()
	{
		object = Meshes::SocketDefinitionSet::importer().import(importFromFileName, importOptions);
	}

private:
};

bool Mesh::sub_load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	generateLODs.load_from_xml(_node, TXT("generateLODs"));
	useLODFully.load_from_xml(_node, TXT("useLODFully"));
	use1AsSubStepBase.load_from_xml(_node, TXT("use1AsSubStepBase"));

	return result;
}

bool Mesh::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	make_sure_mesh_exists();

	bool result = LibraryStored::load_from_xml(_node, _lc);

	if (_node->get_bool_attribute_or_from_child_presence(TXT("generateOnDemand")))
	{
		generateOnDemandForLibrary = _lc.get_library();
	}

	result &= movementCollisionModel.load_from_xml_child_node(_node, TXT("movementCollisionModel"), TXT("name"), _lc);
	result &= preciseCollisionModel.load_from_xml_child_node(_node, TXT("preciseCollisionModel"), TXT("name"), _lc);

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
	mesh->set_debug_mesh_name(String(TXT("Framework::Mesh (loaded from file) ")) + get_name().to_string());
#endif

	result &= sub_load_from_xml(_node, _lc);
	for_every(node, _node->children_named(TXT("for")))
	{
		if (CoreUtils::Loading::should_load_for_system_or_shader_option(node))
		{
			result &= sub_load_from_xml(node, _lc);
		}
	}

	IO::XML::Attribute const * attr = _node->get_attribute(TXT("useMeshGenerator"));
	if (attr)
	{
		warn_loading_xml(_node, TXT("replace useMeshGenerator with meshGenerator"));
	}
	else
	{
		attr = _node->get_attribute(TXT("meshGenerator"));
	}
	if (attr)
	{
		result &= useMeshGenerator.load_from_xml(attr, _lc);
		result &= meshGeneratorParameters.load_from_xml_child_node(_node, TXT("meshGeneratorParameters"));
		useCustomParametersForMeshGeneration = _node->get_bool_attribute_or_from_child_presence(TXT("useCustomParametersForMeshGeneration"), useCustomParametersForMeshGeneration);
		_lc.load_group_into(meshGeneratorParameters);
		for_every(meshNodesNode, _node->children_named(TXT("meshNodes")))
		{
			for_every(node, meshNodesNode->children_named(TXT("meshNode")))
			{
				MeshNodePtr meshNode(new MeshNode());
				if (meshNode->load_from_xml(node, _lc))
				{
					loadedMeshNodes.push_back(meshNode);
				}
				else
				{
					result = false;
					error_loading_xml(node, TXT("could not load mesh node"));
				}
			}
		}
	}
	else if (_node->first_child_named(TXT("noMesh")) == nullptr)
	{
		result &= load_mesh_from_xml(_node, _lc);
	}

	result &= autoCreateMovementCollisionMeshPhysicalMaterial.load_from_xml(_node, TXT("autoCreateMovementCollisionMeshPhysicalMaterial"), _lc);
	result &= autoCreatePreciseCollisionMeshPhysicalMaterial.load_from_xml(_node, TXT("autoCreatePreciseCollisionMeshPhysicalMaterial"), _lc);
	for_every(node, _node->children_named(TXT("autoCreateCollisionMeshPhysicalMaterials")))
	{
		result &= autoCreateMovementCollisionMeshPhysicalMaterial.load_from_xml(node, TXT("forMovement"), _lc);
		result &= autoCreatePreciseCollisionMeshPhysicalMaterial.load_from_xml(node, TXT("forPrecise"), _lc);
	}
	result &= autoCreateMovementCollisionBoxPhysicalMaterial.load_from_xml(_node, TXT("autoCreateMovementCollisionBoxPhysicalMaterial"), _lc);
	result &= autoCreatePreciseCollisionBoxPhysicalMaterial.load_from_xml(_node, TXT("autoCreatePreciseCollisionBoxPhysicalMaterial"), _lc);
	for_every(node, _node->children_named(TXT("autoCreateCollisionBoxPhysicalMaterials")))
	{
		result &= autoCreateMovementCollisionBoxPhysicalMaterial.load_from_xml(node, TXT("forMovement"), _lc);
		result &= autoCreatePreciseCollisionBoxPhysicalMaterial.load_from_xml(node, TXT("forPrecise"), _lc);
	}

	autoCreateSpaceBlocker = _node->get_bool_attribute_or_from_child_presence(TXT("autoCreateSpaceBlocker"), autoCreateSpaceBlocker);

	for_every(poisNode, _node->children_named(TXT("pois")))
	{
		for_every(node, poisNode->children_named(TXT("poi")))
		{
			PointOfInterestPtr poi(new PointOfInterest());
			if (poi->load_from_xml(node, _lc))
			{
				pois.push_back(poi);
			}
			else
			{
				result = false;
				error_loading_xml(node, TXT("could not load poi"));
			}
		}
	}

	for_every(socketsNode, _node->children_named(TXT("sockets")))
	{
		if (socketsNode->has_attribute(TXT("file")))
		{
			MeshSocketsImporter socketsImporter(this);
			result &= socketsImporter.setup_from_xml(socketsNode, _lc.get_library_source() == LibrarySource::Assets, String(TXT("file")));
			if (!socketsImporter.import())
			{
#ifdef AN_OUTPUT_IMPORT_ERRORS
				error_loading_xml(_node, TXT("can't import"));
#endif
				result = false;
			}
		}

		// just add to existing (or even modify already imported ones!)
		result &= sockets->load_from_xml(socketsNode);
	}

	for_every(materialsNode, _node->children_named(TXT("materials")))
	{
		for_every(materialSetupNode, materialsNode->children_named(TXT("material")))
		{
			int idx = materialSetupNode->get_int_attribute(TXT("index"), -1);
			if (idx >= 0)
			{
				memory_leak_suspect;
				materialSetups.set_size(max(materialSetups.get_size(), idx + 1));
				forget_memory_leak_suspect;
				materialSetups[idx].load_from_xml(materialSetupNode, _lc);
			}
		}
	}

	for_every(materialSetupNode, _node->children_named(TXT("material")))
	{
		error_loading_xml(materialSetupNode, TXT("put this node into materials node"));
		result = false;
	}

	return result;
}

bool Mesh::load_mesh_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;
	MeshMeshImporter meshImporter(this);
	result &= meshImporter.setup_from_xml(_node, _lc.get_library_source() == LibrarySource::Assets);
	if (!meshImporter.import())
	{
#ifdef AN_OUTPUT_IMPORT_ERRORS
		error_loading_xml(_node, TXT("can't import"));
#endif
		result = false;
	}
	return result;
}

Mesh const * Mesh::get_lod(int _lod) const
{
	if (_lod > 0)
	{
		--_lod;
		_lod = min(_lod, lods.get_size() - 1);
		if (_lod >= 0)
		{
			return lods[_lod].mesh.get();
		}
	}

	return this;
}

bool Mesh::generate_mesh(Library* _library)
{
	bool result = true;
	an_assert(useMeshGenerator.get() != nullptr);
	useMeshGenerator->load_on_demand_if_required();

#ifdef AN_DEVELOPMENT
	::System::TimeStamp loadingTime;
#endif

	Collision::Model* generatedMovementCollisionModel = nullptr;
	Collision::Model* generatedPreciseCollisionModel = nullptr;
	UsedLibraryStored<Skeleton> generatedSkeleton;
	Array<MeshNodePtr> loadedMeshNodesCopy;
	MeshNode::copy_not_dropped_to(loadedMeshNodes, loadedMeshNodesCopy);
	Random::Generator rg;
	MeshGeneratorRequest request;
	request.for_library_mesh(this);
#ifdef AN_DEVELOPMENT
	request.using_random_regenerator_if_not_set(rg);
#else
	request.using_random_regenerator(rg);
#endif
	if (useCustomParametersForMeshGeneration)
	{
		useMeshGeneratorParameters.clear();
		useMeshGeneratorParameters.set_from(get_custom_parameters());
		useMeshGeneratorParameters.set_from(meshGeneratorParameters);
		request.using_parameters(useMeshGeneratorParameters);
	}
	else
	{
		request.using_parameters(meshGeneratorParameters);
	}
	request.using_mesh_nodes(loadedMeshNodesCopy);
	request.store_movement_collision(generatedMovementCollisionModel);
	request.store_precise_collision(generatedPreciseCollisionModel);
	request.store_generated_skeleton(generatedSkeleton);
	request.store_sockets(*sockets);
	request.store_mesh_nodes(meshNodes);
	request.store_pois(pois);
	request.store_space_blockers(spaceBlockers);
	request.store_appearance_controllers(controllerDatas);
	request.no_lods();
	MeshGeneratorRequest requestLOD;
	requestLOD.using_random_regenerator(rg);
	if (useCustomParametersForMeshGeneration)
	{
		requestLOD.using_parameters(useMeshGeneratorParameters);
	}
	else
	{
		requestLOD.using_parameters(meshGeneratorParameters);
	}
	requestLOD.using_mesh_nodes(loadedMeshNodesCopy);
	//
	if (useLODFully.is_set())
	{
		request.use_lod_fully(useLODFully.get());
		requestLOD.use_lod_fully(useLODFully.get());
	}
	if (use1AsSubStepBase.is_set())
	{
		request.use_1_as_sub_step_base(use1AsSubStepBase.get());
		requestLOD.use_1_as_sub_step_base(use1AsSubStepBase.get());
	}
	if (::Meshes::Mesh3D* mesh = useMeshGenerator->generate_mesh(request))
	{
#ifdef AN_DEVELOPMENT
#ifdef AN_LOG_LOADING_AND_PREPARING
		output(TXT("generated mesh for \"%S\" in %.3fs"), get_name().to_string().to_char(), loadingTime.get_time_since());
#endif
#endif
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		mesh->set_debug_mesh_name(String(TXT("Framework::Mesh (generated) ")) + get_name().to_string());
#endif
		mesh->update_bounding_box(); // to calculate size for LOD

		lods.clear();
		for_count(int, i, max(0, generateLODs.get(useMeshGenerator->get_generate_lods().get(GameConfig::get()? GameConfig::get()->get_auto_generate_lods() : 0)) - 1))
		{
			requestLOD.for_lod(i + 1);
			UsedLibraryStored<Mesh> lodMesh = useMeshGenerator->generate_temporary_library_mesh(_library, requestLOD);
			if (lodMesh.is_set())
			{
				if (lodMesh->materialSetups.is_empty() &&
					! materialSetups.is_empty())
				{
					// copy material setup in case mesh generator didn't provide anything
					// this means that we should have same material setup usage/layout for all mesh parts!
					lodMesh->materialSetups = materialSetups;
				}
				MeshLOD ml;
				ml.mesh = lodMesh;
				lods.push_back(ml);
			}
		}

		update_lods_remapping();
	}
	else
	{
		error(TXT("could not generate mesh"));
		result = false;
	}
		
	return result;
}

void Mesh::generated_on_demand_if_required()
{
	if (generateOnDemandForLibrary)
	{
		auto* library = generateOnDemandForLibrary;
		ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;
		generateOnDemandForLibrary = nullptr;

		if (useMeshGenerator.get())
		{
			bool result = generate_mesh(library);
			if (!result)
			{
				error(TXT("could not generate mesh for \"%S\" on demand"), get_name().to_string().to_char());
			}
		}

		set_missing_materials(library);
	}
}

bool Mesh::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = LibraryStored::prepare_for_game(_library, _pfgContext);
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		result &= autoCreateMovementCollisionMeshPhysicalMaterial.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		result &= autoCreatePreciseCollisionMeshPhysicalMaterial.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		result &= autoCreateMovementCollisionBoxPhysicalMaterial.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		result &= autoCreatePreciseCollisionBoxPhysicalMaterial.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		if (movementCollisionModel.is_name_valid())
		{
			result &= movementCollisionModel.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
		if (preciseCollisionModel.is_name_valid())
		{
			result &= preciseCollisionModel.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
		if (useMeshGenerator.is_name_valid())
		{
			result &= useMeshGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
	}
	if (!generateOnDemandForLibrary)
	{
		IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::GenerateMeshes)
		{
			if (useMeshGenerator.get())
			{
				result &= generate_mesh(_library);
			}
		}
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::GenerateAutoCollisionMeshes)
	{
		// meshes
		{
			auto* movepm = autoCreateMovementCollisionMeshPhysicalMaterial.get();
			auto* precpm = autoCreatePreciseCollisionMeshPhysicalMaterial.get();
			if (movepm || precpm)
			{
				if (mesh)
				{
					Collision::Mesh collisionMesh;

					for_count(int, partIdx, mesh->get_parts_num())
					{
						auto* part = mesh->get_part(partIdx);

						part->for_every_triangle([&collisionMesh](Vector3 const& _a, Vector3 const& _b, Vector3 const& _c)
							{
								if (Vector3::cross(_b - _a, _c - _a).length() >= 0.0001f)
								{
									collisionMesh.add(_a, _b, _c);
								}
							});
					}

					for_count(int, iPM, 2)
					{
						auto* pm = iPM == 0 ? movepm : precpm;
						if (!pm)
						{
							continue;
						}

						collisionMesh.use_material(movepm);

						Collision::Model* model = new Collision::Model();
						model->add(collisionMesh); // <- creates a copy
						model->break_meshes_into_convex_meshes();

						CollisionModel* libraryCollisionMesh = Library::get_current()->get_collision_models().create_temporary(LibraryName::invalid());
						libraryCollisionMesh->replace_model(model);
						if (iPM == 0)
						{
							set_movement_collision_model(libraryCollisionMesh);
						}
						else
						{
							an_assert(iPM == 1)
								set_precise_collision_model(libraryCollisionMesh);
						}

						// check if we should use the same for precise collision model
						if (iPM == 0 &&
							precpm == movepm)
						{
							set_precise_collision_model(libraryCollisionMesh);
							break;
						}
					}
				}
				else
				{
					error(TXT("error preparing auto collisions, no mesh"));
					result = false;
				}
			}
		}
		// boxes
		{
			auto* movepm = autoCreateMovementCollisionBoxPhysicalMaterial.get();
			auto* precpm = autoCreatePreciseCollisionBoxPhysicalMaterial.get();
			if (movepm || precpm)
			{
				if (mesh)
				{
					Range3 boundingBox = Range3::empty;

					for_count(int, partIdx, mesh->get_parts_num())
					{
						auto* part = mesh->get_part(partIdx);

						part->for_every_vertex([&boundingBox](Vector3 const& _a)
							{
								boundingBox.include(_a);
							});
					}
					
					boundingBox.make_non_flat();

					Collision::Box collisionBox;
					collisionBox.setup(boundingBox);

					for_count(int, iPM, 2)
					{
						auto* pm = iPM == 0 ? movepm : precpm;
						if (!pm)
						{
							continue;
						}

						collisionBox.use_material(movepm);

						Collision::Model* model = new Collision::Model();
						model->add(collisionBox); // <- creates a copy

						CollisionModel* libraryCollisionMesh = Library::get_current()->get_collision_models().create_temporary(LibraryName::invalid());
						libraryCollisionMesh->replace_model(model);
						if (iPM == 0)
						{
							set_movement_collision_model(libraryCollisionMesh);
						}
						else
						{
							an_assert(iPM == 1)
								set_precise_collision_model(libraryCollisionMesh);
						}

						// check if we should use the same for precise collision model
						if (iPM == 0 &&
							precpm == movepm)
						{
							set_precise_collision_model(libraryCollisionMesh);
							break;
						}
					}
				}
				else
				{
					error(TXT("error preparing auto collisions, no mesh"));
					result = false;
				}
			}
		}
		if (autoCreateSpaceBlocker)
		{
			if (mesh)
			{
				Range3 boundingBox = Range3::empty;

				for_count(int, partIdx, mesh->get_parts_num())
				{
					auto* part = mesh->get_part(partIdx);

					part->for_every_vertex([&boundingBox](Vector3 const& _a)
						{
							boundingBox.include(_a);
						});
				}

				if (!boundingBox.is_empty())
				{
					boundingBox.make_non_flat();

					SpaceBlocker spaceBlocker;
					spaceBlocker.blocks = true;
					spaceBlocker.requiresToBeEmpty = true;
					spaceBlocker.box = Box(boundingBox);

					spaceBlockers.blockers.push_back(spaceBlocker);
				}
			}
			else
			{
				error(TXT("error preparing auto space blocker, no mesh"));
				result = false;
			}
		}
	}
	if (! generateOnDemandForLibrary)
	{
		IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LoadMaterialIntoMesh)
		{
			for_every(materialSetup, materialSetups)
			{
				result &= materialSetup->prepare_for_game(_library, _pfgContext, LibraryPrepareLevel::LoadMaterialIntoMesh);
			}
			set_missing_materials(_library);
		}
	}
	return result;
}

void Mesh::debug_draw() const
{
#ifdef AN_DEVELOPMENT
	if (!MeshGenerator::does_gather_generation_info() || !MeshGenerator::does_show_generation_info() || !MeshGenerator::does_show_generation_info_only())
#endif
	{
#ifdef AN_DEBUG_RENDERER
#ifdef AN_DEVELOPMENT
		if (debugDrawSkeleton)
#endif
		{
			if (Skeleton const* skeleton = get_skeleton())
			{
				skeleton->debug_draw();
			}
		}
#endif
	}

#ifdef AN_DEVELOPMENT
	if (debugDrawMesh)
#endif
	{
#ifdef AN_DEBUG_RENDERER
		Mesh const* drawMesh = this;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (auto* g = Game::get())
		{
			drawMesh = get_lod(g->get_preview_lod());
		}
#endif
		Meshes::Mesh3DInstance debugDrawMeshInstance;
		::System::MaterialShaderUsage::Type materialShaderUsage = drawMesh->mesh->get_material_usage();
		if (Skeleton const* skeleton = drawMesh->get_skeleton())
		{
			debugDrawMeshInstance.set_mesh(drawMesh->mesh, skeleton->get_skeleton());
			debugDrawMeshInstance.set_render_bone_matrices(skeleton->get_skeleton()->get_bones_default_matrix_MS());
		}
		else
		{
			debugDrawMeshInstance.set_mesh(drawMesh->mesh, nullptr);
		}
		debugDrawMeshInstance.set_placement(Transform::identity);
		Array<MaterialSetup> const& materialSetups = get_material_setups();
		int idx = 0;
		for_every(materialSetup, materialSetups)
		{
			debugDrawMeshInstance.set_material(idx, materialSetup->get_material()->get_material());
			::System::MaterialInstance* materialInstance = debugDrawMeshInstance.access_material_instance(idx);
			materialInstance->clear();
			materialInstance->set_material(materialSetup->get_material()->get_material(), materialShaderUsage);
			materialSetup->params.apply_to(materialInstance, ::System::MaterialSetting::Forced);
			++idx;
		}
		debugDrawMeshInstance.fill_materials_with_missing();
		debug_draw_mesh_bones(false, drawMesh->mesh, Transform::identity, &debugDrawMeshInstance, &debugDrawMeshInstance);
#endif
	}

#ifdef AN_DEVELOPMENT
	if (!MeshGenerator::does_gather_generation_info() || !MeshGenerator::does_show_generation_info() || !MeshGenerator::does_show_generation_info_only())
#endif
	{
#ifdef AN_DEBUG_RENDERER
		float alpha = DebugConfig::previewMode ? 0.5f : 1.0f;

#ifdef AN_DEVELOPMENT
		if (movementCollisionModel.get() && debugDrawMovementCollision)
		{
			movementCollisionModel->debug_draw_coloured(Colour::red);
		}
		if (preciseCollisionModel.get() && debugDrawPreciseCollision)
		{
			preciseCollisionModel->debug_draw_coloured(Colour::cyan);
		}
#endif

		// sockets
#ifdef AN_DEVELOPMENT
		if (debugDrawSockets)
#endif
		{
			for_every(socket, sockets->get_sockets())
			{
				Transform socketPlacement = Meshes::SocketDefinition::calculate_socket_using_ms(socket, nullptr, get_skeleton() ? get_skeleton()->get_skeleton() : nullptr);
#ifdef AN_DEVELOPMENT
				debug_draw_text(true, /*(debugMeshNodeIdx == NONE && debugPOIIdx == NONE  && debugSocketIdx == NONE) || */for_everys_index(socket) == debugSocketIdx ? Colour::white : Colour::grey.with_alpha(0.25f), socketPlacement.get_translation(), NP, true, 1.0f, true, socket->get_name().to_char());
				if (debugSocketIdx == NONE || for_everys_index(socket) == debugSocketIdx)
				{
					debug_draw_transform_size_coloured(true, socketPlacement, 0.1f, Colour::red.with_alpha(alpha), Colour::green.with_alpha(alpha), Colour::blue.with_alpha(alpha));
				}
				else
				{
					debug_draw_transform_size_coloured(true, socketPlacement, 0.1f, Colour::grey.with_alpha(alpha), Colour::grey.with_alpha(alpha), Colour::grey.with_alpha(alpha));
				}
#else
				debug_draw_transform_size(true, socketPlacement, 0.1f);
#endif
			}
		}

#ifdef AN_DEVELOPMENT
		if (debugDrawMeshNodes)
#endif
		{
			for_every_ref(meshNode, meshNodes)
			{
#ifdef AN_DEVELOPMENT
				debug_draw_text(true, /*(debugMeshNodeIdx == NONE && debugPOIIdx == NONE  && debugSocketIdx == NONE) || */for_everys_index(meshNode) == debugMeshNodeIdx ? Colour::white : Colour::grey.with_alpha(0.25f), meshNode->placement.get_translation(), NP, true, 1.0f, true, meshNode->name.to_char());
				if (!meshNode->is_dropped())
				{
					if (debugMeshNodeIdx == NONE || for_everys_index(meshNode) == debugMeshNodeIdx)
					{
						debug_draw_transform_size_coloured(true, meshNode->placement, 0.1f, Colour::red.with_alpha(alpha), Colour::green.with_alpha(alpha), Colour::blue.with_alpha(alpha));
					}
					else
					{
						debug_draw_transform_size_coloured(true, meshNode->placement, 0.1f, Colour::cyan.with_alpha(alpha), Colour::cyan.with_alpha(alpha), Colour::cyan.with_alpha(alpha));
					}
				}
				else
				{
					debug_draw_transform_size_coloured(true, meshNode->placement, 0.05f, Colour::lerp(0.7f, Colour::grey, Colour::red).with_alpha(alpha),
						Colour::lerp(0.7f, Colour::grey, Colour::green).with_alpha(alpha),
						Colour::lerp(0.7f, Colour::grey, Colour::blue).with_alpha(alpha));
				}
#else
				debug_draw_transform_size(true, meshNode->placement, 0.1f);
#endif
			}
		}

#ifdef AN_DEVELOPMENT
		if (debugDrawPOIs)
#endif
		{
			for_every_ref(poi, pois)
			{
#ifdef AN_DEVELOPMENT
				Transform placement = poi->offset;
				if (poi->socketName.is_valid())
				{
					int socketIdx = find_socket_index(poi->socketName);
					if (socketIdx != NONE)
					{
						Transform socketPlacement = calculate_socket_using_ms(socketIdx);
						placement = socketPlacement.to_world(placement);
					}
				}

				debug_draw_text(true, /*(debugMeshNodeIdx == NONE && debugPOIIdx == NONE  && debugSocketIdx == NONE) || */for_everys_index(poi) == debugPOIIdx ? Colour::white : Colour::grey.with_alpha(0.25f), placement.get_translation(), NP, true, 1.0f, true, poi->name.to_char());

				if (debugPOIIdx == NONE || for_everys_index(poi) == debugPOIIdx)
				{
					debug_draw_transform_size_coloured(true, placement, 0.1f, Colour::red.with_alpha(alpha), Colour::green.with_alpha(alpha), Colour::blue.with_alpha(alpha));
				}
				else
				{
					debug_draw_transform_size_coloured(true, placement, 0.1f, Colour::cyan.with_alpha(alpha), Colour::cyan.with_alpha(alpha), Colour::cyan.with_alpha(alpha));
				}
#else
				// ?! debug_draw_transform_size(true, meshNode->placement, 0.1f);
#endif
			}
		}

#ifdef AN_DEVELOPMENT
		if (debugDrawSpaceBlockers)
		{
			spaceBlockers.debug_draw();
		}
#endif

		// selected
#ifdef AN_DEVELOPMENT
		if (debugDrawSockets)
#endif
		{
			for_every(socket, sockets->get_sockets())
			{
#ifdef AN_DEVELOPMENT
				if (debugSocketIdx != NONE && for_everys_index(socket) == debugSocketIdx)
				{
					Transform socketPlacement = Meshes::SocketDefinition::calculate_socket_using_ms(socket, nullptr, get_skeleton() ? get_skeleton()->get_skeleton() : nullptr);
					debug_draw_text(true, Colour::white, socketPlacement.get_translation(), NP, true, 1.0f, true, socket->get_name().to_char());
					debug_draw_transform_size_coloured(true, socketPlacement, 0.2f,
						Colour::lerp(0.2f, Colour::red, get_debug_highlight_colour()),
						Colour::lerp(0.2f, Colour::green, get_debug_highlight_colour()),
						Colour::lerp(0.2f, Colour::blue, get_debug_highlight_colour())
					);
				}
#endif
			}
		}

		// selected
#ifdef AN_DEVELOPMENT
		if (debugDrawMeshNodes)
#endif
		{
			for_every_ref(meshNode, meshNodes)
			{
#ifdef AN_DEVELOPMENT
				if (debugMeshNodeIdx != NONE && for_everys_index(meshNode) == debugMeshNodeIdx)
				{
					debug_draw_text(true, Colour::white, meshNode->placement.get_translation(), NP, true, 1.0f, true, meshNode->name.to_char());
					debug_draw_transform_size_coloured(true, meshNode->placement, 0.2f,
						Colour::lerp(0.2f, Colour::red, get_debug_highlight_colour()),
						Colour::lerp(0.2f, Colour::green, get_debug_highlight_colour()),
						Colour::lerp(0.2f, Colour::blue, get_debug_highlight_colour())
					);
				}
#endif
			}
		}

#ifdef AN_DEVELOPMENT
		if (debugDrawPOIs)
#endif
		{
			for_every_ref(poi, pois)
			{
#ifdef AN_DEVELOPMENT
				if (debugPOIIdx != NONE && for_everys_index(poi) == debugPOIIdx)
				{
					Transform placement = poi->offset;
					if (poi->socketName.is_valid())
					{
						int socketIdx = find_socket_index(poi->socketName);
						if (socketIdx != NONE)
						{
							Transform socketPlacement = calculate_socket_using_ms(socketIdx);
							placement = socketPlacement.to_world(placement);
						}
					}

					debug_draw_text(true, Colour::white, placement.get_translation(), NP, true, 1.0f, true, poi->name.to_char());
					debug_draw_transform_size_coloured(true, placement, 0.2f,
						Colour::lerp(0.2f, Colour::red, get_debug_highlight_colour()),
						Colour::lerp(0.2f, Colour::green, get_debug_highlight_colour()),
						Colour::lerp(0.2f, Colour::blue, get_debug_highlight_colour())
					);
				}
#else
				debug_draw_transform_size(true, poi->offset, 0.1f);
#endif
			}
		}
#ifdef AN_DEVELOPMENT
		if (MeshGenerator::does_gather_debug_data() && MeshGenerator::does_show_debug_data() && useMeshGenerator.is_set())
		{
			if (auto* drf = useMeshGenerator->get_debug_renderer_frame())
			{
				debug_add_frame(drf);
			}
		}
#endif
#endif
	}
#ifdef AN_DEVELOPMENT
	if (MeshGenerator::does_gather_generation_info() && MeshGenerator::does_show_generation_info() && useMeshGenerator.is_set())
	{
		if (auto* gi = useMeshGenerator->get_generation_info())
		{
			gi->debug_draw();
		}
	}
#endif
}

void Mesh::log(LogInfoContext & _context) const
{
	base::log(_context);
#ifdef AN_DEVELOPMENT
	_context.log(TXT(": options"));
	_context.log(TXT("! render all"));
	_context.on_last_log_select([]() {debugDrawMesh = true; debugDrawSkeleton = true; debugDrawMeshNodes = true; debugDrawPOIs = true; debugDrawSockets = true; debugDrawMovementCollision = true; debugDrawPreciseCollision = true; debugDrawSpaceBlockers = true; });
	_context.log(TXT("! mesh + nodes only"));
	_context.on_last_log_select([]() {debugDrawMesh = true; debugDrawSkeleton = true; debugDrawMeshNodes = true; debugDrawPOIs = false; debugDrawSockets = false; debugDrawMovementCollision = false; debugDrawPreciseCollision = false; debugDrawSpaceBlockers = false; });
	_context.log(TXT("! mesh + pois only"));
	_context.on_last_log_select([](){debugDrawMesh = true; debugDrawSkeleton = true; debugDrawMeshNodes = false; debugDrawPOIs = true; debugDrawSockets = false; debugDrawMovementCollision = false; debugDrawPreciseCollision = false; debugDrawSpaceBlockers = false;  });
	_context.log(TXT("! mesh + sockets only"));
	_context.on_last_log_select([]() {debugDrawMesh = true; debugDrawSkeleton = true; debugDrawMeshNodes = false; debugDrawPOIs = false; debugDrawSockets = true; debugDrawMovementCollision = false; debugDrawPreciseCollision = false; debugDrawSpaceBlockers = false; });
	_context.log(TXT("! mesh only"));
	_context.on_last_log_select([](){debugDrawMesh = true; debugDrawSkeleton = true; debugDrawMeshNodes = false; debugDrawPOIs = false; debugDrawSockets = false; debugDrawMovementCollision = false; debugDrawPreciseCollision = false; debugDrawSpaceBlockers = false; });
	_context.log(TXT("! mesh only (no skeleton)"));
	_context.on_last_log_select([](){debugDrawMesh = true; debugDrawSkeleton = false; debugDrawMeshNodes = false; debugDrawPOIs = false; debugDrawSockets = false; debugDrawMovementCollision = false; debugDrawPreciseCollision = false; debugDrawSpaceBlockers = false; });
	_context.log(TXT("! collisions only"));
	_context.on_last_log_select([](){debugDrawMesh = false; debugDrawSkeleton = false; debugDrawMeshNodes = false; debugDrawPOIs = false; debugDrawSockets = false; debugDrawMovementCollision = true; debugDrawPreciseCollision = true; debugDrawSpaceBlockers = false; });
	_context.log(TXT("! movement collisions only"));
	_context.on_last_log_select([](){debugDrawMesh = false; debugDrawSkeleton = false; debugDrawMeshNodes = false; debugDrawPOIs = false; debugDrawSockets = false; debugDrawMovementCollision = true; debugDrawPreciseCollision = false; debugDrawSpaceBlockers = false; });
	_context.log(TXT("! precise collisions only"));
	_context.on_last_log_select([](){debugDrawMesh = false; debugDrawSkeleton = false; debugDrawMeshNodes = false; debugDrawPOIs = false; debugDrawSockets = false; debugDrawMovementCollision = false; debugDrawPreciseCollision = true; debugDrawSpaceBlockers = false; });
	_context.log(TXT("! mesh + space blockers only"));
	_context.on_last_log_select([](){debugDrawMesh = true; debugDrawSkeleton = false; debugDrawMeshNodes = false; debugDrawPOIs = false; debugDrawSockets = false; debugDrawMovementCollision = false; debugDrawPreciseCollision = false; debugDrawSpaceBlockers = true; });
	_context.log(TXT("! space blockers only"));
	_context.on_last_log_select([](){debugDrawMesh = false; debugDrawSkeleton = false; debugDrawMeshNodes = false; debugDrawPOIs = false; debugDrawSockets = false; debugDrawMovementCollision = false; debugDrawPreciseCollision = false; debugDrawSpaceBlockers = true; });
	_context.log(TXT(": content"));
#endif
	Mesh const* self = this;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (auto* g = Game::get())
	{
		self = get_lod(g->get_preview_lod());
	}
#endif

	{	// calculate totals
		LOG_INDENT(_context);
		int vertices = 0;
		int tris = 0;
		for (int i = 0; i < self->mesh->get_parts_num(); ++i)
		{
			if (auto const * meshPart = self->mesh->get_part(i))
			{
				vertices += meshPart->get_number_of_vertices();
				tris += meshPart->get_number_of_tris();
			}
		}
		if (self->mesh->get_parts_num() > 1)
		{
			_context.log(TXT("summary"));
		}
		else
		{
			_context.log(TXT("summary (single part)"));
		}
		{
			LOG_INDENT(_context);
			_context.log(TXT("vertices: %5i"), vertices);
			if (tris)
			{
				_context.log(TXT("tris:     %5i"), tris);
			}
		}
	}

	if (self->mesh->get_parts_num() > 1)
	{
		for (int i = 0; i < self->mesh->get_parts_num(); ++i)
		{
			if (auto const * meshPart = self->mesh->get_part(i))
			{
				_context.log(TXT("part %i:"), i);
				LOG_INDENT(_context);
				_context.log(TXT("vertices: %5i"), meshPart->get_number_of_vertices());
				if (int tris = meshPart->get_number_of_tris())
				{
					_context.log(TXT("tris:     %5i"), tris);
				}
			}
		}
	}

	_context.log(TXT(": sockets (%i)"), sockets->get_sockets().get_size());
	{
		LOG_INDENT(_context);
		for_every(socket, sockets->get_sockets())
		{
			_context.log(TXT("+-%S (-> %S)"), socket->get_name().to_char(), socket->get_bone_name().to_char());
#ifdef AN_DEVELOPMENT
			int socketIdx = for_everys_index(socket);
			_context.on_last_log_select([this, socketIdx]() {debugSocketIdx = socketIdx; }, [this]() {debugSocketIdx = NONE; });
#endif
		}
	}

	_context.log(TXT(": appearance controllers (%i)"), controllerDatas.get_size());
	if (!controllerDatas.is_empty())
	{
#ifdef AN_DEVELOPMENT
		ARRAY_PREALLOC_SIZE(AppearanceControllerData*, cds, controllerDatas.get_size());
		for_every_ref(cd, controllerDatas)
		{
			cds.push_back(cd);
		}
		sort(cds, AppearanceControllerData::compare_process_at_level);

		LOG_INDENT(_context);
		for_every_ptr(cd, cds)
		{
			_context.log(TXT("+-%03i (%S) %S"), cd->get_process_at_level(), cd->get_desc(), cd->get_location_info());
		}
#endif
	}

	_context.log(TXT(": mesh nodes (%i)"), meshNodes.get_size());
	if (!meshNodes.is_empty())
	{
		LOG_INDENT(_context);
		for_every_ref(meshNode, meshNodes)
		{
			if (!meshNode->is_dropped())
			{
				_context.log(TXT("+-%S (%S)"), meshNode->name.to_char(), meshNode->tags.to_string().to_char());
			}
			else
			{
				_context.log(TXT("+ %S (%S) *"), meshNode->name.to_char(), meshNode->tags.to_string().to_char());
			}
#ifdef AN_DEVELOPMENT
			int meshNodeIdx = for_everys_index(meshNode);
			_context.on_last_log_select([this, meshNodeIdx](){debugMeshNodeIdx = meshNodeIdx; }, [this](){debugMeshNodeIdx = NONE; });
#endif
			{
				LOG_INDENT(_context);
				meshNode->variables.log(_context);
			}
		}
	}

	_context.log(TXT(": pois (%i)"), pois.get_size());
	if (!pois.is_empty())
	{
		LOG_INDENT(_context);
		for_every_ref(poi, pois)
		{
			_context.log(TXT("+-%S"), poi->name.to_char());
#ifdef AN_DEVELOPMENT
			int poiIdx = for_everys_index(poi);
			_context.on_last_log_select([this, poiIdx](){debugPOIIdx = poiIdx; }, [this](){debugPOIIdx = NONE; });
#endif
		}
	}

	todo_note(TXT("controllers?"));

	if (Skeleton const * skeleton = get_skeleton())
	{
		_context.log(TXT(": skeleton"));
		skeleton->log(_context);
	}

	_context.log(TXT(": collisions"));

	_context.log(TXT(": movement collision"));
	if (auto const * mc = get_movement_collision_model())
	{
		mc->log(_context);
	}

	_context.log(TXT(": precise collision"));
	if (auto const * mc = get_precise_collision_model())
	{
		mc->log(_context);
	}

	_context.log(TXT(": space blockers"));
	get_space_blockers().log(_context);
}

int Mesh::get_all_materials_in_mesh_count() const
{
	return mesh ? mesh->get_parts_num() : 0;
}

void Mesh::set_missing_materials(Library* _library)
{
	if (mesh)
	{
		for (int index = 0, partsNum = mesh->get_parts_num(); index < partsNum; ++index)
		{
			if (materialSetups.is_index_valid(index))
			{
				// make sure we have a material, not just its name
				materialSetups[index].material.find_if_required(_library);
			}
			if (!get_material(index))
			{
				set_material(index, Material::get_default(_library));
			}
		}
	}
}

void Mesh::make_sure_mesh_exists()
{
	if (!mesh)
	{
		mesh = new Meshes::Mesh3D();
		mesh->set_usage(meshUsage);
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		mesh->set_debug_mesh_name(String(TXT("Framework::Mesh "))+get_name().to_string());
#endif
	}
}

void Mesh::replace_mesh(Meshes::Mesh3D* _mesh)
{
	if (mesh == _mesh)
	{
		return;
	}
	defer_mesh_delete();
	an_assert(::Meshes::Usage::are_of_same_kind(meshUsage, _mesh->get_usage()), TXT("both meshes for \"%S\" (\"%S\" and \"%S\") should be either static or skinned (\"%S\" v \"%S\")"),
		get_name().to_string().to_char(),
		mesh? mesh->get_debug_mesh_name() : TXT("??"),
		_mesh->get_debug_mesh_name(),
		::Meshes::Usage::to_char(meshUsage),
		::Meshes::Usage::to_char(_mesh->get_usage()));
	mesh = _mesh;
	meshUsage = mesh->get_usage();
}

Meshes::SocketDefinition const * Mesh::get_socket_in(int _socketIndex, Meshes::SocketDefinitionSet const & _sockets)
{
	if (!_sockets.get_sockets().is_index_valid(_socketIndex))
	{
		return nullptr;
	}
	return &_sockets.get_sockets()[_socketIndex];
}

int Mesh::find_socket_index_in(Name const & _socketName, Meshes::SocketDefinitionSet const & _sockets)
{
	return _sockets.find_socket_index(_socketName);
}

int Mesh::find_socket_index(Name const & _socketName) const
{
	an_assert(sockets);
	return find_socket_index_in(_socketName, *sockets);
}

Transform Mesh::calculate_socket_using_ms(int _socketIndex, Meshes::Pose const* _poseMS) const
{
	auto const * s = get_skeleton();
	return Meshes::SocketDefinition::calculate_socket_using_ms(get_socket(_socketIndex), _poseMS, s ? s->get_skeleton() : nullptr);
}

Transform Mesh::calculate_socket_using_ls(int _socketIndex, Meshes::Pose const* _poseLS) const
{
	auto const * s = get_skeleton();
	return Meshes::SocketDefinition::calculate_socket_using_ls(get_socket(_socketIndex), _poseLS, s ? s->get_skeleton() : nullptr);
}

bool Mesh::get_socket_info(int _socketIndex, OUT_ int & _boneIdx, OUT_ Transform & _relativePlacement) const
{
	auto const * fs = get_skeleton();
	return Meshes::SocketDefinition::get_socket_info(get_socket(_socketIndex), fs? fs->get_skeleton() : nullptr, OUT_ _boneIdx, OUT_ _relativePlacement);
}

Meshes::SocketDefinition const * Mesh::get_socket(int _socketIndex) const
{
	an_assert(sockets);
	return get_socket_in(_socketIndex, *sockets);
}

int Mesh::get_socket_count() const
{
	an_assert(sockets);
	return sockets->get_sockets().get_size();
}

bool Mesh::fill_on_create_from_template(LibraryStored& _newObject, CreateFromTemplate const & _createFromTemplate) const
{
	Mesh& _newMesh = *(fast_cast<Mesh>(&_newObject));
	an_assert(sockets);
	bool result = base::fill_on_create_from_template(_newMesh, _createFromTemplate);

	_newMesh.meshUsage = meshUsage;
	*_newMesh.sockets = *sockets;

	_newMesh.movementCollisionModel = movementCollisionModel;
	_createFromTemplate.get_renamer().apply_to(_newMesh.movementCollisionModel);

	_newMesh.preciseCollisionModel = preciseCollisionModel;
	_createFromTemplate.get_renamer().apply_to(_newMesh.preciseCollisionModel);

	_newMesh.materialSetups = materialSetups;
	for_every(materialSetup, _newMesh.materialSetups)
	{
		materialSetup->apply_renamer(_createFromTemplate.get_renamer());
	}

	_newMesh.useMeshGenerator = useMeshGenerator;
	_createFromTemplate.get_renamer().apply_to(_newMesh.useMeshGenerator);

	_newMesh.meshGeneratorParameters = meshGeneratorParameters;
	if (useCustomParametersForMeshGeneration)
	{
		_newMesh.meshGeneratorParameters.set_missing_from(get_custom_parameters());
	}
	_newMesh.meshGeneratorParameters.set_from(_createFromTemplate.get_mesh_generator_parameters());

	return result;
}

void Mesh::set_movement_collision_model(CollisionModel * _cm)
{
	movementCollisionModel.set(_cm);
	// cache collisions
	if (movementCollisionModel.get())
	{
		movementCollisionModel.get()->cache_for(get_skeleton());
	}
}

void Mesh::set_precise_collision_model(CollisionModel * _cm)
{
	preciseCollisionModel.set(_cm);
	// cache collisions
	if (preciseCollisionModel.get())
	{
		preciseCollisionModel.get()->cache_for(get_skeleton());
	}
}

void Mesh::fuse(Array<FuseMesh> const & _meshes, Array<Meshes::RemapBoneArray const *> const * _remapBones)
{
	if (!is_temporary())
	{
		error(TXT("only possible for temporary meshes"));
		return;
	}
	Array<FuseMesh> meshes = _meshes;
	meshes.push_front(FuseMesh(this)); // put in front to have materials properly setup

	Array<Meshes::RemapBoneArray const *> remapBones;
	if (_remapBones)
	{
		remapBones = *_remapBones;
		remapBones.push_front(nullptr);
	}

	Array<Array<Meshes::Mesh3D::FuseMesh3DPart> > parts;
	Array<Array<Meshes::RemapBoneArray const*>> partRemapBones;
	Array<MaterialSetup const *> materialSetupsToFuse;

	for_every(fuseMesh, meshes)
	{
		an_assert(fuseMesh->mesh->meshUsage == meshUsage);
		auto mesh3d = fast_cast<Meshes::Mesh3D>(fuseMesh->mesh->get_mesh());
		an_assert(mesh3d, TXT("can't fuse non mesh3d"));
		if (mesh3d->get_parts_num()) // fuse only non degenerate meshes
		{
			an_assert(mesh3d->get_parts_num() <= fuseMesh->mesh->get_material_setups().get_size(), TXT("not all materials defined for mesh? (parts:%i v materials:%i)"), mesh3d->get_parts_num(), fuseMesh->mesh->get_material_setups().get_size());
			for_every(materialSetup, fuseMesh->mesh->get_material_setups())
			{
				if (auto* partToFuse = mesh3d->access_part(for_everys_index(materialSetup)))
				{
					if (partToFuse->is_empty())
					{
						// no need to fuse it
						continue;
					}
					int existing = NONE;
					for_every_ptr(existingMaterialSetupsToFuse, materialSetupsToFuse)
					{
						if (*existingMaterialSetupsToFuse == *materialSetup)
						{
							existing = for_everys_index(existingMaterialSetupsToFuse);
						}
					}
					if (existing == NONE)
					{
						existing = materialSetupsToFuse.get_size();
						materialSetupsToFuse.push_back(materialSetup);
						int newSize = parts.get_size() + 1;
						memory_leak_suspect;
						parts.set_size(newSize);
						if (_remapBones)
						{
							partRemapBones.set_size(newSize);
						}
						forget_memory_leak_suspect;
					}
					parts[existing].push_back(Meshes::Mesh3D::FuseMesh3DPart(partToFuse, fuseMesh->placement));
					if (_remapBones)
					{
						partRemapBones[existing].push_back(remapBones[for_everys_index(fuseMesh)]);
					}
				}
			}
		}
	}

	// have all materials
	while (get_material_setups().get_size() < materialSetupsToFuse.get_size())
	{
		int idx = get_material_setups().get_size();
		set_material_setup(idx, *materialSetupsToFuse[idx]);
	}

	// fuse all parts together
	mesh->fuse_parts(parts, _remapBones? &partRemapBones : nullptr);

	// fuse collision models
	if (movementCollisionModel.is_set())
	{
		if (movementCollisionModel->is_temporary())
		{
			Array<CollisionModel::FuseModel> fuseCollisionModels;
			for_every(fuseMesh, _meshes)
			{
				if (auto * cm = fuseMesh->mesh->get_movement_collision_model())
				{
					fuseCollisionModels.push_back(CollisionModel::FuseModel(cm, fuseMesh->placement));
				}
			}
			movementCollisionModel->fuse(fuseCollisionModels);
		}
		else
		{
			error(TXT("can't fuse into non temporary model"));
			return;
		}
	}

	if (preciseCollisionModel.is_set())
	{
		if (preciseCollisionModel->is_temporary())
		{
			Array<CollisionModel::FuseModel> fuseCollisionModels;
			for_every(fuseMesh, _meshes)
			{
				if (auto * cm = fuseMesh->mesh->get_precise_collision_model())
				{
					fuseCollisionModels.push_back(CollisionModel::FuseModel(cm, fuseMesh->placement));
				}
			}
			preciseCollisionModel->fuse(fuseCollisionModels);
		}
		else
		{
			error(TXT("can't fuse into non temporary model"));
			return;
		}
	}

	// and finally...

	for_every(fuseMesh, _meshes)
	{
		auto* mesh = fuseMesh->mesh;
		sockets->add_sockets_from(mesh->get_sockets(), fuseMesh->placement);
		MeshNode::copy_not_dropped_to(mesh->get_mesh_nodes(), OUT_ meshNodes, fuseMesh->placement);
		if (fuseMesh->placement.is_set())
		{
			for_every(poiRef, mesh->get_pois())
			{
				auto* poi = poiRef->get();
				if (poi->socketName.is_valid())
				{
					pois.push_back(*poiRef); // may share it
				}
				else
				{
					auto* newPoi = poi->create_copy();
					newPoi->apply(fuseMesh->placement.get());
					pois.push_back(PointOfInterestPtr(newPoi));
				}
			}
		}
		else
		{
			pois.add_from(mesh->get_pois());
		}
		controllerDatas.add_from(mesh->get_controllers());
	}

	cache_collisions();

	// fuse lods, they are meshes too
	for_every(lod, lods)
	{
		Array<FuseMesh> givenLODMeshes;
		for_every(fuseMesh, _meshes)
		{
			givenLODMeshes.push_back(FuseMesh(fuseMesh->mesh->get_lod(for_everys_index(lod) + 1), fuseMesh->placement));
		}
		lod->mesh->fuse(givenLODMeshes, _remapBones);
	}

	update_lods_remapping();
}

void Mesh::cache_collisions()
{
	// cache collisions
	if (movementCollisionModel.get())
	{
		movementCollisionModel.get()->cache_for(get_skeleton());
	}
	if (preciseCollisionModel.get())
	{
		preciseCollisionModel.get()->cache_for(get_skeleton());
	}
}

MeshNode const * Mesh::find_mesh_node(Name const & _name) const
{
	for_every_ref(meshNode, meshNodes)
	{
		if (meshNode->name == _name)
		{
			return meshNode;
		}
	}
	return nullptr;
}

MeshNode * Mesh::access_mesh_node(Name const& _name)
{
	for_every_ref(meshNode, meshNodes)
	{
		if (meshNode->name == _name)
		{
			return meshNode;
		}
	}
	return nullptr;
}

Mesh* Mesh::create_temporary_hard_copy() const
{
	todo_important(TXT("implement_ me!"));
	return nullptr;
}

void Mesh::copy_from(Mesh const * _mesh)
{
	mesh = _mesh->mesh->create_copy();
	meshUsage = _mesh->meshUsage;
	sockets->add_sockets_from(*_mesh->sockets);
	an_assert(!movementCollisionModel.is_set());
	if (_mesh->movementCollisionModel.is_set())
	{
		movementCollisionModel = _mesh->movementCollisionModel->create_temporary_hard_copy();
	}
	an_assert(!preciseCollisionModel.is_set());
	if (_mesh->preciseCollisionModel.is_set())
	{
		preciseCollisionModel = _mesh->preciseCollisionModel->create_temporary_hard_copy();
	}
	materialSetups = _mesh->materialSetups;
	useMeshGenerator = _mesh->useMeshGenerator;
	meshGeneratorParameters = _mesh->meshGeneratorParameters;
	useCustomParametersForMeshGeneration = _mesh->useCustomParametersForMeshGeneration;
	useMeshGeneratorParameters = _mesh->useMeshGeneratorParameters;
	meshNodes = _mesh->meshNodes;
	pois = _mesh->pois;
	controllerDatas = _mesh->controllerDatas;
	lods.set_size(_mesh->lods.get_size());
	for_every(lod, lods)
	{
		auto& src = _mesh->lods[for_everys_index(lod)];
		lod->mesh = src.mesh->create_temporary_hard_copy();
		lod->remapBones = src.remapBones;
	}
}

void Mesh::be_unique_instance()
{
	isUniqueInstance = true;
	for_every(ml, lods)
	{
		if (auto* m = ml->mesh.get())
		{
			m->isUniqueInstance = true;
		}
	}
}

void Mesh::build_remap_bones(Mesh const* _toMeshToUse, Array<int>& _remapBones) const
{
	_remapBones.clear();
}

void Mesh::update_lods_remapping()
{
	scoped_call_stack_info(TXT("updatae lods remapping"));

	for_every(ml, lods)
	{
		scoped_call_stack_info_str_printf(TXT("for lod %i"), for_everys_index(ml));
		if (auto* m = ml->mesh.get())
		{
			m->build_remap_bones(this, ml->remapBones);
		}
	}
}

Array<int> const & Mesh::get_remap_bones_for_lod(int _lod) const
{
	an_assert(_lod > 0, TXT("makes no sense to remap main to main?"));
	an_assert(_lod <= lods.get_size(), TXT("we don't have so many"));
	return lods[_lod - 1].remapBones;
}

void Mesh::set_material(int _idx, Material* _material)
{
	materialSetups.set_size(max(materialSetups.get_size(), _idx + 1));
	materialSetups[_idx].set_material(_material);
	for_every(lod, lods)
	{
		lod->mesh->set_material(_idx, _material);
	}
}

void Mesh::set_material_setup(int _idx, MaterialSetup const& _materialSetup)
{
	materialSetups.set_size(max(materialSetups.get_size(), _idx + 1));
	materialSetups[_idx] = _materialSetup;
	for_every(lod, lods)
	{
		lod->mesh->set_material_setup(_idx, _materialSetup);
	}
}

void Mesh::set_skeleton(Skeleton* _skeleton)
{
	warn_dev_ignore(TXT("maybe you should use skinned?"));
}
