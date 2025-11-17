#pragma once

#include "..\..\..\game\airshipObstacle.h"

#include "..\..\..\..\core\random\randomNumber.h"
#include "..\..\..\..\core\tags\tag.h"
#include "..\..\..\..\core\wheresMyPoint\wmpParam.h"

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\world\worldSettingCondition.h"

#define SHOOTER_WAVES_SLOT_LIMIT 8

namespace Framework
{
	class ActorType;
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
				class AirPursuitManagerData;

				struct PursuitSetup
				{
					TagCondition storyFactRequired;
					TagCondition storyFactReady; // if set will wait with creating objects till this is ok
					TagCondition storyFactToStopPermanently;

					SimpleVariableStorage instantTransitSpeedAIMessageParams;
					SimpleVariableStorage addTargetAIMessageParams;
					SimpleVariableStorage setupAIMessageParams;

					Array<Vector3> followOffsetPoints;
					Vector3 concurrentWaveFollowOffset = Vector3::zero;

					Range3 individualWaveFollowOffsetPoint = Range3::zero;
					float individualWaveFollowOffsetPointWaveMultiplier = 1.0f;

					// this is relative to followed object
					struct SpawnPointInfo
					{
						Transform placementRel;
						Optional<float> speed; // we assume they move forward
					};
					Array<SpawnPointInfo> spawnPointInfos;

					Random::Number<int> count = 1;
					Range interval = Range::empty;

					Framework::UsedLibraryStored<Framework::ActorType> actorType;
					Tags tagSpawned;
					SimpleVariableStorage spawnParameters;
				};

				struct PursuitInstance
				{
					PursuitSetup const* setup;

					bool permanentlyStopped = false;

					int wave = NONE;

					float intervalLeft = 0.0f;

					struct Wave
					{
						bool inLockerRoom = false;
						bool getReady = false; // this wave is just to get ready
						bool aiMessagesPending = false; // waiting to send messages, we need them teleported out of the locker room
						struct Pursuiter
						{
							SafePtr<Framework::IModulesOwner> imo;
							Optional<Vector3> followOffsetPoint;
							float sendAIMessageDelay = 0.0f;
						};
						Vector3 individualWaveFollowOffsetPoint = Vector3::zero;
						Array<Pursuiter> pursuiters;
						int addDocCount = 0;
						Array<RefCountObjectPtr<Framework::DelayedObjectCreation>> docs;
					};

					Array<Wave> activeWaves;
				};

				/**
				 *	We always have one wave ready (of each), ie. pursuiters are created and put into the locker room
				 */
				class AirPursuitManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(AirPursuitManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new AirPursuitManager(_mind, _logicData); }

				public:
					AirPursuitManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~AirPursuitManager();

					Framework::Room* get_for_room() const { return inRoom.get(); }

				public: // Logic
					override_ void advance(float _deltaTime);

					override_ void debug_draw(Framework::Room* _room) const;
					override_ void log_logic_info(LogInfoContext& _log) const;

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					AirPursuitManagerData const * airPursuitManagerData = nullptr;

					bool started = false;

					Random::Generator random;

					SafePtr<Framework::Room> inRoom; // where we are, we should be spawned in a room, always
					SafePtr<Framework::IModulesOwner> target; // provided by ai message from game script

					Array<PursuitInstance> pursuits;
				};

				class AirPursuitManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(AirPursuitManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new AirPursuitManagerData(); }

					AirPursuitManagerData();
					virtual ~AirPursuitManagerData();

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					List<PursuitSetup> pursuits;

					friend class AirPursuitManager;
				};

				//

			};
		};
	};
};