#include "lhw_image.h"

#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\framework\video\texturePart.h"

using namespace Loader;

//

REGISTER_FOR_FAST_CAST(HubWidgets::Image);

HubWidgets::Image::Image(Name const & _id, Vector2 const & _at, Framework::TexturePart* _texturePart, Optional<Vector2> const & _scale, Optional<Colour> const & _colour)
: base(_id, Range2(_at).include(_at + (_texturePart ? (_texturePart->get_left_bottom_offset() * _scale.get(Vector2::one).x) : Vector2::zero))
					   .include(_at + (_texturePart ? (_texturePart->get_right_top_offset() * _scale.get(Vector2::one).y) : Vector2::zero)))
, texturePart(_texturePart)
, scale(_scale.get(Vector2::one))
, colour(_colour.get(Colour::white))
{
}

void HubWidgets::Image::render_to_display(Framework::Display* _display)
{
	if (texturePart)
	{
		Framework::TexturePartDrawingContext context;
		//context.limits = at;
		Range2 usableAt = at;
		Range2 tpOffsets = Range2::empty;
		tpOffsets.include(texturePart->get_left_bottom_offset() * scale);
		tpOffsets.include(texturePart->get_right_top_offset() * scale);
		usableAt.x.min -= tpOffsets.bottom_left().x;
		usableAt.y.min -= tpOffsets.bottom_left().y;
		usableAt.x.max -= tpOffsets.top_right().x;
		usableAt.y.max -= tpOffsets.top_right().y;
		context.scale = scale;
		context.colour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(colour, screen, this);
		texturePart->draw_at(::System::Video3D::get(), round(usableAt.get_at(alignPt)), context);
	}

	base::render_to_display(_display);
}

