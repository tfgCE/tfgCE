#include "math.h"
#include "..\io\xml.h"

#include "..\collision\checkCollisionContext.h"
#include "..\containers\arrayStack.h"
#include "..\debug\debugRenderer.h"
#include "..\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\mesh\mesh3d.h"
#include "..\mesh\skeleton.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

SkinnedMesh::SkinnedMesh()
{
}

SkinnedMesh::SkinnedMesh(SkinnedMesh const & _other)
{
	copy_from(_other);
}

SkinnedMesh::~SkinnedMesh()
{
}

void SkinnedMesh::clear()
{
	range = Range3::empty;
	triangles.clear();
	points.clear();
}

SkinnedMesh const & SkinnedMesh::operator = (SkinnedMesh const & _other)
{
	copy_from(_other);
	return *this;
}

void SkinnedMesh::copy_from(SkinnedMesh const & _other)
{
	triangles = _other.triangles;
	points = _other.points;
	range = _other.range;
}

void SkinnedMesh::build()
{
	range = Range3::empty;
	for_every(point, points)
	{
		range.include(point->location);
	}
	triangles.prune();
	points.prune();
}

bool SkinnedMesh::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	bool result = false;
	error_loading_xml(_node, TXT("not yet supported"));
	return result;
}

void SkinnedMesh::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Array<Matrix44> const & _defaultInvertedBones, Array<Matrix44> const & _bones) const
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

	if (!_defaultInvertedBones.is_empty() && ! _bones.is_empty())
	{
		ARRAY_PREALLOC_SIZE(Vector3, locations, points.get_size());
		for_every(point, points)
		{
			locations.push_back(point->calculate_location(_defaultInvertedBones, _bones));
		}
		for_every(triangle, triangles)
		{
			debug_draw_triangle(_frontFill, fillColour, locations[triangle->a], locations[triangle->b], locations[triangle->c]);
			debug_draw_triangle_border(_frontFill, colour, locations[triangle->a], locations[triangle->b], locations[triangle->c]);
		}
		for_every(bone, _bones)
		{
			debug_draw_matrix_size(true, *bone, 0.1f);
		}
	}
	else
	{
		ARRAY_PREALLOC_SIZE(Vector3, locations, points.get_size());
		for_every(point, points)
		{
			locations.push_back(point->location);
		}
		for_every(triangle, triangles)
		{
			debug_draw_triangle(_frontFill, fillColour, locations[triangle->a], locations[triangle->b], locations[triangle->c]);
			debug_draw_triangle_border(_frontFill, colour, locations[triangle->a], locations[triangle->b], locations[triangle->c]);
		}
	}
#endif
}

void SkinnedMesh::log(LogInfoContext & _context) const
{
	_context.log(TXT("mesh"));
#ifdef AN_DEVELOPMENT
	_context.on_last_log_select([this](){highlightDebugDraw = true; }, [this](){highlightDebugDraw = false; });
#endif
	LOG_INDENT(_context);
	_context.log(TXT("+- triangles count : %i"), triangles.get_size());
}

