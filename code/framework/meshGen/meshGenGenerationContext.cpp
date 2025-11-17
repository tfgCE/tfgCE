#include "meshGenGenerationContext.h"

#include "meshGenElement.h"
#include "meshGenUtils.h"

#include "..\appearance\appearanceControllerData.h"
#include "..\appearance\skeleton.h"
#include "..\debug\previewGame.h"
#include "..\world\pointOfInterest.h"

#include "..\..\core\collision\model.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\mesh\skeleton.h"
#include "..\..\core\mesh\socketDefinitionSet.h"
#include "..\..\core\system\timeStamp.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

// mesh generation parameters
DEFINE_STATIC_NAME(materialIndex);

//

void GenerationContext::StackElement::on_get()
{
	currentParentBone = Name::invalid();
}

void GenerationContext::StackElement::on_release()
{
	// clean up memory
	parameters.clear();
	
	boneRenamer.clear();
}

//

REGISTER_FOR_FAST_CAST(GenerationContext);

GenerationContext::GenerationContext(WheresMyPoint::IOwner* _wmpOwner)
: wmpOwner(_wmpOwner)
, sockets(new Meshes::SocketDefinitionSet())
{
	randomGeneratorStack.push_back(Random::Generator(124578, 3690));
	generatedSkeleton = new ::Meshes::Skeleton();

	mesh = new ::Meshes::Mesh3D();
	movementCollision = new ::Collision::Model();
	preciseCollision = new ::Collision::Model();

	setup_for_static();

	push_stack(); // to have one element on stack
	push_checkpoint(Checkpoint(*this));
}

GenerationContext::~GenerationContext()
{
	an_assert(stack.get_size() == 1);
	pop_checkpoint();
	pop_stack(); // to make sure we didn't remove more than we had

	delete_and_clear(generatedSkeleton); // in case we're left with it
	delete_and_clear(mesh); // in case we're left with it
	delete_and_clear(movementCollision);
	delete_and_clear(preciseCollision);
	delete_and_clear(sockets);

	an_assert(movementCollisionParts.is_empty(), TXT("should be finalised"));
	an_assert(preciseCollisionParts.is_empty(), TXT("should be finalised"));
}

bool GenerationContext::is_mesh_static() const
{
	return mesh->get_usage() == ::Meshes::Usage::Static;
}

void GenerationContext::setup_for_skinned()
{
	mesh->set_usage(::Meshes::Usage::Skinned);

	// most likely those might be overridden by mesh generator

	indexFormat = System::IndexFormat();
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);
	indexFormat.calculate_stride();

	vertexFormat = System::VertexFormat();
	vertexFormat.with_padding();
	vertexFormat.with_normal();
	vertexFormat.with_texture_uv();
	vertexFormat.with_skinning_data(); // 8 bit is enough
	vertexFormat.calculate_stride_and_offsets();
}

void GenerationContext::setup_for_static()
{
	mesh->set_usage(::Meshes::Usage::Static);

	// most likely those might be overridden by mesh generator

	indexFormat = System::IndexFormat();
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);
	indexFormat.calculate_stride();

	vertexFormat = System::VertexFormat();
	vertexFormat.with_padding();
	vertexFormat.with_normal();
	vertexFormat.with_texture_uv();
	vertexFormat.calculate_stride_and_offsets();
}

void GenerationContext::use_vertex_format(::System::VertexFormat const & _vertexFormat)
{
	vertexFormat = _vertexFormat;
	if (vertexFormat.has_skinning_data())
	{
		if (vertexFormat.get_skinning_element_count() == 1)
		{
			mesh->set_usage(::Meshes::Usage::SkinnedToSingleBone);
		}
		else
		{
			mesh->set_usage(::Meshes::Usage::Skinned);
		}
	}
	else
	{
		mesh->set_usage(::Meshes::Usage::Static);
	}
}

void GenerationContext::use_index_format(::System::IndexFormat const & _indexFormat)
{
	indexFormat = _indexFormat;
}

