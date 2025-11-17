#include "loaderZX.h"

#include "..\..\core\math\math.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\video3dPrimitives.h"

using namespace Loader;

void ZX::update(float _deltaTime)
{
	if (loadingTS.get_time_since() > 0.05f)
	{
		loadingTS.reset();

		startingY.clear();

		loadingBits.set_size(512);
		for_every(loadingBit, loadingBits)
		{
			*loadingBit = randomGenerator.get_bool();
		}
	}
}

void ZX::display(System::Video3D * _v3d, bool _vr)
{
	an_assert(!_vr, TXT("implement_ vr!"));

	System::RenderTarget::bind_none();
	_v3d->set_default_viewport();
	_v3d->set_near_far_plane(0.02f, 100.0f);

	// ready sizes
	Vector2 screenSize(256.0f, 192.0f);
	Vector2 borderSize(32.0f, 24.0f);
	Vector2 wholeDisplaySize = screenSize + borderSize * 2.0f;
	wholeDisplaySize.x = wholeDisplaySize.y * _v3d->get_aspect_ratio();
	Vector2 screenOffset = 0.5f * (wholeDisplaySize - screenSize);

	_v3d->set_2d_projection_matrix_ortho(wholeDisplaySize);
	_v3d->access_model_view_matrix_stack().clear();

	_v3d->setup_for_2d_display();


	// display loading

	// border
	//_v3d->clear_colour_depth_stencil(Colour::zxYellow);
	::System::Video3DPrimitives::fill_rect_2d(Colour::zxYellow, Vector2::zero, wholeDisplaySize);

	float maxBitSize = 3.0f;
	float bitScale = 2.9f;
	
	if (!startingY.is_set())
	{
		if (y > wholeDisplaySize.y)
		{
			y -= wholeDisplaySize.y;
		}
		while (y > 0.0f)
		{
			y -= maxBitSize * bitScale;
		}
		startingY = y;
	}
	y = startingY.get();

	for_every(loadingBit, loadingBits)
	{
		float bitSize = *loadingBit ? 1.0f : 2.5f;
		for_count(int, i, 2)
		{
			if (i)
			{
				::System::Video3DPrimitives::fill_rect_2d(Colour::zxBlue,
					Vector2(0.0f, wholeDisplaySize.y - y - bitSize),
					Vector2(wholeDisplaySize.x, wholeDisplaySize.y - y));
			}
			y += bitSize * bitScale;
			bitSize = maxBitSize - bitSize;
		}
		if (y > wholeDisplaySize.y)
		{
			break;
		}
	}

	// actual screen
	::System::Video3DPrimitives::fill_rect_2d(Colour::zxWhite, screenOffset, screenOffset + screenSize);

	tchar const * bytes[] = {TXT("........ ........ ........ ........ ........ ........ ........ ........ ........ ........ ........ "),
							 TXT(".#####.. ........ ...#.... ........ ........ ........ ........ .....#.. ........ ...#.... ........ "),
							 TXT(".#....#. .#....#. ..###... ..###... ..###... ........ ........ .....#.. ..###... ..###... ..###... "),
							 TXT(".#####.. .#....#. ...#.... .#...#.. .#...... ....#... ........ ..####.. .....#.. ...#.... .....#.. "),
							 TXT(".#....#. .#....#. ...#.... .####... ..###... ........ ........ .#...#.. ..####.. ...#.... ..####.. "),
							 TXT(".#....#. ..#####. ...#.... .#...... .....#.. ........ ........ .#...#.. .#...#.. ...#.... .#...#.. "),
							 TXT(".#####.. ......#. ....##.. ..####.. .####... ....#... ........ ..####.. ..####.. ....##.. ..####.. "),
							 TXT("........ ..####.. ........ ........ ........ ........ ........ ........ ........ ........ ........ ") };

	draw_blocks_at(_v3d, screenOffset, Colour::zxBlack, VectorInt2(0, 22), bytes);
}

void ZX::draw_blocks_at(System::Video3D * _v3d, Vector2 const & _screenOffset, Colour const & _colour, VectorInt2 const & _at, tchar const * const * _bytes)
{
	Vector2 at = _at.to_vector2();
	for (int line = 0; line < 8; ++line)
	{
		tchar const * pixel = _bytes[line];
		Vector2 pixelAt = _screenOffset + at * 8.0f;
		pixelAt.y += (float)(7 - line);
		while (*pixel != 0)
		{
			if (*pixel != ' ')
			{
				if (*pixel == '#')
				{
					::System::Video3DPrimitives::fill_rect_2d(_colour, pixelAt, pixelAt);
				}
				pixelAt.x += 1.0f;
			}
			++pixel;
		}
	}
}
