#include "schematicLine.h"

#include "schematic.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\mesh\builders\mesh3dBuilder_IPU.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(SchematicLine);

void SchematicLine::be_companion_of(SchematicLine const & _other, float _dist)
{
	set_colour(_other.colour, _other.halfColour);
	set_line_width(_other.lineWidth);

	if (_other.points.get_size() < 2)
	{
		return;
	}

	for_count(int, i, _other.points.get_size())
	{
		Vector2 p = _other.points[max(i - 1, 0)];
		Vector2 c = _other.points[i];
		Vector2 n = _other.points[min(i + 1, _other.points.get_size() - 1)];

		if (i == 0)
		{
			Vector2 c2n = (n - c).normal();
			Vector2 r = c2n.rotated_right();
			add(c + r * _dist);
		}
		else if (i == _other.points.get_size() - 1)
		{
			Vector2 p2c = (c - p).normal();
			Vector2 r = p2c.rotated_right();
			add(c + r * _dist);
		}
		else
		{
			Vector2 c2n = (n - c).normal();
			Vector2 p2c = (c - p).normal();
			Vector2 r0 = c2n.rotated_right();
			Vector2 r1 = p2c.rotated_right();
			if (Vector2::dot(c2n, p2c) > 0.9f)
			{
				Vector2 r = (r0 + r1).normal();
				add(c + r * _dist);
			}
			else
			{
				Vector2 iAt;
				if (Vector2::calc_intersection(c + r0 * _dist, c2n, p + r1 * _dist, p2c, iAt))
				{
					add(iAt);
				}
				else
				{
					Vector2 r = (r0 + r1).normal();
					add(c + r * _dist);
				}
			}
		}
	}
}

