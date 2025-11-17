#include "gameSceneLayer_Utils.h"

#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"

using namespace TeaForGodEmperor;
using namespace GameSceneLayers;

void Utils::do_stand_by(Framework::Display* _display)
{
	DEFINE_STATIC_NAME_STR(lsStandBy, TXT("Common HUD stand by message"));

	_display->remove_all_controls(true);
	_display->drop_all_draw_commands();

	_display->add((new Framework::DisplayDrawCommands::CLS()));
	/*
	_display->add((new Framework::DisplayDrawCommands::TextAt())
		->at(0, _display->get_char_resolution().y / 2)
		->length(_display->get_char_resolution().x)
		->h_align_centre()
		->use_default_colour_pair()
		->text(LOC_STR(NAME(lsStandBy))));
	*/
	//_display->hide_cursor();

	DEFINE_STATIC_NAME(waiting);
	_display->use_cursor(NAME(waiting));
}

