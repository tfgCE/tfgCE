#include "mesh3dBuilder_IPU.h"

#include "..\..\containers\arrayStack.h"
#include "..\..\system\video\indexFormat.h"
#include "..\..\system\video\indexFormatUtils.h"
#include "..\..\system\video\vertexFormat.h"
#include "..\..\system\video\vertexFormatUtils.h"
#include "..\..\types\vectorPacked.h"

#include "..\..\debug\debugVisualiser.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define AN_ASSERT_ON_POLYGON_NORMAL_ISSUES
#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define AN_INFO_ON_POLYGON_NORMAL_ISSUES
#endif
//#define AN_INVESTIGATE_POLYGON_NORMAL_ISSUES
#define AN_FIX_POLYGON_NORMAL_ISSUES

#ifdef AN_INVESTIGATE_POLYGON_NORMAL_ISSUES
#ifdef AN_FIX_POLYGON_NORMAL_ISSUES
#undef AN_FIX_POLYGON_NORMAL_ISSUES
#endif
#endif


//

using namespace Meshes;
using namespace Builders;

//

IPU::IPU()
{
	will_need_at_least_points(1000);
}

void IPU::map_u_to_colour(float _u, Colour const& _colour)
{
	MapUToColour m;
	m.u = _u;
	m.colour = _colour;
	mapUToColour.push_back(m);
}

int IPU::add_point(Vector3 const & _point, Optional<Vector2> const& _uv, VertexSkinningInfo const * _vsi)
{
	Point point;
	point.point = _point;
#ifdef WITH_UV_SUPPORT
	point.uv = _uv.get(Vector2::zero);
#endif
	if (_vsi && ! _vsi->is_empty())
	{
		point.skinningInfo = *_vsi;
	}
	points.push_back(point);
	return points.get_size() - 1;
}

int IPU::add_triangle(float _u, int _a, int _b, int _c, bool _reverse)
{
	if (_reverse)
	{
		swap(_b, _c);
	}
	an_assert(_a >= 0 && _a < points.get_size());
	an_assert(_b >= 0 && _b < points.get_size());
	an_assert(_c >= 0 && _c < points.get_size());
	Polygon polygon;
	polygon.u = _u;
	polygon.colour = currentColour;
	polygon.forceMaterialIdx = currentForceMaterialIdx;
	polygon.indices.set_size(3);
	polygon.indices[0] = _a;
	polygon.indices[1] = _b;
	polygon.indices[2] = _c;
	polygons.push_back(polygon);
	assert_slow(is_valid());
	return polygons.get_size() - 1;
}

int IPU::add_quad(float _u, int _a, int _b, int _c, int _d, bool _reverse)
{
	if (_reverse)
	{
		swap(_b, _d);
	}
	an_assert(_a >= 0 && _a < points.get_size());
	an_assert(_b >= 0 && _b < points.get_size());
	an_assert(_c >= 0 && _c < points.get_size());
	an_assert(_d >= 0 && _d < points.get_size());
	Polygon polygon;
	polygon.u = _u;
	polygon.colour = currentColour;
	polygon.forceMaterialIdx = currentForceMaterialIdx;
	polygon.indices.set_size(4);
	polygon.indices[0] = _a;
	polygon.indices[1] = _b;
	polygon.indices[2] = _c;
	polygon.indices[3] = _d;
	polygons.push_back(polygon);
	assert_slow(is_valid());
	return polygons.get_size() - 1;
}

void IPU::weld_points()
{
	if (weldingDistance < 0.0f)
	{
		return;
	}

	Array<bool> indicesToRemove;
	Array<int> remapIndices;
	indicesToRemove.set_size(points.get_size());
	remapIndices.set_size(points.get_size());
	for (int a = 0; a < points.get_size(); ++a)
	{
		indicesToRemove[a] = false;
		remapIndices[a] = a;
	}

	float weldingDistanceSquared = sqr(weldingDistance);
	for (int a = 0; a < points.get_size(); ++a)
	{
		for (int b = 0; b < a; ++b)
		{
			if ((points[a].point - points[b].point).length_squared() <= weldingDistanceSquared &&
				Point::can_be_welded(points[a], points[b]))
			{
				indicesToRemove[a] = true;
				remapIndices[a] = b;
				break;
			}
		}
	}

	// remap those that stay, we will have full list of indices afterwards
	// if it is removed, we may safely use new index
	// and remove actual points here!
	int removedCount = 0;
	for (int remapIdx = 0, pointIdx = 0; remapIdx < remapIndices.get_size(); ++remapIdx, ++pointIdx)
	{
		if (indicesToRemove[remapIdx])
		{
			// we now point at index before removal but it could already move. do that here
			remapIndices[remapIdx] = remapIndices[remapIndices[remapIdx]];
			// and remove actual point
			points.remove_at(pointIdx);
			--pointIdx;
			++removedCount;
		}
		else
		{
			remapIndices[remapIdx] -= removedCount;
		}
	}

	remap_indices(remapIndices);
}

