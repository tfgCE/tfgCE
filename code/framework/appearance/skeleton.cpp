#include "skeleton.h"

#include "appearanceControllerData.h"
#include "meshSkinned.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"

#include "..\..\core\debugConfig.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\serialisation\importerHelper.h"
#include "..\..\core\types\names.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(Framework::Skeleton);
LIBRARY_STORED_DEFINE_TYPE(Skeleton, skeleton);

Skeleton::Skeleton(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, skeleton(nullptr)
, sockets(new Meshes::SocketDefinitionSet())
{
}

Skeleton::~Skeleton()
{
	delete_and_clear(skeleton);
	an_assert(sockets);
	delete_and_clear(sockets);
}

void Skeleton::prepare_to_unload()
{
	base::prepare_to_unload();
	useMeshGenerator.set(nullptr);
}

#ifdef AN_DEVELOPMENT
void Skeleton::ready_for_reload()
{
	base::ready_for_reload();

	useMeshGenerator.set(nullptr);

	delete_and_clear(skeleton);
	an_assert(sockets);
	delete_and_clear(sockets);

	sockets = new Meshes::SocketDefinitionSet();
}
#endif

struct ::Framework::SkeletonSkeletonImporter
: public ImporterHelper<Meshes::Skeleton, Meshes::SkeletonImportOptions>
{
	typedef ImporterHelper<Meshes::Skeleton, Meshes::SkeletonImportOptions> base;

	SkeletonSkeletonImporter(Framework::Skeleton* _skeleton)
	: base(_skeleton->skeleton)
	{}

protected:
	override_ void import_object()
	{
		object = Meshes::Skeleton::importer().import(importFromFileName, importOptions);
		// prepare before saving to have that data saved
		object->prepare_to_use();
	}

private:
};

struct ::Framework::SkeletonSocketsImporter
: public ImporterHelper<Meshes::SocketDefinitionSet, Meshes::SocketDefinitionSetImportOptions>
{
	typedef ImporterHelper<Meshes::SocketDefinitionSet, Meshes::SocketDefinitionSetImportOptions> base;

	SkeletonSocketsImporter(Framework::Skeleton* _skeleton)
	: base(_skeleton->sockets)
	{}

protected:
	override_ void import_object()
	{
		object = Meshes::SocketDefinitionSet::importer().import(importFromFileName, importOptions);
	}

private:
};

bool Skeleton::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (!skeleton)
	{
		skeleton = new Meshes::Skeleton();
	}

	bool result = LibraryStored::load_from_xml(_node, _lc);

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("meshGenerator")))
	{
		result &= useMeshGenerator.load_from_xml(attr, _lc);
		result &= meshGeneratorParameters.load_from_xml_child_node(_node, TXT("meshGeneratorParameters"));
		_lc.load_group_into(meshGeneratorParameters);
	}
	else
	{
		SkeletonSkeletonImporter skeletonImporter(this);
		result &= skeletonImporter.setup_from_xml(_node, _lc.get_library_source() == LibrarySource::Assets);
		if (!skeletonImporter.import())
		{
#ifdef AN_OUTPUT_IMPORT_ERRORS
			error_loading_xml(_node, TXT("can't import"));
#endif
			result = false;
		}

		result &= skeleton->load_from_xml(_node);
		result &= skeleton->prepare_to_use();

		if (IO::XML::Node const * socketsNode = _node->first_child_named(TXT("sockets")))
		{
			SkeletonSocketsImporter socketsImporter(this);
			result &= socketsImporter.setup_from_xml(socketsNode, _lc.get_library_source() == LibrarySource::Assets);
			if (!socketsImporter.import())
			{
#ifdef AN_OUTPUT_IMPORT_ERRORS
				error_loading_xml(_node, TXT("can't import"));
#endif
				result = false;
			}
		}
	}

	if (IO::XML::Node const * socketsNode = _node->first_child_named(TXT("sockets")))
	{
		// just add to existing (or even modify already imported ones!)
		result &= sockets->load_from_xml(socketsNode);
	}

	skeletonIsReady = true;

	{
		Concurrency::ScopedSpinLock lock(deferredMeshesToImportLock);
		// skeleton is loaded, import deferred meshes that use this skeleton
		for_every_ptr(mesh, deferredMeshesToImport)
		{
			result &= mesh->perform_deferred_import(_lc);
		}
		deferredMeshesToImport.clear_and_prune();
	}

	return result;
}

