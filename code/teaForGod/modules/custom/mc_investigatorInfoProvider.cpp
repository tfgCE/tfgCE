#include "mc_investigatorInfoProvider.h"

#include "mc_energyStorage.h"

#include "health\mc_health.h"

#include "..\..\game\persistence.h"

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\module\moduleTemporaryObjectsData.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\object\objectType.h"
#include "..\..\..\framework\text\stringFormatter.h"

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(noSocialInfo);

// custom modules
DEFINE_STATIC_NAME(health);
DEFINE_STATIC_NAME(energyStorage);

//

Framework::ModuleCustom* get_custom_module_for(Framework::IModulesOwner* imo, Name const& _moduleType)
{
	if (_moduleType == NAME(health))
	{
		return imo->get_custom<CustomModules::Health>();
	}
	if (_moduleType == NAME(energyStorage))
	{
		return imo->get_custom<CustomModules::EnergyStorage>();
	}
	error(TXT("not implemented"));
	return nullptr;
}

//

void InvestigatorInfoCachedData::add(SimpleVariableInfo const& svi)
{
	if (svi.type_id() == type_id<float>())
	{
		add<float>(svi.get<float>());
	}
	else if (svi.type_id() == type_id<int>())
	{
		add<int>(svi.get<int>());
	}
	else if (svi.type_id() == type_id<Energy>())
	{
		add<Energy>(svi.get<Energy>());
	}
	else if (svi.type_id() == type_id<EnergyCoef>())
	{
		add<EnergyCoef>(svi.get<EnergyCoef>());
	}
	else
	{
		error(TXT("type not supported!"));
	}
}

void InvestigatorInfoCachedData::add(Framework::Module * _module, Name const& _variable, TypeId _typeId)
{
	auto* moduleData = _module->get_module_data();
	if (moduleData)
	{
		if (_typeId == type_id<float>())
		{
			add<float>(moduleData->get_parameter<float>(_module, _variable, 0));
		}
		else if (_typeId == type_id<int>())
		{
			add<int>(moduleData->get_parameter<int>(_module, _variable, 0));
		}
		else if (_typeId == type_id<Energy>())
		{
			add<Energy>(Energy::get_from_module_data(_module, moduleData, _variable, Energy::zero()));
		}
		else if (_typeId == type_id<EnergyCoef>())
		{
			add<EnergyCoef>(EnergyCoef::get_from_module_data(_module, moduleData, _variable, EnergyCoef::zero()));
		}
		else
		{
			error(TXT("type not supported!"));
		}
	}
	else
	{
		error(TXT("no module data!"));
	}
}

//

bool InvestigatorInfo::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	XML_LOAD_NAME_ATTR(_node, id);

	if (!id.is_valid())
	{
		error_loading_xml(_node, TXT("no \"id\" provided"));
		return false;
	}
	
	XML_LOAD(_node, healthRuleActive);
	XML_LOAD(_node, healthRuleInactive);
	XML_LOAD(_node, extraEffectActive);
	XML_LOAD(_node, extraEffectInactive);
	XML_LOAD(_node, variableTrue);
	XML_LOAD(_node, variableFalse);

	for_every(n, _node->children_named(TXT("icon")))
	{
		XML_LOAD_NAME_ATTR_STR(n, iconAtSocket, TXT("atSocket"));
	}
	icon.load_from_xml_child(_node, TXT("icon"), _lc, String::printf(TXT("%S; icon"), id.to_char()).to_char());
	info.load_from_xml_child(_node, TXT("info"), _lc, String::printf(TXT("%S; info"), id.to_char()).to_char());

	for_every(n, _node->children_named(TXT("value")))
	{
		ValueForInfo value;
		value.provide = n->get_name_attribute(TXT("provide"));
		if (!value.provide.is_valid())
		{
			error_loading_xml(n, TXT("no \"provide\" provided"));
			result = false;
			continue;
		}
		value.fromTemporaryObject.load_from_xml(n, TXT("fromTemporaryObject"));
		value.fromCustomModule.load_from_xml(n, TXT("fromCustomModule"));
		value.fromVariable.load_from_xml(n, TXT("fromVariable"));
		if (value.fromVariable.is_set())
		{
			String readType = n->get_string_attribute(TXT("variableType"));
			error_loading_xml_on_assert(! readType.is_empty(), result, n, TXT("missing \"variableType\""));
			TypeId parsed = RegisteredType::parse_type(readType);
			if (parsed == type_id_none())
			{
				error_loading_xml(n, TXT("invalid or missing \"variableType\""));
				result = false;
			}
			value.fromVariableType = parsed;
		}
		valuesForInfo.push_back(value);
	}

	return result;
}

