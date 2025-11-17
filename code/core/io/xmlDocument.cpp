#include "xmlDocument.h"

#include "io.h"
#include "assetFile.h"
#include "file.h"
#include "xmlAttribute.h"
#include "xmlNode.h"

#include "xmlSerialisationVersion.h"

#include "..\other\parserUtils.h"

#include "..\types\optional.h"
#include "..\types\unicode.h"

#include "..\serialisation\serialiser.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace IO;
using namespace IO::XML;

//

#define DECODING_COUNT 6
Document::EnDeElement const Document::c_decoding[] =
{
	Document::EnDeElement('&', TXT("&amp;")),
	Document::EnDeElement('<', TXT("&lt;")),
	Document::EnDeElement('>', TXT("&gt;")),
	Document::EnDeElement(String::no_break_space_char(), TXT("&#160;")),
	Document::EnDeElement('"', TXT("&quot;")),
	Document::EnDeElement(0, TXT(""))
};

Document::Document()
: SafeObject<Document>(this)
{
	reset();
}

Document::~Document()
{
	rootNode.clear();
	make_safe_object_unavailable();
}

void Document::reset()
{
	header = String(TXT("xml version=\"1.0\""));
	documentInfo = new DocumentInfo();
	rootNode = new XML::Node(this, 0, String::empty(), NodeType::Root);

	loadFilters.clear();
}

Document* Document::from_file(String const & _fileName)
{
	if (String::compare_icase(_fileName.get_right(4), TXT(".cx")))
	{
		return from_cx_file(_fileName);
	}
	else
	{
		return from_xml_file(_fileName);
	}
}

Document* Document::from_xml_file(String const & _fileName)
{
	IO::File file;
	if (! file.open(_fileName))
	{
		return nullptr;
	}

	return from_xml(&file, _fileName);
}

Document* Document::from_cx_file(String const & _fileName)
{
	IO::File file;
	if (!file.open(_fileName))
	{
		return nullptr;
	}

	return from_cx(&file, _fileName);
}

Document* Document::from_asset_file(String const& _fileName)
{
	if (String::compare_icase(_fileName.get_right(4), TXT(".cx")))
	{
		return from_cx_asset_file(_fileName);
	}
	else
	{
		return from_xml_asset_file(_fileName);
	}
}

Document* Document::from_xml_asset_file(String const& _fileName)
{
	IO::AssetFile file;
	if (!file.open(_fileName))
	{
		return nullptr;
	}

	return from_xml(&file, _fileName);
}

Document* Document::from_cx_asset_file(String const& _fileName)
{
	IO::AssetFile file;
	if (!file.open(_fileName))
	{
		return nullptr;
	}

	return from_cx(&file, _fileName);
}

Document* Document::from_xml(IO::Interface* _file, String const& _fileName)
{
	if (!_file)
	{
		return nullptr;
	}

	_file->set_type(IO::InterfaceType::Text);

	Document* doc = new Document();
	doc->documentInfo->loadedFromFile = _fileName;
	doc->documentInfo->originalFilePath = _fileName;
	if (!doc->load_xml(_file))
	{
		delete_and_clear(doc);
	}

	return doc;
}

Document* Document::from_cx(IO::Interface * _file, String const& _fileName)
{
	if (!_file)
	{
		return nullptr;
	}

	_file->set_type(IO::InterfaceType::Binary);

	Document* doc = new Document();
//#ifdef AN_DEVELOPMENT
	doc->documentInfo->loadedFromFile = _fileName.replace(String(TXT(".cx")), String(TXT(".xml")));
//#else
//	doc->documentInfo->loadedFromFile = _fileName;
//#endif
	if (!doc->serialise(Serialiser::reader(_file).temp_ref()))
	{
		delete_and_clear(doc);
	}
	return doc;
}

tchar Document::decode_char(Interface * const _io, int * _atLine /* = nullptr */)
{
	assert_slow(_io->get_last_read_char() == '&', TXT("Last read char is not & (it should be &)"));

	String symbol(TXT("&"));
	while (tchar const ch = _io->read_char(_atLine))
	{
		symbol += ch;
		if (ch == ';')
		{
			break;
		}
	}

	for_count(int, i, DECODING_COUNT)
	{
		if (symbol == c_decoding[i].encoded)
		{
			return c_decoding[i].decoded;
		}
	}

	if (symbol.get_data().get_size() > 1 &&
		symbol.get_data()[1] == '#')
	{
		symbol = symbol.get_left(symbol.get_length() - 1);
		if (symbol.get_data().get_size() > 2 &&
			symbol.get_data()[2] == 'x')
		{
			return Unicode::unicode_to_tchar(ParserUtils::parse_hex(&symbol.get_data()[3], 0));
		}
		else
		{
			return Unicode::unicode_to_tchar(ParserUtils::parse_int(&symbol.get_data()[2], 0));
		}
	}

	return 0;
}

String Document::encode_char(tchar const _ch)
{
	for_count(int, i, DECODING_COUNT)
	{
		if (_ch == c_decoding[i].decoded)
		{
			return String(c_decoding[i].encoded);
		}
	}

	String character;
	character += _ch;
	return character;
}

