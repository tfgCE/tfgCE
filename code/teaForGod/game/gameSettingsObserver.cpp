#include "gameSettingsObserver.h"

#include "gameSettings.h"

using namespace TeaForGodEmperor;

//

GameSettingsObserver::GameSettingsObserver()
: gameSettings(&GameSettings::get())
{
	if (gameSettings)
	{
		gameSettings->add_observer(this);
	}
}

GameSettingsObserver::~GameSettingsObserver()
{
	if (gameSettings)
	{
		gameSettings->remove_observer(this);
	}
}

