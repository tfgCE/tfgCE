#include "mc_pilgrimageDevice.h"

#include "..\..\game\game.h"

#include "..\..\pilgrimage\pilgrimageDeviceStateSensitive.h"

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\object\scenery.h"

#ifdef AN_CLANG
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

REGISTER_FOR_FAST_CAST(CustomModules::PilgrimageDevice);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new CustomModules::PilgrimageDevice(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new PilgrimageDeviceData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & CustomModules::PilgrimageDevice::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("pilgrimage device")), create_module, create_module_data);
}

CustomModules::PilgrimageDevice::PilgrimageDevice(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

CustomModules::PilgrimageDevice::~PilgrimageDevice()
{
}

void CustomModules::PilgrimageDevice::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);

	pilgrimageDeviceData = fast_cast<PilgrimageDeviceData>(_moduleData);
}

void CustomModules::PilgrimageDevice::initialise()
{
	base::initialise();
}

void CustomModules::PilgrimageDevice::store_pilgrim_device_state_variables(SimpleVariableStorage& _stateVariablesStorage)
{
	find_sub_devices();

	if (pilgrimageDeviceData)
	{
		MODULE_OWNER_LOCK_FOR_IMO(get_owner(), TXT("PilgrimageDevice::store_pilgrim_device_state_variables"));
		LogInfoContext log;
		log.log(TXT("store pilgrim device state variables [%S]"), get_owner()->ai_get_name().to_char());
		{
			// sub devices
			for_every(sd, subDevices)
			{
				if (auto* sdIMO = sd->device.get())
				{
					{
						MODULE_OWNER_LOCK_FOR_IMO(sdIMO, TXT("PilgrimageDevice::store_pilgrim_device_state_variables"));
						for_every(sv, sd->stateVariables)
						{
							if (auto* raw = sdIMO->get_variables().get_raw(sv->inDevice, sv->typeId))
							{
								RegisteredType::copy(sv->typeId, _stateVariablesStorage.access_raw(sv->inStateVariables, sv->typeId), raw);
								log.log(TXT("for subDevice \"%S\" store \"%S\" as \"%S\""), sd->varId.to_char(), sv->inDevice.to_char(), sv->inStateVariables.to_char());
								RegisteredType::log_value(log, sv->typeId, raw, sv->inDevice.to_char());
							}
						}
					}
					if (auto* mpd = sdIMO->get_custom<PilgrimageDevice>())
					{
						mpd->store_pilgrim_device_state_variables(_stateVariablesStorage);
					}
				}
			}
		}
		for_every(sv, pilgrimageDeviceData->stateVariables)
		{
			if (auto* raw = get_owner()->get_variables().get_raw(sv->inDevice, sv->typeId))
			{
				RegisteredType::copy(sv->typeId, _stateVariablesStorage.access_raw(sv->inStateVariables, sv->typeId), raw);
				log.log(TXT("store \"%S\" as \"%S\""), sv->inDevice.to_char(), sv->inStateVariables.to_char());
				RegisteredType::log_value(log, sv->typeId, raw, sv->inDevice.to_char());
			}
		}
		log.output_to_output();
	}
}

void CustomModules::PilgrimageDevice::restore_pilgrim_device_state_variables(SimpleVariableStorage const & _stateVariablesStorage)
{
	find_sub_devices();

	if (pilgrimageDeviceData)
	{
		MODULE_OWNER_LOCK_FOR_IMO(get_owner(), TXT("PilgrimageDevice::restore_pilgrim_device_state_variables"));
		LogInfoContext log;
		log.log(TXT("restore pilgrim device state variables [%S]"), get_owner()->ai_get_name().to_char());
		{
			// sub devices
			for_every(sd, subDevices)
			{
				if (auto* sdIMO = sd->device.get())
				{
					{
						MODULE_OWNER_LOCK_FOR_IMO(sdIMO, TXT("PilgrimageDevice::restore_pilgrim_device_state_variables"));
						for_every(sv, sd->stateVariables)
						{
							if (auto* raw = _stateVariablesStorage.get_raw(sv->inStateVariables, sv->typeId))
							{
								RegisteredType::copy(sv->typeId, sdIMO->access_variables().access_raw(sv->inDevice, sv->typeId), raw);
								log.log(TXT("for subDevice \"%S\" restore \"%S\" as \"%S\""), sd->varId.to_char(), sv->inDevice.to_char(), sv->inStateVariables.to_char());
								RegisteredType::log_value(log, sv->typeId, raw, sv->inDevice.to_char());
							}
						}
					}
					if (auto* mpd = sdIMO->get_custom<PilgrimageDevice>())
					{
						mpd->restore_pilgrim_device_state_variables(_stateVariablesStorage);
					}
				}
			}
		}
		for_every(sv, pilgrimageDeviceData->stateVariables)
		{
			if (auto* raw = _stateVariablesStorage.get_raw(sv->inStateVariables, sv->typeId))
			{
				RegisteredType::copy(sv->typeId, get_owner()->access_variables().access_raw(sv->inDevice, sv->typeId), raw);
				log.log(TXT("restore \"%S\" from \"%S\""), sv->inDevice.to_char(), sv->inStateVariables.to_char());
				RegisteredType::log_value(log, sv->typeId, raw, sv->inDevice.to_char());
			}
		}
		log.output_to_output();

		// inform pilgrimage-device-state sensitive devices
		{
			// sub devices
			for_every(sd, subDevices)
			{
				if (auto* sdIMO = sd->device.get())
				{
					{
						for_every_ptr(m, sdIMO->get_customs())
						{
							if (auto* pdss = fast_cast<IPilgrimageDeviceStateSensitive>(m))
							{
								pdss->on_restore_pilgrimage_device_state();
							}
						}
					}
				}
				for_every_ptr(m, get_owner()->get_customs())
				{
					if (auto* pdss = fast_cast<IPilgrimageDeviceStateSensitive>(m))
					{
						pdss->on_restore_pilgrimage_device_state();
					}
				}
			}
		}
	}
}