void IPU::calculate_normals_for_polygons()
{
	// calculate normals for polygons
	int polygonIndex = 0;
	for_every(polygon, polygons)
	{
		bool degenerated = false;

		int indexCount = polygon->indices.get_size();
		int triangleCount = indexCount > 3? indexCount : 1;

		for (int i = 0; i < polygon->indices.get_size(); ++i)
		{
			Vector3 p = points[polygon->indices[i]].point;
			Vector3 pp = points[polygon->indices[mod(i - 1, indexCount)]].point;
			Vector3 np = points[polygon->indices[mod(i + 1, indexCount)]].point;
			if (p == pp || p == np)
			{
				degenerated = true;
				break;
			}
		}

		if (degenerated)
		{
			polygon->normal = Vector3::zero; // shouldn't matter
		}
		else
		{
			Vector3 accumulatedNormal = Vector3::zero;
			for (int i = 0; i < triangleCount; ++i)
			{
				Vector3 p0 = points[polygon->indices[mod(i + 0, indexCount)]].point;
				Vector3 p1 = points[polygon->indices[mod(i + 1, indexCount)]].point;
				Vector3 p2 = points[polygon->indices[mod(i + 2, indexCount)]].point;
				accumulatedNormal += Vector3::cross(p2 - p0, p1 - p0).normal();
			}
			accumulatedNormal.normalise();
			polygon->normal = accumulatedNormal;

#ifdef AN_FIX_POLYGON_NORMAL_ISSUES
			if (polygon->normal.is_almost_zero())
			{
				// try to use weighted normal by area/size
				// it may give incorrect values but it still might be better to have reversed normals than to have black triangles
				// and in most cases we use it for quads that have a small bit flipped - we counter that small bit
				Vector3 accumulatedNormal = Vector3::zero;
				for (int i = 0; i <= triangleCount; ++i)
				{
					Vector3 p0 = points[polygon->indices[mod(i + 0, indexCount)]].point;
					Vector3 p1 = points[polygon->indices[mod(i + 1, indexCount)]].point;
					Vector3 p2 = points[polygon->indices[mod(i + 2, indexCount)]].point;
					// a p0p1
					// b p1p2
					// c p2p0
					float a = (p1 - p0).length();
					float b = (p2 - p1).length();
					float c = (p0 - p2).length();
					// triangle sides a, b, c, angles on opposite sides A, B, C
					// area = ab * sin(C) / 2
					// +
					// 2ba * cos(C) = b^ + a^ - c^
					// cos(C) = (b^ + a^ - c^) / 2ba
					// +
					// sin(C)^ + cos(C)^ = 1
					float cosC = clamp((sqr(b) + sqr(a) - sqr(c)) / (2.0f * a * b), -1.0f, 1.0f);
					float sinC = sqrt(clamp(1.0f - sqr(cosC), 0.0f, 1.0f)); // we're fine with C being 0-180 as it gives 0-1 result
					float area = a * b * sinC / 2.0f;

					accumulatedNormal += Vector3::cross(p2 - p0, p1 - p0).normal() * area;
				}
				accumulatedNormal.normalise();
				polygon->normal = accumulatedNormal;
#ifdef AN_INFO_ON_POLYGON_NORMAL_ISSUES
				if (! polygon->normal.is_almost_zero())
				{
					output(TXT("fixed zero polygon"));
				}
#endif
			}
#endif

#ifdef AN_ASSERT_ON_POLYGON_NORMAL_ISSUES
			an_assert(!polygon->normal.is_almost_zero());
#endif
#ifdef AN_INFO_ON_POLYGON_NORMAL_ISSUES
			if (polygon->normal.is_almost_zero())
			{
				output(TXT("zero polygon issue"));
				for (int i = 0; i < polygon->indices.get_size(); ++i)
				{
					output(TXT("  point %i: [%i] %S"), i, polygon->indices[i], points[polygon->indices[i]].point.to_string().to_char());
				}
			}
#endif
#ifdef AN_FIX_POLYGON_NORMAL_ISSUES
			if (polygon->normal.is_almost_zero() && polygon->indices.get_size() > 0)
			{
				// use just the three first
				Vector3 p0 = points[polygon->indices[0]].point;
				Vector3 p1 = points[polygon->indices[1]].point;
				Vector3 p2 = points[polygon->indices[2]].point;
				polygon->normal = Vector3::cross(p2 - p0, p1 - p0).normal();
#ifdef AN_INFO_ON_POLYGON_NORMAL_ISSUES
				if (!polygon->normal.is_almost_zero())
				{
					output(TXT("fixed zero polygon (degenerated)"));
				}
				if (polygon->normal.is_almost_zero())
				{
					output(TXT("still zero polygon, maybe degenerated?"));
				}
#endif
			}
#endif
#ifdef AN_INVESTIGATE_POLYGON_NORMAL_ISSUES
			if (polygon->normal.is_almost_zero())
			{
				DebugVisualiserPtr dv(new DebugVisualiser(String::printf(TXT("polygon normal zero"))));
				dv->activate();
				dv->start_gathering_data();
				dv->clear();
				Vector3 centre = Vector3::zero;
				for (int i = 0; i < polygon->indices.get_size(); ++i)
				{
					Vector3 pi = points[polygon->indices[i]].point;
					centre += pi;
				}
				centre /= (float)polygon->indices.get_size();
				for (int i = 0; i < polygon->indices.get_size(); ++i)
				{
					int ni = mod(i + 1, polygon->indices.get_size());
					Vector3 pi = points[polygon->indices[i]].point;
					Vector3 npi = points[polygon->indices[ni]].point;

					dv->add_text_3d(pi, String::printf(TXT("%i"), i), Colour::blue);
					dv->add_line_3d(pi, npi, Colour::blue);
					dv->add_line_3d(centre + 0.9f * (pi - centre), centre + 0.9f * (npi - centre), Colour::blue);
				}

				dv->end_gathering_data();
				dv->show_and_wait_for_key();
				dv->deactivate();
			}
#endif

			if (polygon->normal.is_almost_zero())
			{
				++polygonNormalIssues;
			}
		}

		++polygonIndex;
	}
}

