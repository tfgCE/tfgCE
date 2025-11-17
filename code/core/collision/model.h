#pragma once

#include "shapeBox.h"
#include "shapeConvexHull.h"
#include "shapeSphere.h"
#include "shapeCapsule.h"
#include "shapeMesh.h"
#include "shapeMeshSkinned.h"

#include "loadingContext.h"

#include "..\mesh\boneRenameFunc.h"

#include <functional>

struct Colour;

namespace Meshes
{
	struct Pose;
};

namespace Collision
{
	struct CheckSegmentResult;

	class Model
	{
	public:
		Model();
		~Model();

		Model* create_copy() const;

		void clear();

		struct FuseModel
		{
			Model const* model;
			Optional<Transform> placement;
			FuseModel() {}
			explicit FuseModel(Model const* _model, Optional<Transform> const& _placement = NP) : model(_model), placement(_placement) {}
		};
		void fuse(Array<Model const *> const & _models);
		void fuse(Array<Model *> const & _models);
		void fuse(Model const * const * _models, int _count);
		void fuse(Array<FuseModel> const & _models);
		void fuse(Model const * _model, Optional<Transform> const & _placement = NP);

		void reverse_triangles();
		void apply_transform(Matrix44 const & _transform);
		void skin_to(Name const & _boneName, bool _onlyIfNotSkinned = false);
		void reskin(Meshes::BoneRenameFunc);
		void remove_at_or_behind(Plane const & _plane, float threshold = 0.0001f);

		void split_meshes_at(Plane const& _plane, Optional<Range3> const& _inRange = NP, float _threshold = 0.0001f);
		void convexify_static_meshes();

		void add(Box const & _box) { boxes.push_back(_box); }
		void add(ConvexHull const & _hull) { hulls.push_back(_hull); }
		void add(Sphere const & _sphere) { spheres.push_back(_sphere); }
		void add(Capsule const & _capsule) { capsules.push_back(_capsule); }
		void add(Mesh const & _mesh) { meshes.push_back(_mesh); }
		void add(MeshSkinned const & _meshSkinned) { skinnedMeshes.push_back(_meshSkinned); }

		bool is_empty() const { return boxes.is_empty() && hulls.is_empty() && spheres.is_empty() && capsules.is_empty() && meshes.is_empty() && skinnedMeshes.is_empty(); }

		Array<Box> const & get_boxes() const { return boxes; }
		Array<ConvexHull> const & get_hulls() const { return hulls; }
		Array<Sphere> const & get_spheres() const { return spheres; }
		Array<Capsule> const & get_capsules() const { return capsules; }
		Array<Mesh> const & get_meshes() const { return meshes; }
		Array<MeshSkinned> const & get_skinned_meshes() const { return skinnedMeshes; }

		void break_meshes_into_convex_hulls();
		void break_meshes_into_convex_meshes(); // will keep meshes, will just break them into convex patches

		void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef = Meshes::PoseMatBonesRef()) const;
		void log(LogInfoContext & _context) const;

		bool check_segment(REF_ Segment & _segment, REF_ Collision::CheckSegmentResult & _result, Collision::CheckCollisionContext & _context, float _increaseSize = 0.0f) const;
		bool get_closest_shape(Collision::CheckCollisionContext& _context, Vector3 const& _loc, REF_ Optional<float>& _closestDist, REF_ ICollidableShape const*& _closestShape) const;

		void for_every_primitive(std::function<void(ICollidableShape*)> _do);

		PhysicalMaterial const * get_material() const { return material.get(); }

		void refresh_materials();

		void cache_for(Meshes::Skeleton const * _skeleton);
		void clear_cache();

		Range3 calculate_bounding_box(Transform const & _usingPlacement, Meshes::Pose const * _poseMS, bool _quick) const;

	public:
		bool load_from_xml(IO::XML::Node const * _node, LoadingContext const & _context);

	private:
		Array<Box> boxes;
		Array<ConvexHull> hulls;
		Array<Sphere> spheres;
		Array<Capsule> capsules;
		Array<Mesh> meshes;
		Array<MeshSkinned> skinnedMeshes;

		CollisionInfoProvider collisionInfoProvider;
		RefCountObjectPtr<PhysicalMaterial> material;

		template<typename Shape>
		bool try_loading_from_xml(IO::XML::Node const * _node, LoadingContext const & _context, Array<Shape>& _shapes, tchar const * _nodeName);
	};

};
