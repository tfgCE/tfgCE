#include "modelInstance.h"
#include "model.h"

#include "checkSegmentResult.h"

#include "..\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Collision;

ModelInstance::ModelInstance()
: model(nullptr)
{
	set_placement(Transform::identity);
}

ModelInstance::ModelInstance(Model const * _model, Transform const & _placement, int _collisionIdWithinSet)
: model(nullptr)
, idWithinSet(_collisionIdWithinSet)
{
	set_model(_model);
	set_placement(_placement);
}

void ModelInstance::hard_copy_from(ModelInstance const & _instance)
{
	model = _instance.model;
	placement = _instance.placement;
	placementMat = _instance.placementMat;
	boundingBox = _instance.boundingBox;
#ifdef AN_DEVELOPMENT
	boundingBoxValid = _instance.boundingBoxValid;
#endif
}

bool ModelInstance::check_segment(REF_ Segment & _segment, REF_ Collision::CheckSegmentResult & _result, Collision::CheckCollisionContext & _context, float _increaseSize) const
{
#ifdef AN_DEVELOPMENT
	an_assert(boundingBoxValid, TXT("did you forget to update bounding box?"));
#endif
	if (!boundingBox.overlaps(_segment.get_bounding_box()))
	{
		// we're too far
		return false;
	}
	todo_note(TXT("for long segments check if segment hits?"));

	bool ret = false;
	if (model)
	{
		Segment localRay = Segment(placement.to_local(_segment));
		if (model->check_segment(REF_ localRay, REF_ _result, _context, _increaseSize))
		{
			_segment.copy_t_from(localRay);
			_result.to_world_of(placement);
			ret = true;
		}
	}
	return ret;
}

void ModelInstance::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef) const
{
	debug_push_transform(placement);
	model->debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, _poseMatBonesRef);
	debug_pop_transform();
}

void ModelInstance::log(LogInfoContext& _log) const
{
	_log.log(TXT("translation %S"), placement.get_translation().to_string().to_char());
	_log.log(TXT("orientation %S"), placement.get_orientation().to_rotator().to_string().to_char());
	_log.log(TXT("scale %.3f"), placement.get_scale());
	_log.log(TXT("model %p"), model);
	if (model)
	{
		LOG_INDENT(_log);
		model->log(_log);
	}
}

void ModelInstance::update_bounding_box(Meshes::Pose const * _poseMS)
{
	if (model)
	{
		// use identity to make it update faster (using cached ranges)
		boundingBoxForParticularPlacement = model->calculate_bounding_box(placement, _poseMS, true);
	}
	else
	{
		boundingBoxForParticularPlacement = Range3::empty;
	}
	particularPlacementOfBoundingBox = placement;
#ifdef AN_DEVELOPMENT
	boundingBoxForParticularPlacementValid = true;
#endif
	update_bounding_box_placement_only();
}

void ModelInstance::update_bounding_box_placement_only()
{
#ifdef AN_DEVELOPMENT
	an_assert(boundingBoxForParticularPlacementValid);
#endif
	boundingBox.construct_from_placement_and_range3(particularPlacementOfBoundingBox.to_local(placement), boundingBoxForParticularPlacement);
#ifdef AN_DEVELOPMENT
	boundingBoxValid = true;
#endif
}

void ModelInstance::set_placement(Transform const & _placement)
{
	placement = _placement;
	placementMat = _placement.to_matrix();
#ifdef AN_DEVELOPMENT
	boundingBoxValid = false;
#endif
}

bool ModelInstance::get_closest_shape(Collision::CheckCollisionContext& _context, Vector3 const& _loc, REF_ Optional<float>& _closestDist, REF_ ICollidableShape const*& _closestShape) const
{
	if (model)
	{
		Vector3 loc = placement.location_to_local(_loc);
		return model->get_closest_shape(_context, loc, _closestDist, _closestShape);
	}
	return false;
}
