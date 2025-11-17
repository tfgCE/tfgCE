#include "meshGenerator.h"

#include "meshGenElement.h"
#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"

#include "..\appearance\appearanceController.h"
#include "..\appearance\appearanceControllerData.h"
#include "..\appearance\mesh.h"
#include "..\appearance\skeleton.h"
#include "..\game\gameConfig.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\mainSettings.h"
#include "..\..\core\collision\model.h"
#include "..\..\core\types\storeInScope.h"
#include "..\..\core\system\timeStamp.h"
#include "..\..\core\system\video\vertexFormatUtils.h"
#include "..\..\core\utils\utils_loadingForSystemShader.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		//#define SHOW_MESH_GENERATION_TIME
		#ifdef AN_DEVELOPMENT
			//#define SHOW_MESH_GENERATION_TIME_DETAILED
		#endif
	#endif
#endif

//

using namespace Framework;

#ifdef AN_INSPECT_MEMORY_LEAKS
//#define AN_SKIP_MESH_GENERATION
#endif

//#define OUTPUT_VERTEX_SHADER

//#define INSPECT_LOADING_TIMES

//

REGISTER_FOR_FAST_CAST(Framework::MeshGenerator);
LIBRARY_STORED_DEFINE_TYPE(MeshGenerator, meshGenerator);

#ifdef AN_DEVELOPMENT
Optional<Random::Generator> MeshGeneratorRequest::commonRandomGenerator;

bool MeshGenerator::showDebugData = false;
bool MeshGenerator::gatherDebugData = false;
bool MeshGenerator::showGenerationInfo = false;
bool MeshGenerator::showGenerationInfoOnly = true;
bool MeshGenerator::gatherGenerationInfo = false;
#endif

MeshGeneratorRequest::MeshGeneratorRequest()
{
#ifdef AN_DEVELOPMENT
	if (commonRandomGenerator.is_set())
	{
		usingRandomGenerator = &commonRandomGenerator.access();
	}
#endif
	generate_lods();
}

MeshGeneratorRequest& MeshGeneratorRequest::generate_lods(Optional<int> const & _lodCount)
{
	lodCount = _lodCount;
	forLOD = 0;
	return *this;
}

//

MeshGenerator::MeshGenerator(Library * _library, LibraryName const & _name)
: base(_library, _name)
, sockets(new Meshes::SocketDefinitionSet())
{
	// setup defaults
	indexFormat = System::IndexFormat();
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);

	vertexFormat = System::VertexFormat();
	vertexFormat.with_padding();
}

MeshGenerator::~MeshGenerator()
{
	an_assert(sockets);
	delete_and_clear(sockets);
#ifdef AN_DEVELOPMENT
	delete_and_clear(debugRendererFrame);
	delete_and_clear(generationInfo);
#endif
}

void MeshGenerator::clear()
{
	// setup defaults
	indexFormat = System::IndexFormat();
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);

	vertexFormat = System::VertexFormat();
	vertexFormat.with_padding();

	skelGenerator.set(nullptr);
	skeleton.set(nullptr);

	materialSetups.clear();

	mainElement = nullptr;
}

void MeshGenerator::add_material_setups_to(REF_ MeshGeneratorRequest & _request) const
{
	// fill materials with what we have
	while (_request.usingMaterialSetups.get_size() < get_material_setups().get_size())
	{
		_request.usingMaterialSetups.push_back(get_material_setups()[_request.usingMaterialSetups.get_size()]);
	}
}

