#include "mc_healthReporter.h"

#include "health\mc_health.h"

#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

REGISTER_FOR_FAST_CAST(HealthReporter);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new HealthReporter(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & HealthReporter::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("healthReporter")), create_module);
}

HealthReporter::HealthReporter(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

HealthReporter::~HealthReporter()
{
}

void HealthReporter::update()
{
	int frame = ::System::Core::get_frame();
	if (frame == lastFrameUpdate)
	{
		return;
	}
	lastFrameUpdate = frame;

	if (auto* h = get_owner()->get_custom<Health>())
	{
		update_entry(get_owner());
	}

	auto* presence = get_owner()->get_presence();
	{
		Concurrency::ScopedSpinLock lock(presence->access_attached_lock());
		for_every_ptr(attached, presence->get_attached())
		{
			if (attached->get_custom<Health>() ||
				attached->get_custom<HealthReporter>())
			{
				update_entry(attached);
			}
		}
	}

	Energy tempNominalHealth = Energy(0);
	Energy tempActualHealth = Energy(0);
	for (int i = 0; i < entries.get_size(); ++ i)
	{
		auto& e = entries[i];
		if (e.attached.is_set() && e.attached->get_presence()->get_attached_to() != get_owner())
		{
			entries.remove_at(i);
			--i;
			continue;
		}
		tempNominalHealth += e.nominalHealth;
		if (e.attached.is_set())
		{
			tempActualHealth += e.actualHealth;
		}
	}

	nominalHealth = tempNominalHealth;
	actualHealth = tempActualHealth;
}

void HealthReporter::update_entry(Framework::IModulesOwner* _imo)
{
	Entry* entry = nullptr;
	for_every(e, entries)
	{
		if (e->attached == _imo)
		{
			entry = e;
			break;
		}
	}
	if (!entry)
	{
		Entry e;
		e.attached = _imo;
		entries.push_back(e);
		entry = &entries.get_last();
	}

	Energy nominalHealth = Energy(0);
	Energy actualHealth = Energy(0);
	if (_imo != get_owner())
	{
		if (auto * hr = _imo->get_custom<HealthReporter>())
		{
			hr->update();
			nominalHealth += hr->get_nominal_health();
			actualHealth += hr->get_actual_health();
		}
	}
	if (auto * h = _imo->get_custom<Health>())
	{
		nominalHealth += h->get_max_health();
		actualHealth += h->get_health();
	}

	entry->nominalHealth = nominalHealth;
	entry->actualHealth = actualHealth;
}
