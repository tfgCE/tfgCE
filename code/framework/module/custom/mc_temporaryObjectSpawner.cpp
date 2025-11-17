#include "mc_temporaryObjectSpawner.h"

#include "mc_lightSources.h"

#include "..\modules.h"
#include "..\moduleDataImpl.inl"

#include "..\..\appearance\controllers\ac_particlesUtils.h"
#include "..\..\collision\checkCollisionContext.h"
#include "..\..\collision\checkSegmentResult.h"
#include "..\..\game\gameUtils.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\object\temporaryObject.h"
#include "..\..\sound\soundSample.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace CustomModules;

//

REGISTER_FOR_FAST_CAST(CustomModules::TemporaryObjectSpawner);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new CustomModules::TemporaryObjectSpawner(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new TemporaryObjectSpawnerData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & CustomModules::TemporaryObjectSpawner::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("temporaryObjectSpawner")), create_module, create_module_data);
}

CustomModules::TemporaryObjectSpawner::TemporaryObjectSpawner(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

CustomModules::TemporaryObjectSpawner::~TemporaryObjectSpawner()
{
}

void CustomModules::TemporaryObjectSpawner::reset()
{
	base::reset();

	for_every(instance, instances)
	{
		instance->intervalLeft = 0.0f;
		instance->durationLeft = 0.0f;
	}

	mark_requires(all_customs__advance_post);
}

void CustomModules::TemporaryObjectSpawner::initialise()
{
	base::initialise();

	mark_requires(all_customs__advance_post);
}

void CustomModules::TemporaryObjectSpawner::activate()
{
	base::activate();
}

void CustomModules::TemporaryObjectSpawner::on_owner_destroy()
{
	for_every(instance, instances)
	{
		for_every(o, instance->spawned)
		{
			if (auto* to = fast_cast<TemporaryObject>(o->get()))
			{
				ParticlesUtils::desire_to_deactivate(to);
			}
		}
		instance->spawned.clear();
	}
}

void CustomModules::TemporaryObjectSpawner::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	temporaryObjectSpawnerData = fast_cast<TemporaryObjectSpawnerData>(_moduleData);
	if (temporaryObjectSpawnerData)
	{
		Random::Generator rg = Random::Generator();
		instances.clear();
		for_every(e, temporaryObjectSpawnerData->elements)
		{
			Instance i;
			i.interval = e->interval;
			i.duration = e->duration;
			i.intervalLeft = max(0.01f, rg.get(i.interval) * rg.get_float(0.0f, 1.0f));
			i.temporaryObject = e->temporaryObject;
			i.socket = e->socket;
			instances.push_back(i);
		}
	}
}

void CustomModules::TemporaryObjectSpawner::advance_post(float _deltaTime)
{
	base::advance_post(_deltaTime);

	rarer_post_advance_if_not_visible();

	for_every(i, instances)
	{
		if (i->intervalLeft > 0.0f)
		{
			i->intervalLeft = max(0.0f, i->intervalLeft - _deltaTime);

			if (i->intervalLeft == 0.0f)
			{
				i->spawned.clear();
				if (i->duration.is_empty() || i->duration.is_zero())
				{
					i->durationLeft = 0.0f;
					i->intervalLeft = max(0.01f, Random::get(i->interval));

					if (auto* to = get_owner()->get_temporary_objects())
					{
						if (i->socket.is_valid())
						{
							to->spawn(i->temporaryObject, Framework::ModuleTemporaryObjects::SpawnParams().at_socket(i->socket));
						}
						else
						{
							to->spawn_all(i->temporaryObject);
						}
					}
				}
				else
				{
					i->durationLeft = max(0.01f, Random::get(i->duration));
					i->spawned.clear();

					if (auto* to = get_owner()->get_temporary_objects())
					{
						if (i->socket.is_valid())
						{
							if (auto* toi = to->spawn(i->temporaryObject, Framework::ModuleTemporaryObjects::SpawnParams().at_socket(i->socket)))
							{
								i->spawned.push_back(SafePtr<IModulesOwner>(toi));
							}
						}
						else
						{
							to->spawn_all(i->temporaryObject, NP, &i->spawned);
						}
					}
				}
			}
		}
		else if (i->durationLeft > 0.0f)
		{
			i->durationLeft = max(0.0f, i->durationLeft - _deltaTime);

			if (i->durationLeft == 0.0f)
			{
				i->durationLeft = 0.0f;
				i->intervalLeft = max(0.01f, Random::get(i->interval));
				for_every(o, i->spawned)
				{
					if (auto* to = fast_cast<TemporaryObject>(o->get()))
					{
						ParticlesUtils::desire_to_deactivate(to);
					}
				}
				i->spawned.clear();
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(CustomModules::TemporaryObjectSpawnerData);

TemporaryObjectSpawnerData::TemporaryObjectSpawnerData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool TemporaryObjectSpawnerData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	return base::read_parameter_from(_attr, _lc);
}

bool TemporaryObjectSpawnerData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("spawn"))
	{
		Element e;
		e.interval.load_from_xml(_node, TXT("interval"));
		e.duration.load_from_xml(_node, TXT("duration"));
		e.temporaryObject = _node->get_name_attribute_or_from_child(TXT("temporaryObject"), e.temporaryObject);
		e.socket = _node->get_name_attribute_or_from_child(TXT("socket"), e.socket);
		elements.push_back(e);
		return true;
	}
	return base::read_parameter_from(_node, _lc);
}

bool TemporaryObjectSpawnerData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool TemporaryObjectSpawnerData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
