#include "..\meshGenAdvancedShapes.h"

#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenUtils.h"

#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\mesh3dBuilder.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace AdvancedShapes;

//

DEFINE_STATIC_NAME(mirrorHorizontaly);
DEFINE_STATIC_NAME(mirrorVerticaly);
DEFINE_STATIC_NAME(size);

//

/*
 *	Forward is in which direction is forward, from where observer should see the display
 */

class FlatDisplayData
: public ElementShapeData
{
	FAST_CAST_DECLARE(FlatDisplayData);
	FAST_CAST_BASE(ElementShapeData);
	FAST_CAST_END();

	typedef ElementShapeData base;

public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

public:
};

//

ElementShapeData* AdvancedShapes::create_flat_display_data()
{
	return new FlatDisplayData();
}

//

bool AdvancedShapes::flat_display(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Mesh3DBuilder builder;

	builder.setup(Meshes::Primitive::Triangle, _context.get_vertex_format(), 2);

	//

	if (FlatDisplayData const * csData = fast_cast<FlatDisplayData>(_data))
	{
		Vector2 useSize = Vector2::one;
		{
			FOR_PARAM(_context, Vector2, size)
			{
				useSize = *size;
			}
		}
		bool useMirrorHorizontaly = false;
		{
			FOR_PARAM(_context, bool, mirrorHorizontaly)
			{
				useMirrorHorizontaly = *mirrorHorizontaly;
			}
		}
		bool useMirrorVerticaly = false;
		{
			FOR_PARAM(_context, bool, mirrorVerticaly)
			{
				useMirrorVerticaly = *mirrorVerticaly;
			}
		}

		Vector2 halfSize = useSize * 0.5f;

		Vector2 blUV(useMirrorHorizontaly ? 1.0f : 0.0f, useMirrorVerticaly ? 1.0f : 0.0f);
		Vector2 trUV(useMirrorHorizontaly ? 0.0f : 1.0f, useMirrorVerticaly ? 0.0f : 1.0f);

		builder.add_triangle();
		builder.add_point();
		builder.location(Vector3(halfSize.x, 0.0f, -halfSize.y));
		builder.normal(Vector3(0.0f, 0.0f, 1.0f));
		builder.uv(blUV);
		builder.add_point();
		builder.location(Vector3(halfSize.x, 0.0f, halfSize.y));
		builder.normal(Vector3(0.0f, 0.0f, 1.0f));
		builder.uv(Vector2(blUV.x, trUV.y));
		builder.add_point();
		builder.location(Vector3(-halfSize.x, 0.0f, halfSize.y));
		builder.normal(Vector3(0.0f, 0.0f, 1.0f));
		builder.uv(trUV);
		builder.done_triangle();

		builder.add_triangle();
		builder.add_point();
		builder.location(Vector3(halfSize.x, 0.0f, -halfSize.y));
		builder.normal(Vector3(0.0f, 0.0f, 1.0f));
		builder.uv(blUV);
		builder.add_point();
		builder.location(Vector3(-halfSize.x, 0.0f, halfSize.y));
		builder.normal(Vector3(0.0f, 0.0f, 1.0f));
		builder.uv(trUV);
		builder.add_point();
		builder.location(Vector3(-halfSize.x, 0.0f, -halfSize.y));
		builder.normal(Vector3(0.0f, 0.0f, 1.0f));
		builder.uv(Vector2(trUV.x, blUV.y));
		builder.done_triangle();
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

REGISTER_FOR_FAST_CAST(FlatDisplayData);

bool FlatDisplayData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool FlatDisplayData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}