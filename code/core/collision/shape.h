#pragma once

#include "..\math\math.h"

#include "collisionInfoProvider.h"

#include "iCollidableShape.h"

#include "physicalMaterial.h"

// inline

#include "..\mesh\skeleton.h"
#include "..\mesh\pose.h"
#include "..\debug\debugRenderer.h"
#include "..\system\core.h"

namespace Collision
{
	class Model;
	struct CheckSegmentResult;
	struct CheckCollisionContext;
	struct LoadingContext;

	namespace ShapeAgainstPlanes
	{
		enum Type
		{
			Inside,
			Intersecting,
			Outside
		};
	};

	// all shapes should be convex
	template <typename Primitive>
	class Shape
	: public Primitive
	, public ICollidableShape
	{
	public:
		Shape();
		Shape(Shape const & _source);
		~Shape();

		Shape& operator = (Shape const & _source);

		template <typename OtherShape>
		void setup_as(OtherShape const & _shape);

		void use_material(PhysicalMaterial* _material, Optional<Collision::Flags> const & _materialCollisionFlagsOverride = NP);
		void refresh_material(); // update collision flags;

		Optional<Collision::Flags> const & get_material_collision_flags() const { return materialCollisionFlags; }

		CollisionInfoProvider const & get_collision_info_provider() const { return collisionInfoProvider; }

		void set_bone(Name const & _boneName);
		Name const & get_bone() const { return bone; }
		inline int get_bone_idx(Meshes::Skeleton const * _skeleton) const;

		void log(LogInfoContext & _context) const;

	public: // ICollidableShape
		implement_ PhysicalMaterial const * get_material() const { return material.get(); }
		implement_ void set_collidable_shape_bone(Name const & _boneName) { set_bone(_boneName); }
		implement_ Name const & get_collidable_shape_bone() const { return get_bone(); }
		implement_ int get_collidable_shape_bone_index(Meshes::Skeleton const * _skeleton) const { return get_bone_idx(_skeleton); }

	public:
		bool load_from_xml(IO::XML::Node const * _node, LoadingContext const & _context);

	public:
		// ShapeAgainstPlanes::Type check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const = 0;

		// GradientQueryResult get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square = false) const = 0;

		// TODO change this and:
		//	add poralization
		//	gravity direction (using normal, using predefined value)

	public: // same case as below, might be unsafe, what to do? :/
		void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef = Meshes::PoseMatBonesRef()) const;
		Range3 calculate_bounding_box(Transform const & _usingPlacement, Meshes::Pose const * _poseMS, bool _quick) const;

	protected: friend class Model; // only model may call this as we override_ this method in derived classes
		void clear_cache();
		bool cache_for(Meshes::Skeleton const * _skeleton);

		bool check_segment(REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context, float _increaseSize = 0.0f) const;
		Vector3 from_external_to_model(Vector3 const& _loc, CheckCollisionContext& _context) const; // through bones

	protected:
		Name bone;
		CACHED_ int boneIdx = NONE;
		CollisionInfoProvider collisionInfoProvider;
		RefCountObjectPtr<PhysicalMaterial> material;
		Optional<Collision::Flags> materialCollisionFlagsOverride;
		CACHED_ Optional<Collision::Flags> materialCollisionFlags;

		// todo - array>   ??
		CACHED_ mutable Optional<Meshes::Skeleton const *> cachedForSkeleton;
	};

};

#include "shape.inl"

