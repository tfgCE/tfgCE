#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

template <typename Element> class ArrayStack;

namespace Collision
{
	struct CheckCollisionContext;
};

namespace Meshes
{
	class Mesh3D;
	class Skeleton;
	struct Mesh3DPart;
	struct VertexSkinningInfo;
	namespace Builders
	{
		class IPU;
	};
};

/*
 *	Should be used only for collision, for precise collision models
 */
struct SkinnedMesh
{
protected:
	struct Triangle;
	
public:
	SkinnedMesh();
	SkinnedMesh(SkinnedMesh const & _other);
	~SkinnedMesh();

	SkinnedMesh const & operator=(SkinnedMesh const & _other);

	void clear();

	bool is_empty() const { return triangles.is_empty(); }

	void add(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, Meshes::VertexSkinningInfo const & _aSkinning, Meshes::VertexSkinningInfo const & _bSkinning, Meshes::VertexSkinningInfo const & _cSkinning);
	void build(); // range
	void copy_from(SkinnedMesh const & _other);

	void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Array<Matrix44> const & _defaultInvertedBones, Array<Matrix44> const & _bones) const;
	void log(LogInfoContext & _context) const;

	bool check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, Array<Matrix44> const & _defaultInvertedBones, Array<Matrix44> const & _bones, float _increaseSize = 0.0f) const;

	void reverse_triangles();
	void apply_transform(Matrix44 const & _transform);

	Range3 calculate_bounding_box(Transform const & _usingPlacement, Array<Transform> const & _defaultInvertedBones, Array<Transform> const & _bones, bool _quick) const;

	void create_from(::Meshes::Builders::IPU const & _ipu, int _startingAtPolygon = 0, int _polygonCount = NONE);
	void create_from(::Meshes::Mesh3D const * _mesh, OUT_ OPTIONAL_ int * _skinToBoneIdx = nullptr);
	void create_from(::Meshes::Mesh3DPart const * _meshPart, OUT_ OPTIONAL_ int * _skinToBoneIdx = nullptr);

	bool update_for(Meshes::Skeleton const * _skeleton);

	Vector3 get_centre() const { return range.centre(); }
	Range3 const& get_range() const { return range; }

	void split(Plane const& _plane, SkinnedMesh& _behindIntoMesh, float _threshold = 0.0001f);

public:
	bool load_from_xml(IO::XML::Node const * _node);

protected:
	struct Point
	{
		static int const MAX_BONE = 4;
		static int get_MAX_BONE() { return MAX_BONE; }
		Name bones[MAX_BONE];
		int boneIndices[MAX_BONE];
		float weights[MAX_BONE];
		Vector3 location = Vector3::zero;

		Point();
		Point & setup_with(Vector3 const & _loc, Meshes::VertexSkinningInfo const & _skinningInfo);
		bool update_for(Meshes::Skeleton const * _skeleton);
		Vector3 calculate_location(Array<Matrix44> const & _defaultInvertedBones, Array<Matrix44> const & _bones) const;
		Vector3 calculate_location(Array<Transform> const & _defaultInvertedBones, Array<Transform> const & _bones) const;

		Meshes::VertexSkinningInfo to_skinning_info() const;

		bool operator==(Point const & _other) const;
	};
	struct Triangle
	{
		int a, b, c;
	};

	Array<Triangle> triangles;
	Array<Point> points;
	CACHED_ Range3 range;

	int add(Point const & _point);

	void add(Point const& _a, Point const& _b, Point const& _c);

#ifdef AN_DEVELOPMENT
	mutable bool highlightDebugDraw = false;
#endif
};
