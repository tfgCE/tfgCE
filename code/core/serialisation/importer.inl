#ifdef AN_CLANG
#include "..\io\utils.h"
#include "..\concurrency\scopedSpinLock.h"
#endif

template <typename Class, typename ImportOptions>
::Importer<Class, ImportOptions>::Importer()
{
}

template <typename Class, typename ImportOptions>
::Importer<Class, ImportOptions>::~Importer()
{
	fileTypes.clear();
	//an_assert(fileTypes.is_empty(), TXT("importers were not unregistered! it will be cleared but could be reported as memory leak"));
}

template <typename Class, typename ImportOptions>
void ::Importer<Class, ImportOptions>::register_file_type_no_options(String const & _extension, Import _importFunc)
{
	fileTypes[_extension].import = _importFunc;
}

template <typename Class, typename ImportOptions>
void ::Importer<Class, ImportOptions>::register_file_type_with_options(String const & _extension, ImportWithOptions _importFunc)
{
	fileTypes[_extension].import_with_options = _importFunc;
}

template <typename Class, typename ImportOptions>
void ::Importer<Class, ImportOptions>::unregister(String const & _extension)
{
	fileTypes.remove(_extension);
}

template <typename Class, typename ImportOptions>
void ::Importer<Class, ImportOptions>::clear()
{
	fileTypes.clear();
}

inline Concurrency::SpinLock& access_all_import_lock()
{
	static Concurrency::SpinLock lock;
	return lock;
}

template <typename Class, typename ImportOptions>
Class* ::Importer<Class, ImportOptions>::import(String const& _fileName, ImportOptions const& _options)
{
	Concurrency::ScopedSpinLock lock(access_all_import_lock());

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("importing \"%S\"..."), _fileName.to_char());
#endif
	String extension = IO::Utils::get_extension(_fileName);
	if (ImportInfo* info = fileTypes.get_existing(extension))
	{
		if (info->import)
		{
			return info->import(_fileName);
		}
		if (info->import_with_options)
		{
			return info->import_with_options(_fileName, _options);
		}
		an_assert(false, TXT("no import function provided?"));
		return nullptr;
	}
	else
	{
		return nullptr;
	}
}
