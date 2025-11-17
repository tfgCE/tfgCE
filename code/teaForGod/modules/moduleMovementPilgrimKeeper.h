#pragma once

#include "..\..\framework\interfaces\presenceObserver.h"
#include "..\..\framework\module\moduleMovement.h"
#include "..\..\framework\presence\presencePath.h"

#include "..\..\core\system\timeStamp.h"

namespace TeaForGodEmperor
{
	/**
	 *	Follows behind a pilgrim to keep it in the valid world.
	 *	Follows either centre or eyes (depending on pilgrim setting).
	 */
	class ModuleMovementPilgrimKeeper
	: public Framework::ModuleMovement
	, public Framework::IPresenceObserver
	{
		FAST_CAST_DECLARE(ModuleMovementPilgrimKeeper);
		FAST_CAST_BASE(Framework::ModuleMovement);
		FAST_CAST_BASE(Framework::IPresenceObserver);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static Framework::RegisteredModule<Framework::ModuleMovement> & register_itself();

		static bool is_disabled();
		static void disable_for(float _time);

		ModuleMovementPilgrimKeeper(Framework::IModulesOwner* _owner);
		virtual ~ModuleMovementPilgrimKeeper();
	
		void set_pilgrim(Framework::IModulesOwner* _pilgrim);
		void invalidate(); // will find a new location as soon as possible

		bool is_at_pilgrim(OPTIONAL_ OUT_ bool * _warning = nullptr, OPTIONAL_ OUT_ float* _penetrationPt = nullptr) const;

		Optional<Transform> const & get_valid_placement_vr() const { return validPlacementVR; }
		float get_max_distance() const { return maxDistance; }

		void request_teleport();

	public: // Module
		override_ void activate();
		override_ void on_owner_destroy();
		override_ void setup_with(Framework::ModuleData const* _moduleData);

	public: // IPresenceObserver
		implement_ void on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room* _intoRoom, Transform const& _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const* _throughDoors);
		implement_ void on_presence_added_to_room(Framework::ModulePresence* _presence, Framework::Room* _room, Framework::DoorInRoom* _enteredThroughDoor);
		implement_ void on_presence_removed_from_room(Framework::ModulePresence* _presence, Framework::Room* _room);
		implement_ void on_presence_destroy(Framework::ModulePresence* _presence);

	protected: // ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext& _context);

	private:
		static ::System::TimeStamp s_disabled;
		static float s_disabledLimit;

		bool disabled = false;

		float notInControlDisabledTimeLeft = 0.0f;

		SafePtr<Framework::IModulesOwner> pilgrim;
		Framework::ModulePresence* observingPresence = nullptr; // store pointer to a presence as safe pointer to pilgrim may be unavailable on destruction
		Framework::PresencePath pathToPilgrim;

		float maxDistance = 0.2f;
		float maxDistanceWarn = 0.1f; // if we're below this distance, a warning should appear

		Optional<Transform> validPlacementVR;
		float invalidPlacementTime = 0.0f;
		float collisionTime = 0.0f;
	};
};

