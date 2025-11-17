#pragma once

#include "..\vertexSkinningInfo.h"

#include "..\..\containers\array.h"
#include "..\..\math\math.h"

namespace System
{
	struct VertexFormat;
	struct IndexFormat;
};

//#define WITH_UV_SUPPORT

namespace Meshes
{
	namespace Builders
	{
		// Indexed, Point, U (texturing, is kept same for whole polygon)
		// normal is calculated (if needed)
		class IPU
		{

		public:
			IPU();

			void will_need_at_least_points(int _pointCount) { points.make_space_for(_pointCount); }
			void will_need_more_points(int _pointCount) { points.make_space_for_additional(_pointCount); }

			void will_need_at_least_polygons(int _polygonCount) { polygons.make_space_for(_polygonCount); }
			void will_need_more_polygons(int _polygonCount) { polygons.make_space_for_additional(_polygonCount); }

			int get_point_count() const { return points.get_size(); }
			int get_polygon_count() const { return polygons.get_size(); }
			
			void keep_polygons(int _polygonCount) { polygons.set_size(_polygonCount); }

			bool is_empty() const { return polygons.is_empty(); }
			bool is_valid() const;
			
			int get_simple_skin_to_bone_index() const;

			int get_polygon_normal_issues() const { return polygonNormalIssues; }

			void map_u_to_colour(float _u, Colour const& _colour);

			void use_u() { useU = true; }
			void use_uv() { useU = false; }

		public:
			void for_every_triangle(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)> _do, int _startingAt = 0, int _count = NONE) const;
			void for_every_triangle_and_simple_skinning(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx)> _do, int _startingAt = 0, int _count = NONE) const;
			void for_every_triangle_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, float _u)> _do, int _startingAt = 0, int _count = NONE) const;
			void for_every_triangle_simple_skinning_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)> _do, int _startingAt = 0, int _count = NONE) const;
			void for_every_triangle_and_full_skinning(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning)> _do, int _startingAt = 0, int _count = NONE) const;
			void for_every_triangle_full_skinning_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, float _u)> _do, int _startingAt = 0, int _count = NONE) const;

		private:
			void for_every_triangle_worker(bool _getSimpleSkinning, bool _getU, int _startingAt, int _count, std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)> _do) const;

		public: // build
			void use_colour(Colour const& _colour = Colour::white) { currentColour = _colour; }

			void force_material_idx(Optional<int> const & _forceMaterialIdx) { currentForceMaterialIdx = _forceMaterialIdx; }

			int add_point(Vector3 const & _point, Optional<Vector2> const & _uv = NP, VertexSkinningInfo const * _vsi = nullptr);

			int add_triangle(float _u, int _a, int _b, int _c, bool _reverse = false);
			int add_quad(float _u, int _a, int _b, int _c, int _d, bool _reverse = false);

			void get_point(int _idx, OUT_ Vector3 & _point, OUT_ Vector3 & _normal) const;
			void set_point(int _idx, Vector3 const & _point, Vector3 const & _normal);
			void copy_skinning(int _srcIdx, int _destIdx);

		public:
			void set_welding_distance(float _weldingDistance) { weldingDistance = _weldingDistance; }
			void set_smoothing_dot_limit(float _smoothingDotLimit) { smoothingDotLimit = min(1.0f, _smoothingDotLimit); }

			void break_by_material_into(Array<IPU>& ipus); // any with material override will be placed into proper index. arrange memory to avoid resizing or leave empty to auto allocate

			void prepare_data(::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat, OUT_ Array<int8> & _vertexData, OUT_ Array<int8> & _elementsData);
			void prepare_data_as_is(::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat, OUT_ Array<int8> & _vertexData, OUT_ Array<int8> & _elementsData);

		private:
			bool useU = true; // if not, uv is used

			Colour currentColour = Colour::white;
			Optional<int> currentForceMaterialIdx;

			struct Point
			{
				Vector3 point;
				VertexSkinningInfo skinningInfo;
				// these are calculated
				Array<int> belongsToPolygon; // this may be a drag for memory, but maybe we should predict amount of points?
				float u; // separate from uv
#ifdef WITH_UV_SUPPORT
				Vector2 uv;
#endif
				Colour colour;
				Vector3 normal;
				float normalWeight;

				static bool can_be_welded(Point const & _a, Point const & _b) { return _a.skinningInfo == _b.skinningInfo
#ifdef WITH_UV_SUPPORT
					&& _a.uv == _b.uv
#endif
					;
				}
			};
			Array<Point> points;

			struct Polygon
			{
				ArrayStatic<int, 4> indices;
				float u; // u for whole polygon
				Colour colour = Colour::white;
				Optional<int> forceMaterialIdx;
				Vector3 normal; // calculated

				Polygon()
				{
					SET_EXTRA_DEBUG_INFO(indices, TXT("Builders::IPU::Polygon.indices"));
				}
			};
			Array<Polygon> polygons;
			int polygonNormalIssues = 0;

			struct MapUToColour
			{
				float u;
				Colour colour;
			};
			Array<MapUToColour> mapUToColour;

			float weldingDistance = 0.0001f; // negative if shouldn't be welded at all
			float smoothingDotLimit = 0.99f;

			void weld_points();
			void collapse_polygons();
			void find_where_points_belong();
			void calculate_normals_for_polygons();
			void do_points_vs_polygons(); // and copy points if needed
			void remove_unused_points();
			void fill_u_colour_normal();

			void remap_indices(Array<int> const & _remap);

			bool can_polygons_share_point(int _a, int _b) const;

			void prepare_data_fill_data(::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat, OUT_ Array<int8> & _vertexData, OUT_ Array<int8> & _elementsData);

			bool are_points_colinear(ArrayStatic<int, 4> const & _indices) const;

		};
	}
};