::Meshes::Mesh3D* MeshGenerator::generate_mesh(REF_ MeshGeneratorRequest const& _request, REF_ MeshGeneratorRequest const& _skelGeneratorRequest)
{
	load_on_demand_if_required();

#ifdef SHOW_MESH_GENERATION_TIME
	::System::TimeStamp generationTS;
#endif

	if (canOnlyBeIncluded)
	{
#ifdef AN_DEVELOPMENT
		if (is_preview_game())
		{
			output(TXT("mesh generator \"%S\" should not be used as stand alone (allowing for a preview)"), get_name().to_string().to_char());
		}
		else
#endif
		{
			error(TXT("mesh generator \"%S\" should not be used as stand alone"), get_name().to_string().to_char());
			return nullptr;
		}
	}

#ifdef AN_DEVELOPMENT
	// MEASURE_AND_OUTPUT_PERFORMANCE(TXT("generated mesh %S"), get_name().to_string().to_char());
#endif

	MeshGeneratorRequest request = _request;

	add_material_setups_to(REF_ request);

	MeshGeneration::GenerationContext generationContext(_request.wmpOwner);
	UsedLibraryStored<Skeleton> generatedSkeleton;

#ifdef AN_DEVELOPMENT
	if (gatherDebugData && ! suspendedGatherDebugData)
	{
		if (!debugRendererFrame)
		{
			debugRendererFrame = new DebugRendererFrame(false);
		}
		generationContext.debugRendererFrame = debugRendererFrame;
		debugRendererFrame->clear();
	}
	if (gatherGenerationInfo)
	{
		if (!generationInfo)
		{
			generationInfo = new MeshGeneration::GenerationInfo();
		}
		generationContext.generationInfo = generationInfo;
		generationInfo->clear();
	}
#endif

	generationContext.set_allow_vertices_optimisation(allowVerticesOptimisation);

	generationContext.set_force_more_details(_request.forceMoreDetails);
	generationContext.set_for_lod(_request.forLOD);
	generationContext.set_use_lod_fully(_request.useLODFully.get(useLODFully.get(1.0f)));
	generationContext.set_1_as_sub_step_base(_request.use1AsSubStepBase.get(use1AsSubStepBase.get(0.0f)));

	generationContext.set_include_stack_processor(_request.include_stack_processor);

	generationContext.access_sockets().add_sockets_from(*sockets);
	generationContext.access_mesh_nodes().add_from(request.usingMeshNodes);

	for_count(int, idx, request.usingParametersCount)
	{
		if (request.usingParameters[idx])
		{
			generationContext.set_parameters(*request.usingParameters[idx]);
		}
	}

	generationContext.set_require_space_blockers(request.spaceBlockersResultPtr != nullptr);
	generationContext.set_require_appearance_controllers(request.appearanceControllersResultPtr != nullptr);

	if (request.usingRandomRandomGenerator)
	{
		generationContext.use_random_generator(Random::Generator());
	}
	if (request.usingRandomGenerator)
	{
		generationContext.use_random_generator(*request.usingRandomGenerator);
	}

	if (request.usingSkeletonOverride)
	{
		generationContext.for_skeleton(request.usingSkeletonOverride->get_skeleton());
	}
	else if (Skeleton * forSkeleton = skeleton.get())
	{
		generationContext.for_skeleton(forSkeleton->get_skeleton());
	}
	else if (MeshGenerator * useSkelGenerator = skelGenerator.get())
	{
		MeshGeneratorRequest skelRequest = _skelGeneratorRequest;
		for_count(int, idx, request.usingParametersCount)
		{
			if (request.usingParameters[idx])
			{
				skelRequest.using_parameters(*request.usingParameters[idx]);
			}
		}
		if (request.usingRandomRandomGenerator)
		{
			skelRequest.using_random_random_regenerator();
		}
		if (request.usingRandomGenerator)
		{
			skelRequest.using_random_regenerator(*request.usingRandomGenerator);
		}
		generatedSkeleton = useSkelGenerator->generate_temporary_library_skeleton(Library::get_current(), REF_ skelRequest);
		if (generatedSkeleton.get())
		{
			generationContext.for_skeleton(generatedSkeleton.get()->get_skeleton());
		}
	}

	generationContext.use_vertex_format(vertexFormat);
	generationContext.use_index_format(indexFormat);

	generationContext.set_require_movement_collision(request.movementCollisionResultPtr != nullptr);
	generationContext.set_require_precise_collision(request.preciseCollisionResultPtr != nullptr);

	generationContext.use_material_setups(request.usingMaterialSetups);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	generationContext.set_log_mesh_data(logMeshData);
#endif

#ifdef SHOW_MESH_GENERATION_TIME
	::System::TimeStamp justProcessTS;
#endif
#ifdef AN_SKIP_MESH_GENERATION
	warn(TXT("meshes won't be generated in \"inspect memory leaks\" build!"));
#else
	if (mainElement.is_set())
	{
		if (generationContext.process(mainElement.get(), nullptr))
		{
			// all ok
		}
		else
		{
			error(TXT("there were errors processing \"%S\" mesh generator"), get_name().to_string().to_char());
		}
	}
#endif
#ifdef SHOW_MESH_GENERATION_TIME
	float justProcessTime = justProcessTS.get_time_since();
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (logMeshData &&
		generationContext.get_generated_mesh())
	{
		LogInfoContext log;
		log.log(TXT("generated mesh (just post generation)"));
		generationContext.get_generated_mesh()->log(log);
		log.output_to_output();
	}
#endif

	if (generationContext.does_require_movement_collision() && createMovementCollisionMesh.create)
	{
		generationContext.store_movement_collision_parts(create_collision_meshes_from(generationContext, createMovementCollisionMesh));
	}

	if (generationContext.does_require_precise_collision() && createPreciseCollisionMesh.create)
	{
		generationContext.store_precise_collision_parts(create_collision_meshes_from(generationContext, createPreciseCollisionMesh));
	}

	if (generationContext.does_require_precise_collision() && createPreciseCollisionMeshSkinned.create)
	{
		generationContext.store_precise_collision_parts(create_collision_skinned_meshes_from(generationContext, createPreciseCollisionMeshSkinned));
	}

	if (generationContext.does_require_movement_collision() && createMovementCollisionBox.create)
	{
		generationContext.store_movement_collision_part(create_collision_box_from(generationContext, createMovementCollisionBox));
	}

	if (generationContext.does_require_precise_collision() && createPreciseCollisionBox.create)
	{
		generationContext.store_precise_collision_part(create_collision_box_from(generationContext, createPreciseCollisionBox));
	}

	if (generationContext.does_require_space_blockers() && createSpaceBlocker.create)
	{
		generationContext.store_space_blocker(create_space_blocker_from(generationContext, createSpaceBlocker));
	}


#ifdef SHOW_MESH_GENERATION_TIME
	::System::TimeStamp finaliseTS;
#endif
	generationContext.finalise();

#ifdef SHOW_MESH_GENERATION_TIME
	float timeFinalise = finaliseTS.get_time_since();
#endif

#ifdef SHOW_MESH_GENERATION_TIME
	::System::TimeStamp postFinaliseTS;
#endif

	// use skeleton that was created on the way
	if (request.generatedSkeletonResultPtr &&
		!generationContext.get_generated_bones().is_empty())
	{
		if (generatedSkeleton.get())
		{
			error(TXT("can't mix generating skeleton through seperate mesh generator with adding bones by hand"));
		}
		else
		{
			// provide generated skeleton, to allow filling it with bones during finalise
			if (generationContext.does_require_appearance_controllers())
			{
				// use existing one, we are storing appearance controllers any way
				todo_hack(TXT("it is a hack, sort of - to avoid doubled generation"));
				generatedSkeleton = Library::get_current()->get_skeletons().create_temporary(_request.useLibraryNameForNew);
				setup_library_skeleton_with_generated(generatedSkeleton.get(), generationContext.consume_generated_skeleton(), request);
			}
			else
			{
				// create new one
				generatedSkeleton = generate_temporary_library_skeleton(Library::get_current(), REF_ request);
			}
		}
	}

	if (generationContext.does_require_movement_collision() &&
		generationContext.get_generated_movement_collision() &&
		!generationContext.get_generated_movement_collision()->is_empty())
	{
		(*request.movementCollisionResultPtr) = generationContext.consume_generated_movement_collision();
		if (*request.movementCollisionResultPtr)
		{
			// for movement collision...
			// always break meshes into convex meshes
			(*request.movementCollisionResultPtr)->break_meshes_into_convex_meshes();
			//(*request.movementCollisionResultPtr)->break_meshes_into_convex_hulls(); // hulls do not work very good
		}
	}

	if (generationContext.does_require_precise_collision() &&
		generationContext.get_generated_precise_collision() &&
		!generationContext.get_generated_precise_collision()->is_empty())
	{
		(*request.preciseCollisionResultPtr) = generationContext.consume_generated_precise_collision();
		// for precise collision...
		// do not break meshes into convex meshes - no need for that
	}

	if (generatedSkeleton.get() &&
		request.generatedSkeletonResultPtr)
	{
		(*request.generatedSkeletonResultPtr) = generatedSkeleton;
	}

	if (request.socketsResultPtr)
	{
		request.socketsResultPtr->add_sockets_from(generationContext.get_sockets());
	}

	if (request.appearanceControllersResultPtr)
	{
		request.appearanceControllersResultPtr->add_from(generationContext.get_appearance_controllers());
	}

	if (request.meshNodesResultPtr)
	{
		// add only these which one we added
		MeshNode::add_not_dropped_except_to(generationContext.get_mesh_nodes(), request.usingMeshNodes, *request.meshNodesResultPtr);
	}

	if (request.poisResultPtr)
	{
		request.poisResultPtr->add_from(generationContext.get_pois());
	}

	if (generationContext.does_require_space_blockers() &&
		request.spaceBlockersResultPtr)
	{
		request.spaceBlockersResultPtr->add(generationContext.get_space_blockers());
	}

	auto* generatedMesh = generationContext.consume_generated_mesh();

	if (vertexFormat.has_skinning_data())
	{
		if (!generatedMesh->check_if_has_valid_skinning_data())
		{
			error(TXT("no skinning data created using \"%S\", may have degenerated vertices after transform"), get_name().to_string().to_char());
		}
	}

	{
		bool shouldFillVertexInfo = false;
		for_every(vm, ::MainSettings::global().get_vertex_texture_colour_mapping())
		{
			bool allShaderOptions = true;
			vm->shaderOptions.do_for_every_tag([&allShaderOptions](Tag const& _tag)
			{
				if (!::MainSettings::global().get_shader_options().get_tag_as_int(_tag.get_tag()))
				{
					allShaderOptions = false;
				}
			});
			if (allShaderOptions)
			{
				if (vertexFormat.has_custom_data(vm->customVertexData))
				{
					shouldFillVertexInfo = true;
					break;
				}
			}
		}
		if (shouldFillVertexInfo)
		{
			// populate material in material prop using uv and actual materials
			fill_vertex_info(generatedMesh);
		}
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (logMeshData && generatedMesh)
	{
		LogInfoContext log;
		log.log(TXT("generated mesh"));
		generatedMesh->log(log);
		log.output_to_output();
	}
#endif

	if (request.forLibraryMesh)
	{
		setup_library_mesh_with_generated(request.forLibraryMesh, generatedMesh, _request);
	}

#ifdef SHOW_MESH_GENERATION_TIME
	float timePostFinalise = postFinaliseTS.get_time_since();
#endif

#ifdef SHOW_MESH_GENERATION_TIME
	{
		ScopedOutputLock outputLock;
		output_colour(0, 1, 0, 1);
		output(TXT("generated mesh \"%S\" (lod:%i) in %.3fms (p:%.3fms + f:%.3fms, pf:%.3fms)"), get_name().to_string().to_char(), request.forLOD, generationTS.get_time_since() * 1000.0f, justProcessTime * 1000.0f, timeFinalise * 1000.0f, timePostFinalise * 1000.0f);
		output_colour(1, 1, 1, 0);
#ifdef SHOW_MESH_GENERATION_TIME_DETAILED
		String info = generationContext.get_performance_for_elements();
		while (info.get_length() > 16000)
		{
			String subInfo = info.get_left(16000);
			output(subInfo.to_char());
			info = info.get_sub(16000);
		}
		output(info.to_char());
#endif
		output_colour();
	}
#endif

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
	generatedMesh->set_debug_mesh_name(String(TXT("Framework::MeshGenerator ")) + get_name().to_string());
#endif

	return generatedMesh;
}

#ifdef AN_DEVELOPMENT
void MeshGenerator::ready_for_reload()
{
	base::ready_for_reload();

	clear();

	an_assert(sockets);
	delete sockets;

	sockets = new Meshes::SocketDefinitionSet();
}
#endif

bool MeshGenerator::sub_load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	allowVerticesOptimisation = _node->get_bool_attribute_or_from_child_presence(TXT("allowVerticesOptimisation"), allowVerticesOptimisation);
	allowVerticesOptimisation = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowVerticesOptimisation"), ! allowVerticesOptimisation);
	forceMoreDetails = _node->get_bool_attribute_or_from_child_presence(TXT("forceMoreDetails"), forceMoreDetails);
	generateLODs.load_from_xml(_node, TXT("generateLODs"));
	useLODFully.load_from_xml(_node, TXT("useLODFully"));
	use1AsSubStepBase.load_from_xml(_node, TXT("use1AsSubStepBase"));

	return result;
}