bool Skeleton::generate_skeleton(Library* _library, LibraryPrepareContext& _pfgContext, REF_ MeshGeneratorRequest const & _meshGeneratorRequest)
{
	bool result = true;
	an_assert(useMeshGenerator.get() != nullptr);
	useMeshGenerator->load_on_demand_if_required();

#ifdef AN_DEVELOPMENT
	::System::TimeStamp loadingTime;
#endif

	MeshGeneratorRequest meshGeneratorRequest = _meshGeneratorRequest;
	meshGeneratorRequest.for_library_skeleton(this);
	meshGeneratorRequest.using_parameters(meshGeneratorParameters);
	meshGeneratorRequest.store_appearance_controllers(controllerDatas);
	meshGeneratorRequest.no_lods();
	if (::Meshes::Skeleton* skel = useMeshGenerator.get()->generate_skel(meshGeneratorRequest))
	{
#ifdef AN_DEVELOPMENT
		output(TXT("generated skeleton for \"%S\" in %.3fs"), get_name().to_string().to_char(), loadingTime.get_time_since());
#endif
	}
	else
	{
		error(TXT("could not generate skeleton"));
		result = false;
	}

	return result;
}


bool Skeleton::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		result = LibraryStored::prepare_for_game(_library, _pfgContext);
		if (useMeshGenerator.is_name_valid())
		{
			result &= useMeshGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::GenerateSkeletons)
	{
		if (useMeshGenerator.get())
		{
			result &= generate_skeleton(_library, _pfgContext);
		}
	}
	return result;
}

