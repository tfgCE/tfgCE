#pragma once

#include "..\..\..\core\random\randomNumber.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class FactoryArmData;

			/**
			 */
			class FactoryArm
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(FactoryArm);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new FactoryArm(_mind, _logicData); }

			public:
				FactoryArm(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~FactoryArm();

			public: // Logic
				override_ void advance(float _deltaTime);
				override_ void ready_to_activate(); // first thought in world, when placed but before initialised

			private:
				static LATENT_FUNCTION(execute_logic);

			private:
				FactoryArmData const* factoryArmData;

				SimpleVariableInfo handTipPlacementMSVar;
				SimpleVariableInfo handTipIdlePlacementMSVar;
				SimpleVariableInfo handTipRotateSpeedVar;
				SimpleVariableInfo handTipRotateSpeedLengthVar;
				SimpleVariableInfo handTipAtTargetVar;

				float rotateSpeed = 0.0f;
				float rotateSpeedTarget = 0.0f;

				Transform targetPlacement;

				bool is_rotate_speed_at_target() const;
				bool is_hand_tip_at_target() const;

				void start_movement(Transform const & _to);
			};

			class FactoryArmData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(FactoryArmData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new FactoryArmData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class FactoryArm;

				Name handTipPlacementMSVar;
				Name handTipIdlePlacementMSVar;
				Name handTipRotateSpeedVar;
				Name handTipRotateSpeedLengthVar;
				Name handTipAtTargetVar;

				Random::Number<int> rotateCount;
				Range rotateTime = Range(1.0f, 2.0f);
				Range handTipRotateSpeed = Range(100.0f, 600.0f);
				float handTipRotateSpeedBlendTime = 1.0f;

				float handTipTargetOffset = 0.1f; // to move back and to
			};

		};
	};
};