bool MeshGenerator::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
#ifdef INSPECT_LOADING_TIMES
	::System::TimeStamp loadingTS;
#endif
	bool result = base::load_from_xml(_node, _lc);

	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	createPreciseCollisionMeshSkinned.load_from_xml(_node, TXT("createPreciseCollisionMeshSkinned"), _lc);

	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));

	result &= MeshGeneration::Element::load_single_element_from_xml(_node, _lc, TXT("mainElement"), mainElement);

	skeleton.load_from_xml(_node, TXT("skeleton"), _lc);
	skelGenerator.load_from_xml(_node, TXT("meshGeneratorForSkeleton"), _lc);

	todo_note(TXT("how to approach skel generators?"));
	canOnlyBeIncluded = false;

	result &= sub_load_from_xml(_node, _lc);
	for_every(node, _node->children_named(TXT("for")))
	{
		if (CoreUtils::Loading::should_load_for_system_or_shader_option(node))
		{
			result &= sub_load_from_xml(node, _lc);
		}
	}

	if (IO::XML::Node const * node = _node->first_child_named(TXT("vertexFormat")))
	{
		canOnlyBeIncluded = false;
		result &= vertexFormat.load_from_xml(node);
	}
	else
	{
		vertexFormat.with_normal();
		vertexFormat.with_texture_uv();
		if (skeleton.is_name_valid())
		{
			// assume skinned
			vertexFormat.with_skinning_data(); // 8 bit is enough
		}
		else
		{
			vertexFormat.no_skinning_data();
		}
	}

	{
		for_every(vm, ::MainSettings::global().get_vertex_texture_colour_mapping())
		{
			bool allShaderOptions = true;
			vm->shaderOptions.do_for_every_tag([&allShaderOptions](Tag const& _tag)
			{
				if (!::MainSettings::global().get_shader_options().get_tag_as_int(_tag.get_tag()))
				{
					allShaderOptions = false;
				}
			});
			if (allShaderOptions)
			{
				vertexFormat.with_custom(vm->customVertexData, vm->dataType, ::System::VertexFormatAttribType::Float, 4);
			}
		}
	}

	if (IO::XML::Node const * node = _node->first_child_named(TXT("indexFormat")))
	{
		canOnlyBeIncluded = false;
		result &= indexFormat.load_from_xml(node);
	}

	canOnlyBeIncluded = _node->get_bool_attribute_or_from_child_presence(TXT("onlyIncluded"), canOnlyBeIncluded);

	vertexFormat.calculate_stride_and_offsets();
	indexFormat.calculate_stride();
