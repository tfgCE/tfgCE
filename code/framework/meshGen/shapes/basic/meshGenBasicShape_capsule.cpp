#include "..\meshGenBasicShapes.h"

#include "meshGenBasicShapeData.h"

#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenUtils.h"
#include "..\..\meshGenSubStepDef.h"

#include "..\..\..\collision\physicalMaterial.h"

#include "..\..\..\..\core\collision\model.h"
#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\builders\mesh3dBuilder_IPU.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace BasicShapes;

//

DEFINE_STATIC_NAME(capsuleA);
DEFINE_STATIC_NAME(capsuleB);
DEFINE_STATIC_NAME(dir);
DEFINE_STATIC_NAME(halfDir);
DEFINE_STATIC_NAME(radius);
DEFINE_STATIC_NAME(size);

//

class CapsuleData
: public Data
{
	FAST_CAST_DECLARE(CapsuleData);
	FAST_CAST_BASE(Data);
	FAST_CAST_END();

	typedef Data base;

public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
public:
	bool legacyGenerator = false;
};

REGISTER_FOR_FAST_CAST(CapsuleData);

bool CapsuleData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	legacyGenerator = _node->get_string_attribute_or_from_child(TXT("generator")) == TXT("legacy");

	return result;
}

//

ElementShapeData* BasicShapes::create_capsule_data()
{
	return new CapsuleData();
}

//

