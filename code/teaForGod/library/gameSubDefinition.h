#pragma once

#include "..\game\energy.h"
#include "..\game\gameSettings.h"
#include "..\game\weaponPart.h"
#include "..\pilgrim\pilgrimGear.h"
#include "..\pilgrim\pilgrimSetup.h"

#include "..\..\core\memory\safeObject.h"
#include "..\..\core\other\remapValue.h"

#include "..\..\framework\library\libraryStored.h"

struct Tags;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct LibraryLoadingContext;
};

namespace TeaForGodEmperor
{
	class GameSubDefinition
	: public Framework::LibraryStored
	, public SafeObject<GameSubDefinition>
	{
		FAST_CAST_DECLARE(GameSubDefinition);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		typedef ::SafePtr<GameSubDefinition> GameSubDefinitionSafePtr;

	public:
		GameSettings::IncompleteDifficulty const& get_difficulty() const { return difficultySettings; }
		
	public:
		GameSubDefinition(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~GameSubDefinition();

		float get_selection_placement() const { return selectionPlacement; }
		TagCondition const& get_player_profile_requirements() const { return playerProfileRequirements; }

		Framework::LocalisedString const& get_name_for_ui() const { return nameForUI; }
		Framework::LocalisedString const& get_desc_for_selection_ui() const { return descForSelectionUI; }
		Framework::LocalisedString const& get_desc_for_ui() const { return descForUI; }
		
	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		float selectionPlacement = 0.0f;
		TagCondition playerProfileRequirements; // to hide some from the player

		Framework::LocalisedString nameForUI;
		Framework::LocalisedString descForSelectionUI;
		Framework::LocalisedString descForUI;

		GameSettings::IncompleteDifficulty difficultySettings;
	};
};