#ifdef OUTPUT_VERTEX_SHADER
	{
		if (vertexFormat.get_stride() <= 64)
		{
			output(TXT("mg:%S, vf-stride %i"), get_name().to_string().to_char(), vertexFormat.get_stride());
		}
		else
		{
			output(TXT("mg:%S, vf-stride-exceeded %i"), get_name().to_string().to_char(), vertexFormat.get_stride());
		}
		for_every(attrib, vertexFormat.get_attribs())
		{
			output(TXT("   %03i-%03i:%02i %S"), attrib->offset, attrib->offset + attrib->size - 1, attrib->size, attrib->name.to_char());
		}
	}
#endif

	if (!mainElement.is_set())
	{
		if (!_node->first_child_named(TXT("mainElement")) ||
			!_node->first_child_named(TXT("mainElement"))->first_child_named(TXT("empty")))
		{
			warn_loading_xml(_node, TXT("could not find or load main element for mesh generator (or add <empty/>)"));
		}
	}

	for_every(materialsNode, _node->children_named(TXT("materials")))
	{
		defaultMaterialSetupAs = materialsNode->get_int_attribute_or_from_child(TXT("defaultAs"), defaultMaterialSetupAs);
		for_every(materialSetupNode, materialsNode->children_named(TXT("material")))
		{
			int idx = materialSetupNode->get_int_attribute(TXT("index"), -1);
			if (idx >= 0)
			{
				materialSetups.set_size(max(materialSetups.get_size(), idx + 1));
				materialSetups[idx].load_from_xml(materialSetupNode, _lc);
			}
		}
	}

	for_every(socketsNode, _node->children_named(TXT("sockets")))
	{
		result &= sockets->load_from_xml(socketsNode);
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	logMeshData = _node->get_bool_attribute_or_from_child_presence(TXT("logMeshData"), logMeshData);
#endif
		
#ifdef INSPECT_LOADING_TIMES
	{
		float loadingTime = loadingTS.get_time_since();
		output(TXT("[mesh-gen-load] %7.5f : %S"), loadingTime, get_name().to_string().to_char());
	}
#endif

	return result;
}

