#pragma once

#include "..\containers\array.h"
#include "..\math\math.h"
#include "..\types\colour.h"

class DebugVisualiser;

namespace VR
{
	struct Zone
	{
	public:
		bool is_empty() const { return points.is_empty(); }
		void clear();
		void add(Vector2 const & _point);
		void build();

		void be_range(Range2 const & _range);
		void be_rect(Vector2 const & _rectSize, Vector2 const & _offset = Vector2::zero);
		void place_at(Vector2 const & _at, Vector2 const & _dir);

		Zone to_world_of(Transform const & _placement) const;
		Zone to_local_of(Transform const & _placement) const;

		bool does_intersect_with(Zone const & _zone) const;
		bool does_intersect_with(Segment2 const & _seg) const;
		bool does_contain(Zone const & _p, float _inside = 0.0f) const;
		bool does_contain(Vector2 const & _p, float _inside = 0.0f) const;
		Vector2 get_inside(Vector2 const & _p, float _inside = 0.0f) const;
		bool calc_intersection(Vector2 const & _a, Vector2 const & _dir, Vector2 & _intersection, Optional<float> const & _inside = NP, Optional<float> const& _halfWidth = NP) const;
		float calc_intersection_t(Vector2 const& _a, Vector2 const& _dir, Vector2& _intersection, Optional<float> const& _inside = NP) const; // -1 if none

		Range2 to_range2() const;

#ifdef AN_DEBUG_RENDERER
		void debug_render(Colour const & _colour = Colour::green, Transform const & _placement = Transform::identity) const;
#endif

		void debug_visualise(DebugVisualiser* _dv, Colour const & _colour = Colour::black) const;

	private:
		struct Edge
		{
			Vector2 start;
			Vector2 end;
			Vector2 dir;
			Vector2 inside; // right of dir
			float length;
		};
		Array<Vector2> points;
		Array<Edge> edges; // same number as points
	};
};
