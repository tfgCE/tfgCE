#pragma once

#include "..\..\teaEnums.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

#include "..\..\game\weaponSetup.h"
#include "..\..\pilgrimage\pilgrimageInstanceObserver.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;

	namespace AI
	{
		namespace Logics
		{
			class CatcherData;

			/**
			 *	Put in the catcher room to track weapons and automatically put them into the persistence
			 * 
			 *	NOT USED FOR TIME BEING
			 *	"end mission" moves weapons to the persistence (all!)
			 *	the armoury if detects a weapon that exists, doesn't spawn it
			 */
			class Catcher
			: public ::Framework::AI::LogicWithLatentTask
			, public IPilgrimageInstanceObserver
			{
				FAST_CAST_DECLARE(Catcher);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_BASE(IPilgrimageInstanceObserver);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Catcher(_mind, _logicData); }

			public:
				Catcher(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Catcher();

			private:
				void add_room_to_track(Framework::Room* _room); // call before tracking
				void track_weapons(); // will get all weapons in the pilgrimage we're in, so when we switch or end, the weapons will be put into the persistence

				void transfer_weapons_to_persistence(bool _weaponsInTrackedRoomsOnly);

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			protected:
				implement_ void on_pilgrimage_instance_switch(PilgrimageInstance const* _from, PilgrimageInstance const* _to);
				implement_ void on_pilgrimage_instance_end(PilgrimageInstance const* _pilgrimage, PilgrimageResult::Type _pilgrimageResult);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				CatcherData const * catcherData = nullptr;

				Array<SafePtr<Framework::Room>> roomsToTrack;

				struct Weapon
				{
					SafePtr<Framework::IModulesOwner> imo;
					//todo_implement //WeaponSetup setup;
				};
				Array<Weapon> weapons;
			};

			class CatcherData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(CatcherData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new CatcherData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:

				friend class Catcher;
			};
		};
	};
};