struct Instantiate
{
	template <typename Class>
	static void variable(Framework::IModulesOwner* imo, Optional<SimpleVariableInfo>& _svi, Optional<Name> const& _variable)
	{
		if (_variable.is_set())
		{
			_svi.set(SimpleVariableInfo());
			_svi.access().set_name(_variable.get());
			_svi.access().look_up<Class>(imo->access_variables());
		}
	}
	static void variable(Framework::IModulesOwner* imo, Optional<SimpleVariableInfo>& _svi, Optional<Name> const& _variable, TypeId const& _typeId)
	{
		if (_variable.is_set())
		{
			_svi.set(SimpleVariableInfo());
			_svi.access().set_name(_variable.get());
			_svi.access().look_up(imo->access_variables(), _typeId);
		}
	}
};

InvestigatorInfo::Instance InvestigatorInfo::create_instance_for(Framework::IModulesOwner* imo) const
{
	InvestigatorInfo::Instance i;
	i.healthRuleActive = healthRuleActive;
	i.healthRuleInactive = healthRuleInactive;
	i.extraEffectActive = extraEffectActive;
	i.extraEffectInactive = extraEffectInactive;

	if (iconAtSocket.is_valid())
	{
		i.iconAtSocket.set_name(iconAtSocket);
		if (auto* a = imo->get_appearance())
		{
			i.iconAtSocket.look_up(a->get_mesh());
		}
	}
	Instantiate::variable<bool>(imo, i.variableFalse, variableFalse);
	Instantiate::variable<bool>(imo, i.variableTrue, variableTrue);

	i.icon = icon;
	i.info = info;

	for_every(v, valuesForInfo)
	{
		Instance::ValueForInfo dv;
		dv.provide = v->provide;
		dv.fromTemporaryObject = v->fromTemporaryObject;
		dv.fromCustomModule = v->fromCustomModule;
		if (dv.fromTemporaryObject.is_set())
		{
			dv.fromTemporaryObjectVariable = v->fromVariable;
		}
		else if (dv.fromCustomModule.is_set())
		{
			dv.fromCustomModuleVariable = v->fromVariable;
		}
		else
		{
			Instantiate::variable(imo, dv.fromVariable, v->fromVariable, v->fromVariableType);
		}
		if (dv.fromVariable.is_set() ||
			dv.fromTemporaryObjectVariable.is_set() ||
			dv.fromCustomModuleVariable.is_set())
		{
			dv.fromVariableType = v->fromVariableType;
		}
		i.valuesForInfo.push_back(dv);
	}

	return i;
}

//

REGISTER_FOR_FAST_CAST(InvestigatorInfoProvider);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new InvestigatorInfoProvider(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new InvestigatorInfoProviderData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & InvestigatorInfoProvider::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("investigator info provider")), create_module, create_module_data);
}

InvestigatorInfoProvider::InvestigatorInfoProvider(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

InvestigatorInfoProvider::~InvestigatorInfoProvider()
{
}

void InvestigatorInfoProvider::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	infos.clear();
	investigatorInfoProviderData = fast_cast<InvestigatorInfoProviderData>(_moduleData);
	if (investigatorInfoProviderData)
	{
		for_every(i, investigatorInfoProviderData->infos)
		{
			infos.push_back(i->create_instance_for(get_owner()));
		}
	}
	noSocialInfo = _moduleData->get_parameter<bool>(this, NAME(noSocialInfo));
}

int InvestigatorInfoProvider::get_info_count() const
{
	return infos.get_size();
}

