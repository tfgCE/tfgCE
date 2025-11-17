#include "math.h"
#include "..\io\xml.h"

#include "..\collision\checkCollisionContext.h"
#include "..\debug\debugRenderer.h"
#include "..\mesh\builders\mesh3dBuilder_IPU.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

Box::Box()
{
}

void Box::setup(Vector3 const & _centre, Vector3 const & _halfSize, Vector3 const & _xAxis, Vector3 const & _yAxis)
{
	setup(_centre, _halfSize, _xAxis, _yAxis, Vector3::cross(_xAxis, _yAxis).normal());
}

void Box::setup(Vector3 const & _centre, Vector3 const & _halfSize, Vector3 const & _xAxis, Vector3 const & _yAxis, Vector3 const & _zAxis)
{
	centre = _centre;
	halfSize = _halfSize;
	xAxis = _xAxis.normal();
	zAxis = Vector3::cross(xAxis, Vector3::cross(_zAxis, xAxis)).normal();
	yAxis = Vector3::cross(zAxis, xAxis);
	yAxis = Vector3::dot(yAxis, _yAxis) > 0.0f ? yAxis : -yAxis;

	Vector3 xhs = xAxis * halfSize.x;
	Vector3 yhs = yAxis * halfSize.y;
	Vector3 zhs = zAxis * halfSize.z;

	points[0] = centre + xhs + yhs + zhs;
	points[1] = centre - xhs + yhs + zhs;
	points[2] = centre + xhs - yhs + zhs;
	points[3] = centre - xhs - yhs + zhs;
	points[4] = centre + xhs + yhs - zhs;
	points[5] = centre - xhs + yhs - zhs;
	points[6] = centre + xhs - yhs - zhs;
	points[7] = centre - xhs - yhs - zhs;

	an_assert(!xAxis.is_almost_zero());
	an_assert(!yAxis.is_almost_zero());
	an_assert(!zAxis.is_almost_zero());
}

void Box::setup(Range3 const & _range)
{
	an_assert(_range.length().x != 0.0f);
	an_assert(_range.length().y != 0.0f);
	an_assert(_range.length().z != 0.0f);
	setup(_range.centre(), _range.length() * 0.5f, Vector3::xAxis, Vector3::yAxis, Vector3::zAxis);
}

void Box::setup(Vector3 const & _refLoc, Range3 const & _rangeLocal, Vector3 const & _xAxis, Vector3 const & _yAxis, Vector3 const & _zAxis)
{
	an_assert(_rangeLocal.length().x != 0.0f);
	an_assert(_rangeLocal.length().y != 0.0f);
	an_assert(_rangeLocal.length().z != 0.0f);
	Vector3 offset = _rangeLocal.centre();
	setup(_refLoc + _xAxis * offset.x + _yAxis * offset.y + _zAxis * offset.z, _rangeLocal.length() * 0.5f, _xAxis, _yAxis, _zAxis);
}

void Box::setup_allow_zero(Range3 const & _range)
{
	setup(_range.centre(), _range.length() * 0.5f, Vector3::xAxis, Vector3::yAxis, Vector3::zAxis);
}

void Box::query_distance_axis(Vector3 const & _diff, Vector3 const & _axis, float const _halfSize, OUT_ float & _offsetOutsideBox)
{
	float dotDiffAxis = Vector3::dot(_diff, _axis);
	_offsetOutsideBox = (dotDiffAxis < 0.0f? -1.0f : 1.0f) * max(0.0f, abs(dotDiffAxis) - _halfSize);
}

