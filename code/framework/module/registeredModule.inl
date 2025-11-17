template <typename BaseModuleClass>
RegisteredModule<BaseModuleClass>::RegisteredModule(String const & _name, CREATE_MODULE_FUNCTION(_cm), CREATE_MODULE_DATA_FUNCTION(_cmd))
{
	defaultModule = false;
	name = Name(_name);
	create_module_fn = _cm;
	create_module_data_fn = _cmd;
}

//

template <typename BaseModuleClass, typename BaseModuleDataClass>
RegisteredModule<BaseModuleClass> & RegisteredModuleSet<BaseModuleClass, BaseModuleDataClass>::register_module(String const & _name, CREATE_MODULE_FUNCTION(_create_module), CREATE_MODULE_DATA_FUNCTION(_create_module_data))
{
	registeredModules.push_back(RegisteredModule<BaseModuleClass>(_name, _create_module, _create_module_data));
	return registeredModules.get_last();
}

template <typename BaseModuleClass, typename BaseModuleDataClass>
BaseModuleClass* RegisteredModuleSet<BaseModuleClass, BaseModuleDataClass>::create(IModulesOwner* _owner, ModuleDefBase const * _moduleDefBase, SetupModule _setup_module_pre)
{
	if (_moduleDefBase != nullptr && _moduleDefBase->get_data())
	{
		if (!_moduleDefBase->get_data()->get_type().is_valid())
		{
			return nullptr;
		}
		for_every_reverse_const(rm, registeredModules)
		{
			if (rm->get_name() == _moduleDefBase->get_data()->get_type())
			{
				BaseModuleClass* created = rm->create_module(_owner);
				if (Module* module = fast_cast<Module>(created))
				{
					Game::get()->pre_setup_module(_owner, created);
					if (_setup_module_pre)
					{
						_setup_module_pre(module);
					}
					module->setup_with(_moduleDefBase->get_data());
				}
				return created;
			}
		}
		//TODO Utils.Log.error("couldn't recognise module %S for %S", _moduleDefBase->get_data().type.asString, _owner.debugName);
	}
	// fallback - create default module (better to have "default for")
	if (! _moduleDefBase->type.is_valid())
	{
		return nullptr;
	}
	if (_moduleDefBase->type != Names::default_)
	{
		error(TXT("non default? (%S)"), _moduleDefBase->type.to_char());
		return nullptr;
	}
	RegisteredModule<BaseModuleClass> const * defaultModule = nullptr;
	// go in reverse to use latest default
	for_every_reverse_const(rm, registeredModules)
	{
		if (rm->is_default_for(_owner->get_object_type_name()))
		{
			defaultModule = rm;
			break;
		}
		if (rm->is_default())
		{
			defaultModule = rm;
		}
	}
	if (defaultModule)
	{
		BaseModuleClass* created = defaultModule->create_module(_owner);
		if (Module* module = fast_cast<Module>(created))
		{
			Game::get()->pre_setup_module(_owner, created);
			if (_setup_module_pre)
			{
				_setup_module_pre(module);
			}
			module->setup_with(_moduleDefBase->get_data());
		}
		return created;
	}
	return nullptr;
}

template <typename BaseModuleClass, typename BaseModuleDataClass>
ModuleData* RegisteredModuleSet<BaseModuleClass, BaseModuleDataClass>::create_data(Name const & _type, ::Framework::LibraryStored* _inLibraryStored) const
{
	for_every_reverse_const(rm, registeredModules)
	{
		if (rm->get_name() == _type)
		{
			ModuleData* result = nullptr;
			if (rm->can_create_module_data())
			{
				result = rm->create_module_data(_inLibraryStored);
			}
			else
			{
				memory_leak_info(_type.to_char());
				result = new BaseModuleDataClass(_inLibraryStored);
				forget_memory_leak_info;
			}
			an_assert(result);
			result->set_type(rm->get_name());
			return result;
		}
	}
	for_every_reverse_const(rm, registeredModules)
	{
		if (rm->is_default())
		{
			ModuleData* result = nullptr;
			if (rm->can_create_module_data())
			{
				result = rm->create_module_data(_inLibraryStored);
			}
			else
			{
				memory_leak_info(_type.to_char());
				result = new BaseModuleDataClass(_inLibraryStored);
				forget_memory_leak_info;
			}
			an_assert(result);
			result->set_type(rm->get_name());
			return result;
		}
	}
	return nullptr;
}

template <typename BaseModuleClass, typename BaseModuleDataClass>
void RegisteredModuleSet<BaseModuleClass, BaseModuleDataClass>::clear()
{
	registeredModules.clear();
}

