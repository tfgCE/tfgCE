#pragma once

// include only if source files!

#include "..\mainSettings.h"
#include "..\serialisation\serialiser.h"
#include "..\..\core\io\assetFile.h"

template <typename Class, typename ImportOptions>
struct ImporterHelper
{
public:
	ImporterHelper(Class*& _object);
	virtual ~ImporterHelper() {}

	void set_import_options(ImportOptions const& _importOptions) { importOptions = _importOptions; }

	void allow_from_assets(bool _allowFromAssets = true) { allowFromAssets = _allowFromAssets; }

	void set_file_names_from_xml(IO::XML::Node const * _node, String const & _fileNameAttribute = String(TXT("file")), String const & _importFromFileNameAttribute = String(TXT("importFromFile")));

	bool has_file_attribute(IO::XML::Node const * _node, String const & _fileNameAttribute = String(TXT("file")));

	bool setup_from_xml(IO::XML::Node const * _node, bool _allowFromAssets, String const & _fileNameAttribute = String(TXT("file")), String const & _importFromFileNameAttribute = String(TXT("importFromFile")));
	bool setup_from_xml_use_provided(IO::XML::Node const* _node, bool _allowFromAssets, String const& _fileName, String const& _filePath, String const& _fileExt, String const& _importFromPath, String const& _importFromExt);
	bool import();
	void unload();
	void digest_definition(IO::XML::Node const * _node);

protected:
	virtual void delete_object(); // why? because: if something has to be done on rendering thread, it is safer to use deferred deletion and since we use importer for system stuff we should defer through container of imported object
	virtual void create_object();

	virtual bool on_setup_from_xml(IO::XML::Node const* _node);

	virtual bool check_if_object_actual();
	virtual void import_object();
	virtual void on_import_pre_serialisation_to_file(); // 

protected:
	Class*& object;

	bool forceReimportingIfImportFilePresent;

	bool allowFromAssets = false;
	String fileName;
	String importFromFileName;
	
	IO::Digested digestedDefinition; // xml definition (node) for that particular resource

	ImportOptions importOptions;
};

#include "importerHelper.inl"
