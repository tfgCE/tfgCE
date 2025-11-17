#pragma once

#include "gameSubDefinition.h"
#include "..\game\energy.h"
#include "..\game\gameSettings.h"
#include "..\game\weaponPart.h"
#include "..\pilgrim\pilgrimGear.h"
#include "..\pilgrim\pilgrimSetup.h"
#include "..\pilgrimage\conditionalPilgrimage.h"

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
	class MissionDefinition;
	class Pilgrimage;
	class WeaponCoreModifiers;

	struct MissionsDefinitionSet
	: public RefCountObject
	{
		Name id;
		Array<Framework::UsedLibraryStored<Pilgrimage>> startingPilgrimages; // we always start the same?
		TagCondition missionsTagged;
		CACHED_ Array<Framework::UsedLibraryStored<MissionDefinition>> missions; // cached with tags
	};

	class MissionsDefinition
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(MissionsDefinition);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		MissionsDefinition(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~MissionsDefinition();

		Array<RefCountObjectPtr<MissionsDefinitionSet>> const& get_sets() const { return sets; }

		Tags const& get_loot_progress_milestones() const { return lootProgressMilestones; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		Array<RefCountObjectPtr<MissionsDefinitionSet>> sets;

		Tags lootProgressMilestones;
	};
};
