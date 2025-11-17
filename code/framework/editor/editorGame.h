#pragma once

#include "..\game\game.h"

namespace Framework
{
	class EditorGame
	: public Game
	{
		FAST_CAST_DECLARE(EditorGame);
		FAST_CAST_BASE(Game);
		FAST_CAST_END();

		typedef Game base;
	public:
		template <typename LibraryClass>
		static bool run_if_requested(String const& _byLookingIntoConfigFile, std::function<void(Game*)> _on_game_started = nullptr);

	public:
		EditorGame();

		void use_create_library_func(std::function<void()> _create_library_func) { create_library_func = _create_library_func; }

		bool load_editor_library(LibraryLoadLevel::Type _startAtLibraryLoadLevel);
		void close_editor_library(LibraryLoadLevel::Type _closeAtLibraryLoadLevel);

		bool does_want_to_quit() const { return wantsToQuit; }

	public:
		override_ void create_library();
		override_ void setup_library();
		override_ void close_library(Framework::LibraryLoadLevel::Type _upToLibraryLoadLevel, Loader::ILoader* _loader);

		override_ void before_game_starts();
		override_ void after_game_ended();

	protected:
		override_ void on_advance_and_display_loader(Loader::ILoader* _loader);

	protected:
		override_ void post_start();
		override_ void pre_close();

	protected:
		override_ void pre_update_loader();
		override_ void post_render_loader();

	protected:
		virtual ~EditorGame();

		override_ void initialise_system();
		override_ void close_system();
		override_ void create_render_buffers();
		override_ void prepare_render_buffer_resolutions(REF_ VectorInt2 & _mainResolution, REF_ VectorInt2 & _secondaryViewResolution);

		override_ void pre_advance();
		override_ void advance_controls();

		override_ void pre_game_loop();

		override_ void render();
		override_ void sound();
		override_ void game_loop();

		override_ String get_game_name() const;

	public:
		override_ void render_main_render_output_to_output();

	public: // modes
		override_ void start_mode(Framework::GameModeSetup* _modeSetup);
		override_ void end_mode(bool _abort = false);

	protected:
		void create_mode_on_no_mode();

	protected:
		override_ bool should_input_be_grabbed() const;

	private:
		std::function<void()> create_library_func;

		bool wantsToQuit = false;
	};
};

#include "editorGame.inl"