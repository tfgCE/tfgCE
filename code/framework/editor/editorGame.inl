#include "..\library\library.h"

#include "..\..\core\types\scopedDelete.h"

namespace Framework
{
	template <typename LibraryClass>
	bool EditorGame::run_if_requested(String const & _byLookingIntoConfigFile, std::function<void(Game*)> _on_game_started)
	{
#ifndef WITH_IN_GAME_EDITOR
		// no game editor if not provided at all, in-game editor requires certain functionality
		return false;
#endif
		// how to use:
		//	file	:	_editor.xml
		//	content	:	<editor use="true"/>
		// although it is advised to use in-game editor
		bool useEditor = false;
		{
			IO::XML::Document * xmlConfigDoc = IO::XML::Document::from_file(_byLookingIntoConfigFile);
			if (!xmlConfigDoc)
			{
				return false;
			}
			SCOPED_DELETE(IO::XML::Document, xmlConfigDoc);
			if (!xmlConfigDoc->get_root())
			{
				return false;
			}
			for_every(editorNode, xmlConfigDoc->get_root()->children_named(TXT("editor")))
			{
				useEditor = editorNode->get_bool_attribute_or_from_child_presence(TXT("use"), useEditor);
			}
			if (!useEditor)
			{
				return false;
			}
		}

		{
			MainConfig::access_global().set_thread_limit(max(MainConfig::access_global().get_thread_limit(), 8));

			EditorGame* game = new EditorGame();
			game->use_create_library_func([]() {Library::initialise_static<LibraryClass>(); });

			game->start();

			if (_on_game_started)
			{
				_on_game_started(game);
			}

			if (game->load_editor_library(LibraryLoadLevel::Init))
			{
				game->before_game_starts();

				game->ready_for_continuous_advancement();

				while (!game->does_want_to_quit())
				{
					game->advance();
				}
			}
			else
			{
				todo_important(TXT("what now?"));
			}

			game->after_game_ended();

			game->close_editor_library(LibraryLoadLevel::Close);

			game->close_and_delete();

			return true;
		}

		return false;
	}
};

