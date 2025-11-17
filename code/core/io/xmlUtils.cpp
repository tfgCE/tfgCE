#include "xmlUtils.h"

//

#include "file.h"
#include "outputWriter.h"
#include "xmlDocument.h"

//

using namespace IO;
using namespace XML;

bool IO::XML::is_temp_file(String const& filename)
{
	return filename.has_suffix(TXT(".temp"));
}

bool IO::XML::load_xml_using_temp_file(IO::XML::Document & doc, String const& filenameNormal)
{
	bool fileExists = false;
	for_count(int, tryIdx, 2)
	{
		String filename = filenameNormal;
		if (tryIdx == 1)
		{
			filename += TXT(".temp");
		}
		if (IO::File::does_exist(filename))
		{
			fileExists = true;
			IO::File file;
			if (file.open(filename))
			{
				file.set_type(IO::InterfaceType::Text);
				if (doc.load_xml(&file))
				{
					if (tryIdx == 1)
					{
						// move temp to normal
						if (IO::File::does_exist(filenameNormal))
						{
							IO::File::delete_file(filenameNormal);
						}
						IO::File::rename_file(filename, filenameNormal);
					}
					return true;
				}
			}
		}

		// reset doc
		doc.reset();
	}
	warn(TXT("could not load file \"%S\" (%S)"), filenameNormal.to_char(), fileExists? TXT("file exists") : TXT("file does not exist"));
	return false;
}

void IO::XML::save_xml_to_file_using_temp_file(IO::XML::Document const& doc, String const& filename)
{
	String filenameTemp = filename + TXT(".temp");
	{
		IO::File file;
		if (IO::File::does_exist(filenameTemp))
		{
			IO::File::delete_file(filenameTemp);
		}
		if (file.create(filenameTemp))
		{
			file.set_type(IO::InterfaceType::Text);
			doc.save_xml(&file);
#ifdef AN_DEVELOPMENT
			if (false) // skip writing to output
			{
				IO::OutputWriter ow;
				doc.save_xml(&ow);
			}
#endif
		}
	}
	{
		// this is the crucial bit, if we break here, we're a bit messed
		// but we should not have issues as we use just rename
		if (IO::File::does_exist(filenameTemp))
		{
			if (IO::File::does_exist(filename))
			{
				IO::File::delete_file(filename);
			}
			IO::File::rename_file(filenameTemp, filename);
		}
	}
}