void GenerationContext::for_skeleton(::Meshes::Skeleton const * _skeleton)
{
	providedSkeleton = _skeleton;
	if (providedSkeleton)
	{
		setup_for_skinned();
	}
	else
	{
		setup_for_static();
	}
}

bool GenerationContext::process(Element const * _element, ElementInstance * _parent, int _idx)
{
	if (!_element)
	{
		return true;
	}

	ElementInstance elementInstance(*this, _element, _parent);

	bool useRandomGeneratorStack = _element->should_use_random_generator_stack();

	if (useRandomGeneratorStack)
	{
		push_random_generator_stack();
		advance_random_generator(_idx * 105, _idx * 234);
	}

	bool result = elementInstance.process();

	if (useRandomGeneratorStack)
	{
		pop_random_generator_stack();
	}

	return result;

}

int GenerationContext::get_material_index_for_part(::Meshes::Mesh3DPart* _part) const
{
	for_every(pbm, partsByMaterials)
	{
		for_every(part, *pbm)
		{
			if (part->part == _part)
			{
				return for_everys_index(pbm);
			}
		}
	}
	return 0;
}

void GenerationContext::change_material_index_for_part(::Meshes::Mesh3DPart* _part, int _changeToMaterialIdx)
{
	// remove from existing one
	for_every(pbm, partsByMaterials)
	{
		for_every(part, *pbm)
		{
			if (part->part == _part)
			{
				pbm->remove_at(for_everys_index(part));
			}
		}
	}

	if (partsByMaterials.get_size() <= _changeToMaterialIdx)
	{
		partsByMaterials.set_size(_changeToMaterialIdx + 1);
	}

	partsByMaterials[_changeToMaterialIdx].push_back(::Meshes::Mesh3D::FuseMesh3DPart(_part));
}

void GenerationContext::ignore_part_for_collision(Meshes::Mesh3DPart const * _part)
{
	doNotUsePartsForCollision.push_back(RefCountObjectPtr<Meshes::Mesh3DPart>(_part));
}

bool GenerationContext::should_use_part_for_collision(Meshes::Mesh3DPart const* _part) const
{
	for_every_ref(p, doNotUsePartsForCollision)
	{
		if (p == _part)
		{
			return false;
		}
	}
	return true;
}

int GenerationContext::store_part(Meshes::Mesh3DPart* _part, int _materialIdx)
{
#ifdef AN_DEBUG
	an_assert(_part->get_number_of_vertices() > 0);
#endif
#ifdef AN_ASSERT
	for_every_ref(p, parts)
	{
		an_assert(p != _part);
	}
#endif

	parts.push_back(RefCountObjectPtr<Meshes::Mesh3DPart>(_part));

	if (partsByMaterials.get_size() <= _materialIdx)
	{
		partsByMaterials.set_size(_materialIdx + 1);
	}

	partsByMaterials[_materialIdx].push_back(::Meshes::Mesh3D::FuseMesh3DPart(_part));

	return parts.get_size() - 1;
}

int GenerationContext::store_part(Meshes::Mesh3DPart* _part, ElementInstance & _instance, Optional<int> const & _overrideMaterialIdx)
{
	return store_part(_part, _overrideMaterialIdx.get(_instance.get_material_index_from_params()));
}

int GenerationContext::store_part_just_as(Meshes::Mesh3DPart* _part, Meshes::Mesh3DPart const * _existingPart)
{
	int usingMaterialIndex = 0;
	{
		for_every(partsByMaterial, partsByMaterials)
		{
			if (::Meshes::Mesh3D::FuseMesh3DPart::does_contain(*partsByMaterial, _existingPart))
			{
				usingMaterialIndex = for_everys_index(partsByMaterial);
			}
		}
	}

	return store_part(_part, usingMaterialIndex);
}

void GenerationContext::store_movement_collision_part(Collision::Model* _part)
{
	if (_part)
	{
		if (!_part->is_empty())
		{
			movementCollisionParts.push_back(_part);
		}
		else
		{
			delete _part;
		}
	}
}

void GenerationContext::store_precise_collision_part(Collision::Model* _part)
{
	if (_part)
	{
		if (!_part->is_empty())
		{
			preciseCollisionParts.push_back(_part);
		}
		else
		{
			delete _part;
		}
	}
}

