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
	
	namespace ModuleNavElements
	{
		class Cart;
	};

	namespace AI
	{
		namespace Logics
		{
			class ElevatorCartPoint;

			namespace ElevatorCartPath
			{
				enum Type
				{
					Linear,
					CurveMoveXZ,
				};
			};

			/**
			 *	Always starts at A!
			 *	If you want it to wait for player, spawn it there and use moveOnlyWithPlayer
			 */
			class ElevatorCart
			: public ::Framework::AI::Logic
			, public ::Framework::AI::IMessageHandler
			{
				FAST_CAST_DECLARE(ElevatorCart);
				FAST_CAST_BASE(::Framework::AI::Logic);
				FAST_CAST_BASE(::Framework::AI::IMessageHandler);
				FAST_CAST_END();

				typedef ::Framework::AI::Logic base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new ElevatorCart(_mind, _logicData); }

			public:
				ElevatorCart(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~ElevatorCart();

				ElevatorCartPath::Type get_cart_path() const { return cartPath; }

				Framework::IModulesOwner* get_elevator_stop_a() const { return elevatorStopA.get(); }
				Framework::IModulesOwner* get_elevator_stop_b() const { return elevatorStopB.get(); }
				Framework::IModulesOwner* get_target_stop() const { return targetStop; }

				Transform const & get_elevator_stop_a_cart_point() const { return elevatorStopACartPoint; }
				Transform const & get_elevator_stop_b_cart_point() const { return elevatorStopBCartPoint; }

				Framework::IModulesOwner* get_cart_point_a() const { return cartPointA.get(); }
				Framework::IModulesOwner* get_cart_point_b() const { return cartPointB.get(); }

				Framework::DoorInRoom* get_elevator_stop_a_door() const { return elevatorStopADoor.get(); }
				Framework::DoorInRoom* get_elevator_stop_b_door() const { return elevatorStopBDoor.get(); }

				bool is_vertical() const;

			public: // IMessageHandler
				implement_ void handle_message(::Framework::AI::Message const & _message);

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

				override_ void log_logic_info(LogInfoContext& _log) const;

			private:
				bool initialised = false;
				bool initialisedFromRoom = false;

				bool directSpeedControl = false; // instead of using buttons that tell us where to go, direct speed control is used (assuming we have slider control that goes from -1 to 1 with 0 being neutral)
												 // should not change during runtime
				Optional<float> directSpeedRequested; // normalised -1 to 1, if not set, no player on-board, use normal approach

#ifdef AN_DEVELOPMENT_OR_PROFILER
				float wtAdvancedAt = 0.0f;
#endif

				bool allowActiveButtonWithoutPlayer = true; // parameter/variable "noActiveButtonsForElevatorsWithoutPlayer"(bool)

				bool moveOnlyWithPlayer = false;

				ElevatorCartPath::Type cartPath = ElevatorCartPath::Linear;

				SafePtr<Framework::IModulesOwner> elevatorStopA; // where platform stops (Y is towards exit)
				SafePtr<Framework::IModulesOwner> elevatorStopB;
				Transform elevatorStopACartPoint = Transform::identity; // relative to actor/platform, they can be identity if cart point itself is offset
				Transform elevatorStopBCartPoint = Transform::identity; // relative to actor/platform, they can be identity if cart point itself is offset

				bool closeRoomDoorWhileMoving = false;
				bool openRoomDoorWhileStoppedAndBasesPlayer = false;
				SafePtr<Framework::DoorInRoom> elevatorStopADoor; // if close enough to cart point, might be more important to actually focus on the door instead
				SafePtr<Framework::DoorInRoom> elevatorStopBDoor; // if close enough to cart point, might be more important to actually focus on the door instead
				float checkForDoorBeyondDelay = 0.0f;
				SafePtr<Framework::DoorInRoom> elevatorStopADoorBeyond; // sort of hack: if there is a small room and there is a door really really close
				SafePtr<Framework::DoorInRoom> elevatorStopBDoorBeyond; // those two may disappear when a room beyond is destroyed

				ModuleNavElements::Cart* navElement = nullptr;

				SafePtr<Framework::IModulesOwner> cartPointA;
				SafePtr<Framework::IModulesOwner> cartPointB;
				ElevatorCartPoint* cartPointALogic = nullptr;
				ElevatorCartPoint* cartPointBLogic = nullptr;
				bool cartPointAAware = false;
				bool cartPointBAware = false;
				bool cartPointAOpen = false;
				bool cartPointBOpen = false;
				bool atCartPointA = false;
				bool atCartPointB = false;

				Framework::IModulesOwner* targetStop = nullptr;
				bool isValid = false;

				float speedOccupied = 3.0f;
				float speedEmpty = 8.0f;
				float acceleration = 5.0f;

				bool doorOpenWhenEmpty = false;
				bool doorOpenEarlyWhenStopping = true;
				bool doorOpenOnlyWhenStationary = true; // false - they can open/close while moving, if true, overrides doorOpenWhenEmpty and doorOpenEarlyWhenStopping
				float doorOpenTime = 0.3f;
				float doorOpen = 0.0f;

				Range waitTimeWithoutPlayerAfterPlayerLeftBaseCart = Range(1.5f, 3.0f); // no player on cart
				float waitTimeAfterPlayerLeftBaseCart = 1.0f; // player on cart
				Range waitTimeAtTarget = Range(10.0f, 20.0f); // much less often
				float currentWaitTimeAtTarget = 10.0f;
				float waitTimeAtTargetWithPlayer = 2.0f; // if "stop and stay" with player is not used

				bool forcedMove = false;
				bool forcedStop = false;
				bool shouldMove = false;
				bool isMoving = false;

				bool waitingForSomeone = false;
				bool waitingForAI = false;
				bool aiOnBoard = false;
				bool movedWithPlayer = false;

				float stayFor = 0.0f;

				float forceRemainStationary = 0.0f;
				float timeStationary = 0.0f;
				float timeToCheckPlayer = 0.0f;

				float consecutiveChangesTime = 0.0f;
				int consecutiveChangesCount = 0;

				struct Interactive
				{
					SafePtr<Framework::IModulesOwner> imo;
					Vector3 relativeMovementDir = Vector3::zero;
					bool buttonPressed = false;
					Interactive() {}
					explicit Interactive(Framework::IModulesOwner* _imo) : imo(_imo) {}
					int buttonState = 0;
				};
				Array<Interactive> interactives;

				// AppearanceControllers
				Name doorOpenACVarID; 
				Name doorMovementCollisionOpenACVarID;

				SimpleVariableInfo doorOpenACVar;
				SimpleVariableInfo doorMovementCollisionOpenACVar;

				Framework::IModulesOwner* find_cart_point(Framework::IModulesOwner* _elevatorStop, Transform const & _relativePlacement) const;
				static ElevatorCartPoint* get_cart_point_logic(Framework::IModulesOwner* _actor);

				void be_aware(Framework::IModulesOwner* _cartPoint, bool _beAware, REF_ bool& _isAware);

				void be_open(Framework::IModulesOwner* _cartPoint, REF_ bool & _cartPointOpen);
				void be_closed(Framework::IModulesOwner* _cartPoint, REF_ bool & _cartPointOpen);

				void be_at_cart_point(Framework::IModulesOwner* _cartPoint, REF_ bool & _atCartPoint);
				void be_not_at_cart_point(Framework::IModulesOwner* _cartPoint, REF_ bool & _atCartPoint);

				float get_wait_time_at_target(Framework::IModulesOwner* cart) const;

				void order_to_move(Framework::IModulesOwner* _target = nullptr);
				Framework::IModulesOwner* where_would_order_to_move() const;

				void order_to_stop();

				bool is_player_waiting_at(bool _atA) const;

			private:
				bool findStopsAndCartPoints = false; // if we won't be able to do it when starting
				bool find_stops_and_cart_points(bool _mayFail = true); // true if found
			};

		};
	};
};