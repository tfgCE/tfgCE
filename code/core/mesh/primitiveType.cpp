#include "primitiveType.h"

#include "..\serialisation\serialiser.h"

using namespace Meshes;
using namespace Meshes::Primitive;

//

SIMPLE_SERIALISER_AS_INT32(Meshes::Primitive::Type);

//

uint32 Primitive::primitive_count_to_vertex_count(Primitive::Type _primitiveType, uint32 _numberOfPrimitives)
{
	if (_primitiveType == Primitive::Triangle)
	{
		return _numberOfPrimitives * 3;
	}
	else if (_primitiveType == Primitive::TriangleFan)
	{
		return _numberOfPrimitives + 2;
	}
#ifdef AN_GL
	else if (_primitiveType == Primitive::Quad)
	{
		return _numberOfPrimitives * 4;
	}
#endif
	else if (_primitiveType == Primitive::Line)
	{
		return _numberOfPrimitives * 2;
	}
	else if (_primitiveType == Primitive::LineStrip)
	{
		return _numberOfPrimitives + 1;
	}
	else if (_primitiveType == Primitive::LineLoop)
	{
		return _numberOfPrimitives;
	}
	else if (_primitiveType == Primitive::Point)
	{
		return _numberOfPrimitives;
	}
	else
	{
		an_assert(false, TXT("not recognised primitive type, implement_ it please"));
		return 0;
	}
}

uint32 Primitive::vertex_count_to_primitive_count(Primitive::Type _primitiveType, uint32 _numberOfVertices)
{
	if (_primitiveType == Primitive::Triangle)
	{
		return _numberOfVertices / 3;
	}
	else if (_primitiveType == Primitive::TriangleFan)
	{
		an_assert(_numberOfVertices >= 3, TXT("not enough points for triangle fan"));
		return _numberOfVertices - 2;
	}
#ifdef AN_GL
	else if (_primitiveType == Primitive::Quad)
	{
		return _numberOfVertices / 4;
	}
#endif
	else if (_primitiveType == Primitive::Line)
	{
		return _numberOfVertices / 2;
	}
	else if (_primitiveType == Primitive::LineStrip)
	{
		an_assert(_numberOfVertices >= 2, TXT("not enough points for line strip"));
		return _numberOfVertices - 1;
	}
	else if (_primitiveType == Primitive::LineLoop)
	{
		return _numberOfVertices;
	}
	else if (_primitiveType == Primitive::Point)
	{
		return _numberOfVertices;
	}
	else
	{
		an_assert(false, TXT("not recognised primitive type, implement_ it please"));
		return 0;
	}
}