void CustomModules::PilgrimageDevice::find_sub_devices()
{
	ASSERT_ASYNC;
	if (pilgrimageDeviceData)
	{
		subDevices.set_size(pilgrimageDeviceData->subDevices.get_size());
		for_every(sd, subDevices)
		{
			if (!sd->varId.is_valid())
			{
				// initialise
				auto& srcSD = pilgrimageDeviceData->subDevices[for_everys_index(sd)];
				sd->varId = srcSD.varId;
				sd->objectVarId = srcSD.objectVarId;
				sd->stateVariables = srcSD.stateVariables;
			}
			if (!sd->device.get() && sd->varId.is_valid() && sd->objectVarId.is_valid())
			{
				if (auto* imo = get_owner())
				{
					Game::get()->perform_sync_world_job(TXT("find sub device"), [imo, sd]()
					{
						if (auto* id = imo->get_variables().get_existing<int>(sd->varId))
						{
							imo->get_in_world()->for_every_object_with_id(sd->objectVarId, *id,
								[sd, imo](Framework::Object* _object)
								{
									if (_object != imo)
									{
										sd->device = _object;
										return true;
									}
									return false; // keep going on
								});
						}
					});
				}
			}
		}
	}
}

void CustomModules::PilgrimageDevice::set_guide_towards(Framework::IModulesOwner* _guideTowards)
{
	if (guideTowards != _guideTowards)
	{
		guideTowards = _guideTowards;
		SafePtr<Framework::IModulesOwner> safeOwner(get_owner());
		SafePtr<Framework::IModulesOwner> safeGuideTowards(get_guide_towards());
		Game::get()->add_async_world_job_top_priority(TXT("update tag cells"), [safeOwner, safeGuideTowards]()
			{
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->force_update_pilgrims_destination();

					Framework::Room* room = nullptr;
					if (auto* imo = safeGuideTowards.get())
					{
						room = imo->get_presence()->get_in_room();
					}
					else if (auto* imo = safeOwner.get())
					{
						room = imo->get_presence()->get_in_room();
					}

					if (room)
					{
						piow->tag_open_world_directions_for_cell(room, true);
					}
				}
			});
	}
}

//

REGISTER_FOR_FAST_CAST(PilgrimageDeviceData);

PilgrimageDeviceData::PilgrimageDeviceData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

PilgrimageDeviceData::~PilgrimageDeviceData()
{
}

bool PilgrimageDeviceData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

void PilgrimageDeviceData::prepare_to_unload()
{
	base::prepare_to_unload();
}

bool PilgrimageDeviceData::read_parameter_from(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	if (_node->get_name() == TXT("stateVariables"))
	{
		bool result = true;
		for_every(node, _node->all_children())
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
		return result;
	}
	else if (_node->get_name() == TXT("subDevice"))
	{
		bool result = true;
		SubDevice sd;
		sd.varId = _node->get_name_attribute(TXT("varId"));
		sd.objectVarId = _node->get_name_attribute(TXT("objectVarId"), sd.varId); /* default from varId */
		for_every(stNode, _node->children_named(TXT("stateVariables")))
		{
			for_every(node, stNode->all_children())
			{
				PilgrimageDeviceStateVariable pdsv;
				if (pdsv.load_from_xml(node, _lc))
				{
					sd.stateVariables.push_back(pdsv);
				}
				else
				{
					result = false;
				}
			}
		}
		if (result)
		{
			subDevices.push_back(sd);
		}
		return result;
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

//

bool PilgrimageDeviceStateVariable::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;
	typeId = RegisteredType::get_type_id_by_name(_node->get_name().to_char());
	error_loading_xml_on_assert(typeId != RegisteredType::none(), result, _node, TXT("variable type not recognised"));
	inStateVariables = _node->get_name_attribute(TXT("name"), inStateVariables);
	inDevice = _node->get_name_attribute(TXT("name"), inDevice);
	inStateVariables = _node->get_name_attribute(TXT("nameInStateVariables"), inStateVariables);
	inDevice = _node->get_name_attribute(TXT("nameInDevice"), inDevice);
	error_loading_xml_on_assert(inStateVariables.is_valid(), result, _node, TXT("no \"name\" nor \"nameInStateVariables\" provided"));
	error_loading_xml_on_assert(inDevice.is_valid(), result, _node, TXT("no \"name\" nor \"nameInDevice\" provided"));
	return result;
}
