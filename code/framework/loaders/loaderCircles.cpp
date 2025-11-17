#include "loaderCircles.h"

#include "..\..\core\math\math.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\video3dPrimitives.h"

using namespace Loader;

Circles::Circles(Colour const & _backgroundColour, Colour const & _circleColour)
: angle(0.0f)
, speed(360.0f)
, circleCount(6)
, radius(0.1f)
, circleRadius(0.01f)
, backgroundColour(_backgroundColour)
, circleColour(_circleColour)
{
}

void Circles::update(float _deltaTime)
{
	angle += _deltaTime * speed;
	while (angle > 360.0f)
	{
		angle -= 360.0f;
	}
}

void Circles::display(System::Video3D * _v3d, bool _vr)
{
	an_assert(!_vr, TXT("implement_ vr!"));

	System::RenderTarget::bind_none();
	_v3d->set_default_viewport();
	_v3d->set_near_far_plane(0.02f, 100.0f);

	// ready sizes
	Vector2 size(100.0f, 100.0f);
	size.x = size.y * _v3d->get_aspect_ratio();

	_v3d->set_2d_projection_matrix_ortho(size);
	_v3d->access_model_view_matrix_stack().clear();

	_v3d->setup_for_2d_display();


	// display circles

	_v3d->clear_colour_depth_stencil(backgroundColour);

	for (int i = 0; i < circleCount; ++i)
	{
		Vector2 centre = size * 0.5f + Vector2(radius * size.y, 0.0f).rotated_by_angle(angle + 360.0f * (float)i / (float)circleCount);
		::System::Video3DPrimitives::fill_circle_2d(circleColour, centre, circleRadius * size.y);
	}
}
