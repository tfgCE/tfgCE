#pragma once

#include "digested.h"
#include "xmlAttribute.h"
#include "xmlNode.h"
#include "memoryStorage.h"
#include "..\memory\safeObject.h"
#include "..\serialisation\serialisableResource.h"
#include "..\types\optionalNP.h"

#define XML_DEFAULT_INDENT TXT(" ")

namespace IO
{
	class Interface;

	namespace XML
	{
		class Document;

		/**
		 *	Nodes may live longer than actual document (it is pointed with safe ptr).
		 *	Although some nodes may require info about from where the document has been loaded, hence DocumentInfo is stored separately and managed with ref counting.
		 */
		struct DocumentInfo
		: public RefCountObject
		{
		public:
			String const& get_loaded_from_file() const { return loadedFromFile; }
			String const& get_original_file_path() const { return originalFilePath; }

		private:
			String loadedFromFile; // loadedFromFile always points at the file we loaded from
			String originalFilePath; // originalFilePath is for the xml we loaded from. if we change to cx file, move somewhere else, we still have same originalFilePath. and we do move it somewhere else for android

			friend class Attribute;
			friend class Node;
			friend class Document;
		};

		/**
		 *	This is not a proper XML document, it has some additions (<.node></node> comments out node)
		 *	It can be also stored as a cx file. It is stored in more game-friendly format making it easier to read (doesn't require parsing, just reads nodes and stuff)
		 *	cx file is also used for one more thing. Because listing directories through asset manager (android) doesn't list directories, it is required to have a flat structure for library.
		 *	But it only is relevant to cx files (as they are only loaded directly, when loading a library. That's why when deploying, cx file hierarchy is flattened but other files are left where they are
		 *	Location of files is stored relative, that's why we need to rebuild full path to them. We do that using originalFilePath stored in cx file (as xml document).
		 */
		class Document
		: public SerialisableResource
		, public SafeObject<Document>
		{
			friend class Attribute;
			friend class Node;

			typedef SerialisableResource base;
		public:
			Document();
			virtual ~Document();

			void reset();

			DocumentInfo const* get_document_info() const { return documentInfo.get(); }

			static Document* from_file(String const & _fileName);
			static Document* from_xml_file(String const & _fileName);
			static Document* from_cx_file(String const & _fileName);

			static Document* from_asset_file(String const& _fileName);
			static Document* from_xml_asset_file(String const& _fileName);
			static Document* from_cx_asset_file(String const& _fileName);

			static Document* from_xml(IO::Interface* _file, String const& _fileName);
			static Document* from_cx(IO::Interface* _file, String const& _fileName);

			bool load_xml(Interface * const);
			bool save_xml(Interface * const) const;
			bool save_xml(Interface * const, Optional<bool> const & _header, Optional<bool> const & _includeComments) const;
			bool save_cx(Interface * const);

			Node * const get_root() { return rootNode.get(); }

			bool add_includable(Name const& _id, Node const* _node);
			bool get_includable(Name const& _id, OUT_ RefCountObjectPtr<Node> & _node);

			void set_indent_spaces(int _count);
			void set_indent_tab();

		public: // SerialisableResource
			override_ bool serialise(Serialiser & _serialiser);

		private:
			String header;
			RefCountObjectPtr<DocumentInfo> documentInfo;
			RefCountObjectPtr<Node> rootNode;

			String indentString = String(XML_DEFAULT_INDENT);

			Array<Name> loadFilters; // load only filters specified here, use <_load filter="name"> + <_section filter="name"> _load nodes are not added to the document, _section nodes are transparent

		private:
			struct Includable
			{
				Name id;
				IO::MemoryStorage memoryStorage;
			};
			List<Includable> includables;

		private:
			struct EnDeElement // encoding decoding
			{
				tchar const decoded;
				tchar const * encoded;
				EnDeElement(tchar _decoded, tchar const * _encoded)
				: decoded(_decoded)
				, encoded(_encoded)
				{}
			};

			static EnDeElement const c_decoding[]; // last one should be empty

			static tchar decode_char(Interface * const, int * _atLine = nullptr); // last read char8 should be &
			static String encode_char(tchar const); // encodes char8 (if needed, otherwise, returns char8)
			static bool is_separator(tchar const); // checks if char8 is separator that doesn't matter
			static void indent(Document const * _doc, Interface * const, uint);

		private:
			Document(Document const& _other);
			Document& operator=(Document const& _other);
		};

		struct DocumentHandle
		: public RefCountObject
		{
		public:
			Document& access() { return document; }
		private:
			Document document;
		};
	};

};
