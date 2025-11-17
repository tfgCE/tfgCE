#pragma once

#include "..\navNode.h"

#include "..\..\..\core\memory\softPooledObject.h"

namespace Framework
{
	namespace Nav
	{
		namespace Nodes
		{
			/**
			 *	Placement is in the centre of points, with z axis pointing perpendicular to plane
			 */
			class ConvexPolygon
			: public Nav::Node
			, public SoftPooledObject<ConvexPolygon>
			{
				FAST_CAST_DECLARE(ConvexPolygon);
				FAST_CAST_BASE(Node);
				FAST_CAST_END();

				typedef Nav::Node base;
			public:
				void add_clockwise(Vector3 const & _point);
				void build(float _in = 0.0f);

				bool is_inside(Vector3 const & _point) const;
				bool does_intersect(Segment const & _segment, OUT_ Vector3 & _intersectionPoint) const;

				float calculate_greater_radius() const;

				Optional<Segment> calculate_segment_for_edge(int _edgeIdx, float _inRadius) const;

			public: // Nav::Node
				override_ void reset_to_new();
				override_ void connect_as_open_node();
				override_ void unify_as_open_node() {}

				override_ void debug_draw();

				override_ bool find_location(Vector3 const & _at, OUT_ Vector3 & _found, REF_ float & _dist) const;

			protected: // RefCountObject
				override_ void destroy_ref_count_object() { release(); }

			protected: // SoftPooledObject
				override_ void on_release() { reset_to_new(); }

			private:
				Array<Vector3> points; // local placement

				struct Edge
				{
					// all local
					Vector3 start;
					Vector3 end;
					Vector3 dir;
					Plane in;
				};
				CACHED_ Array<Edge> edges;
			};
		};
	};
};

TYPE_AS_CHAR(Framework::Nav::Nodes::ConvexPolygon);