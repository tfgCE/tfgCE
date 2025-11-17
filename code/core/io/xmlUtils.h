#pragma once

struct String;

namespace IO
{
	class Interface;

	namespace XML
	{
		class Document;

		bool is_temp_file(String const& filename);
		bool load_xml_using_temp_file(IO::XML::Document& doc, String const& filenameNormal);
		void save_xml_to_file_using_temp_file(IO::XML::Document const& doc, String const& filename);
	};
};
