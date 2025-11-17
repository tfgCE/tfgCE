#pragma once

#include "modelInstance.h"

namespace Collision
{
	class ModelInstanceSet
	{
	public:
		~ModelInstanceSet();

		void hard_copy_from(ModelInstanceSet const & _instanceSet);

		void clear();
		ModelInstance* add(Model const * _model, Transform const & _placement = Transform::identity);
		void remove_by_id(int _id);

		bool is_empty() const { return instances.is_empty(); }
		int get_instances_num() const { return instances.get_size(); }
		ModelInstance* access_instance(int _index) { return instances.is_index_valid(_index) ? &instances[_index] : nullptr; }
		ModelInstance const* get_instance(int _index) const { return instances.is_index_valid(_index) ? &instances[_index] : nullptr; }
		ModelInstance const* get_instance_by_id(int _id) const;

		Array<ModelInstance> const & get_instances() const { return instances; }
		Array<ModelInstance> & access_instances() { return instances; }

		bool check_segment(REF_ Segment & _segment, REF_ Collision::CheckSegmentResult & _result, Collision::CheckCollisionContext & _context, float _increaseSize = 0.0f) const;

		bool update_bounding_box(Meshes::Pose const * _poseMS = nullptr); // returns true if something has changed
		void update_bounding_box_placement_only();

		Range3 const & get_bounding_box() const {
#ifdef AN_DEVELOPMENT
			an_assert(boundingBoxValid, TXT("did you forget to update bounding box?"));
#endif
			return boundingBox;
		}

		void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef = Meshes::PoseMatBonesRef()) const;
		void log(LogInfoContext& _log) const;

	private:
		Array<ModelInstance> instances;
		int instanceId = 0;

		Range3 boundingBox = Range3::empty;
#ifdef AN_DEVELOPMENT
		bool boundingBoxValid = false;
#endif

		ModelInstanceSet const & operator = (ModelInstanceSet const & _other); // do not implement
	};

};
