#include "lhu_colours.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\widgets\lhw_grid.h"

#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\core\types\hand.h"

//

using namespace Loader;
using namespace Utils;

//

Optional<Colour> ForGridDraggable::get_colourise(HubWidgets::Grid* w, HubScreen const* screen, HubWidgets::GridDraggable const* _element, int _flags)
{
	Optional<Colour> colourise;
	if (_element)
	{
		bool cursorOver = false;
		auto* hub = screen->hub;
		for_count(int, handIdx, Hand::MAX)
		{
			auto& hand = hub->get_hand((Hand::Type)handIdx);
			if (hand.onScreen == screen &&
				!hand.dragged.is_set() &&
				_element->does_contain(hub->get_hand((Hand::Type)handIdx).onScreenAt, w->elementSize))
			{
				cursorOver = true;
				break;
			}
		}

		if (is_flag_set(_flags, HighlightSelected) && hub->get_selected_draggable() == _element->draggable.get())
		{
			colourise = HubColours::selected_highlighted();
		}
		else if (!_element->validPlacement)
		{
			colourise = HubColours::warning();
		}
		else if (_element->draggable->hoveringOver)
		{
			colourise = Colour::lerp(0.4f, HubColours::highlight(), Colour::white);
		}
		else if (cursorOver)
		{
			colourise = Colour::lerp(0.8f, HubColours::highlight(), Colour::white);
		}
	}

	return colourise;
}

Colour ForGridDraggable::get_border(Optional<Colour> const& _colourise)
{
	return HubColours::border().with_alpha(0.8f) * _colourise.get(Colour::white);
}

void ForGridDraggable::adjust_colours(REF_ Optional<Colour>& colourise, REF_ Colour& border, HubWidget const* w, HubWidgets::GridDraggable const* _element)
{
	if (!colourise.is_set())
	{
		colourise = Colour::white;
	}
	colourise = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(colourise.get(), w->screen, w, _element? _element->draggable.get() : nullptr);
	border = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(border, w->screen, w, _element ? _element->draggable.get() : nullptr);
}
