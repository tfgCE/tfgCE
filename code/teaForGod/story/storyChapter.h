#pragma once

#include "..\..\framework\library\libraryStored.h"

#include "storyScene.h"

namespace TeaForGodEmperor
{
	namespace Story
	{
		class Scene;

		class Chapter
		: public Framework::LibraryStored
		{
			FAST_CAST_DECLARE(Chapter);
			FAST_CAST_BASE(LibraryStored);
			FAST_CAST_END();
			LIBRARY_STORED_DECLARE_TYPE();

			typedef LibraryStored base;
		public:
			Chapter(Framework::Library* _library, Framework::LibraryName const& _name);
			virtual ~Chapter();

			Array<Framework::UsedLibraryStored<Scene>> const& get_scenes() const { return scenes; }

			AlterExecution const& get_at_clean_start() const { return atCleanStart; }
			AlterExecution const& get_at_start() const { return atStart; }
			AlterExecution const& get_at_end() const { return atEnd; }

			void load_data_on_demand_if_required();

		public: // LibraryStored
			override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		private:
			Array<Framework::UsedLibraryStored<Scene>> scenes;

			// atStart as it may break storyFacts restored from game state, use it carefully
			// we do it for scenes as they change during a chapter anyway and we don't save mid scene, but chapters are different
			// atCleanStart means when we reach new chapter or we start a new game
			AlterExecution atCleanStart;
			AlterExecution atStart;
			AlterExecution atEnd;
		};
	};
};
