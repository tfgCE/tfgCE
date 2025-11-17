#pragma once

#include "aiLogic_h_craft.h"

#include "components\aiLogicComponent_airfighterArms.h"

#include "..\..\..\core\mesh\boneID.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"

namespace Framework
{
	class Display;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class AirfighterData;

			class Airfighter
			: public H_Craft
			{
				FAST_CAST_DECLARE(Airfighter);
				FAST_CAST_BASE(H_Craft);
				FAST_CAST_END();

				typedef H_Craft base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Airfighter(_mind, _logicData); }

			public:
				Airfighter(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Airfighter();

			public: // AILogic
				override_ void advance(float _deltaTime);

			private:
				AirfighterData const * airfighterData = nullptr;

				Framework::AI::MessageHandler messageHandler;

				AirfighterArms arms;

				bool shooterWaveActive = false; // driven by shooter wave
				bool shooterWaveActiveStart = false; // indicates that the initial message should be sent
				bool beActive = false; // will be set to active when anything provided
				bool beActiveAttack = false; // only for attack (doesn't alter beActive)

				struct TargetInfo
				{
					SafePtr<Framework::IModulesOwner> target;
					Optional<float> attackLength;
					Optional<float> attackDeadLimit;
				};
				Array<TargetInfo> targetList;

				SafePtr<Framework::IModulesOwner> currentTarget;
				Name currentTargetPlacementSocket;

				struct ShooterWaveSetup
				{
					SafePtr<Framework::IModulesOwner> followIMO;
					SafePtr<Framework::IModulesOwner> targetIMO;
					Optional<Name> targetPlacementSocket;
					Optional<Vector3> moveTo; // offset to followIMO or WS
					Optional<Vector3> moveDir; // offset to followIMO or WS
					Optional<float> limitFollowRelVelocity;
					Optional<Vector3> followRelVelocity;
					Optional<bool> readyToAttack;
					Optional<bool> attack;
					Optional<float> attackLength;
					Optional<float> shootStartOffsetPt;
					Optional<Vector3> shootStartOffsetTS;
				} shooterWaveSetup;

				struct AirfighterSetup
				{
					Optional<float> attackDistance;
					Optional<float> readyToAttackDistance;
					Optional<float> targetDistance;
					Optional<float> flyByAttackDistance;
					Optional<float> preferredDistance;
					Optional<float> preferredAltitude; // relative
					Optional<float> preferredApproachAltitude; // relative
					Optional<float> preferredAwayAltitude; // relative
					Optional<float> limitFollowRelVelocity;
					Optional<float> breakOffDelay; // when no target
					Optional<float> breakOffYaw; // when no target
					SafePtr<Framework::IModulesOwner> followIMO;
					bool useFollowIMO = false;
					bool allowFollowOffsetAdditionalRange = false;
					Optional<Vector3> followOffset;
					bool flyByAttack = false;
				} airfighterSetup;
				 
				// run
				bool readyToAttack = false;
				bool attack = false;
				float attackTime = 0.0f;
				float attackDeadTime = 0.0f; // to allow shooting at a dead target
				bool breakingOff = false;
				enum State
				{
					None,
					
					FlyByApproach,
					FlyByAttack,
					FlyAway,

					RemainClose
				};
				State state = State::None;
				Optional<float> requestedDistance = 0.0f;
			};

			class AirfighterData
			: public H_CraftData
			{
				FAST_CAST_DECLARE(AirfighterData);
				FAST_CAST_BASE(H_CraftData);
				FAST_CAST_END();

				typedef H_CraftData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new AirfighterData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class Airfighter;

				AirfighterArmsData armsData;
			};

		};
	};
};