bool InvestigatorInfoProvider::does_require_update(int _infoIdx, InvestigatorInfoCachedData& _cachedData) const
{
	if (UNLIKELY(!infos.is_index_valid(_infoIdx)))
	{
		return false;
	}

	auto& i = infos[_infoIdx];

	InvestigatorInfoCachedData cachedData;
	{
		// collect cached data
		{
			bool active = is_active(_infoIdx);
			cachedData.add(active);
		}
		// collect values to present
		for_every(vfi, i.valuesForInfo)
		{
			if (vfi->fromTemporaryObject.is_set() &&
				vfi->fromTemporaryObjectVariable.is_set())
			{
				if (auto* to = get_owner()->get_temporary_objects())
				{
					if (auto* mtos = to->get_info_for(vfi->fromTemporaryObject.get()))
					{
						if (auto* svi = mtos->variables.find_existing(vfi->fromTemporaryObjectVariable.get(), vfi->fromVariableType))
						{
							cachedData.add(*svi);
						}
						else
						{
							error(TXT("no variable for temporary object"));
						}
					}
					else
					{
						error(TXT("no temporary object called \"%S\""), vfi->fromTemporaryObject.get().to_char());
					}
				}
			}
			if (vfi->fromCustomModule.is_set() &&
				vfi->fromCustomModuleVariable.is_set())
			{
				cachedData.add<tchar>('T'); // we don't need an exact value as it should not change, just tchar to indicate there is something we should update
				/* in case we need, here's the code
				if (auto* cm = get_custom_module_for(get_owner(), vfi->fromCustomModule.get()))
				{
					cachedData.add(cm, vfi->fromCustomModuleVariable.get(), vfi->fromVariableType);
				}
				*/
			}
			{
				if (vfi->fromVariable.is_set())
				{
					cachedData.add(vfi->fromVariable.get());
				}
			}
		}
	}

	bool result = (cachedData != _cachedData);
	if (result)
	{
		_cachedData = cachedData;
	}

	return result;
}

bool InvestigatorInfoProvider::is_active(int _infoIdx) const
{
	if (infos.is_index_valid(_infoIdx))
	{
		auto& i = infos[_infoIdx];

		if (i.healthRuleActive.is_set() ||
			i.healthRuleInactive.is_set() ||
			i.extraEffectActive.is_set() ||
			i.extraEffectInactive.is_set())
		{
			if (auto* h = get_owner()->get_custom<CustomModules::Health>())
			{
				if (i.healthRuleActive.is_set())
				{
					bool r = h->is_rule_health_active(i.healthRuleActive.get());
					if (!r) return false;
				}
				if (i.healthRuleInactive.is_set())
				{
					bool r = !h->is_rule_health_active(i.healthRuleInactive.get());
					if (!r) return false;
				}
				if (i.extraEffectActive.is_set())
				{
					bool r = h->has_extra_effect(i.extraEffectActive.get());
					if (!r) return false;
				}
				if (i.extraEffectInactive.is_set())
				{
					bool r = !h->has_extra_effect(i.extraEffectInactive.get());
					if (!r) return false;
				}
			}
		}
		if (i.variableTrue.is_set())
		{
			bool r = i.variableTrue.get().get<bool>();
			if (!r) return false;
		}
		if (i.variableFalse.is_set())
		{
			bool r = !i.variableFalse.get().get<bool>();
			if (!r) return false;
		}

		return true;
	}
	return false;
}

bool InvestigatorInfoProvider::has_icon(int _infoIdx) const
{
	if (infos.is_index_valid(_infoIdx))
	{
		return infos[_infoIdx].icon.is_valid();
	}
	return false;
}

Optional<Framework::SocketID> InvestigatorInfoProvider::get_icon_at_socket(int _infoIdx) const
{
	if (infos.is_index_valid(_infoIdx))
	{
		if (infos[_infoIdx].iconAtSocket.is_valid())
		{
			return infos[_infoIdx].iconAtSocket;
		}
	}
	return NP;
}

Optional<tchar> InvestigatorInfoProvider::get_icon(int _infoIdx) const
{
	if (infos.is_index_valid(_infoIdx) &&
		is_active(_infoIdx))
	{
		auto& s = infos[_infoIdx].icon.get();
		if (!s.is_empty())
		{
			return s[0];
		}
	}
	return NP;
}

