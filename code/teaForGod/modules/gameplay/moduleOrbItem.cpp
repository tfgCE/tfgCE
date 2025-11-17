#include "moduleOrbItem.h"

#include "moduleEquipment.h"
#include "modulePilgrim.h"

#include "..\custom\health\mc_health.h"

#include "..\..\game\damage.h"
#include "..\..\game\game.h"
#include "..\..\game\gameSettings.h"

#include "..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\framework\debug\previewGame.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\world\world.h"

//

using namespace TeaForGodEmperor;

//

// module params
DEFINE_STATIC_NAME(orbItemType);
#ifdef WITH_CRYSTALITE
DEFINE_STATIC_NAME(crystaliteAmount);
#endif
DEFINE_STATIC_NAME(addVelocityImpulseInterval);
DEFINE_STATIC_NAME(addVelocityImpulse);

// orb item types
#ifdef WITH_CRYSTALITE
DEFINE_STATIC_NAME(crystalite);
#endif

//

REGISTER_FOR_FAST_CAST(ModuleOrbItem);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleOrbItem(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleOrbItemData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleOrbItem::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("orbItem")), create_module, create_module_data);
}

ModuleOrbItem::ModuleOrbItem(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

ModuleOrbItem::~ModuleOrbItem()
{
}

void ModuleOrbItem::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	orbItemData = fast_cast<ModuleOrbItemData>(_moduleData);
	if (_moduleData)
	{
		orbItemType = _moduleData->get_parameter<Name>(this, NAME(orbItemType), orbItemType);

		addVelocityImpulseInterval = _moduleData->get_parameter<Range>(this, NAME(addVelocityImpulseInterval), addVelocityImpulseInterval);
		addVelocityImpulse = _moduleData->get_parameter<Range>(this, NAME(addVelocityImpulse), addVelocityImpulse);

#ifdef WITH_CRYSTALITE
		if (orbItemType == NAME(crystalite))
		{
			crystalite = _moduleData->get_parameter<int>(this, NAME(crystaliteAmount), crystalite);
		}
#endif
	}
}

void ModuleOrbItem::reset()
{
	base::reset();
}

void ModuleOrbItem::initialise()
{
	base::initialise();
}

#ifdef WITH_CRYSTALITE
bool ModuleOrbItem::is_crystalite() const
{
	return orbItemType == NAME(crystalite);
}
#endif

void ModuleOrbItem::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

	if (!addVelocityImpulse.is_zero())
	{
		timeToAddVelocityImpulse -= _deltaTime;
		if (timeToAddVelocityImpulse < 0.0f)
		{
			timeToAddVelocityImpulse = Random::get(addVelocityImpulseInterval);
			if (timeToAddVelocityImpulse > 0.0f)
			{
				get_owner()->get_presence()->add_velocity_impulse(Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal() * Random::get(addVelocityImpulse));
			}
		}
	}

	bool disappear = false;

	if (taken)
	{
		disappear = true;
	}

	if (disappear)
	{
		lifeScale -= (disappear ? 5.0f : 1.0f) * _deltaTime;
	}

	float targetFinalScale = lifeScale;
	finalScale = finalScale.get(targetFinalScale); // if not set, snap
	finalScale = blend_to_using_time(finalScale.get(), targetFinalScale, 0.1f, _deltaTime);
	get_owner()->get_presence()->set_scale(finalScale.get());
	if (finalScale.get() <= 0.0f)
	{
		get_owner()->cease_to_exist(true);
	}
}

#ifdef WITH_CRYSTALITE
void ModuleOrbItem::set_crystalite(int _crystalite)
{
	crystalite = _crystalite;
}
#endif

bool ModuleOrbItem::process_taken(ModulePilgrim* _byPilgrim)
{
	if (was_taken())
	{
		return false;
	}

#ifdef WITH_CRYSTALITE
	if (orbItemType == NAME(crystalite))
	{
		_byPilgrim->add_crystalite(crystalite);

		mark_taken();

		return true;
	}
#endif

	todo_important(TXT("implement me?"));
	error(TXT("nothing to take?"));

	return true;
}
//

REGISTER_FOR_FAST_CAST(ModuleOrbItemData);

ModuleOrbItemData::ModuleOrbItemData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleOrbItemData::read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	return base::read_parameter_from(_node, _lc);
}
