#include "..\meshGenAdvancedShapes.h"

#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenUtils.h"

#include "..\..\..\library\library.h"

#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\mesh3dBuilder.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace AdvancedShapes;

//

DEFINE_STATIC_NAME(range);
DEFINE_STATIC_NAME(size);

//

/*
 *	Forward is in which direction is forward, from where observer should see the display
 */

class DoorHoleComplementData
: public ElementShapeData
{
	FAST_CAST_DECLARE(DoorHoleComplementData);
	FAST_CAST_BASE(ElementShapeData);
	FAST_CAST_END();

	typedef ElementShapeData base;

public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

public:
	MeshGenParam<LibraryName> doorType;
};

//

ElementShapeData* AdvancedShapes::create_door_hole_complement_data()
{
	return new DoorHoleComplementData();
}

//

bool AdvancedShapes::door_hole_complement(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Mesh3DBuilder builder;

	builder.setup(Meshes::Primitive::Triangle, _context.get_vertex_format(), 2);

	//

	if (DoorHoleComplementData const * csData = fast_cast<DoorHoleComplementData>(_data))
	{
		Range2 useRange = Range2::empty;
		{
			FOR_PARAM(_context, Range2, range)
			{
				useRange = *range;
			}
			FOR_PARAM(_context, Vector2, size)
			{
				useRange.x = Range(-0.5f * size->x, 0.5f * size->x);
				useRange.y = Range(0.0f, size->y);
			}
		}

		LibraryName doorTypeName = csData->doorType.get(&_context, LibraryName::invalid());
		auto* doorType = Library::get_current()->get_door_types().find(doorTypeName);

		if (! doorType)
		{
			error_generating_mesh(_instance, TXT("couldn't find door type \"%S\""), doorTypeName.to_string().to_char());
			return false;
		}

		if (auto* doorHole = doorType->get_hole())
		{
			Array<Vector2> points;
			doorHole->provide_outbound_points_2d(DoorSide::A, OUT_ points);

			if (!points.is_empty())
			{
				Range2 pointsRange = Range2::empty;
				Vector2 centre = Vector2::zero;
				{
					float sum = 0.0f;
					for_every(p, points)
					{
						pointsRange.include(*p);
						centre += *p;
						sum += 1.0f;
					}
					centre /= sum;
				}

				if (useRange.x.min == useRange.x.max ||
					useRange.x.is_empty())
				{
					useRange.x = pointsRange.x;
				}

				if (useRange.y.min == useRange.y.max ||
					useRange.y.is_empty())
				{
					useRange.y = pointsRange.y;
				}

				// we want to find points for bottom, top, left and right
				// they should be as close to the axis (relative to centre) as possible and on each side
				int const TOP = 0;
				int const RIGHT = 1;
				int const BOTTOM = 2;
				int const LEFT = 3;
				int sidePoints[4];
				{
					Vector2 sideDir = Vector2::yAxis;
					Vector2 sideRight = Vector2::xAxis;
					for_count(int, sideIdx, 4)
					{
						float closestDist = 0.0f;
						int& sidePoint = sidePoints[sideIdx];
						sidePoint = NONE;
						for_every(p, points)
						{
							Vector2 diff = *p - centre;
							if (Vector2::dot(diff, sideDir) > 0.0f)
							{
								// on the right side
								float dist = abs(Vector2::dot(diff, sideDir));
								if (closestDist > dist ||
									sidePoint == NONE)
								{
									closestDist = dist;
									sidePoint = for_everys_index(p);
								}
							}
						}
						sideDir = sideDir.rotated_right();
						sideRight = sideRight.rotated_right();
					}
				}
				if (sidePoints[0] == NONE ||
					sidePoints[1] == NONE ||
					sidePoints[2] == NONE ||
					sidePoints[3] == NONE)
				{
					error_generating_mesh(_instance, TXT("invalid door, couldn't decide on side points at all \"%S\""), doorTypeName.to_string().to_char());
					return false;
				}
#ifdef AN_DEVELOPMENT
				Optional<int> testSideIdx;
#endif
				for_count(int, sideIdx, 4)
				{
					int pointIdx = sidePoints[sideIdx];
					int nextPointIdx = sidePoints[mod(sideIdx + 1, 4)];

					// for each side point we create a triangle that stretches to the "ranges" edge
					{
						Vector2 p = points[pointIdx];
						Optional<Vector2> p0pt;
						Optional<Vector2> p1pt;
						if (sideIdx == TOP && p.y < useRange.y.max)
						{
							p0pt = Vector2(0.0f, 1.0f);
							p1pt = Vector2(1.0f, 1.0f);
						}
						if (sideIdx == RIGHT && p.x < useRange.x.max)
						{
							p0pt = Vector2(1.0f, 1.0f);
							p1pt = Vector2(1.0f, 0.0f);
						}
						if (sideIdx == BOTTOM && p.y > useRange.y.min)
						{
							p0pt = Vector2(1.0f, 0.0f);
							p1pt = Vector2(0.0f, 0.0f);
						}
						if (sideIdx == LEFT && p.x > useRange.x.min)
						{
							p0pt = Vector2(0.0f, 0.0f);
							p1pt = Vector2(0.0f, 1.0f);
						}

#ifdef AN_DEVELOPMENT
						if (testSideIdx.get(sideIdx) == sideIdx)
#endif
						if (p0pt.is_set() &&
							p1pt.is_set())
						{
							builder.add_triangle();
							builder.add_point();
							builder.location(useRange.get_at(p0pt.get()).to_vector3_xz());
							builder.normal(-Vector3::yAxis);
							builder.uv(p0pt.get());
							builder.add_point();
							builder.location(useRange.get_at(p1pt.get()).to_vector3_xz());
							builder.normal(-Vector3::yAxis);
							builder.uv(p1pt.get());
							builder.add_point();
							builder.location(p.to_vector3_xz());
							builder.normal(-Vector3::yAxis);
							builder.uv(useRange.get_pt_from_value(p));
							builder.done_triangle();
						}
					}

					// and edges between that would stretch to corner between
					{
						int currPointIdx = pointIdx;
						Vector2 cornersPt[4] = { Vector2(1.0f, 1.0f),
												 Vector2(1.0f, 0.0f),
												 Vector2(0.0f, 0.0f),
												 Vector2(0.0f, 1.0f) };
						Vector2 cornerPt = cornersPt[sideIdx];
						Vector2 corner = useRange.get_at(cornerPt);
						Vector2 prevP = points[currPointIdx];
						while (true)
						{
							currPointIdx = mod(currPointIdx + 1, points.get_size());
							//
							Vector2 currP = points[currPointIdx];
#ifdef AN_DEVELOPMENT
							if (testSideIdx.get(sideIdx) == sideIdx)
#endif
							{
								// check if degenerated
								if (!(prevP - corner).drop_using(currP - corner).is_almost_zero())
								{
									builder.add_triangle();
									builder.add_point();
									builder.location(corner.to_vector3_xz());
									builder.normal(-Vector3::yAxis);
									builder.uv(cornerPt);
									builder.add_point();
									builder.location(currP.to_vector3_xz());
									builder.normal(-Vector3::yAxis);
									builder.uv(useRange.get_pt_from_value(currP));
									builder.add_point();
									builder.location(prevP.to_vector3_xz());
									builder.normal(-Vector3::yAxis);
									builder.uv(useRange.get_pt_from_value(prevP));
									builder.done_triangle();
								}
							}
							//
							if (currPointIdx == nextPointIdx)
							{
								break;
							}
							prevP = currP;
						}
					}
				}
			}
		}
	}

	//

	builder.build();

	int partsSoFar = _context.get_generated_mesh()->get_parts_num();
	_context.get_generated_mesh()->load_builder(&builder);

	for_range(int, i, partsSoFar, _context.get_generated_mesh()->get_parts_num() - 1)
	{
		_context.store_part(_context.get_generated_mesh()->access_part(i), _instance);
	}

	return true;
}

//

REGISTER_FOR_FAST_CAST(DoorHoleComplementData);

bool DoorHoleComplementData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	doorType.load_from_xml(_node, TXT("doorType"));

	return result;
}

bool DoorHoleComplementData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}