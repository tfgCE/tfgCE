#pragma once

#include "..\..\..\core\other\simpleVariableStorage.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class PilgrimFakeData;

			/**
			 *	This is used to fake pilgrims in background
			 */
			class PilgrimFake
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(PilgrimFake);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new PilgrimFake(_mind, _logicData); }

			public:
				PilgrimFake(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~PilgrimFake();

			public: // Logic
				implement_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);

			private:
				PilgrimFakeData const * pilgrimFakeData = nullptr;

				Random::Generator rg;

				bool started = false;

				Transform head = Transform::identity;
				Transform handLeft = Transform::identity;
				Transform handRight = Transform::identity;

				Transform animatedHead = Transform::identity;
				Transform animatedHandLeft = Transform::identity;
				Transform animatedHandRight = Transform::identity;

				Transform defaultHead = Transform::identity;
				Transform defaultHandLeft = Transform::identity;
				Transform defaultHandRight = Transform::identity;

				Optional<Transform> forceHandLeft;
				Optional<Transform> forceHandRight;

				struct LookAt
				{
					Framework::RelativeToPresencePlacement at;
					Vector3 offset = Vector3::zero;
					Framework::SocketID atSocket;

					float power = 0.8f;
					Range2 limits = Range2(Range(-90.0f, 90.0f), Range(-40.0f, 50.0f));
				} lookAt;

				struct Move
				{
					bool newOrder = false;

					bool move = false;
					Optional<Name> gait;
					Optional<float> speed;
					Optional<Name> toPOI;
				} move;

				struct Aim
				{
					Framework::RelativeToPresencePlacement at;
					Vector3 offset = Vector3::zero;

					bool shoot = false;

					float use = 0.0f;
					Rotator3 aimAngles = Rotator3::zero;

					Range shootLength = Range::empty;
					Range shootBreakLength = Range::empty;

					float shootLengthTimeLeft = 0.0f;
					float shootBreakLengthTimeLeft = 0.0f;

					float shootRightTimeLeft = 0.0f;
					float shootLeftTimeLeft = 0.0f;

					SimpleVariableInfo aim_0_0_var;
					SimpleVariableInfo aim_L45_0_var;
					SimpleVariableInfo aim_R45_0_var;
					SimpleVariableInfo aim_0_30_var;
					SimpleVariableInfo aim_L45_30_var;
					SimpleVariableInfo aim_R45_30_var;
				} aim;

				SimpleVariableInfo leftHandAIVar;
				SimpleVariableInfo rightHandAIVar;

				SafePtr<Framework::IModulesOwner> leftWeapon;
				SafePtr<Framework::IModulesOwner> rightWeapon;
				float leftWeaponShootInterval = 0.0f;
				float rightWeaponShootInterval = 0.0f;

				void read_animated();
			};

			class PilgrimFakeData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(PilgrimFakeData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PilgrimFakeData(); }

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);

			private:

				friend class PilgrimFake;
			};
		};
	};
};