bool BasicShapes::capsule(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	float useRadius = 1.0f;
	{
		FOR_PARAM(_context, float, radius)
		{
			useRadius = *radius;
		}
		FOR_PARAM(_context, float, size)
		{
			useRadius = *size * 0.5f;
		}
	}

	Vector3 useHalfDir = Vector3::zero;
	{
		FOR_PARAM(_context, Vector3, halfDir)
		{
			useHalfDir = *halfDir;
		}
		FOR_PARAM(_context, Vector3, dir)
		{
			useHalfDir = *dir * 0.5f;
		}
	}

	Vector3 location = Vector3::zero;
	{
		Optional<Vector3> useCapsuleA;
		Optional<Vector3> useCapsuleB;
		FOR_PARAM(_context, Vector3, capsuleA)
		{
			useCapsuleA = *capsuleA;
		}
		FOR_PARAM(_context, Vector3, capsuleB)
		{
			useCapsuleB = *capsuleB;
		}
		if (useCapsuleA.is_set() && useCapsuleB.is_set())
		{
			location = (useCapsuleA.get() + useCapsuleB.get()) * 0.5f;
			useHalfDir = useCapsuleA.get() - location;
		}
	}

	//

	bool legacyGenerator = false;

	if (CapsuleData const * cData = fast_cast<CapsuleData>(_data))
	{
		legacyGenerator = cData->legacyGenerator;
	}

	//

	SubStepDef subStep;
	subStep.load_from_context(_context);
	int useSubStepCount = subStep.calculate_sub_step_count_for(useRadius * pi<float>() * 0.5f, _context, NP, fast_cast<Data>(_data) && fast_cast<Data>(_data)->noMesh); // quarter of circumference

	{
		Transform placeAt = Transform::identity;
		if (!useHalfDir.is_zero())
		{
			Vector3 nonAlignedDir = Vector3::zAxis;
			if (abs(useHalfDir.z) >= abs(useHalfDir.x) &&
				abs(useHalfDir.z) >= abs(useHalfDir.y))
			{
				nonAlignedDir = Vector3::yAxis;
			}
			placeAt.build_from_axes(Axis::Up, Axis::Forward, nonAlignedDir, Vector3::zero /* will be auto calculated */, useHalfDir, location);
		}

		if (legacyGenerator)
		{
			// we will end up with doubled points on ha = 0.0 and on top and bottom
			int viMax = useSubStepCount * 2;
			int hiMax = useSubStepCount * 4;
			ipu.will_need_at_least_points((hiMax + 1) * (viMax + 1));
			float viMaxInverted = 1.0f / (float)viMax;
			float hiMaxInverted = 1.0f / (float)hiMax;
			Vector3 offset(0.0f, 0.0f, -useHalfDir.length());

			for (int vi = 0; vi <= viMax; vi++)
			{
				float va = -90.0f + 180.0f * (float)vi * viMaxInverted;
				float vc = cos_deg(va);
				float vs = sin_deg(va);
				for_count(int, secondRing, (vi == viMax / 2) ? 2 : 1)
				{
					for (int hi = 0; hi <= hiMax; hi++)
					{
						float ha = 360.0f * (float)hi * hiMaxInverted;
						float hc = cos_deg(ha);
						float hs = sin_deg(ha);
						ipu.add_point(placeAt.location_to_world(Vector3(hs * vc, hc * vc, vs) * useRadius + offset));
					}
					if (secondRing)
					{
						offset = -offset;
					}
				}
			}

			for (int vi = 0; vi < viMax + 1; vi++)
			{
				int lower = (hiMax + 1) * vi;
				int upper = (hiMax + 1) * (vi + 1);
				for (int hi = 0; hi < hiMax; hi++)
				{
					ipu.add_quad(0.0f, lower, lower + 1, upper + 1, upper);
					++lower;
					++upper;
				}
			}
		}
		else
		{
			// generate per bezier patch
			int onePatchPointCount = 0;
			int onePatchTriCount = 0;
			/* we go from one point on top to useSubStepCount on the bottom
			 *
			 *						 ^u
			 *						b
			 *				   ab  / \  bc
			 *					  *---*
			 *					 / \ / \
			 *					a---*---c ->v
			 *						ca
			 */
			 // use arithmetic progression sum
			{
				int n = useSubStepCount + 1;
				int r = 1;
				onePatchPointCount = ((2 * 1) + (n - 1) * r) * n / 2;
			}
			{
				int n = useSubStepCount;
				int r = 2;
				onePatchTriCount = ((2 * 1) + (n - 1) * r) * n / 2;
			}
			ipu.will_need_at_least_points(onePatchPointCount * 8);
			ipu.will_need_at_least_polygons(onePatchTriCount * 8 + useSubStepCount * 4);
			int pointIdxOffset = 0;
			for_count(int, patchIdx, 8)
			{
				Vector3 a, b, c;
				{
					float ha = 360.0f * (float)patchIdx / 4.0f;
					float hca = cos_deg(ha);
					float hsa = sin_deg(ha);
					float hc = 360.0f * (float)(patchIdx + (patchIdx >= 4 ? -1 : 1)) / 4.0f;
					float hcc = cos_deg(hc);
					float hsc = sin_deg(hc);

					a.x = hca * useRadius;
					a.y = hsa * useRadius;
					a.z = 0.0f;

					c.x = hcc * useRadius;
					c.y = hsc * useRadius;
					c.z = 0.0f;

					b.x = 0.0f;
					b.y = 0.0f;
					b.z = (patchIdx >= 4 ? -1.0f : 1.0f) * useRadius;
				}
				BezierCurve<Vector3> ab;
				BezierCurve<Vector3> bc;
				BezierCurve<Vector3> ca;

				Vector3 offset(0.0f, 0.0f, (patchIdx >= 4.0f ? -1.0f : 1.0f) * useHalfDir.length());

				ab.p0 = a;
				ab.p3 = b;
				ab.p1 = ab.p2 = a;
				ab.p1.z = ab.p2.z = b.z;
				ab.make_roundly_separated();

				bc.p0 = b;
				bc.p3 = c;
				bc.p1 = bc.p2 = c;
				bc.p1.z = bc.p2.z = b.z;
				bc.make_roundly_separated();

				ca.p0 = c;
				ca.p3 = a;
				ca.p1 = ca.p2 = (c + a).normal() * useRadius * sqrt(2.0f);
				ca.p1.z = ca.p2.z = 0.0f;
				ca.make_roundly_separated();

				BezierSurface patch = BezierSurface::create(ab, bc, ca);

				{	// add points
					int uMax = useSubStepCount;
					float uMaxInv = 1.0f / (float)uMax;
					for_range(int, u, 0, uMax)
					{
						int vMax = useSubStepCount - u;
						for_range(int, v, 0, vMax)
						{
							Vector2 uv = Vector2(clamp((float)u * uMaxInv, 0.0f, 1.0f), clamp((float)v * uMaxInv /* ! sum can't exceed 1 ! */, 0.0f, 1.0f));
							Vector3 p = patch.calculate_at(uv.x, uv.y);
							p = placeAt.location_to_world(p + offset);
							ipu.add_point(p);
						}
					}
				}
				{	// add triangles
					int uCount = useSubStepCount;
					int vCount = uCount * 2 - 1;
					int loIdx = pointIdxOffset;
					int hiIdx = loIdx + useSubStepCount + 1;
					for_count(int, u, uCount)
					{
						int hiStartingIdx = hiIdx;
						for_count(int, v, vCount)
						{
							bool lo = ((v % 2) == 0);
							if (lo)
							{
								ipu.add_triangle(0.0f, loIdx, hiIdx, loIdx + 1);
							}
							else
							{
								ipu.add_triangle(0.0f, hiIdx, hiIdx + 1, loIdx + 1);
								++loIdx;
								++hiIdx;
							}
						}
						vCount -= 2;
						loIdx = hiStartingIdx;
						++hiIdx; // next line
					}
				}

				pointIdxOffset = ipu.get_point_count();
			}

			// sides
			for_count(int, sideIdx, 4)
			{
				int hiIdx = onePatchPointCount * sideIdx;
				int loIdx = onePatchPointCount * (4 + (sideIdx + 1) % 4) + useSubStepCount;
				for_count(int, quad, useSubStepCount)
				{
					ipu.add_quad(0.0f, hiIdx, hiIdx + 1, loIdx - 1, loIdx);
					++hiIdx;
					--loIdx;
				}
			}
		}
	}

	if (_context.does_require_movement_collision() && useRadius > 0.0f)
	{
		if (auto const * data = fast_cast<Data>(_data))
		{
			if (data->createMovementCollision.create)
			{
				Collision::Model* part = new Collision::Model();
				Collision::Capsule capsule;
				capsule.setup(location - useHalfDir, location + useHalfDir, useRadius);
				capsule.use_material(data->createMovementCollision.get_physical_material(_context, _instance));
				part->add(capsule);
				_context.store_movement_collision_part(part);
			}
			if (data->createMovementCollisionBox.create)
			{
				_context.store_movement_collision_part(create_collision_box_from(ipu, _context, _instance, data->createMovementCollisionBox));
			}
			if (data->createMovementCollisionMesh.create)
			{
				_context.store_movement_collision_parts(create_collision_meshes_from(ipu, _context, _instance, data->createMovementCollisionMesh));
			}
		}
	}

	if (_context.does_require_precise_collision() && useRadius > 0.0f)
	{
		if (auto const * data = fast_cast<Data>(_data))
		{
			if (data->createPreciseCollision.create)
			{
				Collision::Model* part = new Collision::Model();
				Collision::Capsule capsule;
				capsule.setup(location - useHalfDir, location + useHalfDir, useRadius);
				capsule.use_material(data->createPreciseCollision.get_physical_material(_context, _instance));
				part->add(capsule);
				_context.store_precise_collision_part(part);
			}
			if (data->createPreciseCollisionBox.create)
			{
				_context.store_precise_collision_part(create_collision_box_from(ipu, _context, _instance, data->createPreciseCollisionBox));
			}
			if (data->createPreciseCollisionMesh.create)
			{
				_context.store_precise_collision_parts(create_collision_meshes_from(ipu, _context, _instance, data->createPreciseCollisionMesh));
			}
		}
	}

	if (_context.does_require_space_blockers() && useRadius > 0.0f)
	{
		if (auto const* data = fast_cast<Data>(_data))
		{
			if (data->createSpaceBlocker.create)
			{
				_context.store_space_blocker(create_space_blocker_from(ipu, _context, _instance, data->createSpaceBlocker));
			}
		}
	}

	if (fast_cast<Data>(_data) && fast_cast<Data>(_data)->noMesh)
	{
		ipu.keep_polygons(0);
	}

	//

	// prepare data for vertex and index formats, so further modifications may be applied and mesh can be imported
	ipu.prepare_data(_context.get_vertex_format(), _context.get_index_format(), vertexData, elementsData);

	int vertexCount = vertexData.get_size() / _context.get_vertex_format().get_stride();
	int elementsCount = elementsData.get_size() / _context.get_index_format().get_stride();

	//

	if (!ipu.is_empty())
	{
		// create part
		Meshes::Mesh3DPart* part = _context.get_generated_mesh()->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, _context.get_vertex_format(), _context.get_index_format());

#ifdef AN_DEVELOPMENT
		part->debugInfo = String::printf(TXT("capsule @ %S"), _instance.get_element()->get_location_info().to_char());
#endif

		_context.store_part(part, _instance);

		if (auto const * data = fast_cast<Data>(_data))
		{
			if (data->ignoreForCollision)
			{
				_context.ignore_part_for_collision(part);
			}
		}
	}

	return true;
}
