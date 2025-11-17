#pragma once

#include "..\..\..\framework\game\gameMode.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class RegionType;
	class SubWorld;
};

namespace TeaForGodEmperor
{
	class Game;
	class Tutorial;

	namespace GameModes
	{
		class TutorialSetup
		: public Framework::GameModeSetup
		{
			FAST_CAST_DECLARE(TutorialSetup);
			FAST_CAST_BASE(Framework::GameModeSetup);
			FAST_CAST_END();
			typedef Framework::GameModeSetup base;
		public:
			TutorialSetup(Game* _game, TeaForGodEmperor::Tutorial const* _tutorial);

#ifdef AN_DEVELOPMENT
			virtual tchar const * get_mode_name() const { return TXT("tutorial"); }
#endif

			TeaForGodEmperor::Tutorial const* get_tutorial() const { return tutorial; }

		public: // Framework::GameModeSetup
			override_ Framework::GameMode* create_mode();

		protected:
			TeaForGodEmperor::Tutorial const* tutorial = nullptr;
		};

		class Tutorial
		: public Framework::GameMode
		{
			FAST_CAST_DECLARE(Tutorial);
			FAST_CAST_BASE(Framework::GameMode);
			FAST_CAST_END();
			typedef Framework::GameMode base;
		public:
			Tutorial(Framework::GameModeSetup* _setup);
			virtual ~Tutorial();

		public:
			override_ void on_start();
			override_ void on_end(bool _abort = false);

			override_ void pre_advance(Framework::GameAdvanceContext const & _advanceContext);

		public:
			override_ void log(LogInfoContext & _log) const;

		protected:
			override_ Framework::GameSceneLayer* create_layer_to_replace(Framework::GameSceneLayer* _replacingLayer);
			override_ bool does_use_auto_layer_management() const { return true; }

		protected:
			RefCountObjectPtr<TutorialSetup> tutorialSetup;
			TeaForGodEmperor::Tutorial const* tutorial = nullptr;
		};
	};
};
