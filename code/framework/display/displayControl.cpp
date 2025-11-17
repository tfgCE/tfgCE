#include "displayControl.h"

#include "display.h"
#include "displayDrawCommands.h"

using namespace Framework;

//

DEFINE_STATIC_NAME_STR(cursorMove, TXT("cursor move"));
DEFINE_STATIC_NAME_STR(cursorAction, TXT("cursor action"));

//

REGISTER_FOR_FAST_CAST(DisplayControl);

void DisplayControl::on_added_to(Display* _display)
{
	an_assert(insideDisplay == nullptr);
	insideDisplay = _display;

	if (inRegionName.is_set())
	{
		inRegion = _display->get_region(inRegionName.get());
	}

	recalculate(_display);

	if (isVisible)
	{
		redraw(_display);
	}
}

void DisplayControl::on_removed_from(Display* _display, bool _noRedrawNeeded)
{
	if (isVisible && !_noRedrawNeeded)
	{
		redraw(_display, true);
	}

	an_assert(insideDisplay != nullptr);
	insideDisplay = nullptr;
}

void DisplayControl::process_deselected(Display* _display)
{
	if (isVisible)
	{
		isSelected = false;
		redraw(_display);
	}
}

void DisplayControl::process_selected(Display* _display, DisplayControl* _prevButton, Optional<VectorInt2> _cursor, Optional<VectorInt2> _goingInDir, bool _silent)
{
	if (is_selectable())
	{
		isSelected = true;
		if (on_selected_fn)
		{
			on_selected_fn(_display, this, _prevButton, _cursor, _goingInDir);
			if (! isSelected)
			{
				// selection has changed
				return;
			}
		}
		redraw(_display);
		if (!_silent)
		{
			_display->play_sound(NAME(cursorMove));
		}
	}
}

void DisplayControl::process_activate(Display* _display, Optional<VectorInt2> _cursor)
{
	if (isVisible)
	{
		if (on_activate_fn)
		{
			_display->play_sound(NAME(cursorAction));
			RefCountObjectPtr<DisplayControl> keepControlAlive(this); // in case we would destroy ourselves
			on_activate_fn(_display, this, _cursor);
		}
	}
}

void DisplayControl::process_hover(Display* _display, VectorInt2 const & _cursor)
{
	// no action required
}

void DisplayControl::show(Display* _display, bool _show)
{
	if (!_show)
	{
		hide(_display);
		return;
	}
	if (!isVisible)
	{
		isVisible = true;
		if (_display)
		{
			redraw(_display);
		}
	}
}

void DisplayControl::hide(Display* _display)
{
	if (isVisible)
	{
		isVisible = false;
		if (_display)
		{
			redraw(_display, true);
			if (isSelected)
			{
				process_deselected(_display);
			}
		}
	}
}

void DisplayControl::update(Display* _display)
{
	recalculate(_display);
}

void DisplayControl::redraw(Display* _display, bool _clear)
{
	todo_important(TXT("implement_?"));
}

DisplaySelectorDir::Type DisplayControl::process_navigation(Display* _display, DisplaySelectorDir::Type _inDir)
{
	return _inDir;
}

void DisplayControl::recalculate(Display* _display)
{
	actualAt = calculate_at(_display);
	actualSize = calculate_size(_display);
}

CharCoords DisplayControl::calculate_at(Display* _display) const
{
	CharCoords retVal = CharCoords::zero;

	if (inRegion)
	{
		if (atParam.is_set())
		{
			retVal = atParam.get() + inRegion->get_left_bottom();
		}
		else
		{
			retVal = inRegion->get_left_bottom();
		}
		if (fromTopLeftParam.is_set())
		{
			retVal = CharCoords(inRegion->get_left_bottom().x, inRegion->get_top_right().y) + fromTopLeftParam.get();
		}

		if (hAlignmentInRegionParam.is_set())
		{
			CharCoords useSize = calculate_size(_display);
			if (hAlignmentInRegionParam.get() == DisplayHAlignment::Centre)
			{
				retVal.x = inRegion->get_left_bottom().x + (inRegion->get_length() - useSize.x) / 2;
			}
			if (hAlignmentInRegionParam.get() == DisplayHAlignment::CentreRight)
			{
				retVal.x = inRegion->get_left_bottom().x + (inRegion->get_length() - useSize.x) / 2;
				if ((inRegion->get_length() - useSize.x) % 2 == 1)
				{
					retVal.x += 1;
				}
			}
			if (hAlignmentInRegionParam.get() == DisplayHAlignment::Left)
			{
				retVal.x = inRegion->get_left_bottom().x;
			}
			if (hAlignmentInRegionParam.get() == DisplayHAlignment::Right)
			{
				retVal.x = inRegion->get_top_right().x - useSize.x;
			}
		}
	}
	else
	{
		if (atParam.is_set())
		{
			retVal = atParam.get();
		}
	}

	return retVal;
}

CharCoords DisplayControl::calculate_size(Display* _display) const
{
	CharCoords retVal = CharCoords(8, 1);

	if (sizeParam.is_set())
	{
		retVal = sizeParam.get();
	}

	if (inRegion)
	{
		if (! sizeParam.is_set())
		{
			retVal = inRegion->get_top_right() - inRegion->get_left_bottom() + CharCoords::one;
		}
	}

	return retVal;
}

void DisplayControl::add_draw_commands_clear(Display* _display)
{
	CharCoords placement = actualAt;
	CharCoords useSize = actualSize;

	_display->add((new DisplayDrawCommands::ClearRegion())->rect(placement, useSize)->use_default_colour_pair());
	/*
	for (int y = 0; y < useSize.y; ++y)
	{
	_display->add((new DisplayDrawCommands::TextAt())->at(placement + CharCoords(0, y))->length(useSize.x)->use_default_colour_pair());
	}
	*/
}
