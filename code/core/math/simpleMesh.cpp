#include "math.h"
#include "..\io\xml.h"

#include "..\collision\checkCollisionContext.h"
#include "..\containers\arrayStack.h"
#include "..\debug\debugRenderer.h"
#include "..\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\mesh\mesh3d.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

SimpleMesh::SimpleMesh()
{
}

SimpleMesh::SimpleMesh(SimpleMesh const & _other)
{
	copy_from(_other);
}

SimpleMesh::~SimpleMesh()
{
}

void SimpleMesh::clear()
{
	range = Range3::empty;
	triangles.clear();
}

SimpleMesh const & SimpleMesh::operator = (SimpleMesh const & _other)
{
	copy_from(_other);
	return *this;
}

void SimpleMesh::copy_from(SimpleMesh const & _other)
{
	triangles = _other.triangles;
	range = _other.range;
	points = _other.points;
}

void SimpleMesh::make_space_for_triangles(int _count)
{
	triangles.make_space_for(_count);
	points.make_space_for(_count * 3);
}

void SimpleMesh::add(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
{
	if (Vector3::are_almost_equal(_a, _b) ||
		Vector3::are_almost_equal(_b, _c) ||
		Vector3::are_almost_equal(_c, _a))
	{
		// ignore if points are same(y)
		return;
	}
	Triangle tri(_a, _b, _c);
	if (tri.abIn.is_zero() ||
		tri.bcIn.is_zero() ||
		tri.caIn.is_zero())
	{
		// most likely are colinear
		return;
	}
	triangles.push_back(tri);
	points.push_back_unique(_a);
	points.push_back_unique(_b);
	points.push_back_unique(_c);
	range.include(_a);
	range.include(_b);
	range.include(_c);
}

void SimpleMesh::add_just_triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
{
	if (Vector3::are_almost_equal(_a, _b) ||
		Vector3::are_almost_equal(_b, _c) ||
		Vector3::are_almost_equal(_c, _a))
	{
		// ignore if points are same(y)
		return;
	}
	Triangle tri(_a, _b, _c);
	triangles.push_back(tri);
}

void SimpleMesh::build()
{
	range = Range3::empty;
	points.clear();
	points.make_space_for(triangles.get_size() * 3);
	for_every(tri, triangles)
	{
		tri->build();
		points.push_back_unique(tri->a);
		points.push_back_unique(tri->b);
		points.push_back_unique(tri->c);
		range.include(tri->a);
		range.include(tri->b);
		range.include(tri->c);
	}
	points.prune();
	triangles.prune();
}

bool SimpleMesh::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	bool result = true;
	clear();
	for_every(node, _node->children_named(TXT("triangle")))
	{
		Array<IO::XML::Node const*> pointNodes;
		if (node->get_children_named(pointNodes, TXT("point")) >= 3)
		{
			Vector3 p[3];
			for_count(int, i, 3)
			{
				result &= p[i].load_from_xml(pointNodes[i]);
			}
			add(p[0], p[1], p[2]);
		}
		else
		{
			result = false;
			error_loading_xml(node, TXT("not enough points for triangle"));
		}
	}
	return result;
}

void SimpleMesh::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const
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

	for_every(triangle, triangles)
	{
		debug_draw_triangle(_frontFill, fillColour, triangle->a, triangle->b, triangle->c);
		debug_draw_triangle_border(_frontFill, colour, triangle->a, triangle->b, triangle->c);
		{
			Vector3 centre = (triangle->a + triangle->b + triangle->c) / 3.0f;
			float size = min((triangle->a - triangle->b).length(),
						 min((triangle->b - triangle->c).length(),
							 (triangle->c - triangle->a).length()));
			debug_draw_arrow(_frontFill, colour, centre, centre + triangle->abc.normal * size * 0.5f);
		}
	}
#endif
}

void SimpleMesh::log(LogInfoContext & _context) const
{
	_context.log(TXT("mesh"));
#ifdef AN_DEVELOPMENT
	_context.on_last_log_select([this](){highlightDebugDraw = true; }, [this](){highlightDebugDraw = false; });
#endif
	LOG_INDENT(_context);
	_context.log(TXT("+- triangles count : %i"), triangles.get_size());
}

