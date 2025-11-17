#pragma once

#include "..\..\framework\library\libraryStored.h"

namespace Framework
{
	namespace GameScript
	{
		class Script;
	};
};

namespace TeaForGodEmperor
{
	namespace Story
	{
		class Execution;

		struct AlterExecution
		{
		public:
			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

			void perform(Execution& _execution) const;

		private:
			Tags setsStoryFacts;
			Tags clearStoryFacts;
			String clearStoryFactsPrefixed;
		};

		struct Requirements
		{
		public:
			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

			bool check(Execution const& _execution) const;

		private:
			TagCondition storyFacts; // using story state
		};

		/*
		 *	Scenes cannot be interrupted (externaly). Only if the finish running (no script being executed) or they end themselves (by ending script execution)
		 */
		class Scene
		: public Framework::LibraryStored
		{
			FAST_CAST_DECLARE(Scene);
			FAST_CAST_BASE(LibraryStored);
			FAST_CAST_END();
			LIBRARY_STORED_DECLARE_TYPE();

			typedef LibraryStored base;
		public:
			Scene(Framework::Library* _library, Framework::LibraryName const& _name);
			virtual ~Scene();

			Framework::GameScript::Script const* get_game_script() const { return gameScript.get(); }

			Requirements const & get_requirements() const { return requirements; }
			AlterExecution const& get_at_start() const { return atStart; }
			AlterExecution const& get_at_end() const { return atEnd; }

			void load_data_on_demand_if_required();

		public: // LibraryStored
			override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		private:
			Framework::UsedLibraryStored<Framework::GameScript::Script> gameScript;

			Requirements requirements;

			AlterExecution atStart;
			AlterExecution atEnd;
		};
	};
};