void Skeleton::debug_draw() const
{
#ifdef AN_DEBUG_RENDERER
	float alpha = DebugConfig::previewMode ? 0.5f : 1.0f;

	// bones
	for_every(boneTransform, skeleton->get_bones_default_placement_MS())
	{
#ifdef AN_DEVELOPMENT
		if (debugBoneIdx == NONE || for_everys_index(boneTransform) == debugBoneIdx)
		{
			debug_draw_transform_size_coloured(true, *boneTransform, 0.1f, Colour::red.with_alpha(alpha), Colour::green.with_alpha(alpha), Colour::blue.with_alpha(alpha));
		}
		else
		{
			debug_draw_transform_size_coloured(true, *boneTransform, 0.1f,
				Colour::red.blended_to(Colour::grey, 0.5f).with_alpha(alpha),
				Colour::green.blended_to(Colour::grey, 0.5f).with_alpha(alpha),
				Colour::blue.blended_to(Colour::grey, 0.5f).with_alpha(alpha));
		}
#else
		debug_draw_transform_size(true, *boneTransform, 0.1f);
#endif
	}
	int boneIndex = 0;
	for_every(bone, skeleton->get_bones())
	{
#ifdef AN_DEVELOPMENT
		debug_draw_text(true, /*(debugBoneIdx == NONE && debugSocketIdx == NONE) || */for_everys_index(bone) == debugBoneIdx ? Colour::white : Colour::grey.with_alpha(0.25f), skeleton->get_bones_default_placement_MS()[boneIndex].get_translation(), NP, true, 1.0f, true, bone->get_name().to_char());
#endif
		if (boneIndex != 0)
		{
			int parentIndex = bone->get_parent_index();
			if (parentIndex != NONE)
			{
				debug_draw_line(true, Colour::grey.with_alpha(alpha),
					skeleton->get_bones_default_placement_MS()[boneIndex].get_translation(),
					skeleton->get_bones_default_placement_MS()[parentIndex].get_translation());
			}
		}
		++boneIndex;
	}

	// sockets
	for_every(socket, sockets->get_sockets())
	{
		Transform socketPlacement = Meshes::SocketDefinition::calculate_socket_using_ms(socket, nullptr, skeleton);
#ifdef AN_DEVELOPMENT
		debug_draw_text(true, /*(debugBoneIdx == NONE && debugSocketIdx == NONE) || */for_everys_index(socket) == debugSocketIdx ? Colour::white : Colour::grey.with_alpha(0.25f), socketPlacement.get_translation(), NP, true, 1.0f, true, socket->get_name().to_char());
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

	// selected
	for_every(boneTransform, skeleton->get_bones_default_placement_MS())
	{
#ifdef AN_DEVELOPMENT
		if (for_everys_index(boneTransform) == debugBoneIdx)
		{
			debug_draw_text(true, Colour::white, boneTransform->get_translation(), NP, true, 1.0f, true, skeleton->get_bones()[for_everys_index(boneTransform)].get_name().to_char());
			debug_draw_transform_size_coloured(true, *boneTransform, 0.2f,
				Colour::lerp(0.2f, Colour::red, get_debug_highlight_colour()),
				Colour::lerp(0.2f, Colour::green, get_debug_highlight_colour()),
				Colour::lerp(0.2f, Colour::blue, get_debug_highlight_colour())
				);
		}
#endif
	}
	for_every(socket, sockets->get_sockets())
	{
#ifdef AN_DEVELOPMENT
		if (debugSocketIdx != NONE && for_everys_index(socket) == debugSocketIdx)
		{
			Transform socketPlacement = Meshes::SocketDefinition::calculate_socket_using_ms(socket, nullptr, skeleton);
			debug_draw_text(true, Colour::white, socketPlacement.get_translation(), NP, true, 1.0f, true, socket->get_name().to_char());
			debug_draw_transform_size_coloured(true, socketPlacement, 0.2f,
				Colour::lerp(0.2f, Colour::red, get_debug_highlight_colour()),
				Colour::lerp(0.2f, Colour::green, get_debug_highlight_colour()),
				Colour::lerp(0.2f, Colour::blue, get_debug_highlight_colour())
				);
		}
#endif
	}
#endif
}

void Skeleton::debug_draw_pose_MS(Array<Matrix44> const & _matMS) const
{
#ifdef AN_DEBUG_RENDERER
	float alpha = DebugConfig::previewMode ? 0.5f : 1.0f;

	// bones
	for_every(boneMatrix, _matMS)
	{
#ifdef AN_DEVELOPMENT
		debug_draw_text(true, /*(debugBoneIdx == NONE && debugSocketIdx == NONE) || */for_everys_index(boneMatrix) == debugBoneIdx ? Colour::white.with_alpha(0.9f) : Colour::grey.with_alpha(0.75f), boneMatrix->get_translation(), NP, true, 0.1f, true, skeleton->get_bones()[for_everys_index(boneMatrix)].get_name().to_char());
		if (debugBoneIdx == NONE || for_everys_index(boneMatrix) == debugBoneIdx)
		{
			debug_draw_matrix_size_coloured(true, *boneMatrix, 0.1f, Colour::red.with_alpha(alpha), Colour::green.with_alpha(alpha), Colour::blue.with_alpha(alpha));
		}
		else
		{
			debug_draw_matrix_size_coloured(true, *boneMatrix, 0.1f, Colour::grey.with_alpha(alpha), Colour::grey.with_alpha(alpha), Colour::grey.with_alpha(alpha));
		}
#else
		debug_draw_matrix_size(true, *boneMatrix, 0.1f);
#endif
	}
	int boneIndex = 0;
	for_every(bone, skeleton->get_bones())
	{
		if (boneIndex != 0)
		{
			int parentIndex = bone->get_parent_index();
			if (parentIndex != NONE)
			{
				debug_draw_line(true, Colour::grey.with_alpha(alpha),
					_matMS[boneIndex].get_translation(),
					_matMS[parentIndex].get_translation());
			}
		}
		++boneIndex;
	}

	// sockets
	for_every(socket, sockets->get_sockets())
	{
		Transform socketPlacement = Meshes::SocketDefinition::calculate_socket_using_ms(socket, _matMS, skeleton);
#ifdef AN_DEVELOPMENT
		debug_draw_text(true, /*(debugBoneIdx == NONE && debugSocketIdx == NONE) || */for_everys_index(socket) == debugSocketIdx ? Colour::white.with_alpha(0.9f) : Colour::grey.with_alpha(0.75f), socketPlacement.get_translation(), NP, true, 0.1f, true, socket->get_name().to_char());
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

	// selected
	for_every(boneMatrix, _matMS)
	{
#ifdef AN_DEVELOPMENT
		if (debugBoneIdx != NONE && for_everys_index(boneMatrix) == debugBoneIdx)
		{
			debug_draw_text(true, Colour::white, boneMatrix->get_translation(), NP, true, 0.1f, true, skeleton->get_bones()[for_everys_index(boneMatrix)].get_name().to_char());
			debug_draw_matrix_size(true, *boneMatrix, 0.2f);
		}
#endif
	}
	for_every(socket, sockets->get_sockets())
	{
#ifdef AN_DEVELOPMENT
		if (debugSocketIdx != NONE && for_everys_index(socket) == debugSocketIdx)
		{
			Transform socketPlacement = Meshes::SocketDefinition::calculate_socket_using_ms(socket, _matMS, skeleton);
			debug_draw_text(true, Colour::white, socketPlacement.get_translation(), NP, true, 0.1f, true, socket->get_name().to_char());
			debug_draw_transform_size(true, socketPlacement, 0.2f);
		}
#endif
	}
#endif
}

void Skeleton::defer_mesh_skinned_import(MeshSkinned* _mesh, LibraryLoadingContext & _lc)
{
	Concurrency::ScopedSpinLock lock(deferredMeshesToImportLock);

	if (skeletonIsReady)
	{
		_mesh->perform_deferred_import(_lc);
	}
	else
	{
		deferredMeshesToImport.push_back_unique(_mesh);
	}
}

void Skeleton::log(LogInfoContext & _context) const
{
	base::log(_context);
	{
		LOG_INDENT(_context);
		for_every(bone, skeleton->get_bones())
		{
			if (bone->get_parent_index() == NONE)
			{
				log_bone_info(for_everys_index(bone), _context);
			}
		}
	}

	_context.log(TXT(": sockets"));
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
}

void Skeleton::log_bone_info(int _boneIdx, LogInfoContext & _context) const
{
	_context.log(TXT("+-%S"), skeleton->get_bones()[_boneIdx].get_name().to_char());
#ifdef AN_DEVELOPMENT
	_context.on_last_log_select([this, _boneIdx](){debugBoneIdx = _boneIdx; }, [this](){debugBoneIdx = NONE; });
#endif
	LOG_INDENT(_context);
	for_every(bone, skeleton->get_bones())
	{
		if (bone->get_parent_index() == _boneIdx)
		{
			log_bone_info(for_everys_index(bone), _context);
		}
	}
}

void Skeleton::replace_skeleton(Meshes::Skeleton* _skeleton)
{
	if (skeleton == _skeleton)
	{
		return;
	}
	delete_and_clear(skeleton);
	skeleton = _skeleton;
}

void Skeleton::fuse(Skeleton const * _skeleton, Meshes::RemapBoneArray & _remapBones)
{
	an_assert(is_temporary());
	skeleton->fuse(_skeleton->skeleton, _remapBones);
	// no need to remap sockets (they contain actual bone names)
	sockets->add_sockets_from(*_skeleton->sockets);
}

Skeleton* Skeleton::create_temporary_hard_copy() const
{
	Skeleton* copy = new Skeleton(get_library(), LibraryName::invalid());
	copy->be_temporary();
	copy->copy_from(this);
	return copy;
}

void Skeleton::copy_from(Skeleton const * _skeleton)
{
	skeleton = _skeleton->skeleton->create_copy();
	sockets->add_sockets_from(*_skeleton->sockets);
	useMeshGenerator = _skeleton->useMeshGenerator;
	meshGeneratorParameters = _skeleton->meshGeneratorParameters;
	controllerDatas = _skeleton->controllerDatas;
}
