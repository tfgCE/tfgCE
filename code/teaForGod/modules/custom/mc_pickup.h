#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\module\moduleData.h"
#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\types\colour.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		/**
		 *	Tells that object can be picked up 
		 */
		class Pickup
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(Pickup);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			Pickup(Framework::IModulesOwner* _owner);
			virtual ~Pickup();

			float get_throw_velocity_muliplier() const { return throwVelocityMultiplier; }
			Rotator3 get_throw_rotation_muliplier() const { return throwRotationMultiplier; }
			bool can_be_put_into_pocket() const { return canBePutIntoPocket; }
			Name const & get_pickup_reference_socket() const { return pickupReferenceSocket; }

		public:
			void mark_interested_picking_up() { interestedPickingUp.increase(); }
			void mark_no_longer_interested_picking_up() { interestedPickingUp.decrease(); }
			int get_interested_picking_up_count() const { return interestedPickingUp.get(); }

		public:
			// change and return last one
			bool set_can_be_picked_up(bool _canBePickedUp) { Concurrency::ScopedSpinLock lock(canBePickedUpLock); bool result = canBePickedUp; canBePickedUp = _canBePickedUp; return result; }
			bool can_be_picked_up() const { return canBePickedUp; }

			void be_held_by(Framework::IModulesOwner* _byOwner, Framework::IModulesOwner* _byHand = nullptr) { isHeldByOwner = _byOwner; isHeldBy = _byHand ? _byHand : _byOwner; }
			bool is_held() const { return isHeldByOwner.is_set(); }
			Framework::IModulesOwner* get_held_by() const { return isHeldBy.get(); }
			Framework::IModulesOwner* get_held_by_owner() const { return isHeldByOwner.get(); }

		public:
			void set_in_pocket_of(Framework::IModulesOwner* _inPocketOf) { inPocketOf = _inPocketOf; }
			void clear_in_pocket_of(Framework::IModulesOwner* _inPocketOf) { if (inPocketOf == _inPocketOf) { inPocketOf.clear(); } }

			bool is_in_pocket() const { return inPocketOf.is_set(); }

		public:
			void set_in_holder_of(Framework::IModulesOwner* _inHolderOf) { inHolderOf = _inHolderOf; }
			void clear_in_holder_of(Framework::IModulesOwner* _inHolderOf) { if (inHolderOf == _inHolderOf) { inHolderOf.clear(); } }

			bool is_in_holder() const { return inHolderOf.is_set(); }
			bool is_in_holder(Framework::IModulesOwner* _inHolderOf) const { return inHolderOf.get() == _inHolderOf; }

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

		private:
			Concurrency::SpinLock canBePickedUpLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.CustomModules.Pickup.canBePickedUpLock")); // to disallow changes
			bool canBePickedUp = true;
			bool canBePutIntoPocket = true;
			Name pickupReferenceSocket; // socket that is used when choosing/finding object to pickup

			SafePtr<Framework::IModulesOwner> isHeldBy; // the holding hand
			SafePtr<Framework::IModulesOwner> isHeldByOwner; // the actual holder, not its part (pilgrim, not pilgrim's hand)
			SafePtr<Framework::IModulesOwner> inPocketOf;
			SafePtr<Framework::IModulesOwner> inHolderOf;

			float throwVelocityMultiplier = 1.0f;
			Rotator3 throwRotationMultiplier = Rotator3(1.0f, 1.0f, 1.0f);

			Concurrency::Counter interestedPickingUp;
		};
	};
};

