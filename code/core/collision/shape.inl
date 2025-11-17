#ifdef AN_CLANG
#include "checkCollisionContext.h"
#include "checkSegmentResult.h"
#endif

template <typename Primitive>
::Collision::Shape<Primitive>::Shape()
: material(nullptr)
{
}

template <typename Primitive>
::Collision::Shape<Primitive>::Shape(Shape const & _source)
: Primitive(_source)
, bone(_source.bone)
, boneIdx(_source.boneIdx)
, material(_source.material)
, materialCollisionFlags(_source.materialCollisionFlags)
, cachedForSkeleton(_source.cachedForSkeleton)
{
}

template <typename Primitive>
::Collision::Shape<Primitive>::~Shape()
{
}

template <typename Primitive>
::Collision::Shape<Primitive>& ::Collision::Shape<Primitive>::operator =(Shape const & _source)
{
	Primitive::operator=(_source);
	bone = _source.bone;
	boneIdx = _source.boneIdx;
	material = _source.material;
	materialCollisionFlags = _source.materialCollisionFlags;
	cachedForSkeleton = _source.cachedForSkeleton;
	return *this;
}

template <typename Primitive>
void ::Collision::Shape<Primitive>::use_material(PhysicalMaterial* _material, Optional<::Collision::Flags> const & _materialCollisionFlagsOverride)
{
	material = _material;
	materialCollisionFlagsOverride = _materialCollisionFlagsOverride;
	refresh_material();
}

template <typename Primitive>
void ::Collision::Shape<Primitive>::refresh_material()
{
	if (materialCollisionFlagsOverride.is_set())
	{
		materialCollisionFlags = materialCollisionFlagsOverride;
	}
	else if (material.is_set() && ! material->get_collision_flags().is_empty()) // if nothing set, ignore them
	{
		materialCollisionFlags = material->get_collision_flags();
	}
	else
	{
		materialCollisionFlags.clear();
	}
}

