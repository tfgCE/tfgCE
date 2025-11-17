#pragma once

#include "..\..\..\core\other\simpleVariableStorage.h"

#include "..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\framework\ai\aiIMessageHandler.h"
#include "..\..\..\framework\presence\presencePath.h"

namespace Framework
{
	class Actor;
	class DoorInRoom;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class ElevatorCart;

			/**
			 *	Only receives messages to open and close
			 */
			class ElevatorCartPoint
			: public ::Framework::AI::Logic
			, public ::Framework::AI::IMessageHandler
			{
				FAST_CAST_DECLARE(ElevatorCartPoint);
				FAST_CAST_BASE(::Framework::AI::Logic);
				FAST_CAST_BASE(::Framework::AI::IMessageHandler);
				FAST_CAST_END();

				typedef ::Framework::AI::Logic base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new ElevatorCartPoint(_mind, _logicData); }

			public:
				ElevatorCartPoint(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~ElevatorCartPoint();

				float get_door_open() const { return doorOpen; }

			public: // IMessageHandler
				implement_ void handle_message(::Framework::AI::Message const & _message);

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private: friend class ElevatorCart;

			private:
				bool isValid = false;
				
				bool beAware = false;

				int openCount = 0;
				int atCartPointCount = 0;

				float doorOpen = 0.0f;
				float doorOpenTime = 0.0f;

				Optional<float> keepNonRareUpdateFor;

				// AppearanceControllers
				Name doorOpenACVarID;
				Name doorMovementCollisionOpenACVarID;

				SimpleVariableInfo doorOpenACVar;
				SimpleVariableInfo doorMovementCollisionOpenACVar;
			};

		};
	};
};