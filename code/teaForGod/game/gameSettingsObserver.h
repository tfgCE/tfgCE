#pragma once

#include "gameSettings.h"

namespace TeaForGodEmperor
{
	struct GameSettings;

	class GameSettingsObserver
	{
	public:
		GameSettingsObserver();
		virtual ~GameSettingsObserver();

		virtual void on_game_settings_channged(GameSettings& _gameSettings) = 0;

		GameSettings& get_game_settings() const { return *gameSettings; }

	private:
		GameSettings* gameSettings;
	};

};
