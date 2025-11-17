#include "displayDrawCommand.h"

#include "display.h"

using namespace Framework;

bool DisplayDrawCommand::get_use_colourise(Display * _display) const
{
	return colourise.is_set() ? colourise.get() : _display->get_colourise();
}

Colour const & DisplayDrawCommand::get_use_colourise_ink(Display * _display) const
{
	return colouriseInk.is_set() ? colouriseInk.get() : _display->get_current_ink();
}

Colour const & DisplayDrawCommand::get_use_colourise_paper(Display * _display) const
{
	return colourisePaper.is_set() ? colourisePaper.get() : _display->get_current_paper();
}

DisplayCoordinates::Type DisplayDrawCommand::get_use_coordinates(Display * _display) const
{
	return useCoordinates.is_set() ? useCoordinates.get() : _display->get_use_coordinates();
}

void DisplayDrawCommand::prepare(Display* _display)
{
#ifdef AN_DEVELOPMENT
	prepared = true;
#endif
}