bool Box::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	{
		IO::XML::Node const* centreNode = _node->first_child_named(TXT("centre"), TXT("center"));
		if (!centreNode)
		{
			centreNode = _node->first_child_named(TXT("at"));
		}
		if (centreNode || _node->first_child_named(TXT("size")))
		{
			Vector3 lCentre = Vector3::zero;
			Vector3 lXAxis = Vector3::xAxis;
			Vector3 lYAxis = Vector3::yAxis;
			Vector3 lZAxis = Vector3::zAxis;
			Vector3 lSize = Vector3::zero;
			Vector3 lAtPt = Vector3::half;
			if (centreNode)
			{
				lCentre.load_from_xml(centreNode);
			}
			lXAxis.load_from_xml_child_node(_node, TXT("xAxis"));
			lYAxis.load_from_xml_child_node(_node, TXT("yAxis"));
			lSize.load_from_xml_child_node(_node, TXT("size"));
			lAtPt.load_from_xml_child_node(_node, TXT("atPt"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);

			setup(lCentre, lSize * 0.5f, lXAxis, lYAxis, lZAxis);

			centre += xAxis * (0.5f - lAtPt.x) * lSize;
			centre += yAxis * (0.5f - lAtPt.y) * lSize;
			centre += zAxis * (0.5f - lAtPt.z) * lSize;

			return true;
		}
	}
	{
		Array<IO::XML::Node const*> cornerNodes;
		if (_node->get_children_named(cornerNodes, TXT("corner")) >= 3)
		{
			// first corner is actual corner
			// following corners tell about axes
			Vector3 corner0;
			Vector3 corner1;
			Vector3 corner2;
			Vector3 corner3;
			corner0.load_from_xml(cornerNodes[0]);
			corner1.load_from_xml(cornerNodes[1]);
			corner2.load_from_xml(cornerNodes[2]);
			Vector3 c0to1 = corner1 - corner0;
			Vector3 c0to2 = corner2 - corner0;
			xAxis = c0to1.normal();
			yAxis = Vector3::cross(xAxis, Vector3::cross(c0to2, xAxis)).normal();
			zAxis = Vector3::cross(xAxis, yAxis);
			halfSize.x = Vector3::dot(xAxis, c0to1) * 0.5f;
			halfSize.y = Vector3::dot(yAxis, c0to2) * 0.5f;
			Vector3 startsAt = corner0;
			if (cornerNodes.get_size() == 4)
			{
				corner3.load_from_xml(cornerNodes[3]);
				Vector3 c0to3 = corner3 - corner0;
				halfSize.z = Vector3::dot(zAxis, c0to3) * 0.5f;
				if (halfSize.z < 0.0f)
				{
					halfSize.z = -halfSize.z;
					zAxis = -zAxis;
				}
			}
			else if (IO::XML::Node const * thicknessNode = _node->first_child_named(TXT("thickness")))
			{
				float thickness = thicknessNode->get_float();
				halfSize.z = thickness * 0.5f;
				startsAt -= halfSize.z * zAxis;
			}
			else
			{
				todo_important(TXT("error"));
				return false;
			}
			centre = startsAt
				   + xAxis * halfSize.x 
				   + yAxis * halfSize.y
				   + zAxis * halfSize.z;

			setup(centre, halfSize, xAxis, yAxis, zAxis);
			return true;
		}
	}
	return false;
}

void Box::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const
{
#ifdef AN_DEBUG_RENDERER
	Colour colour = _colour;
#ifdef AN_DEVELOPMENT
	if (highlightDebugDraw)
	{
		colour = get_debug_highlight_colour();
	}
#endif

	Colour fillColour = colour * Colour::alpha(_alphaFill);

	Vector3 xhs = xAxis * halfSize.x;
	Vector3 yhs = yAxis * halfSize.y;
	Vector3 zhs = zAxis * halfSize.z;

	Vector3 ppp = centre + xhs + yhs + zhs;
	Vector3 ppn = centre + xhs + yhs - zhs;
	Vector3 pnp = centre + xhs - yhs + zhs;
	Vector3 pnn = centre + xhs - yhs - zhs;
	Vector3 npp = centre - xhs + yhs + zhs;
	Vector3 npn = centre - xhs + yhs - zhs;
	Vector3 nnp = centre - xhs - yhs + zhs;
	Vector3 nnn = centre - xhs - yhs - zhs;

	// along x
	debug_draw_line(_frontBorder, colour, npp, ppp);
	debug_draw_line(_frontBorder, colour, nnp, pnp);
	debug_draw_line(_frontBorder, colour, npn, ppn);
	debug_draw_line(_frontBorder, colour, nnn, pnn);

	// along y
	debug_draw_line(_frontBorder, colour, pnp, ppp);
	debug_draw_line(_frontBorder, colour, nnp, npp);
	debug_draw_line(_frontBorder, colour, pnn, ppn);
	debug_draw_line(_frontBorder, colour, nnn, npn);

	// along z 
	debug_draw_line(_frontBorder, colour, ppn, ppp);
	debug_draw_line(_frontBorder, colour, pnn, pnp);
	debug_draw_line(_frontBorder, colour, npn, npp);
	debug_draw_line(_frontBorder, colour, nnn, nnp);

	// xy
	debug_draw_triangle(_frontFill, fillColour, ppp, pnp, nnp);
	debug_draw_triangle(_frontFill, fillColour, npp, ppp, nnp);
	debug_draw_triangle(_frontFill, fillColour, ppn, pnn, nnn);
	debug_draw_triangle(_frontFill, fillColour, npn, ppn, nnn);

	// xz
	debug_draw_triangle(_frontFill, fillColour, ppp, ppn, npn);
	debug_draw_triangle(_frontFill, fillColour, npp, ppp, npn);
	debug_draw_triangle(_frontFill, fillColour, pnp, pnn, nnn);
	debug_draw_triangle(_frontFill, fillColour, nnp, pnp, nnn);

	// yz
	debug_draw_triangle(_frontFill, fillColour, ppp, ppn, pnn);
	debug_draw_triangle(_frontFill, fillColour, pnp, ppp, pnn);
	debug_draw_triangle(_frontFill, fillColour, npp, npn, nnn);
	debug_draw_triangle(_frontFill, fillColour, nnp, npp, nnn);
#endif
}

