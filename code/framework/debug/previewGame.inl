#include "..\library\library.h"

namespace Framework
{

	template <typename LibraryClass>
	bool PreviewGame::run_if_requested(String const & _byLookingIntoConfigFile, std::function<void(Game*)> _on_game_started)
	{
		IO::XML::Document * xmlConfigDoc = IO::XML::Document::from_file(_byLookingIntoConfigFile);
		if (!xmlConfigDoc)
		{
			return false;
		}
		if (!xmlConfigDoc->get_root())
		{
			delete xmlConfigDoc;
			return false;
		}
		// <preview type="mesh" name="base:testMesh"/>
		Array<LibraryName> previewObjects;
		Array<Name> previewObjectTypes;
		for_every(previewNode, xmlConfigDoc->get_root()->children_named(TXT("preview")))
		{
			LibraryLoadingContext lc(nullptr, String::empty(), LibrarySource::Files, LibraryLoadingMode::Tools);

			Name previewObjectType = previewNode->get_name_attribute(TXT("type"));
			LibraryName previewObject;
			if (previewObjectType.is_valid() &&
				previewObject.load_from_xml(previewNode, TXT("name"), lc))
			{
				previewObjects.push_back(previewObject);
				previewObjectTypes.push_back(previewObjectType);
			}
		}

		Array<String> loadFiles;
		Array<String> useLibraries;
		bool noLibrary = false;
		LibraryName defaultEnvironmentType;
		struct UseEnvironmentType
		{
			LibraryName environmentType;
			Array<LibraryName> environmentOverlays;
		};
		Array<UseEnvironmentType> useEnvironmentTypes;
		for_every(previewSetupNode, xmlConfigDoc->get_root()->children_named(TXT("previewSetup")))
		{
			if (IO::XML::Attribute const * attr = previewSetupNode->get_attribute(TXT("useLibrary")))
			{
				String const & useLibrary = attr->get_as_string();
				if (!useLibrary.is_empty())
				{
					useLibraries.push_back_unique(useLibrary);
				}
			}
			if (IO::XML::Attribute const * attr = previewSetupNode->get_attribute(TXT("loadFile")))
			{
				String const & loadFile = attr->get_as_string();
				if (!loadFile.is_empty())
				{
					loadFiles.push_back_unique(loadFile);
				}
			}

			if (IO::XML::Attribute const * attr = previewSetupNode->get_attribute(TXT("useEnvironmentType")))
			{
				LibraryName useEnvironmentType;
				useEnvironmentType = LibraryName::from_string(attr->get_as_string());
				if (previewSetupNode->get_bool_attribute_or_from_child_presence(TXT("default")))
				{
					defaultEnvironmentType = useEnvironmentType;
				}
				if (useEnvironmentType.is_valid())
				{
					UseEnvironmentType uet;
					uet.environmentType = useEnvironmentType;
					for_every(node, previewSetupNode->children_named(TXT("add")))
					{
						LibraryName addEnvironmentOverlay;
						if (addEnvironmentOverlay.load_from_xml(node, TXT("environmentOverlay")))
						{
							uet.environmentOverlays.push_back(addEnvironmentOverlay);
						}
					}
					useEnvironmentTypes.push_back(uet);
				}
			}
			
			if (IO::XML::Attribute const * attr = previewSetupNode->get_attribute(TXT("noLibrary")))
			{
				noLibrary = attr->get_as_bool();
			}
		
		}

		delete xmlConfigDoc;
		if (! previewObjects.is_empty())
		{
			MainConfig::access_global().set_thread_limit(max(MainConfig::access_global().get_thread_limit(), 8));

			PreviewGame* game = new PreviewGame([](){Library::initialise_static<LibraryClass>(); });

			game->set_started_using_preview_config_file(_byLookingIntoConfigFile);

			game->clear_library_files();
			for_every(loadFile, loadFiles)
			{
				game->add_library_file(*loadFile);
			}
			game->clear_library_directories();
			for_every(useLibrary, useLibraries)
			{
				game->add_library_directory(*useLibrary);
			}
			game->no_library(noLibrary);

			for_every(et, useEnvironmentTypes)
			{
				if (auto* e = game->add_environment_type(et->environmentType, et->environmentType == defaultEnvironmentType))
				{
					e->addEnvironmentOverlays = et->environmentOverlays;
				}
			}

			game->start();

			if (_on_game_started)
			{
				_on_game_started(game);
			}

			if (game->load_preview_library(LibraryLoadLevel::Init))
			{
				game->before_game_starts();

				game->create_world();

				game->ready_for_continuous_advancement();

				for_count(int, i, previewObjects.get_size())
				{
					game->add_to_show_object_list(previewObjects[i], previewObjectTypes[i]);
				}
				game->show_next_object_from_list();

				while (!game->does_want_to_quit())
				{
					game->advance();
				}
			}
			else
			{
				todo_important(TXT("what now?"));
			}

			// end game
			game->destroy_world();

			// game has ended
			game->after_game_ended();

			// empty library before closing game
			game->close_preview_library(LibraryLoadLevel::Close);

			// close game
			game->close_and_delete();

			return true;
		}

		return false;
	}
};

