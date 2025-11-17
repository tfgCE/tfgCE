#include "gameSceneLayer_editor.h"

#include "..\..\..\core\system\video\renderTarget.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\editor\editorWorkspace.h"
#include "..\..\editor\editors\editorBase3D.h"
#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"

#include "..\..\ui\uiCanvasButton.h"
#include "..\..\ui\uiCanvasInputContext.h"
#include "..\..\ui\uiCanvasWindow.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameSceneLayers;

//

REGISTER_FOR_FAST_CAST(GameSceneLayers::Editor);

GameSceneLayers::Editor::Editor()
{
	editorContext.edit(nullptr); // will start the default editor
	editorContext.load_last_workspace_or_default();
}

GameSceneLayers::Editor::~Editor()
{
}

Optional<bool> GameSceneLayers::Editor::is_input_grabbed() const
{
	if (auto* editor = editorContext.get_editor())
	{
		return editor->is_input_grabbed().get(false);
	}
	return false;
}

void GameSceneLayers::Editor::pre_advance(Framework::GameAdvanceContext const& _advanceContext)
{
}

void GameSceneLayers::Editor::advance(Framework::GameAdvanceContext const& _advanceContext)
{
}

void GameSceneLayers::Editor::process_controls(Framework::GameAdvanceContext const& _advanceContext)
{
}

void GameSceneLayers::Editor::process_all_time_controls(Framework::GameAdvanceContext const& _advanceContext)
{
	if (auto* editor = editorContext.get_editor())
	{
		RefCountObjectPtr<Framework::Editor::Base> keepEditor(editor);
		editor->process_controls();
	}
}

void GameSceneLayers::Editor::process_vr_controls(Framework::GameAdvanceContext const& _advanceContext)
{
}

void GameSceneLayers::Editor::prepare_to_sound(Framework::GameAdvanceContext const& _advanceContext)
{
}

void GameSceneLayers::Editor::prepare_to_render(Framework::CustomRenderContext* _customRC)
{
	if (auto* editor = editorContext.get_editor())
	{
		editor->pre_render(_customRC);
	}
}

void GameSceneLayers::Editor::render_on_render(Framework::CustomRenderContext* _customRC)
{
	if (auto* editor = editorContext.get_editor())
	{
		editor->render_main(_customRC);
		editor->render_debug(_customRC);
	}
}

void GameSceneLayers::Editor::render_after_post_process(Framework::CustomRenderContext* _customRC)
{
	if (auto* editor = editorContext.get_editor())
	{
		editor->render_ui(_customRC);
	}
}
