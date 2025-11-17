// not auto included! include wherever required

#include "registeredModule.h"

template <typename BaseModuleClass, typename BaseModuleDataClass>
bool Framework::ModuleDef<BaseModuleClass, BaseModuleDataClass>::load_from_xml(Framework::LibraryStored* _inLibraryStored, IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc, Framework::RegisteredModuleSet<BaseModuleClass, BaseModuleDataClass> const & _registeredModuleSet)
{
	bool result = true;
	if (_node)
	{
		type = _node->get_name_attribute(TXT("type"), type);
		if (!type.is_valid())
		{
			// because there is node at least
			type = Names::default_;
		}
	}
	if (type.is_valid())
	{
		RefCountObjectPtr<ModuleData> moduleData(_registeredModuleSet.create_data(type, _inLibraryStored));
		data = fast_cast<BaseModuleDataClass>(moduleData.get());
		if (!data.is_set())
		{
			error_loading_xml(_node, TXT("could not create module data of type \"%S\""), type.to_char());
			result = false;
		}
	}
	if (data.is_set())
	{
		result &= data->load_from_xml(_node, _lc);
	}
	return result;
}

template <typename BaseModuleClass, typename BaseModuleDataClass>
bool Framework::ModuleDef<BaseModuleClass, BaseModuleDataClass>::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	if (data.is_set())
	{
		result &= data->prepare_for_game(_library, _pfgContext);
	}
	return result;
}

template <typename BaseModuleClass, typename BaseModuleDataClass>
void Framework::ModuleDef<BaseModuleClass, BaseModuleDataClass>::prepare_to_unload()
{
	if (data.is_set())
	{
		data->prepare_to_unload();
	}
}

//

template <typename Class>
bool Framework::ModuleData::has_parameter(Module* _module, Name const & _parName) const
{
	auto const & param = parameters.get(_parName);
	if (param.is_set())
	{
		return true;
	}
	if (_module)
	{
		if (Class const * value = _module->get_wmp_variables().get_existing<Class>(_parName))
		{
			return true;
		}
		if (Class const * value = _module->get_owner()->get_variables().get_existing<Class>(_parName))
		{
			return true;
		}
	}
	return false;
}

template <typename Class>
Class Framework::ModuleData::get_parameter(Module* _module, Name const & _parName) const
{
	auto const & param = parameters.get(_parName);
	if (param.is_set())
	{
		if (param.is_param_name() && _module)
		{
			WheresMyPoint::Value value;
			if (_module->restore_value_for_wheres_my_point(param.get_as_param_name(), value))
			{
				return value.get_as<Class>();
			}
			return Class();
		}
		return param.get_as<Class>();
	}
	if (_module)
	{
		if (Class const * value = _module->get_wmp_variables().get_existing<Class>(_parName))
		{
			return *value;
		}
		if (Class const * value = _module->get_owner()->get_variables().get_existing<Class>(_parName))
		{
			return *value;
		}
	}
	return Class();
}

template <typename Class>
Class Framework::ModuleData::get_parameter(Module* _module, Name const & _parName, Class const & _defValue) const
{
	auto const & param = parameters.get(_parName);
	if (_module)
	{
		// more important than local ones
		if (Class const * value = _module->get_wmp_variables().get_existing<Class>(_parName))
		{
			return *value;
		}
		if (Class const * value = _module->get_owner()->get_variables().get_existing<Class>(_parName))
		{
			return *value;
		}
	}
	if (param.is_set())
	{
		if (param.is_param_name() && _module)
		{
			WheresMyPoint::Value value;
			if (_module->restore_value_for_wheres_my_point(param.get_as_param_name(), value))
			{
				return value.get_as<Class>();
			}
			return _defValue;
		}
		return param.get_as<Class>(_defValue);
	}
	return _defValue;
}
