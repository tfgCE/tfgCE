#pragma once

#include "..\library\gameDefinition.h"
#include "..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\tags\tag.h"

#include "gameStats.h"

namespace TeaForGodEmperor
{
	class PilgrimageDevice;

	namespace GameStateLevel
	{
		enum Type
		{
			LastMoment,
			Checkpoint,
			AtLeastHalfHealth,
			ChapterStart
		};

		inline tchar const* to_char(Type _type)
		{
			if (_type == LastMoment) return TXT("last moment");
			if (_type == Checkpoint) return TXT("checkpoint");
			if (_type == AtLeastHalfHealth) return TXT("at least half health");
			if (_type == ChapterStart) return TXT("chapter start");
			return TXT("??");
		}
	};

	struct GameState
	: public RefCountObject
	{
	public:
		//
		// only valid post creation to determine where to save
		bool atLeastHalfHealth = false;

		//
		// by game

		Framework::DateTime when;

		bool interrupted = false; // interrupted on purpose

		GameStats gameStats; // copied from the used ones

		Framework::UsedLibraryStored<GameDefinition> gameDefinition;
		Array<Framework::UsedLibraryStored<GameSubDefinition>> gameSubDefinitions;
		Framework::UsedLibraryStored<Framework::ActorType> pilgrimActorType; // can be anything, this is used when we switch
		Framework::UsedLibraryStored<Pilgrimage> pilgrimage; // should be one from game definition

		Tags gameModifiers;

		Tags storyFacts;

		Tags restartInRoomTagged;

		//
		// by pilgrimage

		SimpleVariableStorage pilgrimageState; // set by pilgrimage object itself
		SimpleVariableStorage pilgrimageVariables; // used by other entites

		struct OpenWorld
		{
			// only devices that have been altered are stored
			struct Cell
			{
				VectorInt2 at;
				ArrayStatic<PilgrimageInstanceOpenWorld::KnownPilgrimageDevice, 16> knownPilgrimageDevices;

				Cell()
				{
					SET_EXTRA_DEBUG_INFO(knownPilgrimageDevices, TXT("GameState::OpenWorld::Cell.knownPilgrimageDevices"));
				}
			};
			Array<Cell> cells;

			Array<VectorInt2> knownExitsCells;
			Array<VectorInt2> knownDevicesCells;
			Array<VectorInt2> knownSpecialDevicesCells;
			Array<VectorInt2> knownCells;
			Array<VectorInt2> visitedCells;
			PilgrimageInstanceOpenWorld::LongDistanceDetection longDistanceDetection;
			Array<Framework::UsedLibraryStored<PilgrimageDevice>> unobfuscatedPilgrimageDevices;
			Array<Name> pilgrimageDevicesUsageGroup; // count them

		} openWorld;

		Array<PilgrimageKilledInfo> killedInfos;

		//
		// by pilgrim

		RefCountObjectPtr<PilgrimGear> gear;

		GameState();
		virtual ~GameState();

		bool load_from_xml(IO::XML::Node const* _node);
		bool resolve_library_links();
		bool save_to_xml(IO::XML::Node * _node) const;

		void debug_log(tchar const* _nameInfo);

	private:
		GameState(GameState const& _other); // do not implement
		GameState& operator=(GameState const& _other); // do not implement
	};
};
