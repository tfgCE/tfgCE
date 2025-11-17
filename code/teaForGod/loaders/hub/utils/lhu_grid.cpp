#include "lhu_grid.h"

#include "lhu_colours.h"

#include "..\widgets\lhw_grid.h"

#include "..\..\..\..\core\system\video\video3dPrimitives.h"

//

using namespace Loader;

//

// shader param
DEFINE_STATIC_NAME(highlightColour);
DEFINE_STATIC_NAME(emissiveColour);
DEFINE_STATIC_NAME(desaturateColour);
DEFINE_STATIC_NAME(alphaChannel);
DEFINE_STATIC_NAME(exmEmiColours);
DEFINE_STATIC_NAME(exmEmiPower);

//

void Utils::Grid::draw_grid_element(Framework::Display* _display, HubWidgets::Grid* w, Range2 const& _at, HubScreen const* screen, HubWidgets::GridDraggable const* _element, Optional<float> const& _smallerByPt, Optional<Colour> const & _overrideColour, Optional<float> const& _doubleLine, Optional<Colour> const& _blendTotalColour)
{
	Optional<Colour> colourise = _overrideColour;
	if (!colourise.is_set())
	{
		colourise = Utils::ForGridDraggable::get_colourise(w, screen, _element, Utils::ForGridDraggable::HighlightSelected);
	}	

	Colour border = Utils::ForGridDraggable::get_border(colourise);
	Utils::ForGridDraggable::adjust_colours(REF_ colourise, REF_ border, w, _element);
	Colour fill = border.with_alpha(0.7f);
	if (_blendTotalColour.is_set())
	{
		border = Colour::lerp(_blendTotalColour.get().a, border, _blendTotalColour.get().with_alpha(1.0f));
		fill = Colour::lerp(_blendTotalColour.get().a, fill, _blendTotalColour.get().with_alpha(1.0f));
	}

	{
		Range2 at = _at.expanded_by(-floor(_at.length() * _smallerByPt.get(0.05f)));
		if (colourise.is_set())
		{
			::System::Video3DPrimitives::fill_rect_2d(fill, at, true);
		}
		::System::Video3DPrimitives::rect_2d(border, Vector2(at.x.min, at.y.min), Vector2(at.x.max, at.y.max), true);

		if (_doubleLine.is_set())
		{
			Range2 ati = at.expanded_by(-Vector2(_doubleLine.get()));
			::System::Video3DPrimitives::rect_2d(border, ati.bottom_left(), ati.top_right(), true);
		}
	}
}