void GenerationContext::store_movement_collision_parts(Array<::Collision::Model*> const & _parts)
{
	for_every_ptr(part, _parts)
	{
		store_movement_collision_part(part);
	}
}

void GenerationContext::store_precise_collision_parts(Array<::Collision::Model*> const & _parts)
{
	for_every_ptr(part, _parts)
	{
		store_precise_collision_part(part);
	}
}

void GenerationContext::store_space_blocker(SpaceBlocker const& _sb)
{
	if (!_sb.is_none())
	{
		spaceBlockers.blockers.push_back(_sb);
	}
}

#ifdef SHOW_MESH_GENERATION_TIME
//#define SHOW_MESH_GENERATION_TIME_FINALISE_DETAILED
#endif

#ifdef SHOW_MESH_GENERATION_TIME_FINALISE_DETAILED
#define MESH_GENERATION_TIME_START(time) ::System::TimeStamp TS##time;
#define MESH_GENERATION_TIME_END(time) { float timeTaken = TS##time.get_time_since(); output(TXT(" : %S : %.3f"), TXT(#time), timeTaken); }
#else
#define MESH_GENERATION_TIME_START(time)
#define MESH_GENERATION_TIME_END(time)
#endif

void GenerationContext::finalise()
{
	MESH_GENERATION_TIME_START(skeleton);
	if (!bones.is_empty())
	{
		if (providedSkeleton)
		{
			error(TXT("bones generated when skeleton was provided"));
		}

		generatedSkeleton->access_bones().clear();
		generatedSkeleton->access_bones().make_space_for(bones.get_size());
		for_every(bone, bones)
		{
			an_assert(bone->boneName != bone->parentName);
			::Meshes::BoneDefinition boneDefinition;
			boneDefinition.set_name(bone->boneName);
			boneDefinition.set_parent_name(bone->parentName);
			boneDefinition.set_placement_MS(bone->placement);
			generatedSkeleton->access_bones().push_back(boneDefinition);
		}

		if (!generatedSkeleton->prepare_to_use())
		{
			error(TXT("skeletal generation failed: skeleton could not be prepared to use properly"));
		}
	}
	MESH_GENERATION_TIME_END(skeleton);

	MESH_GENERATION_TIME_START(fuseParts);
	mesh->fuse_parts(partsByMaterials, nullptr);
	MESH_GENERATION_TIME_END(fuseParts);

	if (allowVerticesOptimisation)
	{
#ifdef AN_DEVELOPMENT
		auto * pg = fast_cast<PreviewGame>(Game::get());
		if (! pg || pg->should_optimise_vertices())
		{
#endif
			MESH_GENERATION_TIME_START(optimiseAndPruneVertices);
			mesh->optimise_vertices();
			MESH_GENERATION_TIME_END(optimiseAndPruneVertices);
#ifdef AN_DEVELOPMENT
		}
#endif
	}

	MESH_GENERATION_TIME_START(finaliseCollisions);
	movementCollision->fuse(movementCollisionParts);
	preciseCollision->fuse(preciseCollisionParts);
	partsByMaterials.clear();
	parts.clear();

	for_every_ptr(mcp, movementCollisionParts)
	{
		delete mcp;
	}
	movementCollisionParts.clear();

	for_every_ptr(pcp, preciseCollisionParts)
	{
		delete pcp;
	}
	preciseCollisionParts.clear();
	MESH_GENERATION_TIME_END(finaliseCollisions);

#ifdef AN_ASSERT
	finalised = true;
#endif
}

void GenerationContext::push_stack()
{
	stack.push_back(StackElement::get_one());

	if (stack.get_size() > 1)
	{
		StackElement & curr = *(stack.get_last());
		StackElement & prev = *(stack[stack.get_size() - 2]);

		// copy only those that go alongs
		curr.parameters = prev.parameters;
		curr.currentParentBone = prev.currentParentBone;
	}
}

Name const & GenerationContext::get_current_parent_bone() const
{
	an_assert(!stack.is_empty());

	return stack.get_last()->currentParentBone;
}

