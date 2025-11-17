#pragma once

#include "..\..\..\..\core\tags\tag.h"

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\world\door.h"

namespace Framework
{
	class ObjectType;
	class Region;
	struct PointOfInterestInstance;

	namespace GameScript
	{
		class Script;
	};
};

namespace TeaForGodEmperor
{
	class PilgrimageDevice;
	namespace AI
	{
		namespace Logics
		{
			namespace Managers
			{
				class RoomWithEnergySourceData;

				/**
				 */
				class RoomWithEnergySource
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(RoomWithEnergySource);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new RoomWithEnergySource(_mind, _logicData); }

				public:
					RoomWithEnergySource(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~RoomWithEnergySource();

					Framework::Room* get_for_room() const { return context->inRoom.get(); }

				public: // Logic
					override_ void advance(float _deltaTime);

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					struct Context
					: public RefCountObject
					{
						RoomWithEnergySourceData const* roomWithEnergySourceData = nullptr;
						SafePtr<Framework::Room> inRoom;
						VectorInt2 mapCell;

						TagCondition pilgrimageDeviceTagged;
						Framework::UsedLibraryStored<Framework::GameScript::Script> runGameScript;
						Framework::UsedLibraryStored<PilgrimageDevice> pilgrimageDevice;
						SafePtr<Framework::IModulesOwner> pilgrimageDeviceObject;
						SafePtr<Framework::Door> door;
					};

					RefCountObjectPtr<Context> context;
				};

				class RoomWithEnergySourceData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(RoomWithEnergySourceData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new RoomWithEnergySourceData(); }

					RoomWithEnergySourceData();
					virtual ~RoomWithEnergySourceData();

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					friend class RoomWithEnergySource;

					Framework::UsedLibraryStored<Framework::DoorType> doorType; // lockable

					struct ForCell
					{
						VectorInt2 at;
						TagCondition pilgrimageDeviceTagged;
						Framework::UsedLibraryStored<Framework::GameScript::Script> runGameScript;
					};
					Array<ForCell> forCells;
				};


				//

			};
		};
	};
};