bool IPU::are_points_colinear(ArrayStatic<int, 4> const & _indices) const
{
	Vector3 p0 = points[_indices[0]].point;
	Vector3 p1 = points[_indices[1]].point;
	Vector3 dir = (p1 - p0).normal();

	// find non zero dir
	if (dir.is_zero())
	{
		for (int i = 1; i < _indices.get_size(); ++i)
		{
			p0 = p1;
			p1 = points[_indices[(i + 1) % _indices.get_size()]].point;
			dir = (p1 - p0).normal();
			if (!dir.is_zero())
			{
				break;
			}
		}
	}
	if (dir.is_zero())
	{
		return true;
	}

	float len = (p1 - p0).length();

	for (int i = 0; i < _indices.get_size(); ++i)
	{
		Vector3 p = points[_indices[i]].point;
		Vector3 diff = (p - p0).drop_using_normalised(dir);
		float dist = diff.length();
		if (dist > 0.0001f * len)
		{
			return false;
		}
	}

	return true;
}

void IPU::do_points_vs_polygons()
{
	// we will need storing information about belonging to polygons
	for_every(point, points)
	{
		point->belongsToPolygon.clear();
	}

	// store where they belong
	int polygonIndex = 0;
	for_every(polygon, polygons)
	{
		for_every(index, polygon->indices)
		{
			points[*index].belongsToPolygon.push_back(polygonIndex);
		}

		++polygonIndex;
	}

	// check for each point whether we need to copy it and create copy of it if needed (update indices in polygons too!)
	for (int a = 0; a < points.get_size(); ++ a)
	{
		for (int b = 1; b < points[a].belongsToPolygon.get_size(); ++b)
		{
			bool different = false;
			{
				auto const & point = points[a];
				int pointBelongsToPolygonB = point.belongsToPolygon[b];
				int const * pointBelongsToPolygonC = point.belongsToPolygon.get_data();
				for (int c = 0; c < b; ++c, ++pointBelongsToPolygonC)
				{
					if (!can_polygons_share_point(pointBelongsToPolygonB, *pointBelongsToPolygonC))
					{
						different = true;
						break;
					}
				}
			}
			if (different)
			{
				// add new point
				points.push_back(points[a]);
				points.get_last().belongsToPolygon.clear(); // we won't be modifying them
				int newA = points.get_size() - 1;

				auto & point = points[a];
				int pointBelongsToPolygonB = point.belongsToPolygon[b];

				// check if other polygons may want to follow this one (as they could share point) 
				ARRAY_PREALLOC_SIZE(int, polygonsToModify, point.belongsToPolygon.get_size());
				polygonsToModify.push_back(pointBelongsToPolygonB);
				if (b + 1 < point.belongsToPolygon.get_size())
				{
					int * pointBelongsToPolygonC = &point.belongsToPolygon[b + 1];
					for (int c = b + 1; c < point.belongsToPolygon.get_size(); ++c, ++pointBelongsToPolygonC)
					{
						if (can_polygons_share_point(pointBelongsToPolygonB, *pointBelongsToPolygonC))
						{
							polygonsToModify.push_back(*pointBelongsToPolygonC);
							point.belongsToPolygon.remove_fast_at(c);
							--c;
							--pointBelongsToPolygonC;
						}
					}
				}
				point.belongsToPolygon.remove_fast_at(b);

				// update indices in those polygons to point at new A
				for_every(polygonToModify, polygonsToModify)
				{
					for_every(index, polygons[*polygonToModify].indices)
					{
						if (*index == a)
						{
							*index = newA;
						}
					}
				}
				// 
				--b;
			}
		}
	}
}

