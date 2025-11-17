#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\safeObject.h"

#include "movementParameters.h"

#include "..\debugSettings.h"
#include "..\nav\navDeclarations.h"
#include "..\nav\navNodeRequestor.h"
#include "..\presence\relativeToPresencePlacement.h"

namespace Framework
{
	interface_class IAIObject;
	interface_class IModulesOwner;
	class Object;
	class Room;

	namespace Nav
	{
		struct PathNode;
	};

	namespace AI
	{
		class Locomotion;
		class MindInstance;

		namespace LocomotionMoveControl
		{
			enum Type
			{
				None,
				DontMove,
				MoveTo2D,
				MoveTo3D,
				FollowPath2D,
				FollowPath3D,
			};

			inline bool is_move_to(Type _type)
			{
				return _type == MoveTo2D ||
					   _type == MoveTo3D;
			}

			inline bool is_path_following(Type _type)
			{
				return _type == FollowPath2D ||
					   _type == FollowPath3D;
			}

			inline bool is_2d(Type _type)
			{
				return _type == MoveTo2D ||
					   _type == FollowPath2D;
			}

			inline tchar const * to_char(Type _type)
			{
				if (_type == None) return TXT("none");
				if (_type == DontMove) return TXT("dont move");
				if (_type == MoveTo2D) return TXT("move to 2d");
				if (_type == MoveTo3D) return TXT("move to 3d");
				if (_type == FollowPath2D) return TXT("follow path 2d");
				if (_type == FollowPath3D) return TXT("follow path 3d");
				return TXT("??");
			}
		};

		namespace LocomotionTurnControl
		{
			enum Type
			{
				None,
				DontTurn,
				InMovementDirection2D,
				InMovementDirection3D,
				TurnTowards2D,
				TurnTowards3D,
				FollowPath2D,
				FollowPath3D,
			};

			inline bool is_in_movement_direction(Type _type)
			{
				return _type == InMovementDirection2D ||
					   _type == InMovementDirection3D;
			}
			
			inline bool is_turn_towards(Type _type)
			{
				return _type == TurnTowards2D ||
					   _type == TurnTowards3D;
			}

			inline bool is_following_path(Type _type)
			{
				return _type == FollowPath2D ||
					   _type == FollowPath3D;
			}

			inline bool is_2d(Type _type)
			{
				return _type == TurnTowards2D ||
					   _type == InMovementDirection2D ||
					   _type == FollowPath2D;
			}

			inline tchar const * to_char(Type _type)
			{
				if (_type == None) return TXT("none");
				if (_type == DontTurn) return TXT("dont turn");
				if (_type == InMovementDirection2D) return TXT("in movement direction 2d");
				if (_type == InMovementDirection3D) return TXT("in movement direction 3d");
				if (_type == TurnTowards2D) return TXT("turn towards 2d");
				if (_type == TurnTowards3D) return TXT("turn towards 3d");
				if (_type == FollowPath2D) return TXT("follow path 2d");
				if (_type == FollowPath3D) return TXT("follow path 3d");
				return TXT("??");
			}
		};

		struct LocomotionReference
		{
		public:
			enum GetLocation
			{
				GetTargetLocation,
				GetActualLocation,
				GetPresenceCentre
			};
			void clear();
			void set(Vector3 const & _location, Room* _room);
			void set(IAIObject * _aiObject);
			void set_placement(RelativeToPresencePlacement* _placement);
			bool get_location(Locomotion const & _locomotion, OUT_ Vector3 & _location, GetLocation _getLocation = GetTargetLocation) const; // returns true if possible/available, actualLocation = false means getting through ai
			IAIObject* get_object() const;
			bool is_set() const { return location.is_set() || (aiObject.is_set() && aiObject.get().is_set()) || placement.is_set(); }

			bool operator ==(LocomotionReference const & _a) const;
		private:
			Optional<Vector3> location; // within same room
			Optional<Room*> room;
			Optional<SafePtr<IAIObject>> aiObject;
			Optional<SafePtr<RelativeToPresencePlacement>> placement;
		};

		/**
		 *	Responsible for movement
		 *
		 *	Every aspect of movement can be treated seperately and on different needs
		 *	(we may allow player to move but don't turn or move only within specific distance from enemy - beyond distance we will auto move to enemy)
		 */
		class Locomotion
		{
		public:
			Locomotion(MindInstance* _mind);
			~Locomotion();

			void advance(float _deltaTime);

			void reset();

			MovementParameters & access_movement_parameters() { return movementParameters; }

			void dont_control(); // don't control any more, allow other input
			void stop(); // don't do a move

			void dont_move(); // don't move
			void dont_turn(); // don't turn

			void allow_any_move(); // anything else can control
			void allow_any_turn(); // anything else can control

			void move_to_2d(Vector3 const & _location, float _moveToRadius, MovementParameters const & _movementParameters); // within room and in local 2d space
			void move_to_2d(IAIObject * _aiObject, float _moveToRadius, MovementParameters const & _movementParameters);
			void move_to_3d(Vector3 const & _location, float _moveToRadius, MovementParameters const & _movementParameters); // within room and in local 2d space
			void move_to_3d(IAIObject * _aiObject, float _moveToRadius, MovementParameters const & _movementParameters);
			void follow_path_2d(Nav::Path * _path, Optional<float> const & _moveToRadius, MovementParameters const & _movementParameters);
			void follow_path_3d(Nav::Path * _path, Optional<float> const & _moveToRadius, MovementParameters const & _movementParameters);

