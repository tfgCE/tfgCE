#include "mc_itemHolder.h"

#include "mc_grabable.h"
#include "mc_pickup.h"

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#include "..\gameplay\equipment\me_gun.h"

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(holdSocket);
DEFINE_STATIC_NAME(heldItemSocket);
DEFINE_STATIC_NAME(holdOnlyTagged);
DEFINE_STATIC_NAME(locked);
DEFINE_STATIC_NAME(lockWhenEmptied);

// held item parameters
DEFINE_STATIC_NAME(gunsUsePorts);

//

REGISTER_FOR_FAST_CAST(ItemHolder);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new ItemHolder(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & ItemHolder::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("itemHolder")), create_module);
}

ItemHolder::ItemHolder(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

ItemHolder::~ItemHolder()
{
}

void ItemHolder::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	holdSocket = _moduleData->get_parameter<Name>(this, NAME(holdSocket), holdSocket);
	heldItemSocket = _moduleData->get_parameter<Name>(this, NAME(heldItemSocket), heldItemSocket);

	{
		String hot = _moduleData->get_parameter<String>(this, NAME(holdOnlyTagged), String::empty());
		holdOnlyTagged.load_from_string(hot);
	}

	locked = _moduleData->get_parameter<bool>(this, NAME(locked), locked);
	lockWhenEmptied = _moduleData->get_parameter<bool>(this, NAME(lockWhenEmptied), lockWhenEmptied);
}

void ItemHolder::advance_post(float _deltaTime)
{
	base::advance_post(_deltaTime);

	{
		MODULE_OWNER_LOCK(TXT("ItemHolder::advance_post"));
		if (heldItem.is_set() &&
			!does_still_hold_object())
		{
			if (auto* hi = heldItem.get())
			{
				if (auto* pickup = hi->get_custom<CustomModules::Pickup>())
				{
					pickup->clear_in_holder_of(get_owner());
				}
			}
			heldItem.clear();
			mark_no_longer_requires(all_customs__advance_post);
			if (lockWhenEmptied)
			{
				set_locked(true);
			}
		}
	}
}

bool ItemHolder::hold(Framework::IModulesOwner* _item, bool _force)
{
	if (_item->get_custom<Grabable>())
	{
		// can't put grabbable into item holder
		return false;
	}

	MODULE_OWNER_LOCK(TXT("ItemHolder::hold"));

	if (heldItem.get() && does_still_hold_object())
	{
		return false;
	}

	if (!_force)
	{
		if (locked)
		{
			return false;
		}
		if (! holdOnlyTagged.is_empty())
		{
			if (auto* o = _item->get_as_object())
			{
				if (!holdOnlyTagged.check(o->get_tags()))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}

	if (auto* p = _item->get_presence())
	{
		if (heldItemSocket.is_valid() && _item->get_appearance()->has_socket(heldItemSocket))
		{
			p->attach_via_socket(get_owner(), heldItemSocket, holdSocket, false);
		}
		else
		{
			Transform offset = Transform::identity;
			if (_item->get_gameplay_as<ModuleEquipments::Gun>() &&
				! _item->get_value<bool>(NAME(gunsUsePorts),false))
			{
				todo_hack(TXT("to offset the gun up"));
				offset = Transform(Vector3(0.0f, 0.0f, 0.05f), Quat::identity);
			}
			p->attach_to_socket(get_owner(), holdSocket, false, offset.to_world(p->get_centre_of_presence_os().inverted()));
		}
		heldItem = _item;
		if (auto* hi = heldItem.get())
		{
			if (auto* pickup = hi->get_custom<CustomModules::Pickup>())
			{
				pickup->set_in_holder_of(get_owner());
			}
		}

		bool holds = does_still_hold_object();

		if (holds)
		{
			mark_requires(all_customs__advance_post);
			return true;
		}
	}

	return false;
}

Framework::IModulesOwner* ItemHolder::get_held() const
{
	MODULE_OWNER_LOCK(TXT("ItemHolder::get_held"));

	if (does_still_hold_object())
	{
		return heldItem.get();
	}
	else
	{
		return nullptr;
	}
}

bool ItemHolder::does_still_hold_object() const
{
	IS_MODULE_OWNER_LOCKED_HERE;

	if (auto* imo = heldItem.get())
	{
		if (auto* p = imo->get_presence())
		{
			if (p->is_attached_at_all_to(get_owner()))
			{
				return true;
			}
		}
	}

	return false;
}
