#pragma once

#include "..\modulesOwner\modulesOwner.h"
#include "moduleData.h"

// for .inl
#include "..\game\game.h"

#define CREATE_MODULE_FUNCTION(_var) BaseModuleClass* (*_var)(::Framework::IModulesOwner* _owner)
#define CREATE_MODULE_DATA_FUNCTION(_var) ModuleData* (*_var)(::Framework::LibraryStored* _inLibraryStored)

namespace Framework
{
	/**
	 *	It is possible to register modules under same name. Last one to register is considered the one to use.
	 *	This is on purpose but should be handled with caution (override_ only with derived class).
	 */
	template <typename BaseModuleClass>
	class RegisteredModule
	{
	public:
		RegisteredModule(String const & _name, CREATE_MODULE_FUNCTION(_cm), CREATE_MODULE_DATA_FUNCTION(_cmd) = nullptr);

		RegisteredModule<BaseModuleClass>& be_default() { defaultModule = true; return *this; }
		RegisteredModule<BaseModuleClass>& be_default_for(Name const & _name) { defaultModuleFor.push_back_unique(_name); return *this; }
		bool is_default() const { return defaultModule; }
		bool is_default_for(Name const & _name) const { return defaultModuleFor.does_contain(_name); }
		Name const & get_name() const { return name; }
		BaseModuleClass* create_module(IModulesOwner* _owner) const { return create_module_fn(_owner); }
		bool can_create_module_data() const { return create_module_data_fn != nullptr; }
		ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored) const { an_assert(create_module_data_fn != nullptr);  return create_module_data_fn(_inLibraryStored); }

	private:
		Array<Name> defaultModuleFor;
		bool defaultModule;
		Name name;
		CREATE_MODULE_FUNCTION(create_module_fn);
		CREATE_MODULE_DATA_FUNCTION(create_module_data_fn);
	};

	template <typename BaseModuleClass, typename BaseModuleDataClass>
	class RegisteredModuleSet
	{
	public:
		RegisteredModule<BaseModuleClass> & register_module(String const & _name, CREATE_MODULE_FUNCTION(_create_module), CREATE_MODULE_DATA_FUNCTION(_create_module_data) = nullptr);
		
		BaseModuleClass* create(IModulesOwner* _owner, ModuleDefBase const * _moduleDefBase = nullptr, SetupModule _setup_module_pre = nullptr);
		ModuleData* create_data(Name const & _type, ::Framework::LibraryStored* _inLibraryStored) const;

		void clear();

	private:
		List<RegisteredModule<BaseModuleClass> > registeredModules;

	};

	#include "registeredModule.inl"
};