void SchematicLine::build(Meshes::Builders::IPU& _ipu, bool _outline) const
{
	if (points.get_size() < 2)
	{
		return;
	}

	if (filled.is_set() && (!_outline || ! outline))
	{
		_ipu.use_colour(_outline? colourAsOutline : filled.get());

		Vector2 centre = Vector2::zero;
		for_every(p, points)
		{
			centre += *p;
		}
		centre /= (float)points.get_size();

		int centreIdx = _ipu.get_point_count();
		_ipu.add_point(centre.to_vector3());

		int firstPointIdx = _ipu.get_point_count();

		for_every(p, points)
		{
			_ipu.add_point(p->to_vector3());
		}

		int lastPointIdx = _ipu.get_point_count() - 1;

		int pointIdx = firstPointIdx;
		int prevPointIdx = lastPointIdx;
		for_count(int, i, points.get_size())
		{
			_ipu.add_triangle(0.0f, centreIdx, prevPointIdx, pointIdx);
			prevPointIdx = pointIdx;
			++pointIdx;
		}
	}

	_ipu.use_colour(_outline ? colourAsOutline : colour);

	if (outline)
	{
		float halfWidth = lineWidth * 0.5f;

		int startingPointLeftIdx = _ipu.get_point_count();
		int startingPointCentreIdx = _ipu.get_point_count() + (halfColour.is_set()? 1 : 0);
		int startingPointRightIdx = _ipu.get_point_count() + (halfColour.is_set() ? 2 : 1);
		int prevPointIdx = _ipu.get_point_count();

		int howMany = looped ? points.get_size() + 1 : points.get_size();
		for_count(int, i, howMany)
		{
			Vector2 p = points[mod(i - 1, points.get_size())];
			Vector2 c = points[mod(i, points.get_size())];
			Vector2 n = points[mod(i + 1, points.get_size())];

			Vector2 p2c = (c - p).normal();
			Vector2 c2n = (n - c).normal();
			Vector2 pr = p2c.rotated_right();
			Vector2 cr = c2n.rotated_right();

			if (i == 0 && ! looped)
			{
				// don't provide the first one, we will be just connecting
				if (!looped)
				{
					Vector2 left, right;

					left = c - cr * halfWidth;
					right = c + cr * halfWidth;

					if (halfColour.is_set())
					{
						_ipu.add_point(left.to_vector3());
						_ipu.add_point(c.to_vector3());
						_ipu.add_point(right.to_vector3());
					}
					else
					{
						_ipu.add_point(left.to_vector3());
						_ipu.add_point(right.to_vector3());
					}
				}
			}
			else if (i == howMany - 1)
			{
				if (looped)
				{
					// connect the last one
					if (halfColour.is_set())
					{
						if (! _outline) _ipu.use_colour(colour);
						_ipu.add_quad(0.0f, prevPointIdx, startingPointLeftIdx, startingPointCentreIdx, prevPointIdx + 1);
						if (!_outline) _ipu.use_colour(halfColour.get());
						_ipu.add_quad(0.0f, prevPointIdx + 1, startingPointCentreIdx, startingPointRightIdx, prevPointIdx + 2);
					}
					else
					{
						_ipu.add_quad(0.0f, prevPointIdx, startingPointLeftIdx, startingPointRightIdx, prevPointIdx + 1);
					}
				}
				else
				{
					Vector2 left, right;

					left = c - pr * halfWidth;
					right = c + pr * halfWidth;

					if (halfColour.is_set())
					{
						_ipu.add_point(left.to_vector3());
						_ipu.add_point(c.to_vector3());
						_ipu.add_point(right.to_vector3());
						if (!_outline) _ipu.use_colour(colour);
						_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 3, prevPointIdx + 4, prevPointIdx + 1);
						if (!_outline) _ipu.use_colour(halfColour.get());
						_ipu.add_quad(0.0f, prevPointIdx + 1, prevPointIdx + 4, prevPointIdx + 5, prevPointIdx + 2);
					}
					else
					{
						_ipu.add_point(left.to_vector3());
						_ipu.add_point(right.to_vector3());
						_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 2, prevPointIdx + 3, prevPointIdx + 1);
					}
				}
			}
			else
			{
				float onRight = Vector2::dot(c2n, pr);

				if (abs(onRight) < 0.05f)
				{
					Vector2 left, right;

					Vector2 ar = (pr + cr).normal(); // average right
					left = c - ar * halfWidth;
					right = c + ar * halfWidth;

					if (halfColour.is_set())
					{
						_ipu.add_point(left.to_vector3());
						_ipu.add_point(c.to_vector3());
						_ipu.add_point(right.to_vector3());
					}
					else
					{
						_ipu.add_point(left.to_vector3());
						_ipu.add_point(right.to_vector3());
					}

					if (i > 0 || !looped)
					{
						if (halfColour.is_set())
						{
							if (!_outline) _ipu.use_colour(colour);
							_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 3, prevPointIdx + 4, prevPointIdx + 1);
							if (!_outline) _ipu.use_colour(halfColour.get());
							_ipu.add_quad(0.0f, prevPointIdx + 1, prevPointIdx + 4, prevPointIdx + 5, prevPointIdx + 2);
							prevPointIdx += 3;
						}
						else
						{
							_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 2, prevPointIdx + 3, prevPointIdx + 1);
							prevPointIdx += 2;
						}
					}
				}
				else
				{
					Vector2 left, right;
				
					Vector2::calc_intersection(p - pr * halfWidth, p2c, n - cr * halfWidth, -c2n, OUT_ left);
					Vector2::calc_intersection(p + pr * halfWidth, p2c, n + cr * halfWidth, -c2n, OUT_ right);

					if ((left - c).length() > lineWidth)
					{
						//						5
						//		   3		   /|
						//		  /|		  / 6
						//		 / |		 / /|
						//		2--4		3-4-7
						//		|  |		| | |
						//		|  |		| | |
						//		0--1		0-1-2
						if (halfColour.is_set())
						{
							Vector2 leftp = c - pr * halfWidth;
							Vector2 leftc = c - cr * halfWidth;
							Vector2 cp = (leftp + right) * 0.5f;
							Vector2 cc = (leftc + right) * 0.5f;
							_ipu.add_point(leftp.to_vector3());
							_ipu.add_point(cp.to_vector3());
							_ipu.add_point(leftc.to_vector3());
							_ipu.add_point(cc.to_vector3());
							_ipu.add_point(right.to_vector3());

							if (i > 0 || !looped)
							{
								if (!_outline) _ipu.use_colour(colour);
								_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 3, prevPointIdx + 4, prevPointIdx + 1);
								_ipu.add_quad(0.0f, prevPointIdx + 3, prevPointIdx + 5, prevPointIdx + 6, prevPointIdx + 4);
								if (!_outline) _ipu.use_colour(halfColour.get());
								_ipu.add_quad(0.0f, prevPointIdx + 1, prevPointIdx + 4, prevPointIdx + 7, prevPointIdx + 2);
								_ipu.add_triangle(0.0f, prevPointIdx + 4, prevPointIdx + 6, prevPointIdx + 7);
								prevPointIdx += 3 + 5 - 3;
							}
							else
							{
								startingPointLeftIdx = prevPointIdx + 3 - 3;
								startingPointCentreIdx = prevPointIdx + 4 - 3;
								startingPointRightIdx = prevPointIdx + 7 - 3;
								if (!_outline) _ipu.use_colour(colour);
								_ipu.add_quad(0.0f, prevPointIdx + 3 - 3, prevPointIdx + 5 - 3, prevPointIdx + 6 - 3, prevPointIdx + 4 - 3);
								if (!_outline) _ipu.use_colour(halfColour.get());
								_ipu.add_triangle(0.0f, prevPointIdx + 4 - 3, prevPointIdx + 6 - 3, prevPointIdx + 7 - 3);
								prevPointIdx += 5 - 3;
							}
						}
						else
						{
							left = c - pr * halfWidth;
							_ipu.add_point(left.to_vector3());
							left = c - cr * halfWidth;
							_ipu.add_point(left.to_vector3());
							_ipu.add_point(right.to_vector3());

							if (i > 0 || !looped)
							{
								_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 2, prevPointIdx + 4, prevPointIdx + 1);
								_ipu.add_triangle(0.0f, prevPointIdx + 2, prevPointIdx + 3, prevPointIdx + 4);
								prevPointIdx += 2 + 3 - 2;
							}
							else
							{
								startingPointLeftIdx = prevPointIdx + 2 - 2;
								startingPointRightIdx = prevPointIdx + 4 - 2;
								_ipu.add_triangle(0.0f, prevPointIdx + 2 - 2, prevPointIdx + 3 - 2, prevPointIdx + 4 - 2);
								prevPointIdx += 3 - 2;
							}
						}
					}
					else if ((right - c).length() > lineWidth)
					{
						//				7
						//		4		|\
						//		|\		6 \
						//		| \		|\ \
						//		3--2	5-3-4
						//		|  |	| | |
						//		|  |	| | |
						//		0--1	0-1-2
						if (halfColour.is_set())
						{
							Vector2 rightp = c + pr * halfWidth;
							Vector2 rightc = c - cr * halfWidth;
							Vector2 cp = (left + rightp) * 0.5f;
							Vector2 cc = (left + rightc) * 0.5f;
							_ipu.add_point(cp.to_vector3());
							_ipu.add_point(rightp.to_vector3());
							_ipu.add_point(left.to_vector3());
							_ipu.add_point(cc.to_vector3());
							_ipu.add_point(rightc.to_vector3());

							if (i > 0 || !looped)
							{
								if (!_outline) _ipu.use_colour(colour);
								_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 5, prevPointIdx + 3, prevPointIdx + 1);
								_ipu.add_triangle(0.0f, prevPointIdx + 5, prevPointIdx + 6, prevPointIdx + 3);
								if (!_outline) _ipu.use_colour(halfColour.get());
								_ipu.add_quad(0.0f, prevPointIdx + 1, prevPointIdx + 3, prevPointIdx + 4, prevPointIdx + 2);
								_ipu.add_quad(0.0f, prevPointIdx + 3, prevPointIdx + 6, prevPointIdx + 7, prevPointIdx + 4);
								prevPointIdx += 3;
							}
							else
							{
								startingPointLeftIdx = prevPointIdx + 5 - 3;
								startingPointCentreIdx = prevPointIdx + 3 - 3;
								startingPointRightIdx = prevPointIdx + 4 - 3;
								if (!_outline) _ipu.use_colour(colour);
								_ipu.add_triangle(0.0f, prevPointIdx + 5 - 3, prevPointIdx + 6 - 3, prevPointIdx + 3 - 3);
								if (!_outline) _ipu.use_colour(halfColour.get());
								_ipu.add_quad(0.0f, prevPointIdx + 3 - 3, prevPointIdx + 6 - 3, prevPointIdx + 7 - 3, prevPointIdx + 4 - 3);
								prevPointIdx += 2;
							}
						}
						else
						{
							right = c + pr * halfWidth;
							_ipu.add_point(right.to_vector3());
							_ipu.add_point(left.to_vector3());
							right = c + cr * halfWidth;
							_ipu.add_point(right.to_vector3());

							if (i > 0 || !looped)
							{
								_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 3, prevPointIdx + 2, prevPointIdx + 1);
								_ipu.add_triangle(0.0f, prevPointIdx + 3, prevPointIdx + 4, prevPointIdx + 2);
								prevPointIdx += 3;
							}
							else
							{
								startingPointLeftIdx = prevPointIdx + 3 - 2;
								startingPointRightIdx = prevPointIdx + 2 - 2;
								_ipu.add_triangle(0.0f, prevPointIdx + 3 - 2, prevPointIdx + 4 - 2, prevPointIdx + 2 - 2);
								prevPointIdx += 1;
							}
						}
					}
					else
					{
						if (halfColour.is_set())
						{
							_ipu.add_point(left.to_vector3());
							_ipu.add_point(c.to_vector3());
							_ipu.add_point(right.to_vector3());

							if (i > 0 || !looped)
							{
								if (!_outline) _ipu.use_colour(colour);
								_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 3, prevPointIdx + 4, prevPointIdx + 1);
								if (!_outline) _ipu.use_colour(halfColour.get());
								_ipu.add_quad(0.0f, prevPointIdx + 1, prevPointIdx + 4, prevPointIdx + 5, prevPointIdx + 2);
								prevPointIdx += 3;
							}
						}
						else
						{
							_ipu.add_point(left.to_vector3());
							_ipu.add_point(right.to_vector3());

							if (i > 0 || !looped)
							{
								_ipu.add_quad(0.0f, prevPointIdx, prevPointIdx + 2, prevPointIdx + 3, prevPointIdx + 1);
								prevPointIdx += 2;
							}
						}
					}
				}
			}
		}
	}

	_ipu.use_colour();
}

