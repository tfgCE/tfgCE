#pragma once

#include "..\types\optional.h"
#include "..\types\string.h"

namespace IO
{
	namespace Dialogs
	{
		struct Params
		{
			struct FileExtension
			{
				String info; // xml file
				String extension; // *.xml
			};
			Array<FileExtension> extensions;
			String defaultExtension; // xml

			String existingFile; // full/relative path or file name, if startInDirectory is not provided, will get it from this one
			String startInDirectory;
		};

		void setup_for_xml(Params& _params);

		Optional<String> get_file_to_save(Params& _params);
		Optional<String> get_file_to_open(Params& _params);
	};
};
