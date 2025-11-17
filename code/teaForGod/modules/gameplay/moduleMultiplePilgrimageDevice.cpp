#include "moduleMultiplePilgrimageDevice.h"

#include "..\..\game\game.h"

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\object\scenery.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(ModuleMultiplePilgrimageDevice);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMultiplePilgrimageDevice(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleMultiplePilgrimageDeviceData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleMultiplePilgrimageDevice::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("multiple pilgrimage device")), create_module, create_module_data);
}

ModuleMultiplePilgrimageDevice::ModuleMultiplePilgrimageDevice(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

ModuleMultiplePilgrimageDevice::~ModuleMultiplePilgrimageDevice()
{
	for_every(device, devices)
	{
		if (auto* imo = device->object.get())
		{
			imo->cease_to_exist(true);
		}
		device->object.clear();
	}
}

void ModuleMultiplePilgrimageDevice::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);

	multiplePilgrimageDeviceData = fast_cast<ModuleMultiplePilgrimageDeviceData>(_moduleData);
}

void ModuleMultiplePilgrimageDevice::initialise()
{
	base::initialise();
}

void ModuleMultiplePilgrimageDevice::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);
}

bool ModuleMultiplePilgrimageDevice::is_one_of_pilgrimage_devices(Framework::IModulesOwner* _imo) const
{
	for_every(device, devices)
	{
		if (auto* imo = device->object.get())
		{
			if (imo == _imo)
			{
				return true;
			}
		}
	}

	return false;
}

void ModuleMultiplePilgrimageDevice::spawn_pilgrimage_devices()
{
	ASSERT_ASYNC;

	if (multiplePilgrimageDeviceData)
	{
		auto* o = get_owner();
		auto rg = o->get_individual_random_generator();
		rg.advance_seed(5087087, 925);
		for_every_ref(d, multiplePilgrimageDeviceData->devices)
		{
			if (auto* st = d->sceneryType.get())
			{
				st->load_on_demand_if_required();

				Framework::Object* spawned = nullptr;

				// spawn device
				{
					Framework::Room* inRoom = o->get_presence()->get_in_room();
					an_assert(inRoom);
					Framework::Region* inRegion = inRoom->get_in_region();
					an_assert(inRegion);
					Game::get()->perform_sync_world_job(TXT("spawn via spawn manager"),
						[st, o, &spawned]()
						{
							spawned = st->create(String::printf(TXT("sub device %S"),
								st->create_instance_name().to_char()));
							spawned->init(o->get_in_sub_world());
						});

					{
						inRegion->collect_variables(spawned->access_variables());
						spawned->access_variables().set_from(o->get_variables());
						spawned->access_individual_random_generator() = rg.spawn();

						spawned->initialise_modules();
					}

					{
						Transform placement = o->get_presence()->get_placement();
						inRoom->for_every_point_of_interest(d->poi, [&placement](Framework::PointOfInterestInstance* _poi)
						{
							placement = _poi->calculate_placement();
						});

						spawned->get_presence()->place_in_room(inRoom, placement);
					}
				}

				// store it
				{
					SpawnedDevice sd;
					sd.object = spawned;
					sd.deviceData = d;
					devices.push_back(sd);
				}
			}
		}
	}
}

void ModuleMultiplePilgrimageDevice::store_pilgrim_device_state_variables(SimpleVariableStorage& _stateVariablesStorage)
{
	for_every(d, devices)
	{
		if (auto* object = d->object.get())
		{
			if (auto* dd = d->deviceData.get())
			{
				MODULE_OWNER_LOCK_FOR_IMO(object, TXT("ModuleMultiplePilgrimageDevice::store_pilgrim_device_state_variables"));
				for_every(sv, dd->stateVariables)
				{
					if (auto* raw = object->get_variables().get_raw(sv->inDevice, sv->typeId))
					{
						RegisteredType::copy(sv->typeId, _stateVariablesStorage.access_raw(sv->inStateVariables, sv->typeId), raw);
					}
				}
			}
			if (auto* mpd = object->get_custom<CustomModules::PilgrimageDevice>())
			{
				mpd->store_pilgrim_device_state_variables(_stateVariablesStorage);
			}
		}
	}
}

void ModuleMultiplePilgrimageDevice::restore_pilgrim_device_state_variables(SimpleVariableStorage const & _stateVariablesStorage)
{
	for_every(d, devices)
	{
		if (auto* object = d->object.get())
		{
			if (auto* dd = d->deviceData.get())
			{
				MODULE_OWNER_LOCK_FOR_IMO(object, TXT("ModuleMultiplePilgrimageDevice::restore_pilgrim_device_state_variables"));
				for_every(sv, dd->stateVariables)
				{
					if (auto* raw = _stateVariablesStorage.get_raw(sv->inStateVariables, sv->typeId))
					{
						RegisteredType::copy(sv->typeId, object->access_variables().access_raw(sv->inDevice, sv->typeId), raw);
					}
				}
			}
			if (auto* mpd = object->get_custom<CustomModules::PilgrimageDevice>())
			{
				mpd->restore_pilgrim_device_state_variables(_stateVariablesStorage);
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(ModuleMultiplePilgrimageDeviceData);

ModuleMultiplePilgrimageDeviceData::ModuleMultiplePilgrimageDeviceData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

ModuleMultiplePilgrimageDeviceData::~ModuleMultiplePilgrimageDeviceData()
{
}

bool ModuleMultiplePilgrimageDeviceData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every_ref(d, devices)
	{
		result &= d->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

void ModuleMultiplePilgrimageDeviceData::prepare_to_unload()
{
	base::prepare_to_unload();

	devices.clear();
}

bool ModuleMultiplePilgrimageDeviceData::read_parameter_from(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	if (_node->get_name() == TXT("device"))
	{
		RefCountObjectPtr<MultiplePilgrimageDevice> d;
		d = new MultiplePilgrimageDevice();
		if (d->load_from_xml(_node, _lc))
		{
			devices.push_back(d);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

//

bool MultiplePilgrimageDevice::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	sceneryType.load_from_xml(_node, TXT("sceneryType"), _lc);
	poi = _node->get_name_attribute(TXT("poi"), poi);

	for_every(cnode, _node->children_named(TXT("stateVariables")))
	{
		for_every(node, cnode->all_children())
		{
			PilgrimageDeviceStateVariable pdsv;
			if (pdsv.load_from_xml(node, _lc))
			{
				stateVariables.push_back(pdsv);
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

bool MultiplePilgrimageDevice::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= sceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}
