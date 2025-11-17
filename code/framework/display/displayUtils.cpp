#include "displayUtils.h"

#include "display.h"
#include "displayDrawCommands.h"

using namespace Framework;

void DisplayUtils::clear_all(Display* _display, Optional<Name> const & _colourPair)
{
	if (!_display)
	{
		return;
	}
	_display->drop_all_draw_commands();
	_display->remove_all_controls(true);
	if (_colourPair.is_set())
	{
		_display->use_colour_pair(_colourPair.get());
	}
	_display->add(new DisplayDrawCommands::Border());
	_display->add(new DisplayDrawCommands::CLS());
}

void DisplayUtils::cls(Display* _display, Optional<Name> const & _colourPair)
{
	if (!_display)
	{
		return;
	}
	_display->drop_all_draw_commands();
	_display->remove_all_controls(true);
	if (_colourPair.is_set())
	{
		_display->use_colour_pair(_colourPair.get());
	}
	_display->add(new DisplayDrawCommands::CLS());
}

void DisplayUtils::border(Display* _display, Optional<Name> const & _colourPair)
{
	if (!_display)
	{
		return;
	}
	if (_colourPair.is_set())
	{
		_display->use_colour_pair(_colourPair.get());
	}
	_display->add(new DisplayDrawCommands::Border());
}
