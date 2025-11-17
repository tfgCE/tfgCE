#pragma once

#include "tutorialTree.h"

#include "..\game\persistence.h"
#include "..\pilgrim\pilgrimSetup.h"

#include "..\..\framework\library\libraryStored.h"

#define RETURN_IF_IN_TUTORIAL \
	if (TutorialSystem::get() && TutorialSystem::get()->is_active()) \
	{ \
		return; \
	}

namespace Framework
{
	class RegionType;
	class RoomType;

	namespace GameScript
	{
		class Script;
	};
};

namespace Loader
{
	class Hub;
	class HubScene;
};

namespace TeaForGodEmperor
{
	namespace TutorialType
	{
		enum Type
		{
			World,
			Hub
		};

		Type parse(String const& _t, Type _default = World);
	};

	class Tutorial
	: public Framework::LibraryStored
	, public TutorialTreeElement
	{
		FAST_CAST_DECLARE(Tutorial);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_BASE(TutorialTreeElement);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Tutorial(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~Tutorial();

		TutorialType::Type get_tutorial_type() const { return type; }

		Framework::RegionType* get_region_type() const { return regionType.get(); }
		Framework::RoomType* get_room_type() const { return roomType.get(); }
		Name const& get_player_spawn_poi() const { return playerSpawnPOI; }

		Loader::HubScene* create_hub_scene(Loader::Hub* _hub) const;

		PilgrimSetup const& get_pilgrim_setup() const { return pilgrimSetup; }
		Persistence const& get_persistence() const { return persistence; }

		Framework::GameScript::Script const* get_game_script() const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		TutorialType::Type type = TutorialType::World;

		// world
		Framework::UsedLibraryStored<Framework::RegionType> regionType; // always required
		Framework::UsedLibraryStored<Framework::RoomType> roomType; // optional, if provided, will create room, if not, will go with regions generator
		Name playerSpawnPOI;

		// hub
		Name hubScene;

		SimpleVariableStorage variables;

		Framework::UsedLibraryStored<Framework::GameScript::Script> gameScript;

		Persistence persistence;
		PilgrimSetup pilgrimSetup;
		bool autoSetupPersistence = false;
	};
};