bool IPU::can_polygons_share_point(int _a, int _b) const
{
	if (polygons[_a].u != polygons[_b].u ||
		polygons[_a].colour != polygons[_b].colour)
	{
		return false;
	}
	if (Vector3::dot(polygons[_a].normal,
					 polygons[_b].normal) < smoothingDotLimit)
	{
		return false;
	}
	return true;
}

void IPU::find_where_points_belong()
{
	// we will need storing information about belonging to polygons
	for_every(point, points)
	{
		point->belongsToPolygon.clear();
	}

	// calculate normals for polygons
	int polygonIndex = 0;
	for_every(polygon, polygons)
	{
		for_every(index, polygon->indices)
		{
			points[*index].belongsToPolygon.push_back(polygonIndex);
		}

		++polygonIndex;
	}
}

void IPU::remove_unused_points()
{
	find_where_points_belong();

	Array<int> remapIndices;
	remapIndices.set_size(points.get_size());
	for (int a = 0; a < points.get_size(); ++a)
	{
		remapIndices[a] = a;
	}

	// remap those that stay, we will have full list of indices afterwards
	// if it is removed, we may safely use new index
	// and remove actual points here!
	// go through points, remove unused and decrease new number by how many did we remove already
	int removedCount = 0;
	for (int remapIdx = 0, pointIdx = 0; remapIdx < remapIndices.get_size(); ++remapIdx, ++pointIdx)
	{
		// keep
		remapIndices[remapIdx] -= removedCount;
		if (points[pointIdx].belongsToPolygon.is_empty())
		{
			points.remove_at(pointIdx);
			--pointIdx;
			++removedCount;
		}
	}

	remap_indices(remapIndices);
}

void IPU::remap_indices(Array<int> const & _remap)
{
	for_every(polygon, polygons)
	{
		for_every(index, polygon->indices)
		{
			*index = _remap[*index];
		}
	}
	assert_slow(is_valid());
}

