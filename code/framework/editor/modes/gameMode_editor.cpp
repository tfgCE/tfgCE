#include "gameMode_editor.h"

#include "..\sceneLayers\gameSceneLayer_editor.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"

//

using namespace Framework;
using namespace GameModes;

//

REGISTER_FOR_FAST_CAST(EditorSetup);

EditorSetup::EditorSetup(Game* _game)
: base(_game)
{
}

Framework::GameMode* EditorSetup::create_mode()
{
	return new Editor(this);
}

//

REGISTER_FOR_FAST_CAST(GameModes::Editor);

GameModes::Editor::Editor(Framework::GameModeSetup* _setup)
: base(_setup)
{
	modeSetup = fast_cast<EditorSetup>(_setup);
}

GameModes::Editor::~Editor()
{
}

void GameModes::Editor::on_start()
{
	base::on_start();

	game->clear_scene_layer_stack();

	Framework::GameSceneLayers::Editor* el = new Framework::GameSceneLayers::Editor();
	editorLayer = el;

	game->push_scene_layer(editorLayer.get());
}

void GameModes::Editor::on_end(bool _abort)
{
	base::on_end(_abort);
}

void GameModes::Editor::pre_advance(Framework::GameAdvanceContext const& _advanceContext)
{
	base::pre_advance(_advanceContext);
}

void GameModes::Editor::pre_game_loop(Framework::GameAdvanceContext const& _advanceContext)
{
	base::pre_game_loop(_advanceContext);
}

void GameModes::Editor::log(LogInfoContext& _log) const
{
	base::log(_log);
}

