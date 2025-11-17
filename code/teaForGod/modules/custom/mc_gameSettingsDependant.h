#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\game\gameSettingsObserver.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class GameSettingsDependant
		: public Framework::ModuleCustom
		, public GameSettingsObserver
		{
			FAST_CAST_DECLARE(GameSettingsDependant);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			GameSettingsDependant(Framework::IModulesOwner* _owner);
			virtual ~GameSettingsDependant();

		public:	// Module
			override_ void reset();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

			override_ void initialise();

		public: // GameSettingsObserver
			implement_ void on_game_settings_channged(GameSettings& _gameSettings);

		private:
			Name setting;

			void update();
		};
	};
};

