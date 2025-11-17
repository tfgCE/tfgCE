#pragma once

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\mesh\mesh3d.h"

namespace Framework
{
	namespace DoorSide
	{
		enum Type
		{
			A,
			B,
			Any
		};
	};

	namespace DoorHoleType
	{
		enum Type
		{
			Symmetrical,
			TwoSided			
		};

		Type parse(String const & _string);
	};

	/**
	 *	All edges and points are stored clockwise in outbound matrix (that is true for both sides)
	 *	Outbound - as seen from room
	 *	Mesh is stored as outbound matrix - because we render stencil while entering room
	 */
	class DoorHole
	{
	public:
		DoorHole();
		~DoorHole();

		bool load_from_xml(IO::XML::Node const * _node);

		inline bool is_convex() const;
		inline bool is_symmetrical() const; // x axis

		inline bool is_point_inside(DoorSide::Type _side, Vector2 const & _scale, Vector3 const & _point, float _ext = 0.0f) const; // in door's local space (outbound)
		inline bool is_segment_inside(DoorSide::Type _side, Vector2 const & _scale, SegmentSimple const & _segment, float _ext = 0.0f) const; // in door's local space (outbound)
		inline Vector3 move_point_inside(DoorSide::Type _side, Vector2 const & _scale, Vector3 const & _point, float _moveInside = 0.0f) const; // in door's local space (outbound)
		inline Vector3 move_point_outside_to_edge(DoorSide::Type _side, Vector2 const & _scale, Vector3 const & _point) const; // in door's local space (outbound)

		Range3 calculate_flat_box(DoorSide::Type _side, Vector2 const& _scale, Matrix44 const& _at) const; // x,z is as y=1, but y is kept as it was, makes sense only for y > 0

		inline bool is_behind(DoorSide::Type _side, Plane const & _plane) const;

		void add_point(Vector2 const & LOCATION_ _point, Vector2 const & _customData); // x right, y up
		void make_symmetrical_if_needed();
		void build(); // build edges (planes) and mesh
		void scale_to_nominal_door_size_and_build();

		template <typename Class>
		void setup_frustum_view(DoorSide::Type _side, Vector2 const & _scale, ::System::ViewFrustum & _frustum, Class const & _modelViewMatrix, Class const & _doorInRoomOutboundMatrix) const;
		template <typename Class>
		void setup_frustum_placement(DoorSide::Type _side, Vector2 const & _scale, ::System::ViewFrustum & _frustum, Class const & _placementMatrix, Class const & _doorInRoomOutboundMatrix) const;

		Meshes::Mesh3D const * get_outbound_mesh(DoorSide::Type _side) const { return type == DoorHoleType::Symmetrical || _side == DoorSide::A ? outboundAMesh : outboundBMesh; }
		Vector3 get_centre(DoorSide::Type _side) const { return type == DoorHoleType::Symmetrical || _side == DoorSide::A ? aCentre : bCentre; }

		Range2 calculate_size(DoorSide::Type _side, Vector2 const & _scale) const;

		void debug_draw(DoorSide::Type _side, Vector2 const & _scale, bool _front, Colour const & _colour) const;

		void create_edge_point_array(DoorSide::Type _side, OUT_ Array<Vector2> & _array, OUT_ Vector2 & _centre) const;

		bool is_in_front_of(DoorSide::Type _side, Vector2 const & _scale, Plane const & _plane, float _minDist = 0.0f) const; // at least one point is in front of

		void provide_outbound_points_2d(DoorSide::Type _side, OUT_ Array<Vector2>& _outboundPoints) const;

	public:
		void set_height_from_width(bool _heightFromWidth) { heightFromWidth = _heightFromWidth; }

	private:
		DoorHoleType::Type type;
		float nominalDoorWidth = 0.0f; // if not set (set to 0), won't be scaled
		float nominalDoorHeight = 0.0f; // if not set (set to 0), won't be scaled
		bool heightFromWidth = false;
#ifdef AN_ASSERT
		bool planesValid;
	#endif
		struct Edge
		{
			Vector3 LOCATION_ point; // x right, y forward (should be 0!), z up
			Vector2 LOCATION_ customData; // anything usable by the shader goes in, mapped as UV
			Plane CACHED_ plane;
			Vector3 CACHED_ toNext;
			Vector3 CACHED_ toPrev;
			inline Edge() {}
			inline Edge(Vector3 const & LOCATION_ _point) : point( _point ) {}
		};
		Array<Edge> aEdges; // edges clockwise in outbound space - should be convex, and - if no sides defined -symmetrical (x axis)!
		Array<Edge> bEdges; // edges clockwise in outbound space - should be convex, and - if no sides defined -symmetrical (x axis)!
		Vector3 aCentre = Vector3::zero; // calculated when building
		Vector3 bCentre = Vector3::zero; // bCentre is aCentre * (-1,1,1)

		// mesh to be used for outbound matrix - this is actual hole
		Meshes::Mesh3D* outboundAMesh; // both sides for symmetrical
		Meshes::Mesh3D* outboundBMesh; // if two sides - is reflection of "a"

		void build_side(Array<Edge> & _edges, Meshes::Mesh3D & _outboundMesh);

		inline bool is_convex(Array<Edge> const & _edges) const;
		inline bool is_symmetrical(Array<Edge> const & _edges) const;

		inline Vector3 to_inner_scale(DoorSide::Type _side, Vector2 const & _scale, Vector3 const & _point) const;
		inline Vector3 to_outer_scale(DoorSide::Type _side, Vector2 const & _scale, Vector3 const & _point) const;
	};

	#include "doorHole.inl"
};