bool SimpleMesh::check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increasedSize) const
{
	bool hit = false;
	for_every(triangle, triangles)
	{
		Segment segmentTest = _segment;
		Vector3 segmentHitNormal;
		if (triangle->abc.check_segment(REF_ segmentTest, REF_ segmentHitNormal, _context, _increasedSize))
		{
			Vector3 hitLoc = segmentTest.get_hit();

			float const tre = 0.0f;
			if (triangle->abIn.get_in_front(hitLoc) >= tre &&
				triangle->bcIn.get_in_front(hitLoc) >= tre &&
				triangle->caIn.get_in_front(hitLoc) >= tre)
			{
				if ((!_context || !_context->should_ignore_reversed_normal()) ||
					Vector3::dot(segmentHitNormal, _segment.get_start_to_end()) < 0.0f)
				{
					if (_segment.collide_at(segmentTest.get_end_t()))
					{
						hit = true;
						_hitNormal = segmentHitNormal;
					}
				}
			}
		}
	}
	return hit;
}

bool SimpleMesh::get_triangle(int _idx, OUT_ Vector3 & _a, OUT_ Vector3 & _b, OUT_ Vector3 & _c) const
{
	if (! triangles.is_index_valid(_idx))
	{
		return false;
	}

	auto const & triangle = triangles[_idx];
	_a = triangle.a;
	_b = triangle.b;
	_c = triangle.c;

	return true;
}

void SimpleMesh::reverse_triangles()
{
	// range will remain the same
	for_every(triangle, triangles)
	{
		swap(triangle->a, triangle->c);
		triangle->build();
	}
}

void SimpleMesh::apply_transform(Matrix44 const & _transform)
{
	if (_transform == Matrix44::identity)
	{
		return;
	}
	range = Range3::empty;
	for_every(triangle, triangles)
	{
		triangle->a = _transform.location_to_world(triangle->a);
		triangle->b = _transform.location_to_world(triangle->b);
		triangle->c = _transform.location_to_world(triangle->c);
		triangle->build();
		range.include(triangle->a);
		range.include(triangle->b);
		range.include(triangle->c);
	}
}

Range3 SimpleMesh::calculate_bounding_box(Transform const & _usingPlacement, bool _quick) const
{
	Range3 boundingBox = Range3::empty;
	if (_quick)
	{
		boundingBox.construct_from_placement_and_range3(_usingPlacement, range);
	}
	else
	{
		for_every(triangle, triangles)
		{
			boundingBox.include(_usingPlacement.location_to_world(triangle->a));
			boundingBox.include(_usingPlacement.location_to_world(triangle->b));
			boundingBox.include(_usingPlacement.location_to_world(triangle->c));
		}
	}
	return boundingBox;
}

