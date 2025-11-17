#pragma once

#include "..\..\game\gameMode.h"

namespace Framework
{
	class Game;

	namespace GameModes
	{
		/**
		 *	This is solely to show editor layer which is the actual editor
		 */
		class EditorSetup
		: public Framework::GameModeSetup
		{
			FAST_CAST_DECLARE(EditorSetup);
			FAST_CAST_BASE(GameModeSetup);
			FAST_CAST_END();
			typedef GameModeSetup base;
		public:
			EditorSetup(Game* _game);

#ifdef AN_DEVELOPMENT
			virtual tchar const* get_mode_name() const { return TXT("editor"); }
#endif

		public: // GameModeSetup
			override_ Framework::GameMode* create_mode();

		protected:
		};

		class Editor
		: public Framework::GameMode
		{
			FAST_CAST_DECLARE(Editor);
			FAST_CAST_BASE(Framework::GameMode);
			FAST_CAST_END();
			typedef Framework::GameMode base;
		public:
			Editor(Framework::GameModeSetup* _setup);
			virtual ~Editor();

		public:
			override_ void on_start();
			override_ void on_end(bool _abort = false);

			override_ void pre_advance(Framework::GameAdvanceContext const& _advanceContext);
			override_ void pre_game_loop(Framework::GameAdvanceContext const& _advanceContext);

		public:
			override_ void log(LogInfoContext& _log) const;

		protected:
			RefCountObjectPtr<EditorSetup> modeSetup;

			RefCountObjectPtr<Framework::GameSceneLayer> editorLayer;
		};
	};
};