bool MeshGenerator::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (skeleton.is_name_valid())
	{
		skeleton.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	if (skelGenerator.is_name_valid())
	{
		skelGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	if (mainElement.is_set())
	{
		result &= mainElement->prepare_for_game(_library, _pfgContext);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		for_every(materialSetup, materialSetups)
		{
			result &= materialSetup->prepare_for_game(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
	}

	return result;
}

UsedLibraryStored<Mesh> MeshGenerator::generate_temporary_library_mesh(Library* _library, REF_ MeshGeneratorRequest const & _usingRequest, REF_ MeshGeneratorRequest const & _skelGeneratorRequest)
{
	load_on_demand_if_required();

	if (canOnlyBeIncluded)
	{
		error(TXT("mesh generator \"%S\" should not be used as stand alone"), get_name().to_string().to_char());
		return UsedLibraryStored<Mesh>();
	}

	scoped_call_stack_info_str_printf(TXT("generating mesh using \"%S\""), get_name().to_string().to_char());
	MeshGeneratorRequest usingRequest = _usingRequest;
	usingRequest.lodCount = 0;
	UsedLibraryStored<Mesh> resultMesh;
	if (generate_for_library_mesh(resultMesh, _library, usingRequest, _skelGeneratorRequest))
	{
		resultMesh->lods.clear();
		int lodCount = _usingRequest.lodCount.get(generateLODs.get(GameConfig::get() ? GameConfig::get()->get_auto_generate_lods() : 0));
		if (lodCount > 1)
		{
			resultMesh->get_mesh()->update_bounding_box(); // to calculate size for LOD

			MeshGeneratorRequest requestLOD = _usingRequest;
			for_count(int, lod, lodCount - 1)
			{
				requestLOD.for_lod(lod + 1);
				requestLOD.use_lod_fully(requestLOD.useLODFully.get(useLODFully.get(1.0f)));
				requestLOD.use_1_as_sub_step_base(requestLOD.use1AsSubStepBase.get(use1AsSubStepBase.get(0.0f)));
				UsedLibraryStored<Mesh> resultMeshLOD = generate_temporary_library_mesh(_library, requestLOD, _skelGeneratorRequest);
				if (resultMeshLOD.get())
				{
					MeshLOD ml;
					ml.mesh = resultMeshLOD;
					resultMesh->lods.push_back(ml);
				}
				else
				{
					error(TXT("couldn't generate lod mesh"));
				}
			}
			resultMesh->update_lods_remapping();
		}
		return resultMesh;
	}
	return UsedLibraryStored<Mesh>();
}

bool MeshGenerator::generate_for_library_mesh(UsedLibraryStored<Mesh> & _mesh, Library* _library, REF_ MeshGeneratorRequest const & _usingRequest, REF_ MeshGeneratorRequest const & _skelGeneratorRequest)
{
	load_on_demand_if_required();

	if (canOnlyBeIncluded)
	{
		error(TXT("mesh generator \"%S\" should not be used as stand alone"), get_name().to_string().to_char());
		return false;
	}

#ifdef AN_DEVELOPMENT
	StoreInScope<DebugRendererFrame*> keepDebugRendererFrame(debugRendererFrame);
	StoreInScope<DebugRendererFrame*> keepSuspendedGatherDebugData(suspendedGatherDebugData);
	if (_usingRequest.forLOD != 0)
	{
		suspendedGatherDebugData = debugRendererFrame;
		debugRendererFrame = nullptr;
	}
#endif
	// using "store" to keep values for library mesh
	MeshGeneratorRequest request = _usingRequest;
	an_assert(! request.is_storing_anything(), TXT("when generating a temporary library mesh, do not use storing, use things form the generated mesh"));
	an_assert(! request.lodCount.is_set() || request.lodCount.get() == 0, TXT("if auto generating lods, use generate_temporary_library_mesh"));
	MeshGeneratorRequest skelRequest = _skelGeneratorRequest;
	Collision::Model* generatedMovementCollisionModel = nullptr;
	Collision::Model* generatedPreciseCollisionModel = nullptr;
	Meshes::SocketDefinitionSet sockets;
	UsedLibraryStored<Skeleton> generatedSkeleton;
	Array<MeshNodePtr> meshNodes;
	Array<PointOfInterestPtr> pois;
	Array<AppearanceControllerDataPtr> controllerDatas;
	SpaceBlockers spaceBlockers;
	if (auto* mesh = generate_mesh(request.store_movement_collision(generatedMovementCollisionModel)
										  .store_precise_collision(generatedPreciseCollisionModel)
										  .store_generated_skeleton(generatedSkeleton)
										  .store_sockets(sockets)
										  .store_mesh_nodes(meshNodes)
										  .store_pois(pois)
										  .store_appearance_controllers(controllerDatas)
										  .store_space_blockers(spaceBlockers),
								   skelRequest.store_appearance_controllers(controllerDatas)))
	{
		if (!_mesh.get())
		{
			if (Meshes::Usage::is_static(mesh->get_usage()))
			{
				_mesh = _library->get_meshes_static().create_temporary(_usingRequest.useLibraryNameForNew);
			}
			else
			{
				_mesh = _library->get_meshes_skinned().create_temporary(_usingRequest.useLibraryNameForNew);
			}
		}
		setup_library_mesh_with_generated(_mesh.get(), mesh, request);
		return true;
	}
	return false;
}

void MeshGenerator::setup_library_mesh_with_generated(Mesh* _libraryMesh, ::Meshes::Mesh3D* _generatedMesh, MeshGeneratorRequest const & _request)
{
	_libraryMesh->replace_mesh(_generatedMesh);
	if (_request.generatedSkeletonResultPtr &&
		_request.generatedSkeletonResultPtr->get())
	{
		_libraryMesh->set_skeleton(_request.generatedSkeletonResultPtr->get());
		an_assert_log_always(!_libraryMesh->get_skeleton() || _libraryMesh->get_skeleton() == _request.generatedSkeletonResultPtr->get()); // no skeleton as static or set
	}
	if (_request.movementCollisionResultPtr &&
		*_request.movementCollisionResultPtr)
	{
		CollisionModel* cm = Library::get_current()->get_collision_models().create_temporary(_request.useLibraryNameForNew);
		cm->replace_model(*_request.movementCollisionResultPtr);
		_libraryMesh->set_movement_collision_model(cm);
	}
	if (_request.preciseCollisionResultPtr &&
		*_request.preciseCollisionResultPtr)
	{
		if (!(*_request.preciseCollisionResultPtr)->is_empty())
		{
			CollisionModel* cm = Library::get_current()->get_collision_models().create_temporary(_request.useLibraryNameForNew);
			cm->replace_model(*_request.preciseCollisionResultPtr);
			_libraryMesh->set_precise_collision_model(cm);
		}
		else
		{
			_libraryMesh->set_precise_collision_model(nullptr);
			delete *_request.preciseCollisionResultPtr;
		}
	}
	int materialsProvidedCount = 0;
	for (int i = _libraryMesh->get_material_setups().get_size(); i < _request.usingMaterialSetups.get_size(); ++i)
	{
		_libraryMesh->set_material_setup(i, _request.usingMaterialSetups[i]);
		materialsProvidedCount = max(i + 1, materialsProvidedCount);
	}
	for (int i = _libraryMesh->get_material_setups().get_size(); i < get_material_setups().get_size(); ++i)
	{
		_libraryMesh->set_material_setup(i, get_material_setups()[i]);
		materialsProvidedCount = max(i + 1, materialsProvidedCount);
	}
	if (defaultMaterialSetupAs != NONE && materialsProvidedCount < _libraryMesh->get_all_materials_in_mesh_count())
	{
		if (defaultMaterialSetupAs < materialsProvidedCount)
		{
			MaterialSetup copy = _libraryMesh->get_material_setups()[defaultMaterialSetupAs];
			for (int i = materialsProvidedCount; i < _libraryMesh->get_all_materials_in_mesh_count(); ++i)
			{
				_libraryMesh->set_material_setup(i, copy); // copy as we might be relocating the array
			}
		}
		else
		{
			error(TXT("wanted to use material %i but only %i material(s) provided"), defaultMaterialSetupAs, materialsProvidedCount);
		}
	}
	if (_request.socketsResultPtr &&
		_request.socketsResultPtr != &_libraryMesh->access_sockets())
	{
		_libraryMesh->access_sockets().add_sockets_from(*_request.socketsResultPtr);
	}
	if (_request.meshNodesResultPtr &&
		_request.meshNodesResultPtr != &_libraryMesh->access_mesh_nodes())
	{
		MeshNode::add_not_dropped_to(*_request.meshNodesResultPtr, OUT_ _libraryMesh->access_mesh_nodes());
	}
	if (_request.poisResultPtr &&
		_request.poisResultPtr != &_libraryMesh->access_pois())
	{
		_libraryMesh->access_pois().add_from(*_request.poisResultPtr);
	}
	if (_request.spaceBlockersResultPtr &&
		_request.spaceBlockersResultPtr != &_libraryMesh->access_space_blockers())
	{
		_libraryMesh->access_space_blockers().add(*_request.spaceBlockersResultPtr);
	}
	if (_request.appearanceControllersResultPtr &&
		_request.appearanceControllersResultPtr != &_libraryMesh->access_controllers())
	{
		_libraryMesh->access_controllers().add_from(*_request.appearanceControllersResultPtr);
	}

	// checks
	if (fast_cast<MeshSkinned>(_libraryMesh))
	{
		if (!_libraryMesh->get_skeleton())
		{
			an_assert_log_always(false, TXT("created skinned mesh \"%S\" using mesh generator \"%S\" that has no skeleton (no bones?)"), _libraryMesh->get_name().to_string().to_char(), get_name().to_string().to_char());
		}
	}
}

::Meshes::Skeleton* MeshGenerator::generate_skel(REF_ MeshGeneratorRequest const & _request)
{
	load_on_demand_if_required();

	if (canOnlyBeIncluded)
	{
		error(TXT("mesh generator \"%S\" should not be used as stand alone"), get_name().to_string().to_char());
		return nullptr;
	}

#ifdef AN_DEVELOPMENT
	// MEASURE_AND_OUTPUT_PERFORMANCE(TXT("generated skel %S"), get_name().to_string().to_char());
#endif

	MeshGeneration::GenerationContext generationContext(_request.wmpOwner);

	generationContext.set_allow_vertices_optimisation(allowVerticesOptimisation);

	generationContext.set_force_more_details(_request.forceMoreDetails || forceMoreDetails);
	generationContext.set_for_lod(_request.forLOD);
	generationContext.set_use_lod_fully(_request.useLODFully.get(useLODFully.get(1.0f)));
	generationContext.set_1_as_sub_step_base(_request.use1AsSubStepBase.get(use1AsSubStepBase.get(0.0f)));

	generationContext.set_include_stack_processor(_request.include_stack_processor);

	generationContext.access_sockets().add_sockets_from(*sockets);
	generationContext.access_mesh_nodes().add_from(_request.usingMeshNodes);

	for_count(int, idx, _request.usingParametersCount)
	{
		if (_request.usingParameters[idx])
		{
			generationContext.set_parameters(*_request.usingParameters[idx]);
		}
	}

	generationContext.set_require_space_blockers(_request.spaceBlockersResultPtr != nullptr);
	generationContext.set_require_appearance_controllers(_request.appearanceControllersResultPtr != nullptr);

	if (_request.usingRandomGenerator)
	{
		generationContext.use_random_generator(*_request.usingRandomGenerator);
	}

#ifdef AN_SKIP_SKEl_GENERATION
	warn(TXT("skeletons won't be generated in \"inspect memory leaks\" build!"));
#else
	if (mainElement.is_set())
	{
		if (generationContext.process(mainElement.get(), nullptr))
		{
			// all ok
		}
		else
		{
			error(TXT("there were errors processing \"%S\" skel generator"), get_name().to_string().to_char());
		}
	}
#endif

	generationContext.finalise();

	if (_request.socketsResultPtr)
	{
		_request.socketsResultPtr->add_sockets_from(generationContext.get_sockets());
	}

	if (_request.appearanceControllersResultPtr)
	{
		_request.appearanceControllersResultPtr->add_from(generationContext.get_appearance_controllers());
	}

	auto* generatedSkeleton = generationContext.consume_generated_skeleton();

	if (_request.forLibrarySkeleton)
	{
		setup_library_skeleton_with_generated(_request.forLibrarySkeleton, generatedSkeleton, _request);
	}

#ifdef AN_DEVELOPMENT
	output(TXT("generating skel %S"), get_name().to_string().to_char());
#endif
	return generatedSkeleton;
}

UsedLibraryStored<Skeleton> MeshGenerator::generate_temporary_library_skeleton(Library* _library, REF_ MeshGeneratorRequest const & _usingRequest)
{
	load_on_demand_if_required();

	MeshGeneratorRequest request = _usingRequest;
	Meshes::SocketDefinitionSet sockets; // sockets created here should be stored in skeleton only
	if (auto* skeleton = generate_skel(request.store_sockets(sockets)))
	{
		UsedLibraryStored<Skeleton> generatedSkeleton;
		generatedSkeleton = _library->get_skeletons().create_temporary();
		setup_library_skeleton_with_generated(generatedSkeleton.get(), skeleton, request);
		return generatedSkeleton;
	}
	return UsedLibraryStored<Skeleton>();
}

void MeshGenerator::setup_library_skeleton_with_generated(Skeleton* _librarySkeleton, ::Meshes::Skeleton* _generatedSkeleton, MeshGeneratorRequest const & _request)
{
	_librarySkeleton->replace_skeleton(_generatedSkeleton);

	if (_request.socketsResultPtr)
	{
		_librarySkeleton->access_sockets().add_sockets_from(*_request.socketsResultPtr);
	}
}

#ifdef AN_DEVELOPMENT
void MeshGenerator::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	if (mainElement.is_set())
	{
		mainElement->for_included_mesh_generators(_do);
	}
}
#endif

void MeshGenerator::fill_vertex_info(Meshes::Mesh3D* _mesh)
{
	for_count(int, partIdx, _mesh->get_parts_num())
	{
		if (auto * part = _mesh->access_part(partIdx))
		{
			if (part->is_empty())
			{
				// just skip it
				continue;
			}
			int materialIdx = partIdx;
			if (materialSetups.is_index_valid(materialIdx))
			{
				if (auto * mat = materialSetups[materialIdx].get_material())
				{
					struct Mapping
					{
						Array<Colour> map;
						float materialWidth = 0.0f;
						int offset;
						::System::DataFormatValueType::Type dataType;
						int8* ptr;
					};
					List<Mapping> mappings;
					for_every(vm, ::MainSettings::global().get_vertex_texture_colour_mapping())
					{
						bool allShaderOptions = true;
						vm->shaderOptions.do_for_every_tag([&allShaderOptions](Tag const& _tag)
						{
							if (!::MainSettings::global().get_shader_options().get_tag_as_int(_tag.get_tag()))
							{
								allShaderOptions = false;
							}
						});
						if (allShaderOptions)
						{
							if (auto * matCol = mat->get_params().get(vm->fragmentSamplerInput))
							{
								if (Texture * texture = Library::get_current()->get_textures().find(matCol->textureName))
								{
									if (auto * setup = texture->get_texture()->get_setup())
									{
										mappings.push_back(Mapping());
										Mapping & mapping = mappings.get_last();
										mapping.offset = part->get_vertex_format().get_custom_data_offset(vm->customVertexData);
										mapping.dataType = part->get_vertex_format().get_custom_data_type(vm->customVertexData);
										mapping.materialWidth = (float)setup->size.x;
										mapping.map.set_size(setup->size.x);
										int y = clamp((int)((float)(setup->size.y - 1) * vm->atV), 0, setup->size.y - 1);
										for_count(int, x, setup->size.x)
										{
											mapping.map[x] = setup->get_pixel(0, VectorInt2(x, y));
										}
									}
								}
							}
						}
					}
					if (!mappings.is_empty())
					{
						for_every(m, mappings)
						{
							if (m->materialWidth != mappings.get_first().materialWidth)
							{
								error(TXT("material and prop mismatch"));
							}
							continue;
						}
						float materialWidth = mappings.get_first().materialWidth;
						int const stride = part->get_vertex_format().get_stride();
						int uOffset = part->get_vertex_format().get_texture_uv_offset();
						if (uOffset == NONE)
						{
							error(TXT("requires U component in vertex format to use mapping \"%S\""), get_name().to_string().to_char());
							continue;
						}
						int8 const* uPtr = part->get_vertex_data().get_data() + uOffset;
						for_every(m, mappings)
						{
							m->ptr = part->access_vertex_data().get_data() + m->offset;
						}
						for_count(int, vertexIdx, part->get_number_of_vertices())
						{
							float u = ::System::VertexFormatUtils::restore_as_float(uPtr, part->get_vertex_format().get_texture_uv_type());
							int uAsInt = (int)(floor(u * materialWidth) + 0.1f);
							for_every(m, mappings)
							{
								::System::VertexFormatUtils::store(m->ptr, m->dataType, m->map[clamp(uAsInt, 0, m->map.get_size() - 1)]);
								m->ptr += stride;
							}
							uPtr += stride;
						}
					}
				}
				else
				{
					error(TXT("no material"));
				}
			}
		}
	}
}
