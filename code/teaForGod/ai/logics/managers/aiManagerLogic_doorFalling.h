#pragma once

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class DoorInRoom;
	class TexturePart;
	interface_class IModulesOwner;
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
				 */
				class DoorFallingManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(DoorFallingManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new DoorFallingManager(_mind, _logicData); }

				public:
					DoorFallingManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~DoorFallingManager();

				public: // Logic
					implement_ void advance(float _deltaTime);

					implement_ void learn_from(SimpleVariableStorage & _parameters);

				private:
					static LATENT_FUNCTION(execute_logic);
				
				private:
					Random::Generator rg;

					SafePtr<Framework::DoorInRoom> fallingDoor;

					Transform initialPlacement = Transform::identity;
					Optional<Transform> currentPlacement;

					float verticalVelocity = 0.0f;

					float maxVerticalVelocity = 0.0f;
				
					float gravity = 9.81f;
				
					Rotator3 fallingOrientation = Rotator3::zero;
					Rotator3 fallingOrientationAcceleration = Rotator3::zero;
					Rotator3 fallingOrientationMaxVelocity = Rotator3::zero;

					Rotator3 currentRelOrientation = Rotator3::zero;
					Rotator3 orientationVelocity = Rotator3::zero;
				};
			};
		};
	};
};