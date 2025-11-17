#pragma once

#include "..\io\io.h"
#include "..\containers\map.h"

#include <functional>

struct NoImportOptions
{
	// nothing around here!
};

template <typename Class, typename ImportOptions = NoImportOptions>
class Importer
{
public:
	typedef std::function< Class*(String const & _fileName) > Import;
	typedef std::function< Class*(String const & _fileName, ImportOptions const & _options) > ImportWithOptions;

public:
	Importer();
	~Importer();

	void register_file_type_no_options(String const & _extension, Import _importFunc);
	void register_file_type_with_options(String const & _extension, ImportWithOptions _importFunc);
	void unregister(String const & _extension);
	void clear();

	Class* import(String const & _fileName, ImportOptions const & _options = ImportOptions());

private:
	struct ImportInfo
	{
		Import import;
		ImportWithOptions import_with_options;

		ImportInfo()
		: import(nullptr)
		, import_with_options(nullptr)
		{}
	};

	Map<String, ImportInfo> fileTypes;
};

#include "importer.inl"