void IPU::collapse_polygons()
{
	assert_slow(is_valid());
	for (int a = 0; a < polygons.get_size(); ++a)
	{
		// remove doubled points
		int prev = polygons[a].indices.get_size() - 1;
		for (int b = 0; b < polygons[a].indices.get_size(); ++b)
		{
			if (polygons[a].indices[b] == polygons[a].indices[prev])
			{
				polygons[a].indices.remove_at(b);
				--b;
				prev = min(prev, polygons[a].indices.get_size() - 1);
			}
			else
			{
				prev = b;
			}
		}
		// remove if not a triangle or if degenerated
		if (polygons[a].indices.get_size() < 3 ||
			are_points_colinear(polygons[a].indices))
		{
			polygons.remove_at(a);
			--a;
			assert_slow(is_valid());
		}
	}
	assert_slow(is_valid());
}

void IPU::fill_u_colour_normal()
{
	assert_slow(is_valid());

	find_where_points_belong();

	// rewrite u but for normal, calculate weighted
	for_every(point, points)
	{
		point->u = 0.0f;
		point->colour = Colour::white;
		point->normal = Vector3::zero;
		point->normalWeight = 0.0f;
	}

	for_every(polygon, polygons)
	{
		for_every(index, polygon->indices)
		{
			auto& intoP = points[*index];
			intoP.u = polygon->u;
			intoP.colour = polygon->colour;
			float normalWeight = 1.0f;
			todo_note(TXT("use size of polygon for calculation of normal"));
			intoP.normal += polygon->normal * normalWeight;
			intoP.normalWeight += normalWeight;
		}
	}

	for_every(point, points)
	{
		if (point->normalWeight != 0.0f)
		{
			point->normal /= point->normalWeight;
		}
	}
}

void IPU::prepare_data(::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat, OUT_ Array<int8> & _vertexData, OUT_ Array<int8> & _elementsData)
{
	assert_slow(is_valid());

	weld_points();
	collapse_polygons();
	calculate_normals_for_polygons();
	do_points_vs_polygons();
	remove_unused_points();
	collapse_polygons();
	fill_u_colour_normal();

	prepare_data_fill_data(_vertexFormat, _indexFormat, _vertexData, _elementsData);
}

void IPU::prepare_data_as_is(::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat, OUT_ Array<int8> & _vertexData, OUT_ Array<int8> & _elementsData)
{
	assert_slow(is_valid());

	calculate_normals_for_polygons();
	fill_u_colour_normal();

	prepare_data_fill_data(_vertexFormat, _indexFormat, _vertexData, _elementsData);
}

