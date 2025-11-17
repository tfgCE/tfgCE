#pragma once

#include "..\..\managerDatas\aiManagerData_backgroundMover.h"

#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\sound\soundSource.h"

namespace Framework
{
	class ObjectType;
	class DoorInRoom;
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
				/**
				 * Background movers don't hold any of their own data.
				 * They use data that is defined in the room (type/generator)
				 * 
				 * It only takes "backgroundMoverData" [Name] as input to get the right data from room
				 */
				class BackgroundMover
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(BackgroundMover);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new BackgroundMover(_mind, _logicData); }

				public:
					BackgroundMover(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~BackgroundMover();

					Framework::Room* get_for_room() const { return inRoom.get(); }

				public: // Logic
					override_ void advance(float _deltaTime);

					override_ void debug_draw(Framework::Room* _room) const;

					override_ void log_logic_info(LogInfoContext& _log) const;

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					Random::Generator random;
					bool started = false;
					bool movedDoorsOnStart = false;

					Optional<Transform> applyOffset;

					bool shouldStartMovement = false;
					bool immediateFullSpeed = false;
					bool shouldEnd = false;
					Name triggerTrapWhenReachedEnd;
					Name endOnVoiceoverActor;
					float endOnVoiceoverActorOffset = 0.0f;
					bool endWhenCanExitPilgrimagesStartingRoom = false;
					Optional<float> timeToStop;
#ifdef AN_DEVELOPMENT_OR_PROFILER
					Optional<float> distanceCoveredToTimeToStop;
#endif

					SafePtr<Framework::Room> inRoom;
					RefCountObjectPtr<ManagerDatasLib::BackgroundMover> backgroundMoverData;

					float speed = 0.0f; // set from data
					bool adjustSpeedTarget = false;

					Optional<float> curveRadius;

					float currentSpeed = 0.0f;
					float currentSpeedTarget = 0.0f;
					Optional<float> forcedSpeed;
					float atDistance = 0.0f;
					float distanceCovered = 0.0f; // how much did we move
					Optional<float> endAtDistance;
					Transform chainAnchorPOIPlacement = Transform::identity;
					Transform offsetChainAnchorPOIPlacement = Transform::identity;
					Transform doorAnchorPOIPlacement = Transform::identity;
					Transform offsetDoorAnchorPOIPlacement = Transform::identity;
					Array<SafePtr<Framework::IModulesOwner>> backgroundMoverBasePresenceOwners;

					float appearDistance = 0.0f;
					float appearDistanceOffset = 0.0f; // offset gets reduced with movement, down to zero
					Optional<float> appearDistanceLimit; // if set, will limit appear distance + offset

					struct ProcessChain
					{
						static int const MAX_DEPTH = 8;
						ArrayStatic<int, MAX_DEPTH> at;
						ArrayStatic<int, MAX_DEPTH> repeatCount;

						ProcessChain()
						{
							SET_EXTRA_DEBUG_INFO(at, TXT("BackgroundMover::ProcessChain.at"));
						}
					} processChain;

					struct UsedObject
					{
						SafePtr<Framework::IModulesOwner> object;
						bool inUse = false;
						float alongDirSize = 0.0f;
						float chance = 1.0f;
						Transform placement; // used for curveRadius
					};
					Array<UsedObject> usedObjects;

					struct TempObject
					{
						int uoIdx = NONE;
						float chance = 1.0f;
						TempObject() {}
						TempObject(int _uoIdx, float _chance) : uoIdx(_uoIdx), chance(_chance) {}
					};
					Array<TempObject> tempObjects; // temporary array to find right objects

					struct UsedDoor
					{
						SafePtr<Framework::DoorInRoom> door;
						int order = 0;
						float atDistance = 0.0f;
						bool moveWithBackground = false;
						bool startingDoor = false;
						Name placeAtPOINamed;
						SafePtr<Framework::PointOfInterestInstance> placeAtPOI;
						
						// we want to sort them to have the furthest in the first spot (or highest order value)
						static int compare(void const* _a, void const* _b)
						{
							UsedDoor const & a = *plain_cast<UsedDoor>(_a);
							UsedDoor const & b = *plain_cast<UsedDoor>(_b);
							if (a.order < b.order) return B_BEFORE_A;
							if (a.order > b.order) return A_BEFORE_B;
							return A_AS_B;
						}
					};
					Array<UsedDoor> usedDoors;

					struct Sound
					{
						Name playSound;
						Optional<Name> onSocket;
						struct OnObject
						{
							SafePtr<Framework::IModulesOwner> object;
							Name setSpeedVar;
						};
						Array<OnObject> onObjects;
						Array<Framework::SoundSourcePtr> playedSounds;
					};
					Array<Sound> sounds;

					void on_start_movement();
					void on_stop_movement();

					void reset_background_mover();

					void advance_and_apply(float _deltaTime);

					float get_current_appear_distance() const;

					void offset_placement_on_curve(REF_ Transform& _placement, Vector3 const& _anchorPOI, Vector3 const& _dir) const;

					void on_speed_update();

					Vector3 calculate_move_dir() const;
				};

				//

			};
		};
	};
};