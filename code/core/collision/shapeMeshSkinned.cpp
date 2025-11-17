#include "shapeMeshSkinned.h"

#include "checkCollisionContext.h"
#include "checkSegmentResult.h"
#include "gradientQueryResult.h"
#include "queryPrimitivePoint.h"
#include "shapeConvexHull.h"

using namespace Collision;

GradientQueryResult Collision::MeshSkinned::get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square) const
{
	GradientQueryResult result(_maxDistance, Vector3::zero);
	error_stop(TXT("do not use mesh skinned for gradients!"));
	return result;
}

ShapeAgainstPlanes::Type Collision::MeshSkinned::check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const
{
	ShapeAgainstPlanes::Type result = ShapeAgainstPlanes::Inside;
	error_stop(TXT("do not use mesh skinned with check against planes!"));
	return result;
}

bool Collision::MeshSkinned::check_segment(REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context, float _increaseSize) const
{
	Vector3 hitNormal;
	if ((!material.is_set() || Flags::check(_context.get_collision_flags(), material->get_collision_flags())) &&
		_context.get_pose_mat_bones_ref().is_set() && _context.get_pose_mat_bones_ref().get_skeleton() &&
		SkinnedMesh::check_segment(REF_ _segment, REF_ hitNormal, &_context, _context.get_pose_mat_bones_ref().get_skeleton()->get_bones_default_inverted_matrix_MS(), _context.get_pose_mat_bones_ref().get_bones(), _increaseSize))
	{
		_result.hit = true;
		_result.hitLocation = _segment.get_hit();
		_result.hitNormal = hitNormal;
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

bool Collision::MeshSkinned::cache_for(Meshes::Skeleton const * _skeleton)
{
	bool result = Shape<::SkinnedMesh>::cache_for(_skeleton);

	result &= ::SkinnedMesh::update_for(_skeleton);

	return result;
}

void Collision::MeshSkinned::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef) const
{
	if (_poseMatBonesRef.is_set() && _poseMatBonesRef.get_skeleton())
	{
		::SkinnedMesh::debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, _poseMatBonesRef.get_skeleton()->get_bones_default_inverted_matrix_MS(), _poseMatBonesRef.get_bones());
	}
	else
	{
		::SkinnedMesh::debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, Array<Matrix44>(), Array<Matrix44>());
	}
}

Range3 Collision::MeshSkinned::calculate_bounding_box(Transform const & _usingPlacement, Meshes::Pose const * _poseMS, bool _quick) const
{
	if (_poseMS)
	{
		return ::SkinnedMesh::calculate_bounding_box(_usingPlacement, _poseMS->get_skeleton()->get_bones_default_inverted_placement_MS(), _poseMS->get_bones(), _quick);
	}
	else
	{
		return Range3::empty;
	}
}
