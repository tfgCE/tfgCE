#include "video3dPrimitives.h"

#include "..\..\containers\arrayStack.h"

#include "..\..\math\math.h"
#include "..\..\mesh\mesh3d.h"
#include "video3d.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace ::System;

//

::System::VertexFormat* Video3DPrimitives::s_vertexFormat2D = nullptr;
::System::VertexFormat* Video3DPrimitives::s_vertexFormat3D = nullptr;

void Video3DPrimitives::initialise_static()
{
	// primitives require always to set attrib pointers as this may indicate that the data there has changed
	// otherwise the driver may expect that there is the same data and ignore loading it
	s_vertexFormat2D = new ::System::VertexFormat();
	s_vertexFormat2D->with_location_2d().no_padding().calculate_stride_and_offsets().used_for_dynamic_data();

	s_vertexFormat3D = new ::System::VertexFormat();
	s_vertexFormat3D->no_padding().calculate_stride_and_offsets().used_for_dynamic_data();
}

void Video3DPrimitives::close_static()
{
	delete_and_clear(s_vertexFormat2D);
	delete_and_clear(s_vertexFormat3D);
}

void Video3DPrimitives::fill_rect(Colour const& _colour, Vector3 const& _at, Vector2 const& _size, Vector2 const& _alignPt, bool _blend)
{
	an_assert(Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Range2 r = Range2::empty;
	r.include(_size * (-_alignPt));
	r.include(_size * (Vector2::one - _alignPt));
	Vector3 points[4];
	points[0] = _at + Vector3(r.x.min, r.y.min, 0.0f);
	points[1] = _at + Vector3(r.x.min, r.y.max, 0.0f);
	points[2] = _at + Vector3(r.x.max, r.y.max, 0.0f);
	points[3] = _at + Vector3(r.x.max, r.y.min, 0.0f);
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::fill_rect(Colour const& _colour, Range2 const& _r, bool _blend)
{
	an_assert(Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Range2 r = _r;
	Vector3 points[4];
	points[0] = Vector3(r.x.min, r.y.min, 0.0f);
	points[1] = Vector3(r.x.min, r.y.max, 0.0f);
	points[2] = Vector3(r.x.max, r.y.max, 0.0f);
	points[3] = Vector3(r.x.max, r.y.min, 0.0f);
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::fill_rect_xz(Colour const& _colour, Vector3 const& _at, Vector2 const& _size, Vector2 const& _alignPt, bool _blend)
{
	an_assert(Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Range2 r = Range2::empty;
	r.include(_size * (-_alignPt));
	r.include(_size * (Vector2::one - _alignPt));
	Vector3 points[4];
	points[0] = _at + Vector3(r.x.min, 0.0f, r.y.min);
	points[1] = _at + Vector3(r.x.min, 0.0f, r.y.max);
	points[2] = _at + Vector3(r.x.max, 0.0f, r.y.max);
	points[3] = _at + Vector3(r.x.max, 0.0f, r.y.min);
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::fill_rect_xz(Colour const& _colour, Range2 const& _r, bool _blend)
{
	an_assert(Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Range2 r = _r;
	Vector3 points[4];
	points[0] = Vector3(r.x.min, 0.0f, r.y.min);
	points[1] = Vector3(r.x.min, 0.0f, r.y.max);
	points[2] = Vector3(r.x.max, 0.0f, r.y.max);
	points[3] = Vector3(r.x.max, 0.0f, r.y.min);
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::fill_rect_2d(::System::ShaderProgramInstance* _shaderProgramInstance, Vector2 const & _a, Vector2 const & _b)
{
	an_assert(!Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Vector2 a = floor(_a) + Vector2::half;
	Vector2 b = floor(_b) + Vector2::half;
	if (a.x <= b.x) b.x += 1.0f; else a.x += 1.0f;
	if (a.y <= b.y) b.y += 1.0f; else a.y += 1.0f;

	Video3D* v3d = Video3D::get();
	Vector2 points[4];
	points[0] = Vector2(a.x, a.y);
	points[1] = Vector2(a.x, b.y);
	points[2] = Vector2(b.x, b.y);
	points[3] = Vector2(b.x, a.y);
	Meshes::Mesh3D::render_data(v3d, _shaderProgramInstance, nullptr, Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat2D);
}

void Video3DPrimitives::fill_rect_2d(Colour const& _colour, Range2 const& _r, bool _blend)
{
	fill_rect_2d(_colour, Vector2(_r.x.min, _r.y.min), Vector2(_r.x.max, _r.y.max), _blend);
}

void Video3DPrimitives::fill_rect_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _b, bool _blend)
{
	assert_rendering_thread();
	Vector2 a = floor(_a) + Vector2::half;
	Vector2 b = floor(_b) + Vector2::half;
	if (a.x <= b.x) b.x += 1.0f; else a.x += 1.0f;
	if (a.y <= b.y) b.y += 1.0f; else a.y += 1.0f;

	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	if (v3d->is_for_3d())
	{
		Vector3 points[4];
		points[0] = Vector2(a.x, a.y).to_vector3_xz();
		points[1] = Vector2(a.x, b.y).to_vector3_xz();
		points[2] = Vector2(b.x, b.y).to_vector3_xz();
		points[3] = Vector2(b.x, a.y).to_vector3_xz();
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat3D);
	}
	else
	{
		Vector2 points[4];
		points[0] = Vector2(a.x, a.y);
		points[1] = Vector2(a.x, b.y);
		points[2] = Vector2(b.x, b.y);
		points[3] = Vector2(b.x, a.y);
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat2D);
	}
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::rect_2d(Colour const& _colour, Range2 const& _r, bool _blend)
{
	rect_2d(_colour, Vector2(_r.x.min, _r.y.min), Vector2(_r.x.max, _r.y.max), _blend);
}

void Video3DPrimitives::rect_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _b, bool _blend)
{
	an_assert(!Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Vector2 a = floor(_a) + Vector2::half;
	Vector2 b = floor(_b) + Vector2::half;
	// draw extra last pixel
	if (a.x <= b.x) b.x += 1.0f; else a.x += 1.0f;
	if (a.y <= b.y) b.y += 1.0f; else a.y += 1.0f;

	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Vector2 points[4];
	points[0] = Vector2(a.x, a.y);
	points[1] = Vector2(a.x, b.y);
	points[2] = Vector2(b.x, b.y);
	points[3] = Vector2(b.x, a.y);
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::LineLoop, 4, *s_vertexFormat2D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::line_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _b, bool _blend)
{
	an_assert(!Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Vector2 points[3];
	// for lines we want to render in the centre of pixel to actually render at the pixel - this is how opengl works
	Vector2 a = floor(_a) + Vector2::half;
	Vector2 b = floor(_b) + Vector2::half;
	points[0] = a;
	points[1] = b;
	points[2] = a; // draw that last pixel
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::LineStrip, 2, *s_vertexFormat2D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::line_strip_2d(Colour const & _colour, Vector2 const * _p, int _pointCount, bool _blend)
{
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	if (Video3D::get()->is_for_3d())
	{
		ARRAY_STACK(Vector3, points, _pointCount);
		points.set_size(_pointCount);
		Vector2 const* p = _p;
		for_every(point, points)
		{
			*point = ((*p) + Vector2::half).to_vector3_xz();
			p++;
		}
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), points.get_data(), ::Meshes::Primitive::LineStrip, _pointCount - 1, *s_vertexFormat3D);
	}
	else
	{
		ARRAY_STACK(Vector2, points, _pointCount);
		points.set_size(_pointCount);
		Vector2 const* p = _p;
		for_every(point, points)
		{
			*point = (*p) + Vector2::half;
			p++;
		}
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), points.get_data(), ::Meshes::Primitive::LineStrip, _pointCount - 1, *s_vertexFormat2D);
	}
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}


void Video3DPrimitives::point_2d(Colour const & _colour, Vector2 const & _a, bool _blend)
{
	an_assert(!Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Vector2 points[2];
	// for lines we want to render in the centre of pixel to actually render at the pixel - this is how opengl works
	Vector2 a = floor(_a) + Vector2::half;
	points[0] = a;
	points[1] = a + Vector2(1.0f, 0.0f); // to draw first pixel and skip last one
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::Line, 1, *s_vertexFormat2D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::points_2d(Colour const & _colour, Vector2 const * _a, int _count, bool _blend)
{
	an_assert(!Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	ARRAY_STACK(Vector2, points, _count * 2);
	Vector2 const * src = _a;
	for_count(int, i, _count)
	{
		// for lines we want to render in the centre of pixel to actually render at the pixel - this is how opengl works
		Vector2 a = floor(*src) + Vector2::half;
		points.push_back(a);
		points.push_back(a + Vector2(1.0f, 0.0f)); // to draw first pixel and skip last one
		++src;
	}
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points.get_data(), ::Meshes::Primitive::Line, _count, *s_vertexFormat2D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::fill_circle_2d(Colour const & _colour, Vector2 const & _a, float const & _radius, bool _blend)
{
	an_assert(!Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	int const count = 72;
	Vector2 points[count + 2];
	float const angleStep = 360.0f / (float)count;
	points[0] = _a;
	int i = 1;
	for (float angle = 0; angle <= 360.0f; angle += angleStep, ++ i)
	{
		an_assert(i < count + 2);
		Vector2 b = _a + Vector2(_radius, 0.0f).rotated_by_angle(angle);
		points[i] = b;
	}
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, count, *s_vertexFormat2D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::fill_circle_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _radius, bool _blend)
{
	an_assert(!Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	int const count = 72;
	Vector2 points[count + 2];
	float const angleStep = 360.0f / (float)count;
	points[0] = _a;
	int i = 1;
	for (float angle = 0; angle <= 360.0f; angle += angleStep, ++ i)
	{
		an_assert(i < count + 2);
		Vector2 b = _a + Vector2(1.0f, 0.0f).rotated_by_angle(angle) * _radius;
		points[i] = b;
	}
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, count, *s_vertexFormat2D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::circle_2d(Colour const& _colour, Vector2 const& _a, float const& _radius, bool _blend)
{
	an_assert(!Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	int const count = 72;
	Vector2 points[count + 1];
	float const angleStep = 360.0f / (float)count;
	int i = 0;
	for (float angle = 0; angle <= 360.0f; angle += angleStep, ++i)
	{
		an_assert(i < count + 1);
		Vector2 b = _a + Vector2(_radius, 0.0f).rotated_by_angle(angle);
		points[i] = b;
	}
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::LineStrip, count, *s_vertexFormat2D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::circle_2d(Colour const& _colour, Vector2 const& _a, Vector2 const& _radius, bool _blend)
{
	an_assert(!Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	int const count = 72;
	Vector2 points[count + 1];
	float const angleStep = 360.0f / (float)count;
	int i = 0;
	for (float angle = 0; angle <= 360.0f; angle += angleStep, ++i)
	{
		an_assert(i < count + 1);
		Vector2 b = _a + Vector2(1.0f, 0.0f).rotated_by_angle(angle) * _radius;
		points[i] = b;
	}
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::LineStrip, count, *s_vertexFormat2D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::fill_circle_xz(Colour const & _colour, Vector3 const & _a, float const & _radius, bool _blend)
{
	an_assert(Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	int const count = 72;
	Vector3 points[count + 2];
	float const angleStep = 360.0f / (float)count;
	points[0] = _a;
	int i = 1;
	for (float angle = 0; angle <= 360.0f; angle += angleStep, ++ i)
	{
		an_assert(i < count + 2);
		Vector3 b = _a + Vector3(_radius, 0.0f, 0.0f).rotated_by_roll(angle);
		points[i] = b;
	}
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, count, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::circle_xz(Colour const & _colour, Vector3 const & _a, float const & _radius, bool _blend)
{
	an_assert(Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	int const count = 72;
	Vector3 points[count + 1];
	float const angleStep = 360.0f / (float)count;
	int i = 0;
	for (float angle = 0; angle <= 360.0f; angle += angleStep, ++ i)
	{
		an_assert(i < count + 1);
		Vector3 b = _a + Vector3(_radius, 0.0f, 0.0f).rotated_by_roll(angle);
		points[i] = b;
	}
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::LineStrip, count, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::ring_xz(Colour const& _colour, Vector3 const& _a, float _innerRadius, float _outerRadius, bool _blend, float _startAngle, float _fillPt, Optional<float> const & _border)
{
	an_assert(Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	int const count = 72;
	Vector3 points[count * 6];
	float const angleStep = 360.0f / (float)count;
	int i = 0;
	for (float angle = 0; angle < 360.0f; angle += angleStep)
	{
		float actAngle = angle * _fillPt - 90.0f + _startAngle;
		an_assert(i < count * 6);
		float nextAngle = actAngle + angleStep * _fillPt;
		// first
		{
			Vector3 b = _a + Vector3(_innerRadius, 0.0f, 0.0f).rotated_by_roll(actAngle);
			points[i] = b;
			++i;
		}
		{
			Vector3 b = _a + Vector3(_outerRadius, 0.0f, 0.0f).rotated_by_roll(actAngle);
			points[i] = b;
			++i;
		}
		{
			Vector3 b = _a + Vector3(_outerRadius, 0.0f, 0.0f).rotated_by_roll(nextAngle);
			points[i] = b;
			++i;
		}
		// second
		{
			Vector3 b = _a + Vector3(_innerRadius, 0.0f, 0.0f).rotated_by_roll(actAngle);
			points[i] = b;
			++i;
		}
		{
			Vector3 b = _a + Vector3(_outerRadius, 0.0f, 0.0f).rotated_by_roll(nextAngle);
			points[i] = b;
			++i;
		}
		{
			Vector3 b = _a + Vector3(_innerRadius, 0.0f, 0.0f).rotated_by_roll(nextAngle);
			points[i] = b;
			++i;
		}
	}
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::Triangle, count * 2, *s_vertexFormat3D);

	if (_border.is_set())
	{
		for (int borderIdx = 0; borderIdx < 2; ++borderIdx)
		{
			float borderRadius = borderIdx == 0 ? (_innerRadius - _border.get()) : (_outerRadius + _border.get());
			int const count = 72;
			Vector3 points[count + 1];
			float const angleStep = 360.0f / (float)count;
			int i = 0;
			for (float angle = 0; angle < 360.0f; angle += angleStep)
			{
				float actAngle = angle;
				an_assert(i < count + 1);
				{
					points[i] = _a + Vector3(borderRadius, 0.0f, 0.0f).rotated_by_roll(actAngle);
					++i;
				}
			}
			points[count] = points[0];
			Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
				Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::LineStrip, count, *s_vertexFormat3D);
		}
	}

	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::ring_2d(Colour const& _colour, Vector2 const& _a, float _innerRadius, float _outerRadius, bool _blend, float _startAngle, float _fillPt, Optional<float> const& _border)
{
	an_assert(! Video3D::get()->is_for_3d());
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	int const count = 72;
	Vector2 points[count * 6];
	float const angleStep = 360.0f / (float)count;
	int i = 0;
	for (float angle = 0; angle < 360.0f; angle += angleStep)
	{
		float actAngle = angle * _fillPt - 90.0f + _startAngle;
		an_assert(i < count * 6);
		float nextAngle = actAngle + angleStep * _fillPt;
		// first
		{
			Vector2 b = _a + Vector2(_innerRadius, 0.0f).rotated_by_angle(actAngle);
			points[i] = b;
			++i;
		}
		{
			Vector2 b = _a + Vector2(_outerRadius, 0.0f).rotated_by_angle(actAngle);
			points[i] = b;
			++i;
		}
		{
			Vector2 b = _a + Vector2(_outerRadius, 0.0f).rotated_by_angle(nextAngle);
			points[i] = b;
			++i;
		}
		// second
		{
			Vector2 b = _a + Vector2(_innerRadius, 0.0f).rotated_by_angle(actAngle);
			points[i] = b;
			++i;
		}
		{
			Vector2 b = _a + Vector2(_outerRadius, 0.0f).rotated_by_angle(nextAngle);
			points[i] = b;
			++i;
		}
		{
			Vector2 b = _a + Vector2(_innerRadius, 0.0f).rotated_by_angle(nextAngle);
			points[i] = b;
			++i;
		}
	}
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::Triangle, count * 2, *s_vertexFormat2D);

	if (_border.is_set())
	{
		for (int borderIdx = 0; borderIdx < 2; ++borderIdx)
		{
			float borderRadius = borderIdx == 0 ? (_innerRadius - _border.get()) : (_outerRadius + _border.get());
			int const count = 72;
			Vector2 points[count + 1];
			float const angleStep = 360.0f / (float)count;
			int i = 0;
			for (float angle = 0; angle < 360.0f; angle += angleStep)
			{
				float actAngle = angle;
				an_assert(i < count + 1);
				{
					points[i] = _a + Vector2(borderRadius, 0.0f).rotated_by_angle(actAngle);
					++i;
				}
			}
			points[count] = points[0];
			Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
				Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::LineStrip, count, *s_vertexFormat2D);
		}
	}

	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::line(Colour const & _colour, Vector3 const & _a, Vector3 const & _b, bool _blend)
{
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Vector3 points[2];
	points[0] = _a;
	points[1] = _b;
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr, 
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::Line, 1, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::line_strip(Colour const& _colour, Vector3 const* _p, int _pointCount, bool _blend)
{
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), _p, ::Meshes::Primitive::LineStrip, _pointCount - 1, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::triangle(Colour const & _colour, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, bool _blend)
{
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Vector3 points[3];
	points[0] = _a;
	points[1] = _b;
	points[2] = _c;
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr, 
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::Triangle, 1, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::triangle_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _b, Vector2 const & _c, bool _blend)
{
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	if (v3d->is_for_3d())
	{
		Vector3 points[3];
		points[0] = Vector2(_a.x, _a.y).to_vector3_xz();
		points[1] = Vector2(_b.x, _b.y).to_vector3_xz();
		points[2] = Vector2(_c.x, _c.y).to_vector3_xz();
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr, 
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::Triangle, 1, *s_vertexFormat3D);
	}
	else
	{
		Vector2 points[3];
		points[0] = _a;
		points[1] = _b;
		points[2] = _c;
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr, 
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::Triangle, 1, *s_vertexFormat2D);
	}

	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::quad(Colour const & _colour, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, Vector3 const& _d, bool _blend)
{
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	Vector3 points[4];
	points[0] = _a;
	points[1] = _b;
	points[2] = _c;
	points[3] = _d;
	Meshes::Mesh3D::render_data(v3d, nullptr, nullptr, 
		Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat3D);
	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

void Video3DPrimitives::quad_2d(Colour const & _colour, Vector2 const & _a, Vector2 const & _b, Vector2 const & _c, Vector2 const& _d, bool _blend)
{
	assert_rendering_thread();
	Video3D* v3d = Video3D::get();
	if (_blend)
	{
		v3d->mark_enable_blend();
	}
	else
	{
		v3d->mark_disable_blend();
	}
	v3d->set_fallback_colour(_colour);
	if (v3d->is_for_3d())
	{
		Vector3 points[4];
		points[0] = Vector2(_a.x, _a.y).to_vector3_xz();
		points[1] = Vector2(_b.x, _b.y).to_vector3_xz();
		points[2] = Vector2(_c.x, _c.y).to_vector3_xz();
		points[3] = Vector2(_d.x, _d.y).to_vector3_xz();
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr, 
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat3D);
	}
	else
	{
		Vector2 points[4];
		points[0] = _a;
		points[1] = _b;
		points[2] = _c;
		points[3] = _d;
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr, 
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), points, ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat2D);
	}

	v3d->set_fallback_colour();
	v3d->mark_disable_blend();
}

