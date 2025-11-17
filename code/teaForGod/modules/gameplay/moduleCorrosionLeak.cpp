#include "moduleCorrosionLeak.h"

#include "..\custom/health/mc_health.h"

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

using namespace TeaForGodEmperor;

//

// module params
DEFINE_STATIC_NAME(leakVariable);
DEFINE_STATIC_NAME(leakInstigatorVariable);

// variables
DEFINE_STATIC_NAME(corrosionLeaks);
DEFINE_STATIC_NAME(corrosionLeakInstigator);

// temporary object
DEFINE_STATIC_NAME(leak);

//

REGISTER_FOR_FAST_CAST(ModuleCorrosionLeak);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleCorrosionLeak(_owner);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleCorrosionLeak::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("corrosionLeak")), create_module);
}

ModuleCorrosionLeak::ModuleCorrosionLeak(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

ModuleCorrosionLeak::~ModuleCorrosionLeak()
{
}

void ModuleCorrosionLeak::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	
	leakVariable = _moduleData->get_parameter<Name>(this, NAME(leakVariable), leakVariable);
	leakInstigatorVariable = _moduleData->get_parameter<Name>(this, NAME(leakInstigatorVariable), leakInstigatorVariable);

	if (!leakVariable.is_valid())
	{
		leakVariable = NAME(corrosionLeaks);
	}
	if (!leakInstigatorVariable.is_valid())
	{
		leakInstigatorVariable = NAME(corrosionLeakInstigator);
	}
}

void ModuleCorrosionLeak::reset()
{
	base::reset();

	isLeaking = false;
}

void ModuleCorrosionLeak::initialise()
{
	base::initialise();
}

void ModuleCorrosionLeak::leak_now(DamageInfo const& _damageInfo)
{
	MODULE_OWNER_LOCK(TXT("ModuleCorrosionLeak::leak_now"));

	if (isLeaking)
	{
		return;
	}

	isLeaking = true;

	if (auto* t = get_owner()->get_temporary_objects())
	{
		t->spawn(NAME(leak));
	}

	if (auto* p = get_owner()->get_presence())
	{
		if (p->is_attached())
		{
			auto* imo = p->get_attached_to();
			while (imo)
			{
				if (auto* h = imo->get_custom<CustomModules::Health>())
				{
					break;
				}
				imo = imo->get_presence()->get_attached_to();
			}

			if (imo)
			{
				MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("ModuleCorrosionLeak::leak_now  leaking"));
				imo->access_variables().access<int>(leakVariable)++;
				if (_damageInfo.instigator.is_set())
				{
					imo->access_variables().access<SafePtr<Framework::IModulesOwner>>(leakInstigatorVariable) = _damageInfo.instigator;
				}
			}
		}
	}
}

bool ModuleCorrosionLeak::on_death(Damage const& _damage, DamageInfo const & _damageInfo)
{
	leak_now(_damageInfo);
	return false; // don't continue with death
}

