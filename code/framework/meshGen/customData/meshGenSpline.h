#pragma once

#include "..\meshGenUVDef.h"

#include "..\..\library\customLibraryDatas.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\wheresMyPoint\wmp.h"

namespace Framework
{
	namespace MeshGeneration
	{
		struct SubStepDef;
		struct GenerationContext;

		namespace CustomData
		{
			template <typename Point> struct SplineContext;

			/**
			 *	This is cross section that might be used by various mesh gen shapes.
			 *	In most common uses it is:
			 *		Facing forward, direction in which it should be going (-x is left, +x is right, -y is down, +y is up)
			 *		For circles it assumes circles go in clockwise direction (circle's edge should be at 0,0, interior at +x)
			 *	It is composed of curves (end point of nth section should be same as start point of next section)
			 *	If curves are linear (it is cached value to tell their linear) they are just lines.
			 *	Curves are equally divided.
			 *	Each section has different U texture parameter (TODO maybe extend that in future?
			 */
			template <typename Point>
			class Spline
			: public CustomLibraryData
			{
				FAST_CAST_DECLARE(Spline);
				FAST_CAST_BASE(CustomLibraryData);
				FAST_CAST_END();
				typedef CustomLibraryData base;
			public:
				struct Segment
				{
					BezierCurve<Point> segment;
					UVDef uvDef;

					RefCountObjectPtr<WheresMyPoint::ToolSet> p0ToolSet;
					RefCountObjectPtr<WheresMyPoint::ToolSet> p1ToolSet;
					RefCountObjectPtr<WheresMyPoint::ToolSet> p2ToolSet;
					RefCountObjectPtr<WheresMyPoint::ToolSet> p3ToolSet;
					bool forceLinear = false;
					bool withRoundSeparation = false; // for both tools and general separation, especially when dealing with various transforms

					bool does_require_generation() const { return p0ToolSet.is_set() || p1ToolSet.is_set() || p2ToolSet.is_set() || p3ToolSet.is_set(); }
				};
				struct RefPoint
				{
					Name name;
					Point location = zero<Point>();
					RefPoint() {}
					RefPoint(Name const & _name, Point const & _location) : name(_name), location(_location) {}
				};

				Spline();

				bool has_ref_point(Name const & _name) const;
				Point const * find_ref_point(Name const & _name) const;

				Matrix33 build_transform_matrix_33(Array<RefPoint> const & _refPoints) const;
				Matrix44 build_transform_matrix_44(Array<RefPoint> const & _refPoints) const;

				bool get_segments_for(OUT_ Array<Segment> & _outSegments, WheresMyPoint::Context & _wmpContext, SplineContext<Point> const & _splineContext) const; // this should allow to generate on fly in future! this also generates segments

				static void change_segments_into_points(Array<Segment> const & _segments, Point const & _usingScale, SubStepDef const & _subStepDef, GenerationContext const & _context, OUT_ Array<Point> & _points, OUT_ Array<UVDef> & _us, Optional<bool> const& _ignoreAutoModifiers = NP);
				static void change_segments_into_number_of_points(Array<Segment> const & _segments, Point const & _usingScale, SubStepDef const & _subStepDef, GenerationContext const & _context, OUT_ Array<int> & _numSubStepsPerSegment, Optional<bool> const& _ignoreAutoModifiers = NP);
				static void change_segments_into_points(Array<Segment> const & _segments, Point const & _usingScale, Array<int> const & _numSubStepsPerSegment, OUT_ Array<Point> & _points, OUT_ Array<UVDef> & _us);

				static void make_sure_segments_are_clockwise(Array<Segment> & _segments);

				void do_for_every_segment(std::function<void(Segment& _segment)> _do);

				bool load_replace_u_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

			public: // CustomLibraryData
#ifdef AN_DEVELOPMENT
				override_ void ready_for_reload();
#endif
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

			private:
				Array<Segment> segments;
				Array<RefPoint> refPoints;
			};

			template <typename Point>
			struct SplineContext
			{
				/* use specialised! */
			};

			template <>
			struct SplineContext<Vector2>
			{
				Matrix33 transform = Matrix33::identity;
				Array<typename Spline<Vector2>::RefPoint> const * refPoints = nullptr;
			};

			template <>
			struct SplineContext<Vector3>
			{
				Matrix44 transform = Matrix44::identity;
				Array<typename Spline<Vector3>::RefPoint> const * refPoints = nullptr;
			};
		};
	};
};
