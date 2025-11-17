#pragma once

#include "..\..\..\game\airshipObstacle.h"

#include "..\..\..\..\core\tags\tag.h"
#include "..\..\..\..\core\wheresMyPoint\wmpParam.h"

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\world\worldSettingCondition.h"

namespace Framework
{
	class ObjectType;
	class Region;
	struct PointOfInterestInstance;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Managers
			{
				class AirshipsManagerData;

				/**
				 */
				class AirshipsManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(AirshipsManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new AirshipsManager(_mind, _logicData); }

				public:
					AirshipsManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~AirshipsManager();

					Framework::Room* get_for_room() const { return inRoom.get(); }

					int get_amount() const;
					Framework::IModulesOwner* get_airship(int _idx) const;

				public: // Logic
					override_ void advance(float _deltaTime);

					override_ void debug_draw(Framework::Room* _room) const;

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					AirshipsManagerData const * airshipsManagerData = nullptr;

					Random::Generator random;
					bool started = false;

					SafePtr<Framework::Room> inRoom;

					struct Airship
					{
						SafePtr<Framework::IModulesOwner> airship;
						bool requiresSetup = true;
						Name zone;
						Optional<float> circleInRadius;
						Optional<float> stayBeyondRadius;
						Optional<float> stayInRadius;
						float dir = 0.0f;
						Optional<float> targetDir;
						int avoidingObstaclePriority = 0;
						float dirSpeed = 0.0f;
						float speed = 0.0f;
						float maxDirSpeed = 10.0f;
						float maxDirSpeedBank = 5.0f;
						float dirSpeedBlendTime = 20.0f;
						Vector3 location = Vector3::zero;
					};

					mutable Concurrency::SpinLock airshipsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.AI.Logics.Managers.AirshipsManager"));
					Array<Airship> airships;
					RefCountObjectPtr<Framework::DelayedObjectCreation> spawningAirshipDOC;

					VectorInt2 mapCell = VectorInt2::zero;
					Framework::Room * mapCellDoneForPlayerInRoom = nullptr;

					bool obstaclesValid = false;
					Array<AirshipObstacle> obstacles;
				};

				class AirshipsManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(AirshipsManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new AirshipsManagerData(); }

					AirshipsManagerData();
					virtual ~AirshipsManagerData();

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					Framework::WorldSettingCondition airshipType;

					struct AirshipZone
					{
						int amount = 0;
						Name zone; // main map if none
						Range speed = Range(10);
						Range dirSpeed = Range(20);
						Range dirSpeedBank = Range(5);
						Range dirSpeedBlendTime = Range(10);
						Optional<Range> circleInRadius;
						Optional<Range> stayBeyondRadius;
						Optional<Range> stayInRadius;
						Optional<Range> altitude;
					};
					Array<AirshipZone> zones; // where to they fly
					int amount = 0;

					friend class AirshipsManager;
				};

				//

			};
		};
	};
};