#include "modelInstanceSet.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Collision;

ModelInstanceSet::~ModelInstanceSet()
{
	clear();
}

void ModelInstanceSet::hard_copy_from(ModelInstanceSet const & _instanceSet)
{
	clear();

	instances.set_size(_instanceSet.instances.get_size());

	ModelInstance* destInstance = instances.get_data();
	for_every_const(instance, _instanceSet.instances)
	{
		destInstance->hard_copy_from(*instance);
		++destInstance;
	}

	boundingBox = _instanceSet.boundingBox;
#ifdef AN_DEVELOPMENT
	boundingBoxValid = _instanceSet.boundingBoxValid;
#endif
}

void ModelInstanceSet::clear()
{
	instances.clear();
	boundingBox = Range3::empty;
#ifdef AN_DEVELOPMENT
	boundingBoxValid = true;
#endif
}

ModelInstance* ModelInstanceSet::add(Model const * _model, Transform const & _placement)
{
#ifdef AN_DEVELOPMENT
	boundingBoxValid = false;
#endif
	instances.push_back(ModelInstance(_model, _placement, instanceId));
	++instanceId;
	an_assert(instanceId != NONE, TXT("we shouldn't get here"));
	return &instances.get_last();
}

void ModelInstanceSet::remove_by_id(int _id)
{
	for_every(instance, instances)
	{
		if (instance->get_id_within_set() == _id)
		{
			instances.remove_at(for_everys_index(instance));
			break;
		}
	}
}

ModelInstance const* ModelInstanceSet::get_instance_by_id(int _id) const
{
	for_every(instance, instances)
	{
		if (instance->get_id_within_set() == _id)
		{
			return instance;

		}
	}
	return nullptr;
}

bool ModelInstanceSet::check_segment(REF_ Segment & _segment, REF_ Collision::CheckSegmentResult & _result, Collision::CheckCollisionContext & _context, float _increaseSize) const
{
	if (instances.get_size() > 1)
	{
#ifdef AN_DEVELOPMENT
		an_assert(boundingBoxValid, TXT("did you forget to update bounding box?"));
#endif
		if (!boundingBox.overlaps(_segment.get_bounding_box()))
		{
			// we're too far
			return false;
		}
	}

	bool ret = false;
	for_every_const(instance, instances)
	{
		if (instance->check_segment(REF_ _segment, REF_ _result, _context, _increaseSize))
		{
			ret = true;
		}
	}
	return ret;
}

void ModelInstanceSet::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef) const
{
	for_every_const(instance, instances)
	{
		instance->debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, _poseMatBonesRef);
	}
}

void ModelInstanceSet::log(LogInfoContext& _log) const
{
	for_every_const(instance, instances)
	{
		_log.log(TXT("instance %i"), for_everys_index(instance));
		LOG_INDENT(_log);
		instance->log(_log);
	}
}

bool ModelInstanceSet::update_bounding_box(Meshes::Pose const * _poseMS)
{
	auto prevBoundingBox = boundingBox;
	boundingBox = Range3::empty;
	for_every(instance, instances)
	{
		instance->update_bounding_box(_poseMS);
		boundingBox.include(instance->get_bounding_box());
	}
#ifdef AN_DEVELOPMENT
	boundingBoxValid = true;
#endif
	return prevBoundingBox != boundingBox;
}

void ModelInstanceSet::update_bounding_box_placement_only()
{
	boundingBox = Range3::empty;
	for_every(instance, instances)
	{
		instance->update_bounding_box_placement_only();
		boundingBox.include(instance->get_bounding_box());
	}
#ifdef AN_DEVELOPMENT
	boundingBoxValid = true;
#endif
}
