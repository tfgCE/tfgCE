#pragma once

#include "..\math\math.h"

#include "..\mesh\pose.h"

namespace Collision
{
	class Model;
	class ModelInstanceSet;

	interface_class ICollidableShape;
		
	struct CheckSegmentResult;
	struct CheckCollisionContext;

	class ModelInstance
	{
	public:
		ModelInstance();
		ModelInstance(Model const * _model, Transform const & _placement, int _collisionIdWithinSet = NONE);

		void hard_copy_from(ModelInstance const & _instance);

		int get_id_within_set() const { return idWithinSet; }

		void set_model(Model const * _model) { model = _model; }
		Model const * get_model() const { return model; }

		void set_placement(Transform const & _placement);
		Transform const & get_placement() const { return placement; }
		Matrix44 const & get_placement_matrix() const { return placementMat; }

		Range3 const & get_bounding_box() const {
#ifdef AN_DEVELOPMENT
			an_assert(boundingBoxValid, TXT("did you forget to update bounding box?"));
#endif
			return boundingBox; }

		bool check_segment(REF_ Segment & _segment, REF_ Collision::CheckSegmentResult & _result, Collision::CheckCollisionContext & _context, float _increaseSize = 0.0f) const;
		bool get_closest_shape(Collision::CheckCollisionContext & _context, Vector3 const & _loc, REF_ Optional<float> & _closestDist, REF_ ICollidableShape const*& _closestShape) const;

		void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef = Meshes::PoseMatBonesRef()) const;
		void log(LogInfoContext& _log) const;

	private: friend class ModelInstanceSet;
		void update_bounding_box(Meshes::Pose const * _poseMS = nullptr);
		void update_bounding_box_placement_only();

	private:
		Model const * model;
		Transform placement;
		CACHED_ Matrix44 placementMat;
		int idWithinSet;
		Transform particularPlacementOfBoundingBox = Transform::identity;
		Range3 boundingBoxForParticularPlacement = Range3::empty; // we store non placed bounding box - we will use it for bounding box calculation - in most cases we should not get much bigger one
		Range3 boundingBox = Range3::empty;
#ifdef AN_DEVELOPMENT
		bool boundingBoxValid = false;
		bool boundingBoxForParticularPlacementValid = false;
#endif
	};

};