bool Document::is_separator(tchar const _ch)
{
	return (_ch == ' ' ||
			_ch == '\n' ||
			_ch == '\t' ||
			_ch == '\r')? true : false;
}

bool Document::load_xml(Interface * const _io)
{
	int atLine = 1;

	header = String::empty();
	// find header
	if (_io->read_char(&atLine) == '<' &&
		_io->read_char(&atLine) == '?')
	{
		// read header
		while (tchar const ch = _io->read_char(&atLine))
		{
			if (ch == '?')
			{
				// >
				_io->read_char(&atLine);
				// read next char
				_io->read_char(&atLine);
				break;
			}
			header += ch;
		}
	}
	else
	{
		// start again
		atLine = 1;
		_io->go_to(0);
		_io->read_char(&atLine);
	}

	bool thereWasError = false;
	// load nodes
	while (true)
	{
		RefCountObjectPtr<Node> child;
		child = Node::load_xml(_io, this, rootNode.get(), REF_ atLine, REF_ thereWasError);
		if (auto* ch = child.get())
		{
			rootNode->add_node(ch);
		}
		else
		{
			break;
		}
	}

	if (thereWasError)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool Document::save_xml(Interface* const _io) const { return save_xml(_io, NP, NP); }

bool Document::save_xml(Interface * const _io, Optional<bool> const & _header, Optional<bool> const& _includeComments) const
{
	if	(_header.get(true) &&
		! header.is_empty())
	{
		// handle header
		_io->write_text(TXT("<?"));
		_io->write_text(header);
		_io->write_text(TXT("?>\n"));
	}

	// save nodes
	Node const * node = rootNode->first_sub_node();
	while (node)
	{
		if (!node->save_xml(_io, _includeComments.get(true)))
		{
			return false;
		}
		node = node->next_sub_node();
	}

	return true;
}

bool Document::save_cx(Interface * const _io)
{
	return serialise(Serialiser::writer(_io).temp_ref());
}

void Document::indent(Document const * _doc, Interface * const _io, uint _number)
{
	while (_number > 0)
	{
		if (_doc)
		{
			_io->write_text(_doc->indentString);
		}
		else
		{
			_io->write_text(XML_DEFAULT_INDENT);
		}
		_number --;
	}
}

bool Document::serialise(Serialiser & _serialiser)
{
	bool result = true;

	version = CURRENT_VERSION;
	result &= base::serialise(_serialiser);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid xml version"));
		return false;
	}

	if (version >= VER_FILEPATH)
	{
		result &= serialise_data(_serialiser, documentInfo->originalFilePath);
	}
	else if (_serialiser.is_reading())
	{
		documentInfo->originalFilePath = documentInfo->loadedFromFile;
	}
	if (_serialiser.is_reading() &&
		version < VER_BUILD_NO &&
		CURRENT_VERSION >= VER_BUILD_NO)
	{
		// ignore without version!
#ifdef BUILD_PREVIEW
		error(TXT("no version for cx file, not loading : %S"), documentInfo->loadedFromFile.to_char());
		error(TXT("! you may need to rebuild win32 tool, copy to use as the deploy tool, rebuild deploy data and rebuild apk"));
#endif
		return false;
	}
	if (version >= VER_BUILD_NO)
	{
		int buildNo = BUILD_NO;
		result &= serialise_data(_serialiser, buildNo);
		if (_serialiser.is_reading() &&
			buildNo < BUILD_NO)
		{
			// too old, do not load
#ifdef BUILD_PREVIEW
			error(TXT("older version of a cx file (%i v %i), not loading : %S"), buildNo, BUILD_NO, documentInfo->loadedFromFile.to_char());
			error(TXT("! you may need to rebuild win32 tool, copy to use as the deploy tool, rebuild deploy data and rebuild apk"));
#endif
			return false;
		}
	}
	result &= serialise_data(_serialiser, header);
	result &= rootNode->serialise(_serialiser, version);

	return result;
}

bool Document::add_includable(Name const& _id, Node const* _node)
{
	for_every(i, includables)
	{
		if (i->id == _id)
		{
			error(TXT("can't add includable \"%S\", already added"), _id.to_char());
			return false;
		}
	}
	{
		Includable i;
		i.id = _id;
		includables.push_back(i);
	}
	{
		Includable& i = includables.get_last();
		i.memoryStorage.set_type(IO::InterfaceType::Text);
		_node->save_single_node_to_xml(&i.memoryStorage);
	}
	return true;
}

bool Document::get_includable(Name const& _id, OUT_ RefCountObjectPtr<Node> & _node)
{
	an_assert(!_node.get());
	for_every(i, includables)
	{
		if (i->id == _id)
		{
			int atLine = 0;
			bool error = false;
			i->memoryStorage.go_to(0);
			_node = Node::load_single_node_from_xml(&i->memoryStorage, this, REF_ atLine, REF_ error);
			return true && _node.get() && ! error;
		}
	}

	error(TXT("can't find includable \"%S\""), _id.to_char());
	return false;
}

void Document::set_indent_spaces(int _count)
{
	indentString.access_data().set_size(_count + 1);
	for_count(int, i, _count)
	{
		indentString.access_data()[i] = ' ';
	}
	indentString.access_data()[_count] = 0;
}

void Document::set_indent_tab()
{
	indentString = TXT("\t");
}
