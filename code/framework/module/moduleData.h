#pragma once

#include "..\library\libraryStored.h"
#include "..\..\core\other\customParameters.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\types\string.h"
#include "..\..\core\types\names.h"
#include "..\..\core\wheresMyPoint\iWMPTool.h"

namespace Framework
{
	class Library;
	class Module;
	class ModuleData;
	class ObjectType;

	namespace ModuleDefType
	{
		enum Type
		{
			None,
			Default,
		};
	};

	template <typename BaseModuleClass, typename BaseModuleDataClass>
	class RegisteredModuleSet;

	struct ModuleDefBase
	{
		Name type;

		ModuleDefBase(ModuleDefType::Type _type = ModuleDefType::Default)
		: type(_type == ModuleDefType::Default ? Names::default_: Name::invalid())
		{
		}
		virtual ~ModuleDefBase() {}
		
		virtual ModuleData* get_data() const = 0;
	};

	template <typename BaseModuleClass, typename BaseModuleDataClass>
	struct ModuleDef
	: public ModuleDefBase
	{
		RefCountObjectPtr<BaseModuleDataClass> data;

		ModuleDef(ModuleDefType::Type _type = ModuleDefType::Default)
		: ModuleDefBase(_type)
		, data(nullptr)
		{
		}
		virtual ~ModuleDef() {}

		override_ ModuleData* get_data() const { return data.get(); }

		void provide_default_type(Name const & _type = Names::default_) { if (type == Name::invalid() || type == Names::default_) { type = _type; } }
		bool load_from_xml(LibraryStored* _inLibraryStored, IO::XML::Node const * _node, LibraryLoadingContext & _lc, RegisteredModuleSet<BaseModuleClass, BaseModuleDataClass> const & _registeredModuleSet);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		void prepare_to_unload();
	};

	class ModuleData
	: public RefCountObject
	{
		FAST_CAST_DECLARE(ModuleData);
		FAST_CAST_END();
	public:
		ModuleData(LibraryStored* _inLibraryStored);
		virtual ~ModuleData() {}

		LibraryStored* get_in_library_stored() const { return inLibraryStored; }

		void set_type(Name const & _type) { type = _type; }
		Name const & get_type() const { return type; }

		//public LibraryName get_library_name_parameter(string _parName)
		CustomParameters & access_parameters() { return parameters; }
		CustomParameters const & get_parameters() const { return parameters; }

		WheresMyPoint::ToolSet const & get_wheres_my_point_processor_on_init() const { return wheresMyPointProcessorOnInit; }

		template <typename Class>
		bool has_parameter(Module* _module, Name const & _parName) const;
		template <typename Class>
		Class get_parameter(Module* _module, Name const & _parName) const;
		template <typename Class>
		Class get_parameter(Module* _module, Name const & _parName, Class const & _defValue) const;

		virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc); // loading module data, so then it can be used with proper registered module set
		virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		virtual void prepare_to_unload() {}
		//public void log(String const & _name)
		//protected virtual void debug_log_internal()

	protected:
		virtual bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
		virtual bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	private:
		LibraryStored* inLibraryStored = nullptr; // todo_hack alright, this kinds of break module data as something separate, but we need it for item variant
		Name type; // full type, not class of module

		WheresMyPoint::ToolSet wheresMyPointProcessorOnInit; // this processor is used to transform values from owner to custom parameters to setup this module

		bool dontReadTypeFromXML = false; // do not read from xml
		bool useDefaultIfMentionedInXML = true; // if marker is present, it will auto use "default"
		CustomParameters parameters;
		Name inGroup; // stored when loading from xml
	};
};
