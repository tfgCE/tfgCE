#pragma once

namespace Framework
{
	template <typename Class>
	struct LibraryStoreWithDefaults
	{
		static LibraryStored* construct(::Framework::Library * _library, LibraryName const & _libName)
		{
			Class* newObjectType = new Class(_library, _libName);
			newObjectType->set_defaults();
			return newObjectType;
		}
	};

	template <typename Class>
	struct NormalLibraryStore
	{
		static LibraryStored* construct(::Framework::Library * _library, LibraryName const & _libName)
		{
			memory_leak_info(_libName.to_string().to_char());
			Class* newObject = new Class(_library, _libName);
			forget_memory_leak_info;
			return newObject;
		}
	};
};

#define COMBINED_LIBRARY_STORE(_class, _name, _getter) \
	protected: \
	::Framework::CombinedLibraryStore<_class> * _name; \
	public: \
	inline ::Framework::CombinedLibraryStore<_class> & get_##_getter() { return *_name; }

#define LIBRARY_STORE(_class, _name, _getter) \
	protected: \
	::Framework::LibraryStore<_class> * _name; \
	public: \
	inline ::Framework::LibraryStore<_class> & get_##_getter() { return *_name; }

#define ADD_COMBINED_LIBRARY_STORE(_class, _name, _typeName) \
	_name = new ::Framework::CombinedLibraryStore<_class>(Name(TXT(#_typeName))); \
	add_store(_name);

#define ADD_LIBRARY_STORE_WITH_DEFAULTS(_class, _name) \
	_name = new ::Framework::LibraryStore<_class>(this, _class::library_type(), ::Framework::LibraryStoreWithDefaults<_class>::construct); \
	add_store(_name);

#define ADD_LIBRARY_STORE(_class, _name) \
	_name = new ::Framework::LibraryStore<_class>(this, _class::library_type(), ::Framework::NormalLibraryStore<_class>::construct); \
	add_store(_name);

#define OVERRIDE_LIBRARY_STORE(_class, _name) \
	if (auto* store = fast_cast<::Framework::LibraryStoreBase>(_name)) \
	{ \
		an_assert(_class::library_type() == store->get_type_name(), TXT("if overriding, they have to share library type name")); \
		store->use_construct_object_with_name(::Framework::NormalLibraryStore<_class>::construct); \
	} \
	else \
	{ \
		an_assert(false, TXT("not valid library store to override_")); \
	}

#define ADD_LIBRARY_TOOL(_toolName, _runTool) \
	add_tool(new ::Framework::LibraryTool(_toolName, _runTool));

#define ADD_LIBRARY_TWEAK_TOOL(_toolName, _runTool) \
	add_tweak_tool(new ::Framework::LibraryTool(_toolName, _runTool));

