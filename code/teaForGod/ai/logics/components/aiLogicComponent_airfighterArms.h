#pragma once

#include "..\..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\..\framework\appearance\socketID.h"

#include "..\..\..\..\core\functionParamsStruct.h"
#include "..\..\..\..\core\memory\safeObject.h"
#include "..\..\..\..\core\mesh\boneID.h"
#include "..\..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	class Display;
	class Library;
	interface_class IModulesOwner;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;
	namespace AI
	{
		class MindInstance;
	};
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			struct AirfighterArmsData;

			struct AirfighterArms
			{
			public:
				AirfighterArms();
				~AirfighterArms();

			public:
				void setup(Framework::AI::MindInstance* mind, AirfighterArmsData const & _airfighterData);

				struct AdvanceParams
				{
					ADD_FUNCTION_PARAM_PLAIN_INITIAL(AdvanceParams, float, deltaTime, with_delta_time, 0.0f);
					ADD_FUNCTION_PARAM_PLAIN_INITIAL(AdvanceParams, Framework::IModulesOwner*, owner, with_owner, nullptr);
					ADD_FUNCTION_PARAM_PLAIN_INITIAL(AdvanceParams, Transform, placementWS, with_placement_WS, Transform::identity);
					ADD_FUNCTION_PARAM_PLAIN_INITIAL(AdvanceParams, Vector3, velocityLinear, with_velocity_lineary, Vector3::zero);
					ADD_FUNCTION_PARAM_PLAIN_INITIAL(AdvanceParams, Rotator3, velocityRotation, with_velocity_rotation, Rotator3::zero);
					ADD_FUNCTION_PARAM_PLAIN_INITIAL(AdvanceParams, Vector3, roomVelocity, with_room_velocity, Vector3::zero);
				};
				void advance(AdvanceParams const & _params);

				void fold_arms(float _maxDelay = 0.9f);
				void be_armed(Optional<float> const& _maxDelay = NP, Optional<bool> const& _targetAfterwards = NP);
				void target(Framework::IModulesOwner* imo, Name const & _targetPlacementSocket = Name::invalid(), Vector3 const & _offsetTS = Vector3::zero);

				void shooting(bool _shoot);

			private:
				::Framework::AI::MessageHandler messageHandler;

				Random::Generator rg;

				enum ArmState
				{
					Idle,
					AttackPosition,
					Target,
				};

				struct Arm
				{
					Name id;

					Meshes::BoneID gunBone;
					Meshes::BoneID topBone;
					Framework::SocketID muzzleSocket;
					SimpleVariableInfo provideGunPlacementMSVar;
					SimpleVariableInfo provideGunStraightVar;

					Optional<float> timeToFold;
					Optional<float> timeToArm;
					ArmState timeToArmState = ArmState::AttackPosition;

					float gunToTopMaxLength = 0.0f;

					Transform topMS;
					Transform idleMS;
					Transform attackMS;
					Optional<Transform> axisMS; // to turn location around when aiming
					
					Vector3 targetDir = Vector3::yAxis;
					::SafePtr<Framework::IModulesOwner> aimAtIMO;
					Framework::SocketID targetPlacementSocket;
					Vector3 targetOffsetTS = Vector3::zero;

					ArmState state = ArmState::Idle;
					bool allowShooting = false; // auto, as we want
					bool shootingRequested = false;

					float atAttackWeight = 0.0f;

					Vector3 inertiaOffsetWS = Vector3::zero;

				};
				Array<Arm> arms;

				Vector3 prevVelocityLinear = Vector3::zero;
				Rotator3 prevVelocityRotation = Rotator3::zero;
				Optional<Transform> prevPlacementWS;

				float shootingInterval = 1.0f; // one muzzle to another
				float shootingIntervalTimeLeft = 0.0f;
				int shootingArmIdx = 0;

				bool addOwnersVelocityForProjectiles = false;


				Optional<float> projectileSpeed;
			};

			struct AirfighterArmsData
			{
			public:
				bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend struct AirfighterArms;

				struct Arm
				{
					Name id;
					
					Name gunBone;
					Name topBone;
					Name muzzleSocket;

					Name provideGunPlacementMSVar;
					Name provideGunStraightVar;
				};
				Array<Arm> arms;

				float shootingRPM = 60.0f; // per arm/muzzle
			};

		};
	};
};