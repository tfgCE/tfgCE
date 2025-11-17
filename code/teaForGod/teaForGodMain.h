#pragma once

#include "teaForGod.h"
#include "..\core\globalDefinitions.h"
#include "..\core\tags\tag.h"

namespace TeaForGodEmperor
{
	class Game;
	struct CheckGameEnvironment;

	struct Main
	{
		Tags executionTags;
		bool listLibrary = false;
		Name listLibraryType = Name::invalid();
		bool dumpLanguages = false;
		bool importLanguages = false;
		bool translateLocalisedStrings = false;
		bool updateHashesForLocalisedStrings = false;
		bool createCodeInfo = false;
		bool testWithCodeInfo = false;
		bool loadLibraryAndExit = false;
		bool updateLocalisedCharacters = false;
		bool storeDescriptions = false;
		bool createMainConfigFile = false;
		bool createMainConfigFileAndExit = false;
		bool silentRun = false;
		bool drawVRControls = false;
		String forceVR;

		bool freshStart = false;

		int run();

	private:
		void do_lists_imports_and_dumps();
		bool is_there_anything_for_import_languages();
		static String get_language_import_directory();

	private:
		bool startGame = true;
		bool runNormally = true;

		String codeInfoFilePath = String(TXT("system\\codeInfo"));
		String localisedCharactersFilePath;

		TeaForGodEmperor::Game* game = nullptr;
		
		CheckGameEnvironment* checkGameEnvironment = nullptr;

		void init();
			void pre_init_game();
			void init_system_prior_to_game();
			void log_registered_types();
			void test_code_info();
			void load_code_info();
			bool run_other_game_if_requested(); // return true if run and done
			
			void load_localised_characters();
			void update_localised_characters();
			void store_descriptions();
				
			bool load_system_library();
			bool load_actual_library();

			void close_system_post_game();
		void close();


	};
};