void IPU::prepare_data_fill_data(::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat, OUT_ Array<int8> & _vertexData, OUT_ Array<int8> & _elementsData)
{
	assert_slow(is_valid());

	// build vertex data - fill with what we have, everything else might be filled later by whoever need it
	int invalidSkinningPoints = 0;
	memory_leak_suspect;
	_vertexData.set_size(points.get_size() * _vertexFormat.get_stride());
	forget_memory_leak_suspect;
	memory_zero(_vertexData.get_data(), _vertexData.get_data_size());
	int8* currentVertexData = _vertexData.get_data();
	for_every(point, points)
	{
		if (_vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
		{
			Vector3* outLocation = (Vector3*)(currentVertexData + _vertexFormat.get_location_offset());
			*outLocation = point->point;
		}

		if (_vertexFormat.get_normal() == ::System::VertexFormatNormal::Yes)
		{
			if (_vertexFormat.is_normal_packed())
			{
				VectorPacked* outNormal = (VectorPacked*)(currentVertexData + _vertexFormat.get_normal_offset());
				outNormal->set_as_vertex_normal(point->normal);
			}
			else
			{
				Vector3* outNormal = (Vector3*)(currentVertexData + _vertexFormat.get_normal_offset());
				*outNormal = point->normal;
			}
		}

		if (_vertexFormat.get_colour() != ::System::VertexFormatColour::None)
		{
			Colour c = point->colour;
			if (mapUToColour.get_size() == 1)
			{
				c = mapUToColour.get_first().colour;
			}
			else if (!mapUToColour.is_empty())
			{
				float bestDiff = 100000.0f;
				for_every(m, mapUToColour)
				{
					float diff = abs(m->u - point->u);
					if (diff < bestDiff)
					{
						bestDiff = diff;
						c = m->colour;
					}
				}
			}
			if (_vertexFormat.get_colour() == ::System::VertexFormatColour::RGB)
			{
				float* out = (float*)(currentVertexData + _vertexFormat.get_colour_offset());
				*(out++) = c.r;
				*(out++) = c.g;
				*(out++) = c.b;
			}
			else if (_vertexFormat.get_colour() == ::System::VertexFormatColour::RGBA)
			{
				float* out = (float*)(currentVertexData + _vertexFormat.get_colour_offset());
				*(out++) = c.r;
				*(out++) = c.g;
				*(out++) = c.b;
				*(out++) = c.a;
			}
		}

		if (_vertexFormat.get_texture_uv() != ::System::VertexFormatTextureUV::None)
		{
#ifdef WITH_UV_SUPPORT
			Vector3 uvw = useU ? Vector3(point->u, 0.0f, 0.0f) : point->uv.to_vector3();
#else
			Vector3 uvw = Vector3(point->u, 0.0f, 0.0f);
#endif
			if (_vertexFormat.get_texture_uv() == ::System::VertexFormatTextureUV::U)
			{
				::System::VertexFormatUtils::store(currentVertexData + _vertexFormat.get_texture_uv_offset(), _vertexFormat.get_texture_uv_type(), uvw.x);
			}
			else if (_vertexFormat.get_texture_uv() == ::System::VertexFormatTextureUV::UV)
			{
				::System::VertexFormatUtils::store(currentVertexData + _vertexFormat.get_texture_uv_offset(), _vertexFormat.get_texture_uv_type(), uvw.to_vector2());
			}
			else if (_vertexFormat.get_texture_uv() == ::System::VertexFormatTextureUV::UVW)
			{
				::System::VertexFormatUtils::store(currentVertexData + _vertexFormat.get_texture_uv_offset(), _vertexFormat.get_texture_uv_type(), uvw);
			}
			else
			{
				todo_important(TXT("implement_ other texturing"));
			}
		}

		if (_vertexFormat.has_skinning_data())
		{
			float totalSkinningWeight = 0.0f;
			int skinningOffset = _vertexFormat.get_skinning_element_count() > 1 ? _vertexFormat.get_skinning_indices_offset() : _vertexFormat.get_skinning_index_offset();
			int bonesRequired = _vertexFormat.get_skinning_element_count();
			if (bonesRequired > 4)
			{
				error(TXT("no more than 4 bones supported!"));
				todo_important(TXT("add support for more bones"));
				bonesRequired = 4;
			}
			if (!point->skinningInfo.is_empty())
			{
				if (point->skinningInfo.bones.get_size() > bonesRequired)
				{
					warn(TXT("there are not enough slots for bones when compared to how many bones for skinning are provided for this point!"));
				}
				if (bonesRequired > 1)
				{
					float* out = (float*)(currentVertexData + _vertexFormat.get_skinning_weights_offset());
					for (int idx = 0; idx < bonesRequired; ++idx, ++out)
					{
						*out = point->skinningInfo.bones.is_index_valid(idx) ? point->skinningInfo.bones[idx].weight : 0.0f;
						totalSkinningWeight += point->skinningInfo.bones.is_index_valid(idx) ? point->skinningInfo.bones[idx].weight : 0.0f;
					}
					if (abs(totalSkinningWeight - 1.0f) > 0.001f)
					{
						++invalidSkinningPoints;
					}
				}
			}
			if (_vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint8)
			{
				uint8* out = (uint8*)(currentVertexData + skinningOffset);
				for (int idx = 0; idx < bonesRequired; ++idx, ++out)
				{
					if (point->skinningInfo.is_empty())
					{
						*out = NONE;
					}
					else
					{
						*out = point->skinningInfo.bones.is_index_valid(idx) ? (uint8)point->skinningInfo.bones[idx].bone : 0;
					}
				}
			}
			else if (_vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint16)
			{
				uint16* out = (uint16*)(currentVertexData + skinningOffset);
				for (int idx = 0; idx < bonesRequired; ++idx, ++out)
				{
					if (point->skinningInfo.is_empty())
					{
						*out = NONE;
					}
					else
					{
						*out = point->skinningInfo.bones.is_index_valid(idx) ? (uint16)point->skinningInfo.bones[idx].bone : 0;
					}
				}
			}
			else if (_vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint32)
			{
				uint32* out = (uint32*)(currentVertexData + skinningOffset);
				for (int idx = 0; idx < bonesRequired; ++idx, ++out)
				{
					if (point->skinningInfo.is_empty())
					{
						*out = NONE;
					}
					else
					{
						*out = point->skinningInfo.bones.is_index_valid(idx) ? (uint32)point->skinningInfo.bones[idx].bone : 0;
					}
				}
			}
			else
			{
				todo_important(TXT("implement_ other skinning index type"));
			}
		}

		currentVertexData += _vertexFormat.get_stride();
	}

	if (invalidSkinningPoints)
	{
		warn(TXT("there were some points (%i) that didn't have skinning weights sum up to 1.0"), invalidSkinningPoints);
	}

	// build indices, break quads into triangles etc
	_elementsData.clear();
	int indexSize = ::System::IndexElementSize::to_memory_size(_indexFormat.get_element_size());
	for_every(polygon, polygons)
	{
		for (int b = 0; b <= polygon->indices.get_size() - 3; ++b)
		{
			_elementsData.grow_size(indexSize);
			::System::IndexFormatUtils::set_value(_indexFormat, &_elementsData[_elementsData.get_size() - indexSize], (uint)polygon->indices[0]);
			_elementsData.grow_size(indexSize);
			::System::IndexFormatUtils::set_value(_indexFormat, &_elementsData[_elementsData.get_size() - indexSize], (uint)polygon->indices[b + 1]);
			_elementsData.grow_size(indexSize);
			::System::IndexFormatUtils::set_value(_indexFormat, &_elementsData[_elementsData.get_size() - indexSize], (uint)polygon->indices[b + 2]);
		}
	}

	//_vertexFormat.debug_vertex_data(points.get_size(), _vertexData.get_data());
}

bool IPU::is_valid() const
{
	for_every(polygon, polygons)
	{
		for_every(index, polygon->indices)
		{
			if (*index < 0 || *index >= points.get_size())
			{
				return false;
			}
		}
	}

	return true;
}

void IPU::for_every_triangle(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)> _do, int _startingAt, int _count) const
{
	for_every_triangle_worker(false, false, _startingAt, _count,
		[_do](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c);
	});
}