String InvestigatorInfoProvider::get_info(int _infoIdx) const
{
	if (infos.is_index_valid(_infoIdx) &&
		is_active(_infoIdx))
	{
		auto& i = infos[_infoIdx];

		if (!i.valuesForInfo.is_empty())
		{
			Framework::StringFormatterParams params;
			struct Add
			{
				Framework::StringFormatterParams& params;
				Add(Framework::StringFormatterParams& _p) : params(_p) {}
				void param(Name const& _paramName, SimpleVariableInfo const& svi)
				{
					if (svi.type_id() == type_id<float>())
					{
						params.add(_paramName, svi.get<float>());
					}
					else if (svi.type_id() == type_id<int>())
					{
						params.add(_paramName, svi.get<int>());
					}
					else if (svi.type_id() == type_id<Energy>())
					{
						params.add(_paramName, svi.get<Energy>().as_string());
					}
					else if (svi.type_id() == type_id<EnergyCoef>())
					{
						params.add(_paramName, svi.get<EnergyCoef>().as_string_percentage_auto_decimals());
					}
					else
					{
						error(TXT("type not supported!"));
					}
				}
				void param(Name const& _paramName, Framework::Module* _module, Name const& _variable, TypeId _typeId)
				{
					auto* moduleData = _module->get_module_data();
					if (moduleData)
					{
						if (_typeId == type_id<float>())
						{
							params.add(_paramName, moduleData->get_parameter<float>(_module, _variable, 0));
						}
						else if (_typeId == type_id<int>())
						{
							params.add(_paramName, moduleData->get_parameter<int>(_module, _variable, 0));
						}
						else if (_typeId == type_id<Energy>())
						{
							params.add(_paramName, Energy::get_from_module_data(_module, moduleData, _variable, Energy::zero()).as_string());
						}
						else if (_typeId == type_id<EnergyCoef>())
						{
							params.add(_paramName, EnergyCoef::get_from_module_data(_module, moduleData, _variable, EnergyCoef::zero()).as_string_percentage_auto_decimals());
						}
						else
						{
							error(TXT("type not supported!"));
						}
					}
					else
					{
						error(TXT("no module data!"));
					}
				}
			} add(params);

			for_every(vfi, i.valuesForInfo)
			{
				if (vfi->fromTemporaryObject.is_set() &&
					vfi->fromTemporaryObjectVariable.is_set())
				{
					if (auto* to = get_owner()->get_temporary_objects())
					{
						if (auto* mtos = to->get_info_for(vfi->fromTemporaryObject.get()))
						{
							if (auto* svi = mtos->variables.find_existing(vfi->fromTemporaryObjectVariable.get(), vfi->fromVariableType))
							{
								add.param(vfi->provide, *svi);
							}
							else
							{
								error(TXT("no variable for temporary object"));
							}
						}
						else
						{
							error(TXT("no temporary object called \"%S\""), vfi->fromTemporaryObject.get().to_char());
						}
					}
				}
				if (vfi->fromCustomModule.is_set() &&
					vfi->fromCustomModuleVariable.is_set())
				{
					if (auto* cm = get_custom_module_for(get_owner(), vfi->fromCustomModule.get()))
					{
						add.param(vfi->provide, cm, vfi->fromCustomModuleVariable.get(), vfi->fromVariableType);
					}
				}
				if (vfi->fromVariable.is_set())
				{
					add.param(vfi->provide, vfi->fromVariable.get());
				}

			}

			return Framework::StringFormatter::format_sentence_loc_str(i.info, params);
		}
		else
		{
			return i.info.get();
		}
	}
	return String::empty();
}

//

REGISTER_FOR_FAST_CAST(InvestigatorInfoProviderData);

InvestigatorInfoProviderData::InvestigatorInfoProviderData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

InvestigatorInfoProviderData::~InvestigatorInfoProviderData()
{
}

bool InvestigatorInfoProviderData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(n, _node->children_named(TXT("info")))
	{
		InvestigatorInfo info;
		if (info.load_from_xml(n, _lc))
		{
			infos.push_back(info);
		}
		else
		{
			error_loading_xml(n, TXT("problem loading info"));
			result = false;
		}
	}

	return result;
}

bool InvestigatorInfoProviderData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
