#pragma once

#include "..\..\core\functionParamsStruct.h"
#include "..\..\core\math\math.h"

namespace Framework
{
	class Door;
	class DoorInRoom;
	class Room;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace CollectInTargetCone
	{
		struct Entry
		{
			static int const MAX_DOORS = 32;
			float score; // the higher the score, the better the target
			Framework::IModulesOwner* target = nullptr;
			Framework::IModulesOwner* healthOwner = nullptr;
			Vector3 closestLocationWS; // in starting room
			Transform intoRoomTransform = Transform::identity;
		};
		struct Params
		{
			ADD_FUNCTION_PARAM_DEF(Params, bool, singleRoom, in_single_room, true);
			ADD_FUNCTION_PARAM_DEF(Params, bool, collectHealthOwners, collect_health_owners, true);
			ADD_FUNCTION_PARAM(Params, Name, withTag, with_tag);
			ADD_FUNCTION_PARAM_DEF(Params, bool, notAttached, not_attached, true);
			ADD_FUNCTION_PARAM(Params, Name, attachedWithTag, attached_with_tag);
		};
		void collect(Vector3 const& loc, Vector3 const& dir, float totalDist, float angle, float maxOffDist, Framework::Room* inRoom, float distanceLeft, ArrayStack<Entry>& collected, Framework::DoorInRoom* throughDoor, Transform const& roomTransform, Params const & _params = Params());

	};
};
