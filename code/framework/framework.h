#pragma once

#include "..\core\globalDefinitions.h"

#ifdef AN_WINDOWS
	#define RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
#endif

#define RENDER_SCENE_WITH_ON_SHOULD_ADD_TO_RENDER_SCENE
//#define RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER

#ifndef WITH_IN_GAME_EDITOR
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		// in-game editor available at any time, set it in builver_*_definitions.h
		//#define WITH_IN_GAME_EDITOR
	#endif
#endif

namespace Framework
{
	void initialise_static();
	void close_static();
	
	bool is_preview_game();
	bool does_preview_game_require_info_about_missing_library_stored();

	void set_preview_game_require_info_about_missing_library_stored(bool _require);

#ifdef AN_NO_RARE_ADVANCE_AVAILABLE
	bool & access_no_rare_advance();
#endif
};