void GenerationContext::set_current_parent_bone(Name const & _currentParent)
{
	an_assert(!stack.is_empty());

	stack.get_last()->currentParentBone = _currentParent;
}

void GenerationContext::use_bone_renamer(::Meshes::BoneRenamer const & _boneRenamer)
{
	an_assert(!stack.is_empty());

	stack.get_last()->boneRenamer.add_from(_boneRenamer);
}

void GenerationContext::pop_stack()
{
	an_assert(stack.get_size() > 0, TXT("shouldn't be empty at this point!"));
	stack.get_last()->release();
	stack.pop_back();
}

void GenerationContext::push_element(Element const * _element)
{
	elementStack.push_back(_element);
}

void GenerationContext::pop_element()
{
	an_assert(elementStack.get_size() > 0, TXT("shouldn't be empty at this point!"));
	elementStack.pop_back();
}

int32 GenerationContext::get_bone_parent(int32 _boneIdx) const
{
	if (providedSkeleton)
	{
		return providedSkeleton->get_parent_of(_boneIdx);
	}
	if (!bones.is_empty())
	{
		if (bones.is_index_valid(_boneIdx))
		{
			Name parent = bones[_boneIdx].parentName;;
			for_every(bone, bones)
			{
				if (bone->boneName == parent)
				{
					return for_everys_index(bone);
				}
			}
		}
		return NONE;
	}
	return NONE;
}

int32 GenerationContext::find_bone_index(Name const & _boneName) const
{
	if (providedSkeleton)
	{
		Name findBoneByName = _boneName;
		int32 foundBoneIndex = NONE;

		// go through whole stack
		an_assert(!stack.is_empty());
		for_every_reverse_ptr(stackElement, stack)
		{
			StackElement const & curr = *stackElement;

			findBoneByName = curr.boneRenamer.apply(findBoneByName);
		}

		// try searching bone as we sorted name
		if (foundBoneIndex == NONE)
		{
			foundBoneIndex = providedSkeleton->find_bone_index(findBoneByName);
		}

		return foundBoneIndex;
	}
	if (!bones.is_empty())
	{
		for_every(bone, bones)
		{
			if (bone->boneName == _boneName)
			{
				return for_everys_index(bone);
			}
		}
		error(TXT("bone \"%S\" not found"), _boneName.to_char());
		return NONE;
	}
	if (_boneName.is_valid())
	{
		error(TXT("trying to find bone \"%S\" without skeleton"), _boneName.to_char());
	}
	return NONE;
}

int GenerationContext::get_children_bone(int _index, OUT_ REF_ Array<int>& _childrenBones, bool _andDescendants) const
{
	int startAt = _childrenBones.get_size();
	if (providedSkeleton)
	{
		todo_important();
		return 0;
	}
	if (!bones.is_empty())
	{
		if (bones.is_index_valid(_index))
		{
			for_every(bone, bones) // children should be after the parents, one pass should be enough
			{
				if (bone->parentName == bones[_index].boneName)
				{
					_childrenBones.push_back(for_everys_index(bone));
				}
				else if (_andDescendants)
				{
					for_every(bIdx, _childrenBones)
					{
						if (for_everys_index(bIdx) >= startAt && bone->parentName == bones[*bIdx].boneName)
						{
							_childrenBones.push_back(for_everys_index(bone));
						}
					}
				}
			}
		}
		return _childrenBones.get_size() - startAt;
	}
	return 0;
}

Name const & GenerationContext::find_skeleton_bone_name(int32 _boneIdx) const
{
	if (providedSkeleton)
	{
		return (_boneIdx != NONE && _boneIdx < providedSkeleton->get_num_bones()) ? providedSkeleton->get_bones()[_boneIdx].get_name() : Name::invalid();
	}
	if (!bones.is_empty())
	{
		return bones[_boneIdx].boneName;
	}
	error(TXT("trying to find bone %i without skeleton"), _boneIdx);
	return Name::invalid();

}

bool GenerationContext::store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	set_parameter(_byName, _value.get_type(), _value.get_raw());
	return true;
}

