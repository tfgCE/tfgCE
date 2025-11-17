#pragma once

#include "..\..\math\math.h"

namespace Meshes
{
	namespace Builders
	{
		class IPU;
	};

	namespace Helpers
	{
		struct BezierSurfacePoint;
		struct BezierSurfaceEdge;
		struct BezierSurfaceTriangle;
		class BezierSurfaceHelper;

		namespace BezierSurfaceEdgeFlags
		{
			enum Flag
			{
				None = 0,
				Border = 1,
				Undividable = 2
			};

			typedef int Type;
		};

		struct BezierSurfacePoint
		{
			BezierSurfacePoint();
		private: friend class BezierSurfaceHelper;
			   friend struct BezierSurfaceEdge;
			   friend struct BezierSurfaceTriangle;

			   Vector2 coord;
			   Vector3 point;

			   BezierSurfacePoint(BezierSurface const* _surface, Vector2 const& _coord);
		};

		struct BezierSurfaceEdge
		{
			BezierSurfaceEdge();
		private: friend class BezierSurfaceHelper;
			   friend struct BezierSurfaceTriangle;

			   BezierSurfaceEdgeFlags::Type flags;
			   ArrayStatic<int, 2> points;
			   ArrayStatic<int, 2> belongsToTriangles; // has to belong to two triangles, unless it is border, then it should belong to one triangle
			   Segment2 segmentCoord; // uv
			   float length;

			   BezierSurfaceEdge(BezierSurfaceHelper const* _helper, int _point0, int _point1, BezierSurfaceEdgeFlags::Type _flags);

			   bool has_open_side() const;

			   bool does_share_point_with(BezierSurfaceEdge const& _edge) const;
		};

		struct BezierSurfaceTriangle
		{
			BezierSurfaceTriangle();
		private: friend class BezierSurfaceHelper;

			   float u = 0.0f;
			   float longestEdgeLength = 0.0f;
			   ArrayStatic<int, 3> edges;
			   ArrayStatic<int, 3> points; // ordered clockwise
			   Vector2 centreCoord;
			   ArrayStatic<bool, 3> centreOnRightSideOfEdge;

			   BezierSurfaceTriangle(BezierSurfaceHelper const* _helper, int _edge0, int _edge1, int _edge2, float _u);

			   bool is_coord_inside(BezierSurfaceHelper const* _helper, Vector2 const& _coord, float _epsilon = EPSILON) const;
		};

		class BezierSurfaceHelper
		{

		public:
			BezierSurfaceHelper(BezierSurface const* _surface);

		public: // setup
			void set_max_edge_length(float _maxEdgeLength) { maxEdgeLength = _maxEdgeLength; }
			void set_default_u(float _defaultU) { defaultU = _defaultU; }
			float get_default_u() const { return defaultU; }

			int add_point(Vector2 const& _pointCoord);
			int add_edge(Vector2 const& _pointCoord0, Vector2 const& _pointCoord1, BezierSurfaceEdgeFlags::Type _flags = 0); // returns NONE if edge intersects
			int add_triangle(Vector2 const& _pointCoord0, Vector2 const& _pointCoord1, Vector2 const& _pointCoord2, float _u = -1.0f);

			int add_edge(int _point0, int _point1, BezierSurfaceEdgeFlags::Type _flags = 0); // returns NONE if edge intersects
			int add_triangle(int _edge0, int _edge1, int _edge2, float _u = -1.0f);

			void add_inner_grid(int _innerPointsCountU, int _innerPointsCountV, float _u);

			// adds edges by subdividing start->end UVs using interval or segment count
			void add_edges(Vector2 const& _startingUV, Vector2 const& _endingUV, float _tInterval, BezierSurfaceEdgeFlags::Type _flags);
			void add_edges(Vector2 const& _startingUV, Vector2 const& _endingUV, int _segmentsPerEdgeCount, BezierSurfaceEdgeFlags::Type _flags) { add_edges(_startingUV, _endingUV, 1.0f / (float)(max(1, _segmentsPerEdgeCount)), _flags); }

			void add_border_edge(BezierCurve<Vector3> const& _curve, int _segmentsPerEdgeCount, BezierSurfaceEdgeFlags::Type _flags);
			void add_all_border_edges(int _segmentsPerEdgeCount, BezierSurfaceEdgeFlags::Type _flags);

			void remove_point(int _point);
			void remove_edge(int _edge);
			void remove_triangle(int _triangle);

			int get_point_count() const { return points.get_size(); }
			int get_triangle_count() const { return triangles.get_size(); }

		public: // run
			void process();

		public:
			void into_mesh(::Meshes::Builders::IPU* _builder);

		private:
			BezierSurface const* surface;

			Array<BezierSurfacePoint> points;
			Array<BezierSurfaceEdge> edges;
			Array<BezierSurfaceTriangle> triangles;

			float maxEdgeLength = 0.0f; // if 0 no max edge length

			float defaultU = 0.0f;

			bool processed = false;

			friend struct BezierSurfaceEdge;
			friend struct BezierSurfaceTriangle;

			void mark_dirty() { processed = false; }

			bool check_if_there_are_no_holes();

			void fill();
			void fill_obvious_holes();

			void divide();
		};

		class BezierSurfaceRibbonHelper
		{
		public:
			void set_default_u(float _defaultU) { defaultU = _defaultU; }
			float get_default_u() const { return defaultU; }

			void turn_into_mesh(BezierCurve<Vector3> const& _curveA, BezierCurve<Vector3> const& _curveB, int _segmentsPerEdgeCount, ::Meshes::Builders::IPU* _builder);

		private:
			float defaultU = 0.0f;
		};
	};
};