void SchematicLine::cut_lines_with(SchematicLine const& _convexLine, Schematic * _addNewLinesTo, bool _removeOutside)
{
	if (looped || filled.is_set())
	{
		// we don't cut looped
		return;
	}

	an_assert(_convexLine.looped);

	int clSize = _convexLine.points.get_size();

	Vector2 clCentre = Vector2::zero;
	for_every(p, _convexLine.points)
	{
		clCentre += *p;
	}
	clCentre /= (float)clSize;
	
	struct CuttingLines
	{
		Vector2 at;
		Vector2 to;
		Vector2 at2to;
		Vector2 dir;
		Vector2 inwards;
		float length;

		static bool is_inside(Vector2 const& _p, ArrayStack<CuttingLines> const& _lines)
		{
			for_every(line, _lines)
			{
				if (Vector2::dot(_p - line->at, line->inwards) <= 0.0f)
				{
					return false;
				}
			}

			return true;
		}

		static bool does_cut(Vector2 const & _c, Vector2 const & _n, ArrayStack<CuttingLines> const& _lines, OUT_ Vector2 & _cutAt)
		{
			float earliestCut = 2.0f;
			for_every(line, _lines)
			{
				if (line->length == 0.0f)
				{
					continue;
				}
				float s0t = Vector2::dot(_c - line->at, line->inwards);
				float s1t = Vector2::dot(_n - line->at, line->inwards);
				if (s0t * s1t < 0.0f)
				{
					// check if this is the earliest one
					float cutAtPt = -s0t / (s1t - s0t);
					if (cutAtPt > 0.001f && cutAtPt < earliestCut)
					{
						// we might be cutting this line, check if the cut is between points

						Vector2 cutCandidateAt = _c + (_n - _c) * cutAtPt;

						float ccDist = Vector2::dot(cutCandidateAt - line->at, line->dir);
						if (ccDist >= 0.0f && ccDist <= line->length)
						{
							earliestCut = cutAtPt;
						}
					}
				}
			}

			if (earliestCut >= 0.0f && earliestCut <= 1.0f)
			{
				_cutAt = _c + (_n - _c) * earliestCut;
				return true;
			}

			return false;
		}
	};
	ARRAY_STACK(CuttingLines, cuttingLines, clSize);
	cuttingLines.set_size(clSize);

	for_count(int, i, clSize)
	{
		Vector2 c = _convexLine.points[mod(i, clSize)];
		Vector2 n = _convexLine.points[mod(i + 1, clSize)];

		auto& cl = cuttingLines[i];
		cl.at = c;
		cl.to = n;
		cl.at2to = n - c;
		cl.dir = (n - c).normal();
		cl.length = (n - c).length();
		cl.inwards = cl.dir.rotated_right();
		if (Vector2::dot(cl.inwards, clCentre - c) < 0.0f)
		{
			cl.inwards = -cl.inwards;
		}
	}

	//

	// first cut lines by cuttingLines
	Optional<Vector2> lastCut;
	for(int i = 0; i < points.get_size() - 1; ++ i)
	{
		Vector2 c = points[i];
		Vector2 n = points[i + 1];
		Vector2 cutAt;
		if (CuttingLines::does_cut(c, n, cuttingLines, cutAt))
		{
			if (lastCut.is_set() && lastCut.get() == cutAt)
			{
				float minDistToCut = 0.02f;
				if ((n - c).length() < minDistToCut)
				{
					// skip this one
					continue;
				}
				else
				{
					// cut further
					cutAt = n + (c - n).normal() * minDistToCut;
				}
			}
			if ((c - n).length() > 0.0001f)
			{
				points.insert_at(i + 1, cutAt);
			}
			lastCut = cutAt;
		}
	}

	// get copy of the points and clear current line
	Array<Vector2> cutPoints = points;
	points.clear();

	// add points to current line or create new ones
	bool removing = true;
	SchematicLine* addTo = this;
	for (int i = 0; i < cutPoints.get_size() - 1; ++i)
	{
		Vector2 c = cutPoints[i];
		Vector2 n = cutPoints[min(i + 1, cutPoints.get_size() - 1)]; // to cut the last one
		Vector2 h = (c + n) * 0.5f;
		bool isInside = CuttingLines::is_inside(h, cuttingLines);
		if (isInside ^ _removeOutside)
		{
			addTo = nullptr; // finalise current line
			removing = true;
		}
		else
		{
			if (removing)
			{
				// get next line to add stuff
				if (points.is_empty())
				{
					addTo = this;
				}
				else
				{
					addTo = new SchematicLine();
					addTo->set_colour(colour);
					addTo->set_line_width(lineWidth);
					_addNewLinesTo->add(addTo);
				}
			}
			removing = false;
			if (addTo->points.is_empty())
			{
				// first point
				addTo->add(c);
			}
			addTo->add(n);
		}
	}
}

