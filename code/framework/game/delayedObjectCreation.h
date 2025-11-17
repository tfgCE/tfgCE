#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\random\random.h"
#include "..\..\core\tags\tag.h"

#include "..\world\doorType.h"

#include <functional>

namespace Framework
{
	class DoorType;
	class Game;
	class Module;
	class Object;
	class ObjectType;
	class Room;
	class RoomType;
	class SubWorld;
	interface_class IModulesOwner;

	struct DelayedObjectCreation
	: public RefCountObject
	{
		int priority = 0;
		bool devSkippable = false;
		bool activateImmediatelyEvenIfRoomVisible = false;
		bool processPOIsImmediately = false;
		SafePtr<Room> inRoom; // has to be set, if not, will be ignored
		String name;
		SafePtr<IModulesOwner> imoOwner; // if being created as a sub object of an object
		IModulesOwner* wmpOwnerObject = nullptr; // ai manager etc
		ObjectType* objectType = nullptr;
		RoomType* roomType = nullptr;
		DoorType* doorType = nullptr;
		SubWorld* inSubWorld = nullptr; // doesn't have to be provided, will be taken from room
		Tags tagSpawned;
		bool checkSpaceBlockers = true;
		bool ignorableVRPlacement = false; // for not accessible by the player, when vr anchor is not there
		bool ignoreVRPlacementIfNotPresent = false; // for distant/fake doors we don't care about vr placement, if we can't get one
		Optional<bool> replacableByDoorTypeReplacer;
		bool reverseSide = false; // for door swaps A/B
		int groupId = NONE; // to group for navigation
		bool skipHoleCheckOnMovingThroughForOtherDoor = false; // check POI/DoorInRoom
		bool cantMoveThroughDoor = false; // check POI/DoorInRoom
		bool beSourceForEnvironmentOverlaysRoomVars = false; // check POI/DoorInRoom
		Name placeDoorOnPOI;
		Name connectToPOIInSameRoom;
		TagCondition connectToPOITaggedInSameRoom;
		Optional<DoorOperation::Type> doorOperation;
		Transform placement = Transform::identity;
		std::function<bool(Object* _object)> place_function = nullptr; // returns false if should be destroyed
		SimpleVariableStorage variables;
		Random::Generator randomGenerator = Random::Generator(0, 0);
		std::function<void(Object* _object)> pre_initialise_modules_function = nullptr;
		std::function<void(Module* _module)> pre_setup_module_function = nullptr;
		std::function<void(Object* _object)> post_initialise_modules_function = nullptr;
		std::function<void(Room* _room)> post_create_room_function = nullptr;

		SafePtr<IModulesOwner> createdObject;

		bool can_be_processed() const;
		bool can_ever_be_processed() const; // if there is no room we wanted to create it for, maybe we should drop it?
		void process(Game* _game, bool _immediate = false);

		bool is_done() const { return done; }

	private:
		bool done = false;
	};
};