bool GenerationContext::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return convert_parameter(_byName, _to);
}

bool GenerationContext::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return rename_parameter(_from, _to);
}

bool GenerationContext::restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	TypeId id;
	void const * rawData;

	// get from context's stack
	if (get_any_parameter(_byName, OUT_ id, OUT_ rawData))
	{
		_value.set_raw(id, rawData);
		return true;
	}
	else
	{
		return false;
	}
}

bool GenerationContext::store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	set_global_parameter(_byName, _value.get_type(), _value.get_raw());
	return true;
}

bool GenerationContext::restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	TypeId id;
	void const * rawData;

	// get from context's stack
	if (get_any_global_parameter(_byName, OUT_ id, OUT_ rawData))
	{
		_value.set_raw(id, rawData);
		return true;
	}
	else
	{
		return false;
	}
}

Name GenerationContext::resolve_bone(Name const & _boneName) const
{
	Name boneName = _boneName;

	// go through whole stack
	an_assert(!stack.is_empty());
	for_every_reverse_ptr(stackElement, stack)
	{
		StackElement const & curr = *stackElement;

		boneName = curr.boneRenamer.apply(boneName);
	}

	return boneName;
}

int GenerationContext::get_bones_count() const
{
	if (providedSkeleton)
	{
		return providedSkeleton->get_num_bones();
	}
	if (!bones.is_empty())
	{
		return bones.get_size();
	}
	return 0;
}

Name GenerationContext::get_bone_name(int _index) const
{
	if (providedSkeleton)
	{
		if (_index == NONE)
		{
			return Name::invalid();
		}
		if (_index < providedSkeleton->get_num_bones())
		{
			return providedSkeleton->get_bones()[_index].get_name();
		}
		error(TXT("invalid bone index"));
		return Name::invalid();
	}
	if (!bones.is_empty())
	{
		if (_index == NONE)
		{
			return Name::invalid();
		}
		if (bones.is_index_valid(_index))
		{
			return bones[_index].boneName;
		}
		error(TXT("invalid bone index"));
		return Name::invalid();
	}
	error(TXT("no skeleton"));
	return Name::invalid();
}

Transform GenerationContext::get_bone_placement_MS(int _index) const
{
	if (providedSkeleton)
	{
		todo_important(TXT("check index"));
		return providedSkeleton->get_bones_default_placement_MS()[_index];
	}
	if (!bones.is_empty())
	{
		if (bones.is_index_valid(_index))
		{
			return bones[_index].placement;
		}
		error(TXT("invalid bone index"));
		return Transform::identity;
	}
	error(TXT("no skeleton"));
	return Transform::identity;
}

bool GenerationContext::get_generated_bone_if_exists(Name const & _boneName, OPTIONAL_ OUT_ int * _boneIdx)
{
	for_every(bone, bones)
	{
		if (bone->boneName == _boneName)
		{
			assign_optional_out_param(_boneIdx, for_everys_index(bone));
			return true;
		}
	}
	return false;
}

bool GenerationContext::add_generated_bone(Name const & _boneName, ElementInstance & _instanceInstigator, OPTIONAL_ OUT_ int * _boneIdx)
{
	for_every(bone, bones)
	{
		if (bone->boneName == _boneName)
		{
			assign_optional_out_param(_boneIdx, NONE);
			error_generating_mesh(_instanceInstigator, TXT("bone \"%S\" already added"), _boneName.to_char());
			return false;
		}
	}
	Bone bone;
	bone.boneName = _boneName;
	bone.parentName = get_current_parent_bone();
	bones.push_back(bone);
	assign_optional_out_param(_boneIdx, bones.get_size() - 1);
	return true;
}

Bone const * GenerationContext::get_generated_bone(Name const & _boneName) const
{
	if (!bones.is_empty())
	{
		for_every(bone, bones)
		{
			if (bone->boneName == _boneName)
			{
				return bone;
			}
		}
		return nullptr;
	}
	return nullptr;
}