void IPU::for_every_triangle_and_simple_skinning(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx)> _do, int _startingAt, int _count) const
{
	for_every_triangle_worker(true, false, _startingAt, _count,
		[_do](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _skinToBoneIdx);
	});
}

void IPU::for_every_triangle_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, float _u)> _do, int _startingAt, int _count) const
{
	for_every_triangle_worker(false, true, _startingAt, _count,
		[_do](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _u);
	});
}

void IPU::for_every_triangle_simple_skinning_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)> _do, int _startingAt, int _count) const
{
	for_every_triangle_worker(true, true, _startingAt, _count,
		[_do](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _skinToBoneIdx, _u);
	});
}

void IPU::for_every_triangle_and_full_skinning(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning)> _do, int _startingAt, int _count) const
{
	for_every_triangle_worker(false, true, _startingAt, _count,
		[_do](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _aSkinning, _bSkinning, _cSkinning);
	});
}

void IPU::for_every_triangle_full_skinning_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, float _u)> _do, int _startingAt, int _count) const
{
	for_every_triangle_worker(false, true, _startingAt, _count,
		[_do](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _aSkinning, _bSkinning, _cSkinning, _u);
	});
}

void IPU::for_every_triangle_worker(bool _getSimpleSkinning, bool _getU, int _startingAt, int _count, std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)> _do) const
{
	if (polygons.is_empty() || ! polygons.is_index_valid(_startingAt))
	{
		return;
	}
	an_assert(is_valid());
	auto const * polygon = &polygons[_startingAt];
	if (_count < 0)
	{
		_count = polygons.get_size() - _startingAt;
	}
	for_count(int, i, _count)
	{
		int skinToBoneIdx = NONE;
		float u = polygon->u;
		if (_getSimpleSkinning)
		{
			for_every(pIdx, polygon->indices)
			{
				auto const & point = points[*pIdx];
				if (!point.skinningInfo.is_empty())
				{
					an_assert(point.skinningInfo.bones[0].weight > 0.0f);
					skinToBoneIdx = point.skinningInfo.bones[0].bone;
					break;
				}
			}
		}
		_do(points[polygon->indices[0]].point, points[polygon->indices[1]].point, points[polygon->indices[2]].point,
			points[polygon->indices[0]].skinningInfo, points[polygon->indices[1]].skinningInfo, points[polygon->indices[2]].skinningInfo,
			skinToBoneIdx, u);
		if (polygon->indices.get_size() > 3)
		{
			_do(points[polygon->indices[0]].point, points[polygon->indices[2]].point, points[polygon->indices[3]].point, 
				points[polygon->indices[0]].skinningInfo, points[polygon->indices[2]].skinningInfo, points[polygon->indices[3]].skinningInfo,
				skinToBoneIdx, u);
		}
		++polygon;
	}
}

