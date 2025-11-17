#include "..\meshGenAdvancedShapes.h"

#include "..\..\meshGenCreateCollisionInfo.h"
#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenUtils.h"
#include "..\..\meshGenParam.h"
#include "..\..\meshGenSubStepDef.h"

#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\..\..\..\core\types\vectorPacked.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\..\..\..\core\debug\debugVisualiser.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace AdvancedShapes;

//

DEFINE_STATIC_NAME(size);
DEFINE_STATIC_NAME(duneSpacing);
DEFINE_STATIC_NAME(obstacleSize);
DEFINE_STATIC_NAME(obstacleAt);

//

/*
 *	Forward is in which direction is forward, from where observer should see the display
 */

class DesertData
: public ElementShapeData
{
	FAST_CAST_DECLARE(DesertData);
	FAST_CAST_BASE(ElementShapeData);
	FAST_CAST_END();

	typedef ElementShapeData base;

public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

public:
	SubStepDef subStep;
	CreateCollisionInfo createMovementCollisionMesh; // will create mesh basing on ipu
	CreateCollisionInfo createPreciseCollisionMesh; // will create mesh basing on ipu
	bool noMesh = false;

	Vector2 obstacleSize = Vector2::zero;
	Transform obstacleAt = Transform::identity;
};

//

ElementShapeData* AdvancedShapes::create_desert_data()
{
	return new DesertData();
}

//