#ifdef AN_DEVELOPMENT
int GenerationContext::add_performance_for_element(Element const * _element)
{
	performanceForElements.grow_size(1);
	auto & pfe = performanceForElements.get_last();
	pfe.depth = performanceForElementsDepth;
	pfe.element = _element;
	pfe.timeProcess = 0.0f;
	pfe.timeTotal = 0.0f;
	++performanceForElementsDepth;

	return performanceForElements.get_size() - 1;
}

void GenerationContext::end_performance_for_element()
{
	--performanceForElementsDepth;
	an_assert(performanceForElementsDepth >= 0);
}

String GenerationContext::get_performance_for_elements() const
{
	String result;
	result += TXT(">>\n");
	for_every(pfe, performanceForElements)
	{
		result += String::printf(TXT("[%9.3fms]-[%9.3fms]"), pfe->timeTotal * 1000.0f, pfe->timeProcess * 1000.0f);
		for_count(int, i, pfe->depth)
		{
			result += String::space();
		}
		result += pfe->element->get_location_info();
		result += TXT("\n");
	}
	result += TXT(".");

	return result;
}
#endif

int GenerationContext::find_socket(Name const& _socketName) const
{
	return get_sockets().find_socket_index(_socketName);
}

Transform GenerationContext::calculate_socket_placement_ms(int _idx) const
{
	Transform result = Transform::identity;

	auto const & socket = get_sockets().get_sockets()[_idx];
	if (socket.get_relative_placement().is_set())
	{
		Transform relativePlacement = socket.get_relative_placement().get();
		int boneIdx = socket.get_bone_name().is_valid() ? find_bone_index(socket.get_bone_name()) : NONE;
		if (boneIdx != NONE)
		{
			return get_bone_placement_MS(boneIdx).to_world(relativePlacement);
		}
		else
		{
			return relativePlacement;
		}
	}
	if (socket.get_placement_MS().is_set())
	{
		return socket.get_placement_MS().get();
	}
	// if anything else fails, just try bone placement (if attached to a bone)
	if (socket.get_bone_name().is_valid())
	{
		int boneIdx = find_bone_index(socket.get_bone_name());
		if (boneIdx != NONE)
		{
			return get_bone_placement_MS(boneIdx);
		}
	}
	return Transform::identity;
}

String const GenerationContext::get_include_stack_as_string() const
{
	String result;
	for_every(is, includeStack)
	{
		if (!result.is_empty())
		{
			result += ':';
		}
		result += is->to_string();
	}
	return result;
}

void GenerationContext::log(LogInfoContext& _log) const
{
	{
		_log.log(TXT("parts"));
		LOG_INDENT(_log);
		for_every_ref(part, parts)
		{
			_log.log(TXT("%02i : [0x%p] v:%05i e:%05i t:%05i vs:%2i es:%2i"),
				for_everys_index(part), part, part->get_number_of_vertices(), part->get_number_of_elements(), part->get_number_of_tris(),
				part->get_vertex_format().get_stride(), part->get_index_format().get_element_size() != System::IndexElementSize::None ? part->get_index_format().get_stride() : 0);
		}
	}
	{
		_log.log(TXT("parts by material"));
		LOG_INDENT(_log);
		for_every(pbm, partsByMaterials)
		{
			_log.log(TXT("material: %i"), for_everys_index(pbm));
			LOG_INDENT(_log);
			for_every(fusePart, *pbm)
			{
				auto* part = fusePart->part.get();
				_log.log(TXT("%02i : [0x%p] v:%05i e:%05i t:%05i vs:%2i es:%2i"),
					for_everys_index(fusePart), part, part->get_number_of_vertices(), part->get_number_of_elements(), part->get_number_of_tris(),
					part->get_vertex_format().get_stride(), part->get_index_format().get_element_size() != System::IndexElementSize::None ? part->get_index_format().get_stride() : 0);
			}
		}
	}
}

NamedCheckpoint& GenerationContext::access_named_checkpoint(Name const& _id)
{
	for_every(c, namedCheckpoints)
	{
		if (c->id == _id)
		{
			return *c;
		}
	}

	namedCheckpoints.grow_size(1);
	auto& c = namedCheckpoints.get_last();
	c.id = _id;
	return c;
}