void SimpleMesh::create_from(::Meshes::Builders::IPU const & _ipu, int _startingAtPolygon, int _polygonCount)
{
	clear();
	_ipu.for_every_triangle([this](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
	{
		add(_a, _b, _c);
	}
	, _startingAtPolygon, _polygonCount
	);
}

void SimpleMesh::create_from(::Meshes::Mesh3D const * _mesh, OUT_ OPTIONAL_ int * _skinToBoneIdx)
{
	clear();
	int partIdx = 0;
	int skinToBoneIdx = NONE;
	while (auto const * part = _mesh->get_part(partIdx++))
	{
		part->for_every_triangle_and_simple_skinning([this, &skinToBoneIdx](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx)
		{
			add(_a, _b, _c);
			if (skinToBoneIdx == NONE)
			{
				skinToBoneIdx = _skinToBoneIdx;
			}
		});
	}
	assign_optional_out_param(_skinToBoneIdx, skinToBoneIdx);
}

void SimpleMesh::create_from(::Meshes::Mesh3DPart const * _meshPart, OUT_ OPTIONAL_ int * _skinToBoneIdx)
{
	clear();
	int skinToBoneIdx = NONE;
	_meshPart->for_every_triangle_and_simple_skinning([this, &skinToBoneIdx](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx)
	{
		add(_a, _b, _c);
		if (skinToBoneIdx == NONE)
		{
			skinToBoneIdx = _skinToBoneIdx;
		}
	});
	assign_optional_out_param(_skinToBoneIdx, skinToBoneIdx);
}

void SimpleMesh::split(Plane const& _plane, SimpleMesh& _behindIntoMesh, float _threshold)
{
	_threshold = abs(_threshold);
	Array<Triangle> sourceTriangles = triangles;

	triangles.clear();
	for_every(t, sourceTriangles)
	{
		float aIF = _plane.get_in_front(t->a);
		float bIF = _plane.get_in_front(t->b);
		float cIF = _plane.get_in_front(t->c);
		if (aIF <= _threshold &&
			bIF <= _threshold &&
			cIF <= _threshold)
		{
			_behindIntoMesh.add_just_triangle(t->a, t->b, t->c);
			continue;
		}
		if (aIF >= -_threshold &&
			bIF >= -_threshold &&
			cIF >= -_threshold)
		{
			add_just_triangle(t->a, t->b, t->c);
			continue;
		}
		ArrayStatic<Vector3, 8> inFront;
		ArrayStatic<Vector3, 8> inBack;

		// On PLane
		int aOP = abs(aIF) <= _threshold? 1 : 0;
		int bOP = abs(bIF) <= _threshold? 1 : 0;
		int cOP = abs(cIF) <= _threshold? 1 : 0;
		if (aOP && bOP && cOP)
		{
			// all on plane, just put them in "inFront"
			inFront.push_back(t->a); inFront.push_back(t->b); inFront.push_back(t->c);
		}
		else if (aOP + bOP + cOP == 2)
		{
			// two points on plane
			// depending on the third one choose one side
			if ((aOP + bOP == 2 && cIF > 0.0f) ||
				(aOP + cOP == 2 && bIF > 0.0f) ||
				(bOP + cOP == 2 && aIF > 0.0f))
			{
				inFront.push_back(t->a); inFront.push_back(t->b); inFront.push_back(t->c);
			}
			else
			{
				inBack.push_back(t->a); inBack.push_back(t->b); inBack.push_back(t->c);
			}
		}
		else if (aOP + bOP + cOP == 1)
		{
			// one on plane, if two on one side, add to that side
			if ((aOP && bIF > 0.0f && cIF > 0.0f) ||
				(bOP && aIF > 0.0f && cIF > 0.0f) ||
				(cOP && aIF > 0.0f && bIF > 0.0f))
			{
				inFront.push_back(t->a); inFront.push_back(t->b); inFront.push_back(t->c);
			}
			else if ((aOP && bIF < 0.0f && cIF < 0.0f) ||
					 (bOP && aIF < 0.0f && cIF < 0.0f) ||
					 (cOP && aIF < 0.0f && bIF < 0.0f))
			{
				inBack.push_back(t->a); inBack.push_back(t->b); inBack.push_back(t->c);
			}
			else
			{
				// we need to cut, in the result we will have two triangles
				// rearrange so the first point is on the plane
				ArrayStatic<Vector3, 3> p;
				if (aOP) { p.push_back(t->a); p.push_back(t->b); p.push_back(t->c); }
				if (bOP) { p.push_back(t->b); p.push_back(t->c); p.push_back(t->a); }
				if (cOP) { p.push_back(t->c); p.push_back(t->a); p.push_back(t->b); }
				an_assert(!p.is_empty());

				Segment p12(p[1], p[2]);
				float t = _plane.calc_intersection_t(p12);
				Vector3 it = p12.get_at_t(t);

				if (_plane.get_in_front(p[1]) > 0.0f)
				{
					inFront.push_back(p[0]); inFront.push_back(p[1]); inFront.push_back(it);
					inBack.push_back(it); inBack.push_back(p[2]); inBack.push_back(p[0]);
				}
				else 
				{
					inBack.push_back(p[0]); inBack.push_back(p[1]); inBack.push_back(it);
					inFront.push_back(it); inFront.push_back(p[2]); inFront.push_back(p[0]);
				}
			}
		}
		else
		{
			ArrayStatic<Vector3, 3> ps;
			ps.push_back(t->a);
			ps.push_back(t->b);
			ps.push_back(t->c);
			Vector3 pp = ps[2];
			for_every(cp, ps)
			{
				float cpIF = _plane.get_in_front(*cp);
				float ppIF = _plane.get_in_front(pp);
				if (cpIF * ppIF < 0.0f)
				{
					Segment s(pp, *cp);
					float t = _plane.calc_intersection_t(s);
					Vector3 it = s.get_at_t(t);
					inFront.push_back(it);
					inBack.push_back(it);
				}
				// all are not on the plane, we may cut them as we wish (if it makes sense though)
				if (cpIF > 0.0f) { inFront.push_back(*cp); }
							else { inBack.push_back(*cp); }
				pp = *cp;
			}
		}
		an_assert(inFront.is_empty() || inFront.get_size() >= 3);
		an_assert(inBack.is_empty() || inBack.get_size() >= 3);

		for (int i = 1; i < inFront.get_size() - 1; ++i)
		{
			add_just_triangle(inFront[0], inFront[i], inFront[i + 1]);
		}

		for (int i = 1; i < inBack.get_size() - 1; ++i)
		{
			_behindIntoMesh.add_just_triangle(inBack[0], inBack[i], inBack[i + 1]);
		}
	}

	build();
	_behindIntoMesh.build();
}

void SimpleMesh::convexify(std::function<SimpleMesh* (int _idx)> get_mesh)
{
	Array<Triangle> sourceTriangles = triangles;

	Array<SimpleMesh*> meshes; // filled with get_mesh
	triangles.clear();

	float const threshold = 0.000001f;
	for_every(t, sourceTriangles)
	{
		if (Vector3::are_almost_equal(t->a, t->b) ||
			Vector3::are_almost_equal(t->b, t->c) ||
			Vector3::are_almost_equal(t->c, t->a))
		{
			// ignore if points are same(y)
			continue;
		}
		Optional<int> fineWith;
		for_every_ptr(m, meshes)
		{
			bool willBeConvex = true;
			for_every(mt, m->triangles)
			{
				if (t->abc.get_in_front(mt->a) <= threshold &&
					t->abc.get_in_front(mt->b) <= threshold &&
					t->abc.get_in_front(mt->c) <= threshold &&
					mt->abc.get_in_front(t->a) <= threshold &&
					mt->abc.get_in_front(t->b) <= threshold &&
					mt->abc.get_in_front(t->c) <= threshold)
				{
					continue;
				}
				else
				{
					willBeConvex = false;
					break;
				}
			}
			if (willBeConvex)
			{
				fineWith = for_everys_index(m);
				break;
			}
		}
		if (!fineWith.is_set())
		{
			fineWith = meshes.get_size();
			meshes.push_back(get_mesh(meshes.get_size()));
			// restore pointers to valid meshes (as the array could grow)
			for_count(int, i, meshes.get_size())
			{
				meshes[i] = get_mesh(i);
			}
		}
		meshes[fineWith.get()]->add_just_triangle(t->a, t->b, t->c);
	}

	build(); // to be empty
	for_every_ptr(m, meshes)
	{
		m->build();
	}
}

float SimpleMesh::calculate_outside_box_distance(Vector3 const& _loc) const
{
	float xOffsetOutsideBox = range.x.distance_from(_loc.x);
	float yOffsetOutsideBox = range.y.distance_from(_loc.y);
	float zOffsetOutsideBox = range.z.distance_from(_loc.z);

	float const distanceSq = sqr(xOffsetOutsideBox) + sqr(yOffsetOutsideBox) + sqr(zOffsetOutsideBox);

	return sqrt(distanceSq);
}

//

SimpleMesh::Triangle::Triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
	: a(_a)
	, b(_b)
	, c(_c)
{
	build();
}

void SimpleMesh::Triangle::build()
{
	abc.set(a, b, c);
	a2bDist = (b - a).length();
	b2cDist = (c - b).length();
	c2aDist = (a - c).length();
	a2b = (b - a).normal();
	Vector3 a2c = (c - a).normal();
	Vector3 b2a = (a - b).normal();
	b2c = (c - b).normal();
	c2a = (a - c).normal();
	Vector3 c2b = (b - c).normal();
	// xy->z
	// zx->y
	// calculate normals for each edge towards other point
	Vector3 nab = Vector3::cross(Vector3::cross(a2b, a2c), a2b).normal();
	Vector3 nbc = Vector3::cross(Vector3::cross(b2c, b2a), b2c).normal();
	Vector3 nca = Vector3::cross(Vector3::cross(c2a, c2b), c2a).normal();
	abIn.set(nab, a);
	bcIn.set(nbc, b);
	caIn.set(nca, c);
	an_assert(!abIn.get_normal().is_zero());
	an_assert(!bcIn.get_normal().is_zero());
	an_assert(!caIn.get_normal().is_zero());
}

bool SimpleMesh::Triangle::does_share_edge_with(Triangle const & _tri) const
{
	todo_note(TXT("this is fine but maybe we should rely on checking if equal only?"));
	float threshold = 0.00001f;
																														// a-b (this)
	if (Vector3::are_almost_equal(a, _tri.a, threshold) && Vector3::are_almost_equal(b, _tri.c, threshold)) return true;	// a-c
	if (Vector3::are_almost_equal(a, _tri.b, threshold) && Vector3::are_almost_equal(b, _tri.a, threshold)) return true;	// b-a
	if (Vector3::are_almost_equal(a, _tri.c, threshold) && Vector3::are_almost_equal(b, _tri.b, threshold)) return true;	// c-b
																														// b-c (this)
	if (Vector3::are_almost_equal(b, _tri.a, threshold) && Vector3::are_almost_equal(c, _tri.c, threshold)) return true;	// a-c
	if (Vector3::are_almost_equal(b, _tri.b, threshold) && Vector3::are_almost_equal(c, _tri.a, threshold)) return true;	// b-a
	if (Vector3::are_almost_equal(b, _tri.c, threshold) && Vector3::are_almost_equal(c, _tri.b, threshold)) return true;	// c-b
																														// c-a (this)
	if (Vector3::are_almost_equal(c, _tri.a, threshold) && Vector3::are_almost_equal(a, _tri.c, threshold)) return true;	// a-c
	if (Vector3::are_almost_equal(c, _tri.b, threshold) && Vector3::are_almost_equal(a, _tri.a, threshold)) return true;	// b-a
	if (Vector3::are_almost_equal(c, _tri.c, threshold) && Vector3::are_almost_equal(a, _tri.b, threshold)) return true;	// c-b

	return false;
}

bool SimpleMesh::Triangle::has_same_edge_with(Triangle const & _tri) const
{
	todo_note(TXT("this is fine but maybe we should rely on checking if equal only?"));
	float threshold = 0.00001f;
																														// a-b (this)
	if (Vector3::are_almost_equal(a, _tri.a, threshold) && Vector3::are_almost_equal(b, _tri.b, threshold)) return true;	// a-b
	if (Vector3::are_almost_equal(a, _tri.b, threshold) && Vector3::are_almost_equal(b, _tri.c, threshold)) return true;	// b-c
	if (Vector3::are_almost_equal(a, _tri.c, threshold) && Vector3::are_almost_equal(b, _tri.a, threshold)) return true;	// c-a
																														// b-c (this)
	if (Vector3::are_almost_equal(b, _tri.a, threshold) && Vector3::are_almost_equal(c, _tri.b, threshold)) return true;	// a-b
	if (Vector3::are_almost_equal(b, _tri.b, threshold) && Vector3::are_almost_equal(c, _tri.c, threshold)) return true;	// b-c
	if (Vector3::are_almost_equal(b, _tri.c, threshold) && Vector3::are_almost_equal(c, _tri.a, threshold)) return true;	// c-a
																														// c-a (this)
	if (Vector3::are_almost_equal(c, _tri.a, threshold) && Vector3::are_almost_equal(a, _tri.b, threshold)) return true;	// a-b
	if (Vector3::are_almost_equal(c, _tri.b, threshold) && Vector3::are_almost_equal(a, _tri.c, threshold)) return true;	// b-c
	if (Vector3::are_almost_equal(c, _tri.c, threshold) && Vector3::are_almost_equal(a, _tri.a, threshold)) return true;	// c-a
	return false;
}
