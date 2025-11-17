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
				class ShooterWavesManagerData;
				struct ShooterActionPoint;
				struct ShooterInstance;
				struct ShooterWave;

				struct ShooterActionPoint
				: public RefCountObject
				{
					Framework::WorldSettingCondition shooterType; // for entry
					RUNTIME_ Array<Framework::LibraryStored*> shooterTypes;

					bool entry = false;
					bool exit = false;
					Name id;
					Tags tags;
					TagCondition continueWithTagged; // allows to be continued only with certain action points
					TagCondition relativeToObjectTagged; // may be null, uses WS then
					RUNTIME_ SafePtr<Framework::IModulesOwner> relativeToObject; // used in instance
					struct SlotPoint
					{
						Optional<Vector3> location;
						Optional<Vector3> relVelocity;
					};
					ArrayStatic<SlotPoint, SHOOTER_WAVES_SLOT_LIMIT> slotPoints;

					Optional<float> limitFollowRelVelocity;

					Optional<bool> nextJustOneRequired; // it's enough when the first one reaches the distance
					Optional<float> nextOnDistance; // this close to the location (should be defined)
					
					Optional<bool> attackJustOneRequired; // it's enough when the first one reaches the distance
					Optional<float> readyToAttackOnDistance; // optional, used when waiting for attackOnDistance only
					Optional<float> attackOnDistance; // this close to the location (should be defined)
					Optional<float> beforeAttackDelay;
					Optional<float> attackLength;
					Optional<float> shootStartOffsetPt; // how much time does it require to get onto the target
					Optional<Vector3> shootStartOffsetTS; // for attack length will start here in target's space
					Optional<float> postAttackDelay;
					
					Optional<float> disappearOnDistance; // this close to the location (should be defined)
					Optional<float> disappearOnDistanceAway; // this far from the location (or starting point)

					bool load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc);
					bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);
				};

				struct ShooterInstance
				: public RefCountObject
				{
					SafePtr<Framework::IModulesOwner> shooter;
					bool requiresSetup = true;
					Optional<Transform> placementOnActionPointStart;
				};

				struct ShooterWave
				: public RefCountObject
				{
					RefCountObjectPtr<ShooterActionPoint> currentActionPoint;
					int slotCount = 0;
					int slotIdxOffset = 0; // as setup, raw value
					float waitToStart = 0.0f; // might be spawning there, so we w
					ArrayStatic<ShooterInstance, SHOOTER_WAVES_SLOT_LIMIT> shooters;

					enum State
					{
						Started,

						Spawning,
						PlacePostSpawn,

						WaitForNext,
						//
						WaitForAttack,
						BeforeAttack,
						Attack,
						PostAttack,
						//
						WaitForDisappear,

						Ended,
					};

					bool readyToAttack = false;

					State state = State::Started;

					Optional<float> timeLeft;
				};

				/**
				 */
				class ShooterWavesManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(ShooterWavesManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new ShooterWavesManager(_mind, _logicData); }

				public:
					ShooterWavesManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~ShooterWavesManager();

					Framework::Room* get_for_room() const { return inRoom.get(); }

				public: // Logic
					override_ void advance(float _deltaTime);

					override_ void debug_draw(Framework::Room* _room) const;

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					ShooterWavesManagerData const * shooterWavesManagerData = nullptr;

					Random::Generator random;
					bool started = false;
					bool ended = false;

					float reachedEndDelay = 1.0f; // delay on reaching end
					float reachedEndTime = 0.0f; // delay on reaching end
					Name triggerTrapWhenReachedEnd; // only if all waves finished

					int waveIdx = 0;
					int waveCount = 0;

					SafePtr<Framework::Room> inRoom;
					SafePtr<Framework::IModulesOwner> targetIMO;

					Array<RefCountObjectPtr<Framework::DelayedObjectCreation>> shooterDOCs;
					
					Array< RefCountObjectPtr<ShooterActionPoint>> actionPoints; // created on setup, initialised later
					Array< RefCountObjectPtr<ShooterActionPoint>> entryActionPoints; // selected from above

					Optional<float> nextWaveTimeLeft;

					Array<RefCountObjectPtr<ShooterInstance>> shooters;
					RefCountObjectPtr<Framework::DelayedObjectCreation> spawningShooterDOC;
					RefCountObjectPtr<ShooterWave> currentWave;
					Array<RefCountObjectPtr<ShooterWave>> waves;

					bool done_distance(ShooterWave* wave, Optional<float> const& _distanceClose, Optional<float> const& _distanceAway, Optional<bool> const& _justOneRequired = NP);
					Transform calculate_target_placement(ShooterWave* wave, int _shooterIdx, bool _getOrientation);

					void reached_end(bool _wavesDone = false);
				};

				class ShooterWavesManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(ShooterWavesManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new ShooterWavesManagerData(); }

					ShooterWavesManagerData();
					virtual ~ShooterWavesManagerData();

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					Array<RefCountObjectPtr<ShooterActionPoint>> actionPoints;

					TagCondition targetTagged;
					Name targetPlacementSocket;

					Range waveInterval = Range::empty;
					Range waveIntervalQuick = Range::empty;
					RangeInt waveCount = RangeInt::empty;
					struct Wave
					{
						Random::Number<int> slotCount;
						Name forceActionPoint;
					};
					Array<Wave> waves;

					friend class ShooterWavesManager;
				};

				//

			};
		};
	};
};