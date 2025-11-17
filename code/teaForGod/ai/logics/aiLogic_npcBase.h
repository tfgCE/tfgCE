#pragma once

#include "components\aiLogicComponent_predictTargetPlacement.h"
#include "..\aiPerceptionLock.h"
#include "..\aiPerceptionPause.h"
#include "..\..\..\core\math\math.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

namespace Framework
{
	struct InRoomPlacement;
	struct PresencePath;
	struct RelativeToPresencePlacement;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		class GetCommonVariable;

		namespace Logics
		{
			/**
			 *	Class for common values and defaults.
			 */
			class NPCBase
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(NPCBase);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				struct ShotInfo
				{
					Name projectile; // temporary object
					Name muzzleFlash; // temporary object
					Name sound; // sound
					Optional<Name> socket; // if we want to override and happen at a socket

					ShotInfo() {}
					ShotInfo(Name const& _projectile, Name const& _muzzleFlash, Optional<Name> const& _sound = NP, Optional<Name> const & _socket = NP) : projectile(_projectile), muzzleFlash(_muzzleFlash), sound(_sound.get(_projectile)), socket(_socket) {}
				};
			public:
				NPCBase(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData, LATENT_FUNCTION_VARIABLE(_executeFunction));
				virtual ~NPCBase();

			public:
				bool is_omniscient() const { return omniscient; }
				void be_omniscient(bool _be = true) { omniscient = _be; }
				
				bool should_always_look_for_new_enemy() const { return alwaysLookForNewEnemy; }

				int get_perception_socket_index() const { return perceptionSocketIdx; }
				int get_secondary_perception_socket_index() const { return secondaryPerceptionSocketIdx; }
				int get_scanning_perception_socket_index() const { return useSecondaryPerceptionForScanning? secondaryPerceptionSocketIdx : perceptionSocketIdx; }
				void use_secondary_perception_for_scanning(bool _useSecondaryPerceptionForScanning = true) { useSecondaryPerceptionForScanning = _useSecondaryPerceptionForScanning; }
				Range get_perception_fov() const { return perceptionFOV; }
				Range get_perception_vertical_fov() const { return ! perceptionVerticalFOV.is_empty()? perceptionVerticalFOV : get_perception_fov(); }
				Range const & get_perception_thinking_time() const { return perceptionThinkingTime; }

				float get_scanning_fov() const { return scanningFOV; }
				float get_scanning_speed() const { return scanningSpeed; }
				
				Optional<float> const & get_max_attack_distance() const { return maxAttackDistance; }
				void set_max_attack_distance(Optional<float> const& _max) { maxAttackDistance = _max; }

				int get_related_device_id() const { return relatedDeviceId; }
				Name const & get_related_device_id_var_name() const { return relatedDeviceIdVar; }

				void stay_in_room(bool _stayInRoom = true) { stayInRoom = _stayInRoom; reset_to_stay_in_room(); }
				bool should_stay_in_room() const { return stayInRoom; }
				Framework::RelativeToPresencePlacement & get_to_stay_in_room() { return toStayInRoom; }
				
				bool should_wander_forward() const { return shouldWanderForward; }

				void set_movement_gait(Name const& _movementGait = Name::invalid()) { movementGait = _movementGait; }
				Name const& get_movement_gait() const { return movementGait; }

				float get_glance_chance() const { return glanceChance; }

				bool is_ai_aggressive() const { return aiAggressive; }

				float get_combat_music_indicate_presence_distance() const { return combatMusicIndicatePresenceDistance; }
				bool is_ok_to_play_combat_music(Framework::RelativeToPresencePlacement const& _rtpp) const;
				bool is_ok_to_play_combat_music(Framework::PresencePath const& _pp) const;

			public:
				PredictTargetPlacement& access_predict_target_placement() { return predictTargetPlacement; }
				void use_predicted_target_placement_for_look_at(bool _use = true) { usePredictedTargetPlacementForLookAt = _use; }
				bool should_use_predicted_target_placement_for_look_at() const{ return usePredictedTargetPlacementForLookAt; }

			public:
				Array<ShotInfo> const& get_shot_infos() const { return shotInfos; }
				Array<ShotInfo> & access_shot_infos() { return shotInfos; }
				void auto_fill_shot_infos(); // using temporary objects data (tags), has to be explicitly called, check code for details

			public:
				void give_out_enemy_location(Framework::IModulesOwner* enemy, Framework::Room* inRoom, Transform const& _placement); // ai message
				void give_out_enemy_location(Framework::RelativeToPresencePlacement const & _pathToEnemy);

			public: // Logic
				override_ void advance(float _deltaTime);

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				bool omniscient = false;
				bool alwaysLookForNewEnemy = false;
				Name perceptionSocket; // default: perception
				Name secondaryPerceptionSocket; // default: secondary perception
				int perceptionSocketIdx = NONE;
				int secondaryPerceptionSocketIdx = NONE; // if we have an additional eye somewhere
				bool useSecondaryPerceptionForScanning = false;
				Range perceptionFOV = Range(-90.0f, 90.0f);
				Range perceptionVerticalFOV = Range::empty;
				Range perceptionThinkingTime = Range(0.05f, 0.15f);

				float scanningFOV = 90.0f;
				float scanningSpeed = 40.0f;

				Optional<float> maxAttackDistance;

				int relatedDeviceId = 0;
				Name relatedDeviceIdVar; // in other objects and in this one

				bool stayInRoom = false;
				Framework::RelativeToPresencePlacement toStayInRoom;

				bool shouldWanderForward = false; // information where should wander (forward or random)
				float glanceChance = 1.0f;

				Name movementGait;

				bool aiAggressive = true; // default, although some may ignore if by default they are not aggressive

				float combatMusicIndicatePresenceDistance = 0.0f;

				Array<ShotInfo> shotInfos;

				bool firstAdvance = true;

				PredictTargetPlacement predictTargetPlacement;
				bool usePredictedTargetPlacementForLookAt = false; // not used yet

				void reset_to_stay_in_room();

			private: friend class TeaForGodEmperor::AI::GetCommonVariable; // just pointers that are managed and cached by common variables
				#define VAR(type, var) type* var##Ptr = nullptr;
				#include "..\aiCommonVariablesList.h"
				#undef VAR
			};

		};
	};
};