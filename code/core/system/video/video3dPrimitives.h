#pragma once

#include "..\..\types\optional.h"

struct Colour;
struct Range2;
struct Vector2;
struct Vector3;

namespace System
{
	struct ShaderProgramInstance;
	struct VertexFormat;

	class Video3DPrimitives
	{
	public:
		static void initialise_static();
		static void close_static();

		static void line(Colour const & _colour, Vector3 const & _a, Vector3 const & _b, bool _blend = true);
		static void line_strip(Colour const& _colour, Vector3 const* _p, int _pointCount, bool _blend = true);
		static void triangle(Colour const & _colour, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, bool _blend = true);
		static void quad(Colour const & _colour, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, Vector3 const & _d, bool _blend = true);
		static void quad_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _b, Vector2 const & _c, Vector2 const & _d, bool _blend = true);

		static void fill_rect(Colour const & _colour, Vector3 const & _at, Vector2 const & _size, Vector2 const & _alignPt, bool _blend = true);
		static void fill_rect(Colour const & _colour, Range2 const& _r, bool _blend = true);
		static void fill_rect_xz(Colour const & _colour, Vector3 const & _at, Vector2 const & _size, Vector2 const & _alignPt, bool _blend = true);
		static void fill_rect_xz(Colour const & _colour, Range2 const& _r, bool _blend = true);
		static void fill_rect_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _b, bool _blend = true);
		static void fill_rect_2d(Colour const& _colour, Range2 const& _r, bool _blend = true);
		static void rect_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _b, bool _blend = true);
		static void rect_2d(Colour const & _colour, Range2 const& _r, bool _blend = true);
		static void line_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _b, bool _blend = true);
		static void line_strip_2d(Colour const & _colour, Vector2 const * _p, int _pointCount, bool _blend = true);
		static void triangle_2d(Colour const& _colour, Vector2 const& _a, Vector2 const& _b, Vector2 const& _c, bool _blend = true);
		static void point_2d(Colour const & _colour, Vector2 const & _a, bool _blend = true);
		static void points_2d(Colour const & _colour, Vector2 const * _a, int _count, bool _blend = true);
		static void fill_circle_xz(Colour const & _colour, Vector3 const & _a, float const & _radius, bool _blend = true);
		static void fill_circle_2d(Colour const & _colour, Vector2 const & _a, float const & _radius, bool _blend = true);
		static void fill_circle_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _radius, bool _blend = true);
		static void circle_xz(Colour const & _colour, Vector3 const & _a, float const & _radius, bool _blend = true);
		static void circle_2d(Colour const & _colour, Vector2 const & _a, float const & _radius, bool _blend = true);
		static void circle_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _radius, bool _blend = true);
		static void ring_xz(Colour const & _colour, Vector3 const & _a, float _innerRadius, float _outerRadius, bool _blend = true, float _startAngle = 0.0f, float _fillPt = 1.0f, Optional<float> const& _border = NP);
		static void ring_2d(Colour const & _colour, Vector2 const & _a, float _innerRadius, float _outerRadius, bool _blend = true, float _startAngle = 0.0f, float _fillPt = 1.0, Optional<float> const& _border = NP);

		static void fill_rect_2d(::System::ShaderProgramInstance* _shaderProgramInstance, Vector2 const & _a, Vector2 const & _b);

	private:
		static ::System::VertexFormat* s_vertexFormat2D;
		static ::System::VertexFormat* s_vertexFormat3D;
	};
};