bool AdvancedShapes::desert(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	Range3 useSize = Range3::empty;
	float useDuneSpacing = 10.0f;

	if (DesertData const * dData = fast_cast<DesertData>(_data))
	{
		{
			FOR_PARAM(_context, Range3, size)
			{
				useSize = *size;
			}
		}
		{
			FOR_PARAM(_context, float, duneSpacing)
			{
				useDuneSpacing = *duneSpacing;
			}
		}

		Vector2 useObstacleSize = dData->obstacleSize;
		{
			FOR_PARAM(_context, Vector2, obstacleSize)
			{
				useObstacleSize = *obstacleSize;
			}
		}

		Transform useObstacleAt = dData->obstacleAt;
		{
			FOR_PARAM(_context, Transform, obstacleAt)
			{
				useObstacleAt = *obstacleAt;
			}
		}

		int duneCountX = TypeConversions::Normal::f_i_closest(useSize.x.length() / (useDuneSpacing * 0.5f));
		int duneCountY = TypeConversions::Normal::f_i_closest(useSize.y.length() / (useDuneSpacing * 0.5f));

		duneCountX += 1;
		duneCountY += 1;

		float duneSpacingX = useSize.x.length() / (float)(duneCountX - 1);
		float duneSpacingY = useSize.y.length() / (float)(duneCountY - 1);

		struct DunePoint
		{
			VectorInt2 at;
			bool border;
			bool actualDune;
			Vector3 p; // base for dune
			BezierCurve<Vector3> x;
			BezierCurve<Vector3> y;
			Vector3 dx;
			Vector3 dy;
		};

		struct DuneEdge
		{
			Array<DunePoint> along;
		};

		struct Dunes
		{
			Array<DuneEdge> dunes;

#ifdef AN_DEVELOPMENT_OR_PROFILER
			void visualise(tchar const* _info)
			{
				/*
				DebugVisualiserPtr dv(new DebugVisualiser(String(_info)));
				dv->set_background_colour(Colour::greyLight);
				dv->activate();
				dv->start_gathering_data();
				dv->clear();

				for_every(dune, dunes)
				{
					for_every(a, dune->along)
					{
						if (a->at.x < dunes.get_size() - 1)
						{
							dv->add_line_3d(a->p, dunes[a->at.x + 1].along[a->at.y].p, Colour::black);
						}
						if (a->at.y < dune->along.get_size() - 1)
						{
							dv->add_line_3d(a->p, dune->along[a->at.y + 1].p, a->actualDune? Colour::blue : Colour::black);
						}
					}
				}

				dv->end_gathering_data();
				dv->show_and_wait_for_key();
				*/
			}
#endif
		};

		Dunes dunes;

		Vector3 duneStart(useSize.x.min, useSize.y.min, useSize.z.min);
		float duneHeight = useSize.z.length();
		float duneBase = duneHeight * 0.2f;

		Random::Generator rg = _context.get_random_generator();
		rg.advance_seed(23508, 208762);

		dunes.dunes.set_size(duneCountX);
		for_every(dune, dunes.dunes)
		{
			dune->along.set_size(duneCountY);

			for_every(a, dune->along)
			{
				int x = for_everys_index(dune);
				int y = for_everys_index(a);
				a->at.x = x;
				a->at.y = y;
				a->border = (x == 0 || x == duneCountX - 1 ||
							 y == 0 || y == duneCountY - 1);
				a->actualDune = x % 2;
				a->p = duneStart
					 + Vector3::xAxis * duneSpacingX * (float)x
					 + Vector3::yAxis * duneSpacingY * (float)y;
				a->p.z += rg.get_float(0.0f, 1.0f) * duneBase;
			}
		}

		// smooth
		for_count(int, smoothIter, 20)
		{
			for_every(dune, dunes.dunes)
			{
				for_every(a, dune->along)
				{
					if (a->border)
					{
						a->p.z = duneStart.z;
					}
					else
					{
						float other = 0.0f;
						other += dunes.dunes[a->at.x - 1].along[a->at.y].p.z;
						other += dunes.dunes[a->at.x + 1].along[a->at.y].p.z;
						other += dunes.dunes[a->at.x].along[a->at.y - 1].p.z;
						other += dunes.dunes[a->at.x].along[a->at.y + 1].p.z;
						other /= 4.0f;

						float mixOthers = 0.1f;
						a->p.z = a->p.z * (1.0f - mixOthers) + mixOthers * other;
					}
				}
			}
		}

#ifdef AN_DEVELOPMENT_OR_PROFILER
		dunes.visualise(TXT("smoothed"));
#endif

		// move dune edges randomly
		{
			for_every(dune, dunes.dunes)
			{
				for_every(a, dune->along)
				{
					if (! a->border && a->actualDune)
					{
						a->p.x += rg.get_float(-1.0f, 1.0f) * duneSpacingX * 3.0f;
					}
				}
			}
		}

#ifdef AN_DEVELOPMENT_OR_PROFILER
		dunes.visualise(TXT("moved randomly"));
#endif

		// smooth dune edges
		for_count(int, smoothIter, 50)
		{
			for_every(dune, dunes.dunes)
			{
				for_every(a, dune->along)
				{
					if (! a->border)
					{
						int by = a->actualDune? 2 : 1;
						{
							float left = dunes.dunes[a->at.x > (by-1) ? a->at.x - by : 0].along[a->at.y].p.x;
							float right = dunes.dunes[a->at.x < duneCountX - by ? a->at.x + by : duneCountX - 1].along[a->at.y].p.x;

							float shouldBeAt = (left + right) * 0.5f;
							if (a->actualDune)
							{
								shouldBeAt = (left * 0.55f + right * 0.45f);
								if (abs(shouldBeAt - a->p.x) < duneSpacingX * 1.45f)
								{
									shouldBeAt = a->p.x;
								}
								float away;
								away = 1.0f;
								if (a->p.x < left + duneSpacingX * away)
								{
									shouldBeAt = left + duneSpacingX * away;
								}
								away = 0.65f;
								if (a->p.x > right - duneSpacingX * away)
								{
									shouldBeAt = right - duneSpacingX * away;
								}
							}

							float beAt = a->actualDune ? 0.05f : 0.4f;
							a->p.x = a->p.x * (1.0f - beAt) + beAt * shouldBeAt;
						}
						{
							float down = dune->along[a->at.y > 1 ? a->at.y - 1 : 0].p.x;
							float up= dune->along[a->at.y < duneCountY - 1 ? a->at.y + 1 : 0].p.x; 

							float shouldBeAt = (down + up) * 0.5f;
							float beAt = a->actualDune ? 0.015f : 0.02f;

							if (a->actualDune)
							{
								if (abs(shouldBeAt - a->p.x) < duneSpacingX * 0.6f)
								{
									beAt = 0.001f;
								}
							}

							a->p.x = a->p.x * (1.0f - beAt) + beAt * shouldBeAt;
						}
					}
				}
			}
		}

#ifdef AN_DEVELOPMENT_OR_PROFILER
		dunes.visualise(TXT("smoothed dune edges"));
#endif

		// bump up dune edges
		for_count(int, smoothIter, 4)
		{
			for_every(dune, dunes.dunes)
			{
				for_every(a, dune->along)
				{
					if (!a->border && a->actualDune)
					{
						{
							Vector3 down = dune->along[a->at.y > 1 ? a->at.y - 1 : 0].p;
							Vector3 up = dune->along[a->at.y < duneCountY - 1 ? a->at.y + 1 : 0].p;
							Vector3 down2 = dune->along[a->at.y > 2 ? a->at.y - 2 : 0].p;
							Vector3 up2 = dune->along[a->at.y < duneCountY - 2 ? a->at.y + 2 : 0].p;

							float shouldBeAtX = up.x * 0.3f + down.x * 0.3f + down2.x * 0.2f + up2.x * 0.2f;

							//float beAt = 0.0f;
							//float targetZ = a->p.z;
							if (a->p.x < shouldBeAtX)
							{
								float amount = clamp((shouldBeAtX - a->p.x) / (duneSpacingX * 0.5f), 0.0f, 1.0f);
								a->p.z = max(a->p.z, duneStart.z + duneHeight * amount);
							}
						}
					}
				}
			}
		}

#ifdef AN_DEVELOPMENT_OR_PROFILER
		dunes.visualise(TXT("bumped dune edges"));
#endif

		// smooth dune edges
		for_count(int, smoothIter, 50)
		{
			for_every(dune, dunes.dunes)
			{
				for_every(a, dune->along)
				{
					if (!a->border)
					{
						float shouldBeAt = a->p.z;
						float beAt = 0.02f;

						if (a->actualDune)
						{
							float down = dune->along[a->at.y > 1 ? a->at.y - 1 : 0].p.z;
							float up = dune->along[a->at.y < duneCountY - 1 ? a->at.y + 1 : 0].p.z;

							shouldBeAt = (up + down) * 0.5f;
						}
						else 
						{
							Vector3 left = dunes.dunes[a->at.x > 0 ? a->at.x - 1 : 0].along[a->at.y].p;
							Vector3 right = dunes.dunes[a->at.x < duneCountX - 1 ? a->at.x + 1 : duneCountX - 1].along[a->at.y].p;
							beAt = clamp(1.0f - (a->p.x - left.x) / duneSpacingX * 0.3f, 0.0f, 1.0f) * 0.01f;
							shouldBeAt = duneStart.z * 0.3f + 0.7f * (left.z * 0.6f + 0.4f * right.z);

							float other = 0.0f;
							other += dunes.dunes[a->at.x].along[a->at.y - 1].p.z;
							other += dunes.dunes[a->at.x].along[a->at.y + 1].p.z;
							other /= 2.0f;

							shouldBeAt = other * 0.4f + shouldBeAt * 0.6f;
						}

						if (!useObstacleSize.is_zero())
						{
							Vector3 localP = useObstacleAt.location_to_local(a->p);

							float distX = max(0.0f, abs(localP.x) - useObstacleSize.x * 0.5f);
							float distY = max(0.0f, abs(localP.y) - useObstacleSize.y * 0.5f);
							float dist = sqrt(sqr(distX) + sqr(distY));

							float goUp = clamp(1.0f - dist / (duneSpacingX * 1.5f), 0.0f, 1.0f);

							float obstacleLen = useObstacleSize.length();
							float windSide = useObstacleAt.get_translation().x + obstacleLen * 0.35f;
							float withWind = clamp((windSide - a->p.x) / (obstacleLen * 2.0f), 0.0f, 1.0f);
							withWind = 1.0f - withWind;
							withWind = sqr(withWind);
							withWind = sqr(withWind);
							withWind = 1.0f - withWind;

							/*
							if (goUp > 0.0f)
							{
								//output(TXT("!@# dist %.3f %S ws %.3f gu%.3f ww%.3f"), dist, a->p.to_string().to_char(), windSide, goUp, withWind);
							}
							*/

							goUp *= (1.0f - withWind * 0.7f);

							float goUpHeight = duneHeight * 1.1f * (1.0f - withWind);

							//a->p.z = a->p.z * (1.0f - goUp) + goUp * goUpHeight;
							shouldBeAt = goUpHeight * goUp + (1.0f - goUp) * shouldBeAt;
							beAt = beAt + 0.25f * goUp;
						}

						a->p.z = a->p.z * (1.0f - beAt) + beAt * shouldBeAt;
					}
				}
			}
		}

#ifdef AN_DEVELOPMENT_OR_PROFILER
		dunes.visualise(TXT("smoothed dune edges"));
#endif

		// calculate directions for bezier curves
		for_count(int, x, duneCountX)
		{
			for_count(int, y, duneCountY)
			{
				auto& a = dunes.dunes[x].along[y];
				Vector3 up = dunes.dunes[x].along[min(y + 1, duneCountY - 1)].p;
				Vector3 down = dunes.dunes[x].along[max(y - 1, 0)].p;
				Vector3 left = dunes.dunes[max(x - 1, 0)].along[y].p;

				Vector3 alongY = ((up - a.p).normal() + (a.p - down).normal() + (up - down).normal()).normal();

				a.dy = alongY;
				a.dx = a.actualDune? Vector3::xAxis : (Vector3::xAxis * 0.7f + 0.3f * (a.p - left));
				a.dx.normalise();
			}
		}

		// calculate bezier curves
		for_count(int, x, duneCountX)
		{
			for_count(int, y, duneCountY)
			{
				auto& a = dunes.dunes[x].along[y];
				auto& up = dunes.dunes[x].along[min(y + 1, duneCountY - 1)];
				auto& right = dunes.dunes[min(x + 1, duneCountX - 1)].along[y];

				a.y.p0 = a.p;
				a.y.p3 = up.p;
				a.y.p1 = a.y.p0 + a.dy;
				a.y.p2 = a.y.p3 - up.dy;
				a.y.make_roundly_separated();

				a.x.p0 = a.p;
				a.x.p3 = right.p;
				a.x.p1 = a.x.p0 + a.dx;
				a.x.p2 = a.x.p3 - right.dx;
				if (!a.actualDune)
				{
					a.x.p2 = (a.x.p0 + a.x.p3) * 0.5f;
					a.x.p2.z = a.x.p0.z;
				}
				a.x.make_roundly_separated();
			}
		}

		int subStepCountX = dData->subStep.calculate_sub_step_count_for(duneSpacingX, _context);
		int subStepCountY = dData->subStep.calculate_sub_step_count_for(duneSpacingY, _context);

		for_count(int, y, duneCountY - 1)
		{
			for_count(int, x, duneCountX - 1)
			{
				auto& a = dunes.dunes[x].along[y];
				auto& up = dunes.dunes[x].along[min(y + 1, duneCountY - 1)];
				auto& right = dunes.dunes[min(x + 1, duneCountX - 1)].along[y];

				BezierSurface bs;
				bs = BezierSurface::create(a.x, right.y, up.x.reversed(), a.y.reversed(), BezierSurfaceCreationMethod::AlongV);

				int pointsAt = ipu.get_point_count();

				for_count(int, sy, subStepCountY + 1)
				{
					for_count(int, sx, subStepCountX + 1)
					{
						Vector2 s00((float)sx / (float)subStepCountX, (float)sy / (float)subStepCountY);
						Vector3 p00 = bs.calculate_at(Vector2(s00.x, s00.y));

						ipu.add_point(p00);
					}
				}

				for_count(int, sy, subStepCountY)
				{
					for_count(int, sx, subStepCountX)
					{
						int o00 = pointsAt + 
								  sy * (subStepCountX + 1) + sx;
						int o10 = o00 + 1;
						int o01 = o00 + (subStepCountX + 1);
						int o11 = o01 + 1;

						ipu.add_quad(0.0f, o00, o01, o11, o10);
					}
				}
			}
		}
	}

	if (_context.does_require_movement_collision())
	{
		if (fast_cast<DesertData>(_data) && fast_cast<DesertData>(_data)->createMovementCollisionMesh.create)
		{
			_context.store_movement_collision_parts(create_collision_meshes_from(ipu, _context, _instance, fast_cast<DesertData>(_data)->createMovementCollisionMesh));
		}
	}

	if (_context.does_require_precise_collision())
	{
		if (fast_cast<DesertData>(_data) && fast_cast<DesertData>(_data)->createPreciseCollisionMesh.create)
		{
			_context.store_precise_collision_parts(create_collision_meshes_from(ipu, _context, _instance, fast_cast<DesertData>(_data)->createPreciseCollisionMesh));
		}
	}

	if (fast_cast<DesertData>(_data) && fast_cast<DesertData>(_data)->noMesh)
	{
		ipu.keep_polygons(0);
	}

	//

	// prepare data for vertex and index formats, so further modifications may be applied and mesh can be imported
	ipu.prepare_data(_context.get_vertex_format(), _context.get_index_format(), vertexData, elementsData);

	int vertexCount = vertexData.get_size() / _context.get_vertex_format().get_stride();
	int elementsCount = elementsData.get_size() / _context.get_index_format().get_stride();

	//

	if (vertexCount)
	{
		// create part
		Meshes::Mesh3DPart* part = _context.get_generated_mesh()->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, _context.get_vertex_format(), _context.get_index_format());

		if (!useSize.is_empty())
		{	// make normals at edges point up
			int8* vertexData = part->access_vertex_data().get_data();
			auto const& vf = part->get_vertex_format();
			int stride = vf.get_stride();
			if (vf.get_normal() == ::System::VertexFormatNormal::Yes &&
				vf.get_location() == ::System::VertexFormatLocation::XYZ)
			{
				int8* locData = vertexData + vf.get_location_offset();
				int8* norData = vertexData + vf.get_normal_offset();
				for (int i = 0; i < part->get_number_of_vertices(); ++i, locData += stride, norData += stride)
				{
					Vector3 loc = *(Vector3*)(locData);
					float offEdge = 1000.0f;
					offEdge = min(offEdge, loc.x - useSize.x.min);
					offEdge = min(offEdge, useSize.x.max - loc.x);
					offEdge = min(offEdge, loc.y - useSize.y.min);
					offEdge = min(offEdge, useSize.y.max - loc.y);

					float makeUpward = clamp(1.0f - offEdge / (useDuneSpacing * 0.6f), 0.0f, 1.0f);
					if (makeUpward > 0.0f)
					{
						Vector3 normal = vf.is_normal_packed() ? ((VectorPacked*)(norData))->get_as_vertex_normal() : *((Vector3*)(norData));
						normal = (normal * (1.0f - makeUpward) + makeUpward * Vector3::zAxis).normal();
						if (vf.is_normal_packed())
						{
							((VectorPacked*)(norData))->set_as_vertex_normal(normal);
						}
						else
						{
							*(Vector3*)(norData) = normal;
						}
					}
				}
			}
			else
			{
				todo_implement;
			}
		}
		
#ifdef AN_DEVELOPMENT
		part->debugInfo = String::printf(TXT("desert @ %S"), _instance.get_element()->get_location_info().to_char());
#endif

		_context.store_part(part, _instance);
	}

	return true;
}

//

REGISTER_FOR_FAST_CAST(DesertData);

bool DesertData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	subStep.load_from_xml_child_node(_node);
#ifdef AN_DEVELOPMENT
	if (_node->first_child_named(TXT("devSubStep")))
	{
		subStep.load_from_xml_child_node(_node, TXT("devSubStep"));
	}
#endif
	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	noMesh = _node->get_bool_attribute_or_from_child_presence(TXT("noMesh"), noMesh);

	for_every(node, _node->children_named(TXT("obstacle")))
	{
		obstacleSize.load_from_xml_child_node(node, TXT("size"));
		obstacleAt.load_from_xml_child_node(node, TXT("placement"));
	}

	return result;
}

bool DesertData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}