template <typename Primitive>
bool ::Collision::Shape<Primitive>::check_segment(REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context, float _increaseSize) const
{
	Vector3 hitNormal;
	Segment segment = _segment;
	Matrix44 applyTransform = Matrix44::identity;
	if (bone.is_valid() && _context.get_pose_mat_bones_ref().is_set() && boneIdx != NONE)
	{
#ifdef AN_DEVELOPMENT
		an_assert(cachedForSkeleton.is_set());
		an_assert(_context.get_pose_mat_bones_ref().get_skeleton() == cachedForSkeleton.get() || Meshes::Skeleton::are_compatible(_context.get_pose_mat_bones_ref().get_skeleton(), cachedForSkeleton.get()));
		an_assert(boneIdx < _context.get_pose_mat_bones_ref().get_skeleton()->get_num_bones());
		an_assert(_context.get_pose_mat_bones_ref().get_bones().is_index_valid(boneIdx));
#endif
		auto const & boneTransform = _context.get_pose_mat_bones_ref().get_bones()[boneIdx];
		if (boneTransform.has_at_least_one_zero_scale())
		{
			// skip this one
			return false;
		}
		applyTransform = boneTransform.to_world(_context.get_pose_mat_bones_ref().get_skeleton()->get_bones_default_inverted_matrix_MS()[boneIdx]);
		segment = Segment(applyTransform.to_local(segment));
	}
	if ((!material.is_set() || Flags::check(_context.get_collision_flags(), material->get_collision_flags())) &&
		Primitive::check_segment(REF_ segment, REF_ hitNormal, &_context, _increaseSize))
	{
		_segment.copy_t_from(segment);
		_result.hit = true;
		_result.hitLocation = _segment.get_hit();
		_result.hitNormal = applyTransform.vector_to_world(hitNormal);
		_result.gravityDir.clear();
		_result.gravityForce.clear();
		_result.shape = this;
		_result.object = nullptr;
		_result.material = material.get();
		// apply params to result
		if (_context.is_collision_info_needed())
		{
			collisionInfoProvider.apply_to(_result);
			if (material.is_set())
			{
				material->apply_to(REF_ _result);
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

template <typename Primitive>
Vector3 Collision::Shape<Primitive>::from_external_to_model(Vector3 const& _loc, CheckCollisionContext& _context) const
{
	if (bone.is_valid() && _context.get_pose_mat_bones_ref().is_set() && boneIdx != NONE)
	{
#ifdef AN_DEVELOPMENT
		an_assert(cachedForSkeleton.is_set());
		an_assert(_context.get_pose_mat_bones_ref().get_skeleton() == cachedForSkeleton.get() || Meshes::Skeleton::are_compatible(_context.get_pose_mat_bones_ref().get_skeleton(), cachedForSkeleton.get()));
		an_assert(boneIdx < _context.get_pose_mat_bones_ref().get_skeleton()->get_num_bones());
		an_assert(_context.get_pose_mat_bones_ref().get_bones().is_index_valid(boneIdx));
#endif
		auto const& boneTransform = _context.get_pose_mat_bones_ref().get_bones()[boneIdx];
		if (boneTransform.has_at_least_one_zero_scale())
		{
			// skip this one
			return Vector3::one * 10000.0f; // some bizarly distant point
		}
		Matrix44 applyTransform = boneTransform.to_world(_context.get_pose_mat_bones_ref().get_skeleton()->get_bones_default_inverted_matrix_MS()[boneIdx]);
		return applyTransform.location_to_local(_loc);
	}
	return _loc;
}

template <typename Primitive>
bool ::Collision::Shape<Primitive>::load_from_xml(IO::XML::Node const * _node, LoadingContext const & _context)
{
	bool result = Primitive::load_from_xml(_node);
	result &= PhysicalMaterial::load_material_from_xml(material, _node, _context);
	use_material(material.get());
	result &= collisionInfoProvider.load_from_xml(_node);
	bone = _node->get_name_attribute_or_from_child(TXT("bone"), bone);
	return result;
}

template <typename Primitive>
template <typename OtherShape>
void ::Collision::Shape<Primitive>::setup_as(OtherShape const & _shape)
{
	collisionInfoProvider = _shape.get_collision_info_provider();
	use_material(cast_to_nonconst(_shape.get_material()));
	materialCollisionFlags = _shape.get_material_collision_flags();
}

template <typename Primitive>
void ::Collision::Shape<Primitive>::set_bone(Name const & _boneName)
{
	bone = _boneName;
	cachedForSkeleton.clear();
}

template <typename Primitive>
bool ::Collision::Shape<Primitive>::cache_for(Meshes::Skeleton const * _skeleton)
{
	cachedForSkeleton = _skeleton;
	if (bone.is_valid())
	{
		if (_skeleton)
		{
			boneIdx = _skeleton->find_bone_index(bone);
			if (boneIdx == NONE)
			{
				error(TXT("could not find bone \"%S\" for collision shape"), bone.to_char());
			}
			return boneIdx != NONE;
		}
		else
		{
			boneIdx = NONE;
		}
	}
	return true;
}

template <typename Primitive>
void ::Collision::Shape<Primitive>::clear_cache()
{
	cachedForSkeleton.clear();
	boneIdx = NONE;
}

template <typename Primitive>
Range3 Collision::Shape<Primitive>::calculate_bounding_box(Transform const & _usingPlacement, Meshes::Pose const * _poseMS, bool _quick) const
{
	Transform usingPlacement = _usingPlacement;
	if (bone.is_valid() && boneIdx != NONE && _poseMS)
	{
#ifdef AN_DEVELOPMENT
		an_assert(cachedForSkeleton.is_set());
		an_assert(_poseMS->get_skeleton() == cachedForSkeleton.get() || Meshes::Skeleton::are_compatible(_poseMS->get_skeleton(), cachedForSkeleton.get()));
		an_assert(boneIdx < _poseMS->get_num_bones());
		an_assert(_poseMS->get_bones().is_index_valid(boneIdx));
#endif
		Transform applyTransform = _poseMS->get_bone(boneIdx).to_world(_poseMS->get_skeleton()->get_bones_default_inverted_placement_MS()[boneIdx]);
		usingPlacement = usingPlacement.to_world(applyTransform);
	}
	return Primitive::calculate_bounding_box(usingPlacement, _quick);
}

template <typename Primitive>
void ::Collision::Shape<Primitive>::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef) const
{
	if (boneIdx != NONE && _poseMatBonesRef.is_set())
	{
		if (_poseMatBonesRef.get_bones()[boneIdx].has_at_least_one_zero_scale())
		{
			return;
		}
		debug_push_transform(_poseMatBonesRef.get_bones()[boneIdx].to_world(_poseMatBonesRef.get_skeleton()->get_bones_default_inverted_matrix_MS()[boneIdx]).to_transform());
	}
	Colour colour = _colour;
	if (!get_material())
	{
		float osc = 0.5f + 0.5f * sin_deg(::System::Core::get_timer() * 20.0f);
		colour *= 0.3f + 0.5f * osc;
		colour.a = 0.3f + 0.5f * osc;
	}
	Primitive::debug_draw(_frontBorder, _frontFill, colour, _alphaFill);
	if (boneIdx != NONE && _poseMatBonesRef.is_set())
	{
		debug_pop_transform();
	}
}

template <typename Primitive>
int ::Collision::Shape<Primitive>::get_bone_idx(Meshes::Skeleton const * _skeleton) const
{
	if (!bone.is_valid())
	{
		return NONE;
	}
	an_assert(cachedForSkeleton.is_set());
	if (cachedForSkeleton.get() == _skeleton)
	{
		return boneIdx;
	}
	else if (Meshes::Skeleton::are_compatible(cachedForSkeleton.get(), _skeleton))
	{
		cachedForSkeleton = _skeleton;
		return boneIdx;
	}
	else
	{
		an_assert(!_skeleton, TXT("implement caching for multiple skeletons?"));
		return NONE;
	}
}

template <typename Primitive>
void ::Collision::Shape<Primitive>::log(LogInfoContext & _context) const
{
	Primitive::log(_context);
	LOG_INDENT(_context);
	if (material.is_set())
	{
		material->log_usage_info(_context);
	}
	if (bone.is_valid())
	{
		_context.log(TXT("+--> %S"), bone.to_char());
	}
}