int IPU::get_simple_skin_to_bone_index() const
{
	for_every(point, points)
	{
		if (!point->skinningInfo.is_empty())
		{
			an_assert(point->skinningInfo.bones[0].weight > 0.0f);
			return point->skinningInfo.bones[0].bone;
		}
	}
	return NONE;
}

void IPU::get_point(int _idx, OUT_ Vector3 & _point, OUT_ Vector3 & _normal) const
{
	an_assert(points.is_index_valid(_idx));
	auto const & point = points[_idx];
	_point = point.point;
	_normal = point.normal;
}

void IPU::set_point(int _idx, Vector3 const & _point, Vector3 const & _normal)
{
	an_assert(points.is_index_valid(_idx));
	auto & point = points[_idx];
	point.point = _point;
	point.normal = _normal;
}

void IPU::copy_skinning(int _srcIdx, int _destIdx)
{
	an_assert(points.is_index_valid(_srcIdx));
	an_assert(points.is_index_valid(_destIdx));
	points[_destIdx].skinningInfo = points[_srcIdx].skinningInfo;
}

void IPU::break_by_material_into(Array<IPU>& ipus)
{
	ARRAY_STATIC(int, materialPolygons, 64);
	while (materialPolygons.has_place_left())
	{
		materialPolygons.push_back(0);
	}

	bool needsBreaking = false;
	int highestMaterialIdx = NONE;

	for_every(p, polygons)
	{
		if (p->forceMaterialIdx.is_set())
		{
			int mIdx = p->forceMaterialIdx.get();
			highestMaterialIdx = max(highestMaterialIdx, mIdx);
			++materialPolygons[mIdx];
			needsBreaking = true;
		}
	}

	if (needsBreaking)
	{
		if (ipus.is_empty())
		{
			ipus.set_size(highestMaterialIdx + 1);
		}

		// break per material
		for_every(mp, materialPolygons)
		{
			if (*mp == 0)
			{
				continue;
			}

			int dIpuIdx = for_everys_index(mp);
			an_assert(ipus.get_size() > dIpuIdx, TXT("make enough space or leave ipus empty (to avoid resizing)"));
			if (ipus.get_size() <= dIpuIdx)
			{
				ipus.set_size(dIpuIdx + 1);
			}

			auto& dIpu = ipus[dIpuIdx];
			dIpu.points = points;
			dIpu.mapUToColour = mapUToColour;
			dIpu.polygons.make_space_for(polygons.get_size());

			for_every(p, polygons)
			{
				if (p->forceMaterialIdx.is_set() &&
					p->forceMaterialIdx.get() == dIpuIdx)
				{
					dIpu.polygons.push_back(*p);
				}
			}
		}

		// remove polygons that were taken out
		int startRemovingAt = NONE;
		for (int i = 0; i < polygons.get_size(); ++i)
		{
			if (polygons[i].forceMaterialIdx.is_set())
			{
				if (startRemovingAt == NONE)
				{
					startRemovingAt = i;
				}
			}
			else
			{
				if (startRemovingAt != NONE)
				{
					polygons.remove_at(startRemovingAt, i - startRemovingAt);
					i = startRemovingAt; // we may skip this one
					startRemovingAt = NONE;
				}
			}
		}
		if (startRemovingAt != NONE)
		{
			polygons.remove_at(startRemovingAt, polygons.get_size() - startRemovingAt);
		}
	}
}
