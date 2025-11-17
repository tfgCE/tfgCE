#pragma once

#include "..\globalDefinitions.h"

#include "..\serialisation\serialiserBasics.h"

#include "..\system\video\videoGL.h"

class Serialiser;

namespace Meshes
{
	namespace Primitive
	{
		enum Type
		{
			Triangle	= GL_TRIANGLES,
			TriangleFan	= GL_TRIANGLE_FAN,
#ifdef AN_GL
			Quad		= GL_QUADS,
#endif
			Line		= GL_LINES,
			LineStrip	= GL_LINE_STRIP,
			LineLoop	= GL_LINE_LOOP,
			Point		= GL_POINTS,
		};

		uint32 primitive_count_to_vertex_count(Primitive::Type _primitiveType, uint32 _numberOfPrimitives);
		uint32 vertex_count_to_primitive_count(Primitive::Type _primitiveType, uint32 _numberOfVertices);

		inline tchar const* to_char(Type _v)
		{
			if (_v == Triangle) return TXT("triangle");
			if (_v == TriangleFan) return TXT("triangle fan");
#ifdef AN_GL
			if (_v == Quad) return TXT("quad");
#endif
			if (_v == Line) return TXT("line");
			if (_v == LineStrip) return TXT("line strip");
			if (_v == LineLoop) return TXT("line loop");
			if (_v == Point) return TXT("point");
			return TXT("unknown");
		}
	};
	
};

DECLARE_SERIALISER_FOR(Meshes::Primitive::Type);
