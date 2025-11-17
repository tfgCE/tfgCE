#pragma once

#include "..\player.h"

#include "..\..\..\core\globalDefinitions.h"

#include "..\..\..\framework\game\gameSceneLayer.h"
#include "..\..\..\framework\render\renderCamera.h"

//

#define USE_PLAYER_MOVEMENT_FROM_HEAD

//

namespace Framework
{
	class Object;
	class Player;
};

namespace TeaForGodEmperor
{
	class Game;
	class Player;
	interface_class IControllable;

	namespace GameSceneLayers
	{
		/**
		 *	Scene with world, doesn't go to anything below in stack (prev scenes)
		 */
		class World
		: public Framework::GameSceneLayer
		{
			FAST_CAST_DECLARE(World);
			FAST_CAST_BASE(Framework::GameSceneLayer);
			FAST_CAST_END();
			typedef Framework::GameSceneLayer base;
		public:
			World();
			World(Player* _player, Player* _playerTakenControl = nullptr);
			World(Framework::Render::Camera const & _camera);

			void use(Player* _player, Player* _playerTakenControl = nullptr);
			void use(Framework::Render::Camera const & _camera);

			override_ void on_start();

			override_ void on_paused();
			override_ void on_resumed();

			override_ void pre_advance(Framework::GameAdvanceContext const & _advanceContext);
			override_ void advance(Framework::GameAdvanceContext const & _advanceContext);

			override_ void process_controls(Framework::GameAdvanceContext const & _advanceContext);
			override_ void process_vr_controls(Framework::GameAdvanceContext const & _advanceContext);

			override_ void prepare_to_sound(Framework::GameAdvanceContext const & _advanceContext);

			override_ void prepare_to_render(Framework::CustomRenderContext * _customRC);
			override_ void render_on_render(Framework::CustomRenderContext * _customRC);

		private:
			Player* player;
			Player* playerTakenControl;
			Optional<Framework::Render::Camera> camera;

			ControlMode::Type playerControlMode[2];
			bool usingTakenControl = false;
			float playerControlModeTime[2];
			bool advancedAtLeastOnce = false;

			float slidingLocmotionLastStickRotationX = 0.0f;

			::System::TimeStamp timeSinceControllingTurret;
			Optional<Transform> controllingTurretVRBase; // to get the right relative look

#ifdef USE_PLAYER_MOVEMENT_FROM_HEAD
			struct PlayerMovementFromHead
			{
				Framework::Object* inControlOf = nullptr;
				// if we change rooms, we ignore buffered movement
				Framework::Room* inRoom = nullptr;
				// if set, if we move outside, we update (and reset disallowStandStillForTime)
				// if we reach disallowStandStillForTime, we update,
				Optional<Vector3> standStillLoc;
				float disallowStandStillForTime = 0.5f;

				float usePreciseMovement = 1.0f;

				void mark_moving() { disallowStandStillForTime = 0.5f; }
				bool should_act_as_standing() const { return disallowStandStillForTime == 0.0f; }
			} playerMovementFromHead;
#endif

			void process_controllable_controls(IControllable* _ic, Player & _player, bool _vr, float _deltaTime);
			void process_general_controls(Player & _player, bool _vr, float _deltaTime);

			Framework::Actor* get_player_in_control_actor_for_perception() const;
		};
	};
};
