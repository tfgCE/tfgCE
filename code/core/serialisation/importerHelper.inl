#pragma once

inline Concurrency::SpinLock& access_all_importer_helper_lock()
{
	static Concurrency::SpinLock lock;
	return lock;
}

template <typename Class, typename ImportOptions>
ImporterHelper<Class, ImportOptions>::ImporterHelper(Class*& _object)
: object(_object)
, forceReimportingIfImportFilePresent(false)
{
}

template <typename Class, typename ImportOptions>
void ImporterHelper<Class, ImportOptions>::set_file_names_from_xml(IO::XML::Node const * _node, String const & _fileNameAttribute, String const & _importFromFileNameAttribute)
{
	String currentPath = IO::Utils::get_directory(_node->get_document_info()->get_original_file_path());
	if (IO::XML::Attribute const * attr = _node->get_attribute(_fileNameAttribute))
	{
		fileName = IO::Utils::make_path(currentPath, attr->get_as_string());
	}
	else
	{
		fileName = String::empty();
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(_importFromFileNameAttribute))
	{
		importFromFileName = IO::Utils::make_path(currentPath, attr->get_as_string());
	}
	else
	{
		importFromFileName = String::empty();
	}
}

template <typename Class, typename ImportOptions>
bool ImporterHelper<Class, ImportOptions>::has_file_attribute(IO::XML::Node const * _node, String const & _fileNameAttribute)
{
	return _node->has_attribute(_fileNameAttribute.to_char());
}

template <typename Class, typename ImportOptions>
bool ImporterHelper<Class, ImportOptions>::setup_from_xml(IO::XML::Node const * _node, bool _allowFromAssets, String const & _fileNameAttribute, String const & _importFromFileNameAttribute)
{
	bool result = true;

	allowFromAssets = _allowFromAssets;

	set_file_names_from_xml(_node, _fileNameAttribute, _importFromFileNameAttribute);

	// store definition before importing - we will know if something in node has changed or not
	digest_definition(_node);

	// load options from xml
	importOptions.load_from_xml(_node);

	// post setup
	result &= on_setup_from_xml(_node);

	return result;
}

template <typename Class, typename ImportOptions>
bool ImporterHelper<Class, ImportOptions>::setup_from_xml_use_provided(IO::XML::Node const * _node, bool _allowFromAssets, String const& _fileName, String const & _filePath, String const & _fileExt, String const & _importFromPath, String const & _importFromExt)
{
	bool result = true;

	allowFromAssets = _allowFromAssets;

	String fileExt = _fileExt.is_empty() ? String::empty() : String(TXT(".")) + _fileExt;
	String importFromExt = _importFromExt.is_empty() ? String::empty() : String(TXT(".")) + _importFromExt;

	{
		String currentPath = IO::Utils::get_directory(_node->get_document_info()->get_original_file_path());
		fileName = IO::Utils::as_valid_directory(IO::Utils::make_path(currentPath, _filePath)) + _fileName + fileExt;
		importFromFileName = IO::Utils::as_valid_directory(IO::Utils::make_path(currentPath, _importFromPath)) + _fileName + importFromExt;
	}

	// store definition before importing - we will know if something in node has changed or not
	digest_definition(_node);

	// load options from xml
	importOptions.load_from_xml(_node);

	// post setup
	result &= on_setup_from_xml(_node);

	return result;
}

template <typename Class, typename ImportOptions>
void ImporterHelper<Class, ImportOptions>::digest_definition(IO::XML::Node const * _node)
{
	digestedDefinition.digest(_node);
}
	
template <typename Class, typename ImportOptions>
void ImporterHelper<Class, ImportOptions>::unload()
{
	delete_object();
}

template <typename Class, typename ImportOptions>
bool ImporterHelper<Class, ImportOptions>::import()
{
	bool result = true;

	if (fileName.is_empty() && importFromFileName.is_empty())
	{
		error(TXT("not file name nor import from file name provided when importing"));
		return false;
	}

#ifndef AN_ANDROID
	bool forceReimportingNow = MainSettings::global().should_force_reimporting();
	bool forceReimportingIfImportFilePresentNow = MainSettings::global().should_force_reimporting() || forceReimportingIfImportFilePresent;
#endif
	// doesn't make sense to serialise when reimporting is forced
	if (!fileName.is_empty()
#ifndef AN_ANDROID
		&& (!forceReimportingNow || importFromFileName.is_empty())
#endif
		)
	{
		bool readData = false;
#ifdef AN_ANDROID
		// first try asset
		if (!readData && allowFromAssets)
		{
			IO::AssetFile* asset = new IO::AssetFile(IO::get_asset_data_directory() + fileName);
			if (asset->is_open())
			{
				delete_object();
				create_object();
				result &= serialise_data(Serialiser::reader(asset).temp_ref(), object);
				readData = true;
			}
			delete asset;
		}
#endif
		if (!readData)
		{
			IO::File* file = new IO::File(IO::get_user_game_data_directory() + fileName);
			if (file->is_open())
			{
				delete_object();
				create_object();
				result &= serialise_data(Serialiser::reader(file).temp_ref(), object);
				readData = true;
			}
			delete file;
		}
	}

#ifndef AN_ANDROID
	if (MainSettings::global().should_allow_importing() ||
		forceReimportingIfImportFilePresentNow)
	{
		if (!importFromFileName.is_empty())
		{
			if (IO::File::does_exist(importFromFileName))
			{
				bool import = true;

				if (!forceReimportingIfImportFilePresentNow)
				{
					// check if we should import
					if (object && check_if_object_actual())
					{
						import = false;
					}
				}

				if (import)
				{
					Concurrency::ScopedSpinLock lock(access_all_importer_helper_lock());

					delete_object();
					import_object();
					if (object)
					{
						on_import_pre_serialisation_to_file();
						// store resource in file (with digested value from import source)
						{
							object->digest_source(IO::File(importFromFileName).temp_ptr());
						}
						object->set_digested_definition(digestedDefinition);
						object->serialise_resource_to_file(fileName);
					}
					else
					{
						// couldn't import
						create_object();
						error(TXT("couldn't import object from \"%S\""), importFromFileName.to_char());
						result = false;
					}
				}
			}
			else if (MainSettings::global().should_disallow_no_import_files_present())
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_OUTPUT_IMPORT_ERRORS
				error(TXT("could not find file \"%S\" to import"), importFromFileName.to_char());
#endif
				result = false;
#endif
			}
		}
	}
#endif
	return result;
}

template <typename Class, typename ImportOptions>
void ImporterHelper<Class, ImportOptions>::delete_object()
{
	delete_and_clear(object);
}

template <typename Class, typename ImportOptions>
void ImporterHelper<Class, ImportOptions>::create_object()
{
	object = new Class();
}

template <typename Class, typename ImportOptions>
bool ImporterHelper<Class, ImportOptions>::check_if_object_actual()
{
	return object->check_if_actual(IO::File(importFromFileName).temp_ptr(), digestedDefinition);
}

template <typename Class, typename ImportOptions>
void ImporterHelper<Class, ImportOptions>::import_object()
{
	object = Class::importer().import(importFromFileName);
}

template <typename Class, typename ImportOptions>
void ImporterHelper<Class, ImportOptions>::on_import_pre_serialisation_to_file()
{
}

template <typename Class, typename ImportOptions>
bool ImporterHelper<Class, ImportOptions>::on_setup_from_xml(IO::XML::Node const* _node)
{
	return true;
}