bool SchematicLine::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	looped = _node->get_bool_attribute_or_from_child_presence(TXT("looped"), looped);
	outline = _node->get_bool_attribute_or_from_child_presence(TXT("outline"), outline);
	filled.load_from_xml(_node, TXT("fillColour"));
	colour.load_from_xml(_node, TXT("colour"));
	colourAsOutline.load_from_xml(_node, TXT("colourAsOutline"));
	halfColour.load_from_xml(_node, TXT("halfColour"));
	lineWidth = _node->get_float_attribute_or_from_child(TXT("lineWidth"), lineWidth);

	for_every(node, _node->children_named(TXT("point")))
	{
		Vector2 v = points.is_empty()? Vector2::zero : points.get_last();
		v.load_from_xml(node, TXT("x"), TXT("y"), true);
		points.push_back(v);
	}

	{
		float scale = _node->get_float_attribute_or_from_child(TXT("scale"), 1.0f);
		for_every(point, points)
		{
			*point *= scale;
		}
	}

	return result;
}

void SchematicLine::grow_size(REF_ Range2& _size) const
{
	Range2 fromPoints = Range2::empty;
	for_every(p, points)
	{
		fromPoints.include(*p);
	}

	fromPoints.expand_by(Vector2::one * lineWidth);

	_size.include(fromPoints);
}
