#pragma once

#include "..\..\core\math\math.h"

#include "..\..\framework\library\usedLibraryStored.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class Library;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;
};

namespace TeaForGodEmperor
{
	namespace Story
	{
		class Chapter;
	};

	struct GameDirectorDefinition
	{
	public:
		bool useGameDirector = false;
		bool startWithBlockedDoors = false;

		struct AutoHostiles
		{
			// auto hostiles is about hostiles being time-based
			// used only if defined

			// we do normal/full period, then take a break and then repeat the whole thing

			// full means that there is no limit, for designed time they are spawned constantly

			Optional<float> fullHostilesChance; // if both defined
			Optional<Range> fullHostilesTimeInMinutes; // how long does it last
			Optional<Range> normalHostilesTimeInMinutes; // how long does it last
			Optional<Range> breakTimeInMinutes;

			bool load_from_xml(IO::XML::Node const* _node);

			void override_with(AutoHostiles const& _ah);
		} autoHostiles;

		Framework::UsedLibraryStored<Story::Chapter> storyChapter;

	public:
		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
	};
};
