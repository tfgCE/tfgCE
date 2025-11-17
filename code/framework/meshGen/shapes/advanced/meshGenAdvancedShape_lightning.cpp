#include "..\meshGenAdvancedShapes.h"

#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenUtils.h"
#include "..\..\meshGenParam.h"

#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\mesh3dBuilder.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace AdvancedShapes;

//

DEFINE_STATIC_NAME(radius);
DEFINE_STATIC_NAME(sides);
DEFINE_STATIC_NAME(segmentLength);
DEFINE_STATIC_NAME(segmentCount);
DEFINE_STATIC_NAME(segmentName); // name0suffix
DEFINE_STATIC_NAME(segmentSuffixStart);
DEFINE_STATIC_NAME(segmentSuffixEnd);

//

DEFINE_STATIC_NAME(segment);
DEFINE_STATIC_NAME(start);
DEFINE_STATIC_NAME(end);

//

/*
 *	Forward is in which direction is forward, from where observer should see the display
 */

class LightningData
: public ElementShapeData
{
	FAST_CAST_DECLARE(LightningData);
	FAST_CAST_BASE(ElementShapeData);
	FAST_CAST_END();

	typedef ElementShapeData base;

public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

public:
};

//

ElementShapeData* AdvancedShapes::create_lightning_data()
{
	return new LightningData();
}

//

bool AdvancedShapes::lightning(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Mesh3DBuilder builder;

	builder.setup(Meshes::Primitive::Triangle, _context.get_vertex_format(), 2);

	//

	if (LightningData const * csData = fast_cast<LightningData>(_data))
	{
		float useRadius = 0.01f;
		{
			FOR_PARAM(_context, float, radius)
			{
				useRadius = *radius;
			}
		}
		int useSides = 3;
		{
			FOR_PARAM(_context, int, sides)
			{
				useSides = *sides;
			}
		}
		float segLength = 0.1f;
		{
			FOR_PARAM(_context, float, segmentLength)
			{
				segLength = *segmentLength;
			}
		}
		int segCount = 0;
		{
			FOR_PARAM(_context, int, segmentCount)
			{
				segCount = *segmentCount;
			}
		}
		Name segName = NAME(segment);
		{
			FOR_PARAM(_context, Name, segmentName)
			{
				segName = *segmentName;
			}
		}
		Name segSuffixStart = NAME(start);
		{
			FOR_PARAM(_context, Name, segmentSuffixStart)
			{
				segSuffixStart = *segmentSuffixStart;
			}
		}
		Name segSuffixEnd = NAME(end);
		{
			FOR_PARAM(_context, Name, segmentSuffixEnd)
			{
				segSuffixEnd = *segmentSuffixEnd;
			}
		}

		Array<int> boneIndices;
		boneIndices.set_size(segCount * 2);
		Array<Vector3> prevPoints;
		Array<Vector3> currPoints;
		int prevBone = 0;
		int currBone = 0;
		float aStep = 360.0f / (float)useSides;
		Vector2 uv = Vector2::zero;
		for_count(int, i, segCount)
		{
			float iAsFloat = (float)i;
			_context.add_generated_bone(Name(String::printf(TXT("%S%i%S"), segName.to_char(), i, segSuffixStart.to_char())), _instance, &boneIndices[i * 2 + 0]);
			_context.add_generated_bone(Name(String::printf(TXT("%S%i%S"), segName.to_char(), i, segSuffixEnd.to_char())), _instance, &boneIndices[i * 2 + 1]);
			float yStart = iAsFloat * segLength;
			float yEnd = (iAsFloat + 1.0f) * segLength;
			int boneStart = boneIndices[i * 2 + 0];
			int boneEnd = boneIndices[i * 2 + 1];
			{
				Bone& bone = _context.access_generated_bones()[boneStart];
				bone.placement = look_matrix(Vector3::yAxis * yStart, Vector3::yAxis, Vector3::zAxis).to_transform();
			}
			{
				Bone& bone = _context.access_generated_bones()[boneEnd];
				bone.placement = look_matrix(Vector3::yAxis * yEnd, -Vector3::yAxis, Vector3::zAxis).to_transform();
			}
			yStart += useRadius * 0.5f;
			yEnd -= useRadius * 0.5f;
			for_count(int, c, 2)
			{
				if (c == 0 && i == 0) // start
				{
					currPoints.set_size(useSides);
					currBone = boneStart;
					float a = 0.0f;
					for_count(int, s, useSides)
					{
						currPoints[s].x = useRadius * sin_deg(a);
						currPoints[s].z = useRadius * cos_deg(a);
						currPoints[s].y = yStart;
						a += aStep;
					}
					builder.uv(uv);
					builder.skin_to_single(currBone);
					Vector3 startPoint = Vector3::zero;
					for_count(int, s, useSides)
					{
						int ns = (s + 1) % useSides;
						builder.add_triangle();
						builder.add_point();
						builder.location(startPoint);
						builder.normal(-Vector3::yAxis);
						builder.add_point();
						builder.location(currPoints[s]);
						builder.normal(currPoints[s] * Vector3::xz);
						builder.add_point();
						builder.location(currPoints[ns]);
						builder.normal(currPoints[ns] * Vector3::xz);
						builder.done_triangle();
					}
				}
				else // connector
				{
					prevBone = currBone;
					currBone = c == 0 ? boneStart : boneEnd;
					prevPoints = currPoints;
					currPoints.set_size(useSides);
					for_count(int, s, useSides)
					{
						currPoints[s].y = c == 0? yStart : yEnd;
					}
					builder.uv(uv);
					for_count(int, s, useSides)
					{
						int ns = (s + 1) % useSides;
						builder.add_triangle();
						builder.add_point();
						builder.location(prevPoints[s]);
						builder.normal(prevPoints[s] * Vector3::xz);
						builder.skin_to_single(prevBone);
						builder.add_point();
						builder.location(currPoints[s]);
						builder.normal(currPoints[s] * Vector3::xz);
						builder.skin_to_single(currBone);
						builder.add_point();
						builder.location(currPoints[ns]);
						builder.normal(currPoints[ns] * Vector3::xz);
						builder.skin_to_single(currBone);
						builder.done_triangle();
						builder.add_triangle();
						builder.add_point();
						builder.location(prevPoints[s]);
						builder.normal(prevPoints[s] * Vector3::xz);
						builder.skin_to_single(prevBone);
						builder.add_point();
						builder.location(currPoints[ns]);
						builder.normal(currPoints[ns] * Vector3::xz);
						builder.skin_to_single(currBone);
						builder.add_point();
						builder.location(prevPoints[ns]);
						builder.normal(prevPoints[ns] * Vector3::xz);
						builder.skin_to_single(prevBone);
						builder.done_triangle();
					}
				}

				// extra
				if (c == 1 && i == segCount - 1) // end
				{
					builder.uv(uv);
					builder.skin_to_single(currBone);
					Vector3 endPoint = Vector3::yAxis * ((float)segCount * segLength);
					for_count(int, s, useSides)
					{
						int ns = (s + 1) % useSides;
						builder.add_triangle();
						builder.add_point();
						builder.location(currPoints[s]);
						builder.normal(currPoints[s] * Vector3::xz);
						builder.add_point();
						builder.location(endPoint);
						builder.normal(-Vector3::yAxis);
						builder.add_point();
						builder.location(currPoints[ns]);
						builder.normal(currPoints[ns] * Vector3::xz);
						builder.done_triangle();
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

REGISTER_FOR_FAST_CAST(LightningData);

bool LightningData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool LightningData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}