void Box::log(LogInfoContext & _context) const
{
	_context.log(TXT("box"));
#ifdef AN_DEVELOPMENT
	_context.on_last_log_select([this](){highlightDebugDraw = true; }, [this](){highlightDebugDraw = false; });
#endif
	LOG_INDENT(_context);
	_context.log(TXT("+- centre : %S"), centre.to_string().to_char());
	_context.log(TXT("+- halfSize : %S"), halfSize.to_string().to_char());
	//_context.log(TXT(" +- xAxis : %S"), xAxis.to_string().to_char());
	//_context.log(TXT(" +- yAxis : %S"), yAxis.to_string().to_char());
	//_context.log(TXT(" +- zAxis : %S"), zAxis.to_string().to_char());
}

static inline bool check_along_axis(Segment const & _segment, Vector3 const & _startDiff, Vector3 const & _endDiff, Vector3 const & _axis, float _halfSize, OUT_ float & _hit)
{
	float aStart = Vector3::dot(_startDiff, _axis);
	float aEnd = Vector3::dot(_endDiff, _axis);
	float aDiff = aEnd - aStart;
	if (aDiff != 0.0f)
	{
		float hit = ((aDiff > 0.0f ? -1.0f : 1.0f) * _halfSize - aStart) / aDiff;
		if (hit >= _segment.get_start_t() &&
			hit <= _segment.get_end_t())
		{
			_hit = hit;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool Box::check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increaseSize) const
{
	Vector3 startDiff = _segment.get_start() - centre;
	Vector3 endDiff = _segment.get_end() - centre;
	Vector3 const halfSizeIncreased = halfSize + Vector3(_increaseSize, _increaseSize, _increaseSize);

	// check all in order of earliest to furthest
	// this way even if we cross on one axis but don't end in box, we may check second or third axis
	float hitTs[3];
	hitTs[0] = 2.0f;
	hitTs[1] = 2.0f;
	hitTs[2] = 2.0f;
	check_along_axis(_segment, startDiff, endDiff, xAxis, halfSizeIncreased.x, hitTs[0]);
	check_along_axis(_segment, startDiff, endDiff, yAxis, halfSizeIncreased.y, hitTs[1]);
	check_along_axis(_segment, startDiff, endDiff, zAxis, halfSizeIncreased.z, hitTs[2]);
	order(hitTs[0], hitTs[1], hitTs[2]);
	// there's no need to check for < 0 because check_along_axis will drop such result
	if (hitTs[0] > 1.0f)
	{
		// maybe we started within box?
		hitTs[0] = _segment.get_start_t();
	}

	float const * hitTp = hitTs;
	for (int i = 0; i < 3; ++i, ++hitTp)
	{
		if (*hitTp > 1.0f)
		{
			break;
		}
		// check if hitT is within box
		// no need to check if hitT is before end - it is done in check along axis
		Vector3 hitLoc = _segment.get_at_t(*hitTp);
		Vector3 diff = hitLoc - centre;
		Vector3 diffLoc = Vector3(Vector3::dot(diff, xAxis),
								  Vector3::dot(diff, yAxis),
								  Vector3::dot(diff, zAxis));
		Vector3 absDiffLoc = Vector3(abs(diffLoc.x),
									 abs(diffLoc.y),
									 abs(diffLoc.z));
		Vector3 absDiffLoc_halfSize = absDiffLoc - halfSizeIncreased; // check if hit loc is within half size
		if (absDiffLoc_halfSize.x <= EPSILON &&
			absDiffLoc_halfSize.y <= EPSILON &&
			absDiffLoc_halfSize.z <= EPSILON)
		{
			Vector3 hitNormal;
			if (abs(absDiffLoc_halfSize.x) <= MARGIN)
			{
				hitNormal = diffLoc.x >= 0.0f ? xAxis : -xAxis;
			}
			else if (abs(absDiffLoc_halfSize.y) <= MARGIN)
			{
				hitNormal = diffLoc.y >= 0.0f ? yAxis : -yAxis;
			}
			else if (abs(absDiffLoc_halfSize.z) <= MARGIN)
			{
				hitNormal = diffLoc.z >= 0.0f ? zAxis : -zAxis;
			}
			else
			{
				// inside
				hitNormal = Vector3::zero;
			}
			if ((!_context || !_context->should_ignore_reversed_normal()) ||
				Vector3::dot(hitNormal, _segment.get_start_to_end()) < 0.0f)
			{
				_hitNormal = hitNormal;
				return _segment.collide_at(*hitTp);
			}
		}
	}
	return false;
}

bool Box::check_segment(Segment & _segment) const
{
	Vector3 startDiff = _segment.get_start() - centre;
	Vector3 endDiff = _segment.get_end() - centre;
	Vector3 const halfSizeIncreased = halfSize;

	// check all in order of earliest to furthest
	// this way even if we cross on one axis but don't end in box, we may check second or third axis
	float hitTs[3];
	hitTs[0] = 2.0f;
	hitTs[1] = 2.0f;
	hitTs[2] = 2.0f;
	check_along_axis(_segment, startDiff, endDiff, xAxis, halfSizeIncreased.x, hitTs[0]);
	check_along_axis(_segment, startDiff, endDiff, yAxis, halfSizeIncreased.y, hitTs[1]);
	check_along_axis(_segment, startDiff, endDiff, zAxis, halfSizeIncreased.z, hitTs[2]);
	order(hitTs[0], hitTs[1], hitTs[2]);
	// there's no need to check for < 0 because check_along_axis will drop such result
	if (hitTs[0] > 1.0f)
	{
		// maybe we started within box?
		hitTs[0] = _segment.get_start_t();
	}

	float const * hitTp = hitTs;
	for (int i = 0; i < 3; ++i, ++hitTp)
	{
		if (*hitTp > 1.0f)
		{
			break;
		}
		// check if hitT is within box
		// no need to check if hitT is before end - it is done in check along axis
		Vector3 hitLoc = _segment.get_at_t(*hitTp);
		Vector3 diff = hitLoc - centre;
		Vector3 diffLoc = Vector3(Vector3::dot(diff, xAxis),
								  Vector3::dot(diff, yAxis),
								  Vector3::dot(diff, zAxis));
		Vector3 absDiffLoc = Vector3(abs(diffLoc.x),
									 abs(diffLoc.y),
									 abs(diffLoc.z));
		Vector3 absDiffLoc_halfSize = absDiffLoc - halfSizeIncreased; // check if hit loc is within half size
		if (absDiffLoc_halfSize.x <= EPSILON &&
			absDiffLoc_halfSize.y <= EPSILON &&
			absDiffLoc_halfSize.z <= EPSILON)
		{
			return _segment.collide_at(*hitTp);
		}
	}
	return false;
}

bool Box::check_segment_detect_only(Segment const & _segment) const
{
	Vector3 startDiff = _segment.get_start() - centre;
	Vector3 endDiff = _segment.get_end() - centre;
	Vector3 const halfSizeIncreased = halfSize;

	// check all in order of earliest to furthest
	// this way even if we cross on one axis but don't end in box, we may check second or third axis
	float hitTs[3];
	hitTs[0] = 2.0f;
	hitTs[1] = 2.0f;
	hitTs[2] = 2.0f;
	check_along_axis(_segment, startDiff, endDiff, xAxis, halfSizeIncreased.x, hitTs[0]);
	check_along_axis(_segment, startDiff, endDiff, yAxis, halfSizeIncreased.y, hitTs[1]);
	check_along_axis(_segment, startDiff, endDiff, zAxis, halfSizeIncreased.z, hitTs[2]);
	order(hitTs[0], hitTs[1], hitTs[2]);
	// there's no need to check for < 0 because check_along_axis will drop such result
	if (hitTs[0] > 1.0f)
	{
		// maybe we started within box?
		hitTs[0] = _segment.get_start_t();
	}

	float const * hitTp = hitTs;
	for (int i = 0; i < 3; ++i, ++hitTp)
	{
		if (*hitTp > 1.0f)
		{
			break;
		}
		// check if hitT is within box
		// no need to check if hitT is before end - it is done in check along axis
		Vector3 hitLoc = _segment.get_at_t(*hitTp);
		Vector3 diff = hitLoc - centre;
		Vector3 diffLoc = Vector3(Vector3::dot(diff, xAxis),
								  Vector3::dot(diff, yAxis),
								  Vector3::dot(diff, zAxis));
		Vector3 absDiffLoc = Vector3(abs(diffLoc.x),
									 abs(diffLoc.y),
									 abs(diffLoc.z));
		Vector3 absDiffLoc_halfSize = absDiffLoc - halfSizeIncreased; // check if hit loc is within half size
		if (absDiffLoc_halfSize.x <= EPSILON &&
			absDiffLoc_halfSize.y <= EPSILON &&
			absDiffLoc_halfSize.z <= EPSILON)
		{
			return _segment.would_collide_at(*hitTp);
		}
	}
	return false;
}

void Box::apply_transform(Matrix44 const & _transform)
{
	if (_transform == Matrix44::identity)
	{
		return;
	}
	setup(_transform.location_to_world(centre),
		  halfSize,
		  _transform.vector_to_world(xAxis),
		  _transform.vector_to_world(yAxis));
}

void Box::apply_transform(Transform const & _transform)
{
	if (Transform::same_with_orientation(_transform, Transform::identity))
	{
		return;
	}
	setup(_transform.location_to_world(centre),
		halfSize,
		_transform.vector_to_world(xAxis),
		_transform.vector_to_world(yAxis));
}

void Box::create_from(::Meshes::Builders::IPU const & _ipu, int _startingAtPolygon, int _polygonCount)
{
	Range3 range = Range3::empty;
	_ipu.for_every_triangle([&range](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
	{
		range.include(_a);
		range.include(_b);
		range.include(_c);
	}
	, _startingAtPolygon, _polygonCount
	);

	setup(range);
}

Transform Box::get_placement() const
{
	return look_matrix(centre, yAxis, zAxis).to_transform();
}

Range3 Box::calculate_bounding_box(Transform const & _usingPlacement, bool _quickNotUsed) const
{
	Vector3 t_centre = _usingPlacement.location_to_world(centre);
	Vector3 t_xAxisHalfSize = _usingPlacement.vector_to_world(xAxis) * halfSize.x;
	Vector3 t_yAxisHalfSize = _usingPlacement.vector_to_world(yAxis) * halfSize.y;
	Vector3 t_zAxisHalfSize = _usingPlacement.vector_to_world(zAxis) * halfSize.z;
	Range3 boundingBox = Range3::empty;
	boundingBox.construct_from_location_and_axis_offsets(t_centre,
		-t_xAxisHalfSize, t_xAxisHalfSize,
		-t_yAxisHalfSize, t_yAxisHalfSize,
		-t_zAxisHalfSize, t_zAxisHalfSize);
	return boundingBox;
}

Vector3 Box::get_centre_offset_on_surface(Vector3 const& _localOffset) const
{
	Vector3 loc = _localOffset.normal();
	if (get_half_size().x != 0.0f)
	{
		loc.x /= get_half_size().x;
	}
	if (get_half_size().y != 0.0f)
	{
		loc.y /= get_half_size().y;
	}
	if (get_half_size().z != 0.0f)
	{
		loc.z /= get_half_size().z;
	}
	if (abs(loc.x) > 1.0f)
	{
		loc /= abs(loc.x);
	}
	if (abs(loc.y) > 1.0f)
	{
		loc /= abs(loc.y);
	}
	if (abs(loc.z) > 1.0f)
	{
		loc /= abs(loc.z);
	}

	return loc;
}

Vector3 Box::location_to_world(Vector3 const & _loc) const
{
	return centre + xAxis * _loc.x
				  + yAxis * _loc.y
				  + zAxis * _loc.z;
}

Vector3 Box::vector_to_world(Vector3 const & _loc) const
{
	return xAxis * _loc.x
		 + yAxis * _loc.y
		 + zAxis * _loc.z;
}

Vector3 Box::location_to_local(Vector3 const & _loc) const
{
	Vector3 loc = _loc - centre;
	return Vector3(Vector3::dot(xAxis, loc),
				   Vector3::dot(yAxis, loc),
				   Vector3::dot(zAxis, loc));
}

Vector3 Box::vector_to_local(Vector3 const & _loc) const
{
	return Vector3(Vector3::dot(xAxis, _loc),
				   Vector3::dot(yAxis, _loc),
				   Vector3::dot(zAxis, _loc));
}

float Box::calculate_outside_distance_local(Vector3 const & _local, OUT_ Vector3 * _dir) const
{
	float xOffsetOutsideBox;
	float yOffsetOutsideBox;
	float zOffsetOutsideBox;
	query_distance_axis(_local, xAxis, halfSize.x, OUT_ xOffsetOutsideBox);
	query_distance_axis(_local, yAxis, halfSize.y, OUT_ yOffsetOutsideBox);
	query_distance_axis(_local, zAxis, halfSize.z, OUT_ zOffsetOutsideBox);

	float const distanceSq = sqr(xOffsetOutsideBox) + sqr(yOffsetOutsideBox) + sqr(zOffsetOutsideBox);

	if (_dir)
	{
		*_dir = Vector3(xOffsetOutsideBox, yOffsetOutsideBox, zOffsetOutsideBox).normal();
	}

	return sqrt(distanceSq);
}

bool Box::does_overlap(Range3 const& _range) const
{
	// source: https://www.geometrictools.com/GTE/Mathematics/IntrAlignedBox3OrientedBox3.h

	// get the centered form of the range
	Vector3 c0 = _range.centre();
	Vector3 e0 = _range.length() * 0.5f;

	// Convenience variables.
	Vector3 const& c1 = get_centre();
	Vector3 const  a1[] = { get_x_axis(), get_y_axis(), get_z_axis() };
	Vector3 const& e1 = get_half_size();

	float cutoff = 1.0f - EPSILON;
	bool existsParallelPair = false;

	// Compute the difference of box centers.
	Vector3 d = c1 - c0;

	// dot01[i][j] = Vector3::dot(a0[i], a1[j]) = a1[j][i]
	float dot01[3][3];

	// |dot01[i][j]|
	float absDot01[3][3];

	// interval radii and distance between centers
	float r0, r1, r;

	// r0 + r1
	float r01;

	// Test for separation on the axis C0 + t*A0[0].
	for (int i = 0; i < 3; ++i)
	{
		dot01[0][i] = a1[i].x;
		absDot01[0][i] = abs(a1[i].x);
		if (absDot01[0][i] >= cutoff)
		{
			existsParallelPair = true;
		}
	}

	r = abs(d.x);
	r1 = e1.x * absDot01[0][0] + e1.y * absDot01[0][1] + e1.z * absDot01[0][2];
	r01 = e0.x + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[1].
	for (int i = 0; i < 3; ++i)
	{
		dot01[1][i] = a1[i].y;
		absDot01[1][i] = abs(a1[i].y);
		if (absDot01[1][i] >= cutoff)
		{
			existsParallelPair = true;
		}
	}
	r = abs(d.y);
	r1 = e1.x * absDot01[1][0] + e1.y * absDot01[1][1] + e1.z * absDot01[1][2];
	r01 = e0.y + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[2].
	for (int i = 0; i < 3; ++i)
	{
		dot01[2][i] = a1[i].z;
		absDot01[2][i] = abs(a1[i].z);
		if (absDot01[2][i] >= cutoff)
		{
			existsParallelPair = true;
		}
	}
	r = abs(d.z);
	r1 = e1.x * absDot01[2][0] + e1.y * absDot01[2][1] + e1.z * absDot01[2][2];
	r01 = e0.z + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A1[0].
	r = abs(Vector3::dot(d, a1[0]));
	r0 = e0.x * absDot01[0][0] + e0.y * absDot01[1][0] + e0.z * absDot01[2][0];
	r01 = r0 + e1.x;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A1[1].
	r = abs(Vector3::dot(d, a1[1]));
	r0 = e0.x * absDot01[0][1] + e0.y * absDot01[1][1] + e0.z * absDot01[2][1];
	r01 = r0 + e1.y;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A1[2].
	r = abs(Vector3::dot(d, a1[2]));
	r0 = e0.x * absDot01[0][2] + e0.y * absDot01[1][2] + e0.z * absDot01[2][2];
	r01 = r0 + e1.z;
	if (r > r01)
	{
		return false;
	}

	// At least one pair of box axes was parallel, so the separation is
	// effectively in 2D.  The edge-edge axes do not need to be tested.
	if (existsParallelPair)
	{
		return true;
	}

	// Test for separation on the axis C0 + t*A0[0]xA1[0].
	r = abs(d.z * dot01[1][0] - d.y * dot01[2][0]);
	r0 = e0.y * absDot01[2][0] + e0.z * absDot01[1][0];
	r1 = e1.y * absDot01[0][2] + e1.z * absDot01[0][1];
	r01 = r0 + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[0]xA1[1].
	r = abs(d.z * dot01[1][1] - d.y * dot01[2][1]);
	r0 = e0.y * absDot01[2][1] + e0.z * absDot01[1][1];
	r1 = e1.x * absDot01[0][2] + e1.z * absDot01[0][0];
	r01 = r0 + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[0]xA1[2].
	r = abs(d.z * dot01[1][2] - d.y * dot01[2][2]);
	r0 = e0.y * absDot01[2][2] + e0.z * absDot01[1][2];
	r1 = e1.x * absDot01[0][1] + e1.y * absDot01[0][0];
	r01 = r0 + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[1]xA1[0].
	r = abs(d.x * dot01[2][0] - d.z * dot01[0][0]);
	r0 = e0.x * absDot01[2][0] + e0.z * absDot01[0][0];
	r1 = e1.y * absDot01[1][2] + e1.z * absDot01[1][1];
	r01 = r0 + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[1]xA1[1].
	r = abs(d.x * dot01[2][1] - d.z * dot01[0][1]);
	r0 = e0.x * absDot01[2][1] + e0.z * absDot01[0][1];
	r1 = e1.x * absDot01[1][2] + e1.z * absDot01[1][0];
	r01 = r0 + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[1]xA1[2].
	r = abs(d.x * dot01[2][2] - d.z * dot01[0][2]);
	r0 = e0.x * absDot01[2][2] + e0.z * absDot01[0][2];
	r1 = e1.x * absDot01[1][1] + e1.y * absDot01[1][0];
	r01 = r0 + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[2]xA1[0].
	r = abs(d.y * dot01[0][0] - d.x * dot01[1][0]);
	r0 = e0.x * absDot01[1][0] + e0.y * absDot01[0][0];
	r1 = e1.y * absDot01[2][2] + e1.z * absDot01[2][1];
	r01 = r0 + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[2]xA1[1].
	r = abs(d.y * dot01[0][1] - d.x * dot01[1][1]);
	r0 = e0.x * absDot01[1][1] + e0.y * absDot01[0][1];
	r1 = e1.x * absDot01[2][2] + e1.z * absDot01[2][0];
	r01 = r0 + r1;
	if (r > r01)
	{
		return false;
	}

	// Test for separation on the axis C0 + t*A0[2]xA1[2].
	r = abs(d.y * dot01[0][2] - d.x * dot01[1][2]);
	r0 = e0.x * absDot01[1][2] + e0.y * absDot01[0][2];
	r1 = e1.x * absDot01[2][1] + e1.y * absDot01[2][0];
	r01 = r0 + r1;
	if (r > r01)
	{
		return false;
	}

	return true;
}

bool Box::does_overlap(Box const& _box) const
{
	// simplest way is to make _box local to this box and check if boxLocal(_box) overlaps range3 done with size of this
	Transform placement = get_placement();
	Box boxLocal;
	boxLocal.setup(placement.location_to_local(_box.centre), _box.halfSize, placement.vector_to_local(_box.xAxis), placement.vector_to_local(_box.yAxis));

	Range3 range;
	range.x = Range(-halfSize.x, halfSize.x);
	range.y = Range(-halfSize.y, halfSize.y);
	range.z = Range(-halfSize.z, halfSize.z);

	return boxLocal.does_overlap(range);
}

Vector3 Box::get_point_at_pt(Vector3 const& _pt) const
{
	Vector3 pt = (_pt - Vector3::half) * 2.0f;

	return centre
		+ xAxis * pt.x * halfSize.x
		+ yAxis * pt.y * halfSize.y
		+ zAxis * pt.z * halfSize.z;
}
