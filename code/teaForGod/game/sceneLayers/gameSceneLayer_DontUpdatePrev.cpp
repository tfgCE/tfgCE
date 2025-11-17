#include "gameSceneLayer_DontUpdatePrev.h"

#include "..\game.h"

#include "..\..\..\core\system\video\renderTarget.h"

using namespace TeaForGodEmperor;
using namespace GameSceneLayers;

REGISTER_FOR_FAST_CAST(DontUpdatePrev);

void DontUpdatePrev::on_placed()
{
	base::on_placed();
	base::on_paused();
}

void DontUpdatePrev::on_removed()
{
	base::on_resumed();
	base::on_removed();
}

void DontUpdatePrev::prepare_to_render(Framework::CustomRenderContext * _customRC)
{
	if (provideBackground)
	{
		fast_cast<Game>(game)->no_world_to_render();
	}
}

void DontUpdatePrev::prepare_to_sound(Framework::GameAdvanceContext const & _advanceContext)
{
	if (provideBackground)
	{
		fast_cast<Game>(game)->no_world_to_sound();
	}
}

void DontUpdatePrev::render_on_render(Framework::CustomRenderContext * _customRC)
{
	if (provideBackground)
	{
		fast_cast<Game>(game)->render_empty_background_on_render(provideBackgroundColour, _customRC);
		if (game->does_use_vr())
		{
			// render main output clean too!
			fast_cast<Game>(game)->render_empty_background_on_render(provideBackgroundColour, _customRC, true);
		}
	}
}