bool SkinnedMesh::check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, Array<Matrix44> const & _defaultInvertedBones, Array<Matrix44> const & _bones, float _increasedSize) const
{
	bool hit = false;
	ARRAY_STACK(Vector3, locations, points.get_size());
	for_every(point, points)
	{
		locations.push_back(point->calculate_location(_defaultInvertedBones, _bones));
	}
	for_every(triangle, triangles)
	{
		Segment segmentTest = _segment;
		Vector3 segmentHitNormal;
		Vector3 a = locations[triangle->a];
		Vector3 b = locations[triangle->b];
		Vector3 c = locations[triangle->c];
		if (Plane(a, b, c).check_segment(REF_ segmentTest, REF_ segmentHitNormal, _context, _increasedSize))
		{
			Vector3 hitLoc = segmentTest.get_hit();

			float const tre = 0.0f;

			// based on simple mesh
			Vector3 a2b = (b - a);
			Vector3 a2c = (c - a);
			Vector3 b2a = (a - b);
			Vector3 b2c = (c - b);
			Vector3 c2a = (a - c);
			Vector3 c2b = (b - c);

			if (Plane(Vector3::cross(Vector3::cross(a2b, a2c), a2b).normal(), a).get_in_front(hitLoc) >= tre &&
				Plane(Vector3::cross(Vector3::cross(b2c, b2a), b2c).normal(), b).get_in_front(hitLoc) >= tre &&
				Plane(Vector3::cross(Vector3::cross(c2a, c2b), c2a).normal(), c).get_in_front(hitLoc) >= tre)
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

void SkinnedMesh::reverse_triangles()
{
	for_every(triangle, triangles)
	{
		swap(triangle->a, triangle->c);
	}
}

void SkinnedMesh::apply_transform(Matrix44 const & _transform)
{
	if (_transform == Matrix44::identity)
	{
		return;
	}
	range = Range3::empty;
	for_every(point, points)
	{
		point->location = _transform.location_to_world(point->location);
		range.include(point->location);
	}
}

Range3 SkinnedMesh::calculate_bounding_box(Transform const & _usingPlacement, Array<Transform> const & _defaultInvertedBones, Array<Transform> const & _bones, bool _quick) const
{
	Range3 result = Range3::empty;
	for_every(point, points)
	{
		Vector3 location = _usingPlacement.location_to_world(point->calculate_location(_defaultInvertedBones, _bones));
		result.include(location);
	}
	return result;
}

void SkinnedMesh::create_from(::Meshes::Builders::IPU const & _ipu, int _startingAtPolygon, int _polygonCount)
{
	clear();
	_ipu.for_every_triangle_and_full_skinning([this](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, Meshes::VertexSkinningInfo const & _aSkinning, Meshes::VertexSkinningInfo const & _bSkinning, Meshes::VertexSkinningInfo const & _cSkinning)
	{
		add(_a, _b, _c, _aSkinning, _bSkinning, _cSkinning);
	}
	, _startingAtPolygon, _polygonCount
	);
}

void SkinnedMesh::create_from(::Meshes::Mesh3D const * _mesh, OUT_ OPTIONAL_ int * _skinToBoneIdx)
{
	clear();
	int partIdx = 0;
	while (auto const * part = _mesh->get_part(partIdx++))
	{
		part->for_every_triangle_and_full_skinning([this](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, Meshes::VertexSkinningInfo const & _aSkinning, Meshes::VertexSkinningInfo const & _bSkinning, Meshes::VertexSkinningInfo const & _cSkinning)
		{
			add(_a, _b, _c, _aSkinning, _bSkinning, _cSkinning);
		});
	}
	assign_optional_out_param(_skinToBoneIdx, NONE);
}

void SkinnedMesh::create_from(::Meshes::Mesh3DPart const * _meshPart, OUT_ OPTIONAL_ int * _skinToBoneIdx)
{
	clear();
	_meshPart->for_every_triangle_and_full_skinning([this](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, Meshes::VertexSkinningInfo const & _aSkinning, Meshes::VertexSkinningInfo const & _bSkinning, Meshes::VertexSkinningInfo const & _cSkinning)
	{
		add(_a, _b, _c, _aSkinning, _bSkinning, _cSkinning);
	});
	assign_optional_out_param(_skinToBoneIdx, NONE);
}

void SkinnedMesh::add(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, Meshes::VertexSkinningInfo const & _aSkinning, Meshes::VertexSkinningInfo const & _bSkinning, Meshes::VertexSkinningInfo const & _cSkinning)
{
	add(Point().setup_with(_a, _aSkinning),
		Point().setup_with(_b, _bSkinning),
		Point().setup_with(_c, _cSkinning));
}

void SkinnedMesh::add(Point const & _a, Point const & _b, Point const & _c)
{
	Triangle t;
	t.a = add(_a);
	t.b = add(_b);
	t.c = add(_c);
	triangles.push_back(t);
	range.include(_a.location);
	range.include(_b.location);
	range.include(_c.location);
}

int SkinnedMesh::add(Point const & _p)
{
	for_every(point, points)
	{
		if (_p == *point)
		{
			return for_everys_index(point);
		}
	}
	points.push_back(_p);
	return points.get_size() - 1;
}

bool SkinnedMesh::update_for(Meshes::Skeleton const * _skeleton)
{
	bool result = true;
	for_every(point, points)
	{
		result &= point->update_for(_skeleton);
	}
	return result;
}

void SkinnedMesh::split(Plane const& _plane, SkinnedMesh& _behindIntoMesh, float _threshold)
{
	_threshold = abs(_threshold);
	Array<Point> sourcePoints = points;
	Array<Triangle> sourceTriangles = triangles;

	points.clear();
	triangles.clear();
	range = Range3::empty;
	for_every(t, sourceTriangles)
	{
		auto& spa = sourcePoints[t->a];
		auto& spb = sourcePoints[t->b];
		auto& spc = sourcePoints[t->c];
		float aIF = _plane.get_in_front(spa.location);
		float bIF = _plane.get_in_front(spb.location);
		float cIF = _plane.get_in_front(spc.location);
		if (aIF <= _threshold &&
			bIF <= _threshold &&
			cIF <= _threshold)
		{
			_behindIntoMesh.add(spa, spb, spc);
			continue;
		}
		if (aIF >= -_threshold &&
			bIF >= -_threshold &&
			cIF >= -_threshold)
		{
			add(spa, spb, spc);
			continue;
		}
		ArrayStatic<Point, 8> inFront;
		ArrayStatic<Point, 8> inBack;

		// On PLane
		int aOP = abs(aIF) <= _threshold ? 1 : 0;
		int bOP = abs(bIF) <= _threshold ? 1 : 0;
		int cOP = abs(cIF) <= _threshold ? 1 : 0;
		if (aOP && bOP && cOP)
		{
			// all on plane, just put them in "inFront"
			inFront.push_back(spa); inFront.push_back(spb); inFront.push_back(spc);
		}
		else if (aOP + bOP + cOP == 2)
		{
			// two points on plane
			// depending on the third one choose one side
			if ((aOP + bOP == 2 && cIF > 0.0f) ||
				(aOP + cOP == 2 && bIF > 0.0f) ||
				(bOP + cOP == 2 && aIF > 0.0f))
			{
				inFront.push_back(spa); inFront.push_back(spb); inFront.push_back(spc);
			}
			else
			{
				inBack.push_back(spa); inBack.push_back(spb); inBack.push_back(spc);
			}
		}
		else if (aOP + bOP + cOP == 1)
		{
			// one on plane, if two on one side, add to that side
			if ((aOP && bIF > 0.0f && cIF > 0.0f) ||
				(bOP && aIF > 0.0f && cIF > 0.0f) ||
				(cOP && aIF > 0.0f && bIF > 0.0f))
			{
				inFront.push_back(spa); inFront.push_back(spb); inFront.push_back(spc);
			}
			else if ((aOP && bIF < 0.0f && cIF < 0.0f) ||
				(bOP && aIF < 0.0f && cIF < 0.0f) ||
				(cOP && aIF < 0.0f && bIF < 0.0f))
			{
				inBack.push_back(spa); inBack.push_back(spb); inBack.push_back(spc);
			}
			else
			{
				// we need to cut, in the result we will have two triangles
				// rearrange so the first point is on the plane
				ArrayStatic<Point, 3> p;
				if (aOP) { p.push_back(spa); p.push_back(spb); p.push_back(spc); }
				if (bOP) { p.push_back(spb); p.push_back(spc); p.push_back(spa); }
				if (cOP) { p.push_back(spc); p.push_back(spa); p.push_back(spb); }
				an_assert(!p.is_empty());

				Segment p12(p[1].location, p[2].location);
				float t = _plane.calc_intersection_t(p12);
				Point it;
				it.setup_with(p12.get_at_t(t), Meshes::VertexSkinningInfo::blend(p[1].to_skinning_info(), p[2].to_skinning_info(), t));

				if (_plane.get_in_front(p[1].location) > 0.0f)
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
			ArrayStatic<Point, 3> ps;
			ps.push_back(spa);
			ps.push_back(spb);
			ps.push_back(spc);
			Point pp = ps[2];
			for_every(cp, ps)
			{
				float cpIF = _plane.get_in_front(cp->location);
				float ppIF = _plane.get_in_front(pp.location);
				if (cpIF * ppIF < 0.0f)
				{
					Segment s(pp.location, cp->location);
					float t = _plane.calc_intersection_t(s);
					Point it;
					it.setup_with(s.get_at_t(t), Meshes::VertexSkinningInfo::blend(pp.to_skinning_info(), cp->to_skinning_info(), t)); 
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
			add(inFront[0], inFront[i], inFront[i + 1]);
		}

		for (int i = 1; i < inBack.get_size() - 1; ++i)
		{
			_behindIntoMesh.add(inBack[0], inBack[i], inBack[i + 1]);
		}
	}

	build();
	_behindIntoMesh.build();
}

//

SkinnedMesh::Point::Point()
{
	for_count(int, i, get_MAX_BONE())
	{
		bones[i] = Name::invalid();
		boneIndices[i] = 0;
		weights[i] = 0.0f;
	}
}

SkinnedMesh::Point & SkinnedMesh::Point::setup_with(Vector3 const & _loc, Meshes::VertexSkinningInfo const & _skinningInfo)
{
	location = _loc;
	int i = 0;
	while (i < min(get_MAX_BONE(), _skinningInfo.bones.get_size()))
	{
		bones[i] = Name::invalid();
		boneIndices[i] = _skinningInfo.bones[i].bone;
		weights[i] = _skinningInfo.bones[i].weight;
		++i;
	}
	// clear rest
	while (i < get_MAX_BONE())
	{
		bones[i] = Name::invalid();
		boneIndices[i] = 0;
		weights[i] = 0.0f;
		++i;
	}
	return *this;
}

Vector3 SkinnedMesh::Point::calculate_location(Array<Matrix44> const & _defaultInvertedBones, Array<Matrix44> const & _bones) const
{
	Vector3 result = Vector3::zero;
	int const * boneIndex = boneIndices;
	float const * weight = weights;
#ifdef AN_DEVELOPMENT
	float sumWeight = 0.0f;
#endif
	for_count(int, i, get_MAX_BONE())
	{
		if (*weight > 0.0f)
		{
			result += _bones[*boneIndex].location_to_world(_defaultInvertedBones[*boneIndex].location_to_world(location)) * *weight;
#ifdef AN_DEVELOPMENT
			sumWeight += *weight;
#endif
		}
		++boneIndex;
		++weight;
	}
#ifdef AN_DEVELOPMENT
	an_assert(abs(sumWeight - 1.0f) < 0.01f);
#endif
	return result;
}

Vector3 SkinnedMesh::Point::calculate_location(Array<Transform> const & _defaultInvertedBones, Array<Transform> const & _bones) const
{
	Vector3 result = Vector3::zero;
	int const * boneIndex = boneIndices;
	float const * weight = weights;
	for_count(int, i, get_MAX_BONE())
	{
		if (*weight > 0.0f)
		{
			result += _bones[*boneIndex].location_to_world(_defaultInvertedBones[*boneIndex].location_to_world(location)) * *weight;
		}
		++boneIndex;
		++weight;
	}
	return result;
}

Meshes::VertexSkinningInfo SkinnedMesh::Point::to_skinning_info() const
{
	Meshes::VertexSkinningInfo vsi;
	for_count(int, i, MAX_BONE)
	{
		if (weights[i] > 0.0f)
		{
			vsi.add(Meshes::VertexSkinningInfo::Bone(boneIndices[i], weights[i]));
		}
	}
	return vsi;
}

bool SkinnedMesh::Point::update_for(Meshes::Skeleton const * _skeleton)
{
	bool result = true;
	for_count(int, i, get_MAX_BONE())
	{
		if (bones[i].is_valid() && weights[i] > 0.0f)
		{
			boneIndices[i] = _skeleton->find_bone_index(bones[i]);
			if (boneIndices[i] == NONE)
			{
				error(TXT("could not find bone \"%S\""), bones[i].to_char());
				result = false;
			}
		}
	}
	return result;
}

bool SkinnedMesh::Point::operator==(Point const & _other) const
{
	if (location != _other.location)
	{
		return false;
	}

	int const * boneIndex = boneIndices;
	float const * weight = weights;
	int const * otherBoneIndex = _other.boneIndices;
	float const * otherWeight = _other.weights;
	for_count(int, i, get_MAX_BONE())
	{
		if ((*weight >= 0.0f || *otherWeight >= 0.0f) &&
			(*boneIndex != *otherBoneIndex ||
			*weight != *otherWeight))
		{
			return false;
		}
	}

	return true;
}