			void dont_keep_distance();
			void keep_distance_2d(Vector3 const & _point, Optional<float> const & _min = NP, Optional<float> const & _max = NP, Optional<float> const & _verticalDistance = NP);
			void keep_distance_2d(IAIObject * _aiObject, Optional<float> const & _min = NP, Optional<float> const & _max = NP, Optional<float> const & _verticalDistance = NP);
			void keep_distance_3d(Vector3 const & _point, Optional<float> const & _min = NP, Optional<float> const & _max = NP);
			void keep_distance_3d(IAIObject * _aiObject, Optional<float> const & _min = NP, Optional<float> const & _max = NP);

			void turn_in_movement_direction_2d(); // turn in whatever direction we're moving
			void turn_in_movement_direction_3d(); // turn in whatever direction we're moving
			void turn_towards_2d(Vector3 const & _location, float _turnDeadZone);
			void turn_towards_2d(IAIObject * _aiObject, float _turnDeadZone);
			void turn_towards_2d(RelativeToPresencePlacement & _placement, float _turnDeadZone);
			void turn_towards_3d(Vector3 const & _location, float _turnDeadZone);
			void turn_towards_3d(IAIObject * _aiObject, float _turnDeadZone);
			void turn_follow_path_2d();
			void turn_follow_path_3d();
			// call after setting up turn
			void turn_yaw_offset(Optional<float> const& _offset);

			void stop_on_actors(bool _stopOnActors = true) { stopOnActors = _stopOnActors; }
			void stop_on_collision(bool _stopOnCollision = true) { stopOnCollision = _stopOnCollision; }

		public:
			bool calc_distance_to_keep_distance(float & _distance) const;

			bool where_I_want_to_be_in(OUT_ Room*& _room, OUT_ Transform& _placementWS, float _time, OPTIONAL_ OUT_ Transform* _intoRoom = nullptr) const; // ATM does not have keeping distance and turning

		public:
			bool check_if_path_is_ok(Nav::Path* _path) const; // using this we will know if to check only the part of the path that is left to follow

		public:
			bool is_moving() const { return LocomotionMoveControl::is_move_to(moveControl) || LocomotionMoveControl::is_path_following(moveControl); }
			bool is_following_path() const { return LocomotionMoveControl::is_path_following(moveControl); }
			bool is_following_path(Nav::Path * _path) const { return LocomotionMoveControl::is_path_following(moveControl) && movePath == _path; }
			bool has_finished_move() const { return has_finished_move(max(0.1f, moveToRadius)); }
			bool has_finished_move(float _left) const;
			bool is_done_with_move() const { return moveLost || has_finished_move(); }

			bool is_turning() const { return turnTowards.is_set(); }
			bool has_finished_turn() const { return turnDistanceLeft.is_set() && turnDistanceLeft.get() < max(0.1f, turnDeadZone); }
			bool has_finished_turn(float _left) const { return turnDistanceLeft.is_set() && turnDistanceLeft.get() < _left; }
			bool is_done_with_turn() const { return !is_turning() || has_finished_turn(); }

		public:
			void debug_draw();
			void debug_log(LogInfoContext & _log);

		private:
			MindInstance* mind;

#ifdef AN_USE_AI_LOG
			LogInfoContext pathLog; // to store changes etc
#endif

			LocomotionMoveControl::Type moveControl;
			LocomotionTurnControl::Type turnControl;

			bool stopOnActors = false; // should stop if actor would block us

			bool stopOnCollision = false; // should stop if colliding for too long
			float collisionTime = 0.0f;

			MovementParameters movementParameters;
			LocomotionReference moveTo;
			float moveToRadius;
			Nav::PathPtr movePath;
			int nextPathIdx = 0;
			Nav::NodeRequestor nextPathRequested;
			Nav::NodeRequestor importantPathRequested; // important on the path requested
			Optional<float> moveDistanceLeft;
			bool moveLost = false; // maybe we landed in a different room?

			LocomotionReference keepDistanceTo;
			bool keepDistance2D;
			Optional<float> keepDistanceMin;
			Optional<float> keepDistanceMax;
			float keepDistance2DVerticalSeparation = 1.0f;

			bool wallCrawling = false; // actually crawling on walls
			bool ceilingCrawling = false; // on ceilings
			bool fallFromCrawling = false;
			Room* wallCeilingCrawlingDoneForRoom = nullptr;
			float wallCeilingCrawlingRoomHeight = 2.0f;
			float wallCrawlingYawOffset = 0.0f;
			float wallCrawlingYawOffsetTimeLeft = 0.0f;

			LocomotionReference turnTowards;
			float turnDeadZone;
			Optional<float> turnDistanceLeft;
			Optional<float> turnYawOffset;

			void update_next_path_requested();

			void follow_path(Nav::Path * _path, Optional<float> const & _moveToRadius, MovementParameters const & _movementParameters, bool _2d);

			Vector3 get_up_dir(ModulePresence* _presence) const;
			Transform get_node_placement(Nav::PathNode const & _node, Vector3 const & _imoLoc) const;
			Vector3 get_node_location(Nav::PathNode const & _node, Vector3 const & _imoLoc) const;

			friend struct LocomotionReference;
		};
	};
};
