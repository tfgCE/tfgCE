#include "mc_interactiveButtonHandler.h"

#include "mc_emissiveControl.h"
#include "mc_pickup.h"

#include "..\moduleAI.h"
#include "..\gameplay\modulePilgrim.h"

#include "..\..\game\gameDirector.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(hasToBeHeld);
DEFINE_STATIC_NAME(worksWithUseUsableByHolder);
DEFINE_STATIC_NAME(interactiveDeviceIdVar);
DEFINE_STATIC_NAME(emissiveOnUse);

//

REGISTER_FOR_FAST_CAST(CustomModules::InteractiveButtonHandler);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new CustomModules::InteractiveButtonHandler(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & CustomModules::InteractiveButtonHandler::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("interactiveButtonHandler")), create_module);
}

CustomModules::InteractiveButtonHandler::InteractiveButtonHandler(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

CustomModules::InteractiveButtonHandler::~InteractiveButtonHandler()
{
}

void CustomModules::InteractiveButtonHandler::reset()
{
	base::reset();
}

void CustomModules::InteractiveButtonHandler::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	hasToBeHeld = _moduleData->get_parameter<bool>(this, NAME(hasToBeHeld), hasToBeHeld);
	worksWithUseUsableByHolder = _moduleData->get_parameter<bool>(this, NAME(worksWithUseUsableByHolder), worksWithUseUsableByHolder);
	interactiveDeviceIdVar = _moduleData->get_parameter<Name>(this, NAME(interactiveDeviceIdVar), interactiveDeviceIdVar);
	emissiveOnUse = _moduleData->get_parameter<Name>(this, NAME(emissiveOnUse), emissiveOnUse);

	if (!interactiveDeviceIdVar.is_valid())
	{
		error(TXT("no \"interactiveDeviceIdVar\" set for \"%S\""), get_owner()->ai_get_name().to_char());
	}
}

void CustomModules::InteractiveButtonHandler::on_owner_destroy()
{
	base::on_owner_destroy();
}

void CustomModules::InteractiveButtonHandler::activate()
{
	base::activate();

	interactiveButtonHandler.has_to_be_held(hasToBeHeld);
	interactiveButtonHandler.initialise(get_owner(), interactiveDeviceIdVar);

	mark_requires(all_customs__advance_post);
}

void CustomModules::InteractiveButtonHandler::advance_post(float _deltaTime)
{
	base::advance_post(_deltaTime);

	rarer_post_advance_if_not_visible();

	bool prevWasOn = isOn;

	interactiveButtonHandler.advance(_deltaTime);

	bool buttonOn = interactiveButtonHandler.is_button_pressed();
	if (worksWithUseUsableByHolder)
	{
		if (auto* asPickup = get_owner()->get_custom<CustomModules::Pickup>())
		{
			if (auto* heldBy = asPickup->get_held_by_owner())
			{
				if (auto* pilgrim = heldBy->get_gameplay_as<ModulePilgrim>())
				{
					auto heldingHand = pilgrim->get_helding_hand(get_owner());
					if (heldingHand.is_set())
					{
						if (pilgrim->is_controls_use_as_usable_equipment_pressed(heldingHand.get()))
						{
							buttonOn = true;
						}
					}
				}
			}
		}
		else
		{
			an_assert(false, TXT("works only for pickups"));
		}
	}

	isOn = buttonOn;

	if (emissiveOnUse.is_valid())
	{
		if (isOn ^ prevWasOn)
		{
			if (auto* em = get_owner()->get_custom<CustomModules::EmissiveControl>())
			{
				if (isOn)
				{
					em->emissive_activate(emissiveOnUse);
				}
				else
				{
					em->emissive_deactivate(emissiveOnUse);
				}
			}
			for_every_ref(imo, interactiveButtonHandler.get_buttons())
			{
				if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
				{
					if (isOn)
					{
						em->emissive_activate(emissiveOnUse);
					}
					else
					{
						em->emissive_deactivate(emissiveOnUse);
					}
				}
			}
		}
	}
}
