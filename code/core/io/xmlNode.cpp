#include "xmlNode.h"

#include "io.h"
#include "memoryStorage.h"
#include "xmlDocument.h"
#include "..\other\parserUtils.h"
#include "..\serialisation\serialiser.h"

#include "xmlSerialisationVersion.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_XML_NODE_LIFESPAN

//

using namespace IO;
using namespace IO::XML;

//

SIMPLE_SERIALISER_AS_INT32(::IO::XML::NodeType::Type);

//

// replaces [special] with space, multiple spaces are turned into single spaces (unless no break spaces)
// ~ breaks line, makes following spaces ignored as well as previous ones
// [special] = \t \n \r
void raw_to_normal(String & _string)
{
	auto & data = _string.access_data();
	int orgLength = data.get_size();
	tchar* dest = data.begin();
	tchar* src = dest;
	bool ignoreSpaces = true;
	bool spacePending = false;
	for (int i = 0; i < orgLength - 1; ++i, ++src)
	{
		// treat special characters as spaces
		if (*src == '\t' || *src == '\n' || *src == '\r')
		{
			*src = ' '; // we can replace it here as we are overwriting it anyway
		}

		if (*src == '~')
		{
			spacePending = false; // ignore previous spaces
			ignoreSpaces = true; // ignore following spaces
			*dest = *src;
			++dest;
		}
		else
		{
			if (*src == ' ')
			{
				spacePending = true;
			}
			else
			{
				// add space only if pending and not ignored
				if (spacePending && !ignoreSpaces)
				{
					*dest = ' ';
					++dest;
				}
				// copy char, change no-break-space into (non ignorable!) space
				{
					*dest = *src;
					if (*dest == String::no_break_space_char())
					{
						*dest = ' ';
					}
					++dest;
				}
				// do not ignore following spaces
				spacePending = false;
				ignoreSpaces = false;
			}
		}
	}
	*dest = 0;
	int length = (int)(dest - data.get_data());
	an_assert(length >= 0);
	an_assert(length <= orgLength);
	data.set_size((uint)length + 1);
}


Node::Node(Document const * _document, int _atLine, String const & _content, NodeType::Type const _type)
: document(_document)
, documentInfo(_document->get_document_info())
, atLine(_atLine)
, type(_type)
, contentRaw(_content)
, parentNode(nullptr)
, addChildrenOnLoadTo(this)
, nextNode(nullptr)
, firstSubNode(nullptr)
, firstAttribute(nullptr)
{
	content = contentRaw;
	raw_to_normal(content);
	// TODO check if name is correct

#ifdef DEBUG_XML_NODE_LIFESPAN
	output(TXT("[xml-node-lifespan] new node %p"), this);
#endif
}

Node::Node(Document const * _document)
: document(_document)
, documentInfo(_document->get_document_info())
, atLine(0)
, type(NodeType::Node)
, parentNode(nullptr)
, addChildrenOnLoadTo(this)
, nextNode(nullptr)
, firstSubNode(nullptr)
, firstAttribute(nullptr)
{
#ifdef DEBUG_XML_NODE_LIFESPAN
	output(TXT("[xml-node-lifespan] new node %p"), this);
#endif
}

Node::~Node()
{
#ifdef DEBUG_XML_NODE_LIFESPAN
	output(TXT("[xml-node-lifespan] delete node %p"), this);
#endif
	clear();
}

String Node::get_location_info() const
{
	return String::printf(TXT("%S at %i"), documentInfo.get() ? documentInfo->originalFilePath.to_char() : TXT("??"), atLine);
}

void Node::clear()
{
	while (firstSubNode.get())
	{
		remove_node(firstSubNode.get());
	}
	an_assert(! firstSubNode.is_set());
#ifdef DEBUG_XML_NODE_LIFESPAN
	output(TXT("[xml-node-lifespan] %p clear first %p"), this, firstSubNode.get());
#endif
	firstSubNode.clear();
	while (firstAttribute)
	{
		delete_attribute(firstAttribute);
	}
	firstAttribute = nullptr;
}

Node * const Node::add_text(String const & _text)
{
	String text = _text;
	text.change_consecutive_spaces_to_no_break_spaces();
	Node * const newNode = new Node(document.get(), NONE, text, NodeType::Text);
	push_back_node(newNode);
	return newNode;
}

Node* const Node::add_node(String const & _text)
{
	String text = _text;
	text.change_consecutive_spaces_to_no_break_spaces();
	Node * const newNode = new Node(document.get(), NONE, text, NodeType::Node);
	push_back_node(newNode);
	return newNode;
}

Node* const Node::add_comment(String const & _text)
{
	String text = _text;
	text.change_consecutive_spaces_to_no_break_spaces();
	Node * const newNode = new Node(document.get(), NONE, text, NodeType::Comment);
	push_back_node(newNode);
	return newNode;
}

void Node::add_node(Node * _node)
{
	push_back_node(_node);
}

void Node::set_attribute(tchar const * _attribute, String const & _value)
{
	an_assert(type != NodeType::Text, TXT("Cannot set attribute to text node"));

	Attribute * attribute = find_attribute(_attribute);

	if (! attribute)
	{
		attribute = new Attribute(String(_attribute));
		push_back_attribute(attribute);
	}

	attribute->set_value(_value);
}

void Node::set_bool_attribute(tchar const * _attribute, bool _value)
{
	set_attribute(_attribute, String(_value ? TXT("true") : TXT("false")));
}

void Node::set_int_attribute(tchar const * _attribute, int _value)
{
	set_attribute(_attribute, String::printf(TXT("%i"), _value));
}

void Node::set_float_attribute(tchar const * _attribute, float _value)
{
	set_attribute(_attribute, String::printf(TXT("%.6f"), _value));
}

void Node::set_bool_to_child(tchar const * _name, bool const _value)
{
	Node* node = add_node(_name);
	node->add_text(_value ? TXT("true") : TXT("false"));
}

void Node::set_int_to_child(tchar const * _name, int const _value)
{
	Node* node = add_node(_name);
	node->add_text(String::printf(TXT("%i"), _value));
}

void Node::set_float_to_child(tchar const * _name, float const _value)
{
	Node* node = add_node(_name);
	node->add_text(String::printf(TXT("%.6f"), _value));
}

void Node::set_string_to_child(tchar const * _name, String const & _value)
{
	Node* node = add_node(_name);
	node->add_text(_value);
}

void Node::set_string_to_child(tchar const * _name, tchar const * _value)
{
	Node* node = add_node(_name);
	node->add_text(_value);
}

Attribute const * const Node::get_attribute(tchar const * _attribute) const
{
	Attribute const * attribute = firstAttribute;
	while (attribute)
	{
		if (attribute->get_name() == _attribute)
		{
			return attribute;
		}

		attribute = attribute->nextAttribute;
	}

	return nullptr;
}

Name const Node::get_name_attribute(tchar const * _attribute, Name const & _defVal) const
{
	if (Attribute const * attribute = get_attribute(_attribute))
	{
		return Name(attribute->get_as_string());
	}
	return _defVal;
}

String const & Node::get_string_attribute(tchar const * _attribute, String const & _defVal) const
{
	if (Attribute const * attribute = get_attribute(_attribute))
	{
		return attribute->get_as_string();
	}
	return _defVal;
}

int Node::get_int(int _defVal) const
{
	String text = get_internal_text();
	return text.is_empty() ? _defVal : ParserUtils::parse_int(text);
}

bool Node::get_bool(bool _defVal) const
{
	String text = get_internal_text();
	return text.is_empty() ? _defVal : ParserUtils::parse_bool(text);
}

float Node::get_float(float _defVal) const
{
	String text = get_internal_text();
	return text.is_empty()? _defVal : ParserUtils::parse_float(text);
}

Name Node::get_name(Name const & _defVal) const
{
	String text = get_internal_text();
	return text.is_empty() ? _defVal : Name(text);
}

int Node::get_int_attribute(tchar const * _attribute, int _defVal) const
{
	if (Attribute const * attribute = get_attribute(_attribute))
	{
		return attribute->get_as_int();
	}
	return _defVal;
}

bool Node::get_bool_attribute(tchar const * _attribute, bool _defVal) const
{
	if (Attribute const * attribute = get_attribute(_attribute))
	{
		return attribute->get_as_bool();
	}
	return _defVal;
}

float Node::get_float_attribute(tchar const * _attribute, float _defVal) const
{
	if (Attribute const * attribute = get_attribute(_attribute))
	{
		return attribute->get_as_float();
	}
	return _defVal;
}

String Node::get_string_attribute_or_from_node(tchar const * _name, String const & _defVal) const
{
	if (Attribute const * attribute = get_attribute(_name))
	{
		return attribute->get_as_string();
	}
	return get_text();
}

String Node::get_string_attribute_or_from_child(tchar const * _name, String const & _defVal) const
{
	if (Attribute const * attribute = get_attribute(_name))
	{
		return attribute->get_as_string();
	}
	return get_string_from_child(_name, _defVal);
}

int Node::get_int_attribute_or_from_child(tchar const * _name, int _defVal) const
{
	if (Attribute const * attribute = get_attribute(_name))
	{
		return attribute->get_as_int();
	}
	return get_int_from_child(_name, _defVal);
}

bool Node::get_bool_attribute_or_from_child(tchar const * _name, bool _defVal) const
{
	if (Attribute const * attribute = get_attribute(_name))
	{
		return attribute->get_as_bool();
	}
	return get_bool_from_child(_name, _defVal);
}

bool Node::get_bool_attribute_or_from_child_presence(tchar const * _name, bool _defVal) const
{
	if (Attribute const * attribute = get_attribute(_name))
	{
		return attribute->get_as_bool();
	}
	if (auto* child = first_child_named(_name))
	{
		if (child->is_empty())
		{
			return true; // presence;
		}
		return ParserUtils::parse_bool(child->get_internal_text());
	}
	return _defVal;
}

bool Node::has_attribute_or_child(tchar const * _name) const
{
	return has_attribute(_name) || first_child_named(_name);
}

float Node::get_float_attribute_or_from_child(tchar const * _name, float _defVal) const
{
	if (Attribute const * attribute = get_attribute(_name))
	{
		return attribute->get_as_float();
	}
	return get_float_from_child(_name, _defVal);
}

Attribute * const Node::find_attribute(tchar const * _attribute)
{
	Attribute * attribute = firstAttribute;
	while (attribute)
	{
		if (attribute->get_name() == _attribute)
		{
			return attribute;
		}

		attribute = attribute->nextAttribute;
	}

	return nullptr;
}

Name const Node::get_name_attribute_or_from_child(tchar const * _name, Name const & _defVal) const
{
	if (Attribute const * attribute = get_attribute(_name))
	{
		return attribute->get_as_name();
	}
	return get_name_from_child(_name, _defVal);
}

Name const Node::get_name_from_child(tchar const * _childName, Name const & _defVal) const
{
	if (IO::XML::Node const * child = first_child_named(_childName))
	{
		return Name(child->get_internal_text());
	}
	else
	{
		return _defVal;
	}
}

String Node::get_string_from_child(tchar const * _childName, String const & _defVal) const
{
	if (IO::XML::Node const * child = first_child_named(_childName))
	{
		return child->get_internal_text();
	}
	else
	{
		return _defVal;
	}
}

int Node::get_int_from_child(tchar const * _childName, int _defVal) const
{
	if (IO::XML::Node const * child = first_child_named(_childName))
	{
		return ParserUtils::parse_int(child->get_internal_text());
	}
	else
	{
		return _defVal;
	}
}

bool Node::get_bool_from_child(tchar const * _childName, bool _defVal) const
{
	if (IO::XML::Node const * child = first_child_named(_childName))
	{
		return ParserUtils::parse_bool(child->get_internal_text());
	}
	else
	{
		return _defVal;
	}
}

float Node::get_float_from_child(tchar const * _childName, float _defVal) const
{
	if (IO::XML::Node const * child = first_child_named(_childName))
	{
		if (child->get_internal_text().is_empty())
		{
			return _defVal;
		}
		return ParserUtils::parse_float(child->get_internal_text());
	}
	else
	{
		return _defVal;
	}
}

bool Node::save_single_node_to_xml(Interface* const _io, int _indent) const
{
	return save_xml(_io, _indent);
}

RefCountObjectPtr<Node> Node::load_single_node_from_xml(Interface* const _io, Document* _document, REF_ int& _atLine, REF_ bool& _error)
{
	_io->read_char(&_atLine);
	return load_xml(_io, _document, nullptr, _atLine, _error);
}

RefCountObjectPtr<Node> Node::load_xml(Interface * const _io, Document * _document, Node* _soonBeParentNode, REF_ int & _atLine, REF_ bool & _error)
{
	tchar ch = _io->get_last_read_char();

	uint spaces = 0;

	if (ch != '<')
	{
		spaces += _io->omit_spaces_and_special_characters(true, &_atLine);
		ch = _io->get_last_read_char();

		if (ch == 0)
		{
			// end of file - everything's fine
			return RefCountObjectPtr<Node>();
		}
	}

	if (ch == '<')
	{
		// create node
		_io->omit_spaces_and_special_characters(false, &_atLine);

		if (_io->get_last_read_char() == '!')
		{
			// comment, check if two next chars are --
			// and look for following -->
			ch = _io->read_char(&_atLine);
			if (ch != '-')
			{
				error(TXT("[%S at %i] not a comment?"), _document->get_document_info()->loadedFromFile.to_char(), _atLine);
				_error = true;
				return RefCountObjectPtr<Node>();
			}
			ch = _io->read_char(&_atLine);
			if (ch != '-')
			{
				error(TXT("[%S at %i] not a comment?"), _document->get_document_info()->loadedFromFile.to_char(), _atLine);
				_error = true;
				return RefCountObjectPtr<Node>();
			}

			String contentRaw;

			// try to find -->
			int matches = 0;
			while ((ch = _io->read_char(&_atLine)))
			{
				if (ch == '-' && matches < 2)
				{
					++ matches;
				}
				else
				if (ch == '>' && matches == 2)
				{
					// ignore >
					_io->read_char(&_atLine);
					++ matches;
					break;
				}
				else
				{
					contentRaw += ch;
					matches = 0;
				}
			}

			if (matches == 3)
			{
				return RefCountObjectPtr<Node>(new Node(_document, _atLine, contentRaw, NodeType::Comment));
			}
			else
			{
				// syntax error or unexpected end of file
				error(TXT("[%S at %i] syntax error or unexpected end of file"), _document->get_document_info()->loadedFromFile.to_char(), _atLine);
				_error = true;
				return RefCountObjectPtr<Node>();
			}
		}

		bool readChildren = true;
		bool closure = false;

		String nodeName(TXT(""));
		while ((ch = _io->get_last_read_char()))
		{
			if (ch == '>' ||
				(ch == '/' && nodeName != TXT("")) || // read closure
				Document::is_separator(ch))
			{
				break;
			}
			if (ch == '/') // closure
			{
				readChildren = false;
				closure = true;
			} // no else to allow to read / to have whole closure name
			nodeName += ch;
			_io->read_char(&_atLine);
		}

		if (ch == 0)
		{
			// end of file
			error(TXT("[%S at %i] unexpected end of file"), _document->get_document_info()->loadedFromFile.to_char(), _atLine);
			_error = true;
			return RefCountObjectPtr<Node>();
		}

		if (nodeName.is_empty())
		{
			// empty name?
			error(TXT("[%S at %i] no node name"), _document->get_document_info()->loadedFromFile.to_char(), _atLine);
			_error = true;
			return RefCountObjectPtr<Node>();
		}

		// if last was space or special, omit it and all next
		_io->omit_spaces_and_special_characters(true, &_atLine);
		ch = _io->get_last_read_char();

		NodeType::Type nodeType = NodeType::Node;
		if (nodeName.has_prefix(node_commented_out_prefix()))
		{
			nodeType = NodeType::NodeCommentedOut;
			nodeName = nodeName.without_prefix(node_commented_out_prefix());
		}
		RefCountObjectPtr<Node> newNode(new Node(_document, _atLine, nodeName, nodeType));

		if (ch != '/' && ! closure)
		{
			// read attributes
			while (Attribute * attribute = Attribute::load_xml(_io, REF_ _atLine, REF_ _error))
			{
				newNode->push_back_attribute(attribute);
			}

			if (! _io->get_last_read_char())
			{
				// end of file
				newNode.clear();
				error(TXT("[%S at %i] end of file so soon?"), _document->get_document_info()->loadedFromFile.to_char(), _atLine);
				_error = true;
				return newNode;
			}

			// if last was space or special, omit it and all next
			_io->omit_spaces_and_special_characters(true, &_atLine);
			ch = _io->get_last_read_char();
		}
		else if (closure && ch != '>')
		{
			error(TXT("[%S at %i] closure of node \"%S\" can't have attributes"), _document->get_document_info()->loadedFromFile.to_char(), _atLine, newNode->get_name().to_char());
		}

		if (ch == '/')
		{
			// read >
			readChildren = false;
			if (!_io->read_char(&_atLine))
			{
				// end of file
				newNode.clear();
				error(TXT("[%S at %i] end of file so soon?"), _document->get_document_info()->loadedFromFile.to_char(), _atLine);
				_error = true;
				return newNode;
			}
		}

		// if last was space or special, omit it and all next
		_io->omit_spaces_and_special_characters(true, &_atLine);
		ch = _io->get_last_read_char();

		bool commentItOut = false; // to allow checking if it isn't nested load

		if (nodeName == TXT("_load"))
		{
			if (nodeType != NodeType::NodeCommentedOut) // check if wasn't already commented out
			{
				commentItOut = true; // shouldn't be part of the document
				bool skip = false;
				auto* goUp = _soonBeParentNode? _soonBeParentNode->addChildrenOnLoadTo : nullptr;
				while (goUp)
				{
					if (goUp->type == NodeType::NodeCommentedOut)
					{
						skip = true;
					}
					if (goUp->parentNode == goUp)
					{
						break;
					}
					goUp = goUp->parentNode;
				}
				if (skip)
				{
					nodeType = NodeType::NodeCommentedOut; // shouldn't be part of the document
					newNode->type = NodeType::NodeCommentedOut; // we already created newNode
				}
				else
				{
					Name loadFilter = newNode->get_name_attribute(TXT("filter"));
					_document->loadFilters.push_back(loadFilter);
				}
			}
		}
		if (nodeName == TXT("_section"))
		{
			bool skip = nodeType == NodeType::NodeCommentedOut; // maybe we already commented it out
			nodeType = NodeType::NodeCommentedOut; // shouldn't be part of the document
			newNode->type = NodeType::NodeCommentedOut; // we already created newNode
			Name loadFilter = newNode->get_name_attribute(TXT("filter"));
			Name noFilter = newNode->get_name_attribute(TXT("noFilter"));
			if (loadFilter.is_valid() && !_document->loadFilters.does_contain(loadFilter))
			{
				skip = true;
			}
			if (noFilter.is_valid() && _document->loadFilters.does_contain(noFilter))
			{
				skip = true;
			}
			// if we shouldn't load it, just leave it where it is, in the comment
			if (! skip)
			{
				// otherwise move it to where it should be loaded (ie. parent)
				if (_soonBeParentNode)
				{
					newNode->addChildrenOnLoadTo = _soonBeParentNode->addChildrenOnLoadTo;
				}
			}
		}
		if (nodeName == TXT("_include"))
		{
			if (_soonBeParentNode)
			{
				newNode->addChildrenOnLoadTo = _soonBeParentNode->addChildrenOnLoadTo;
			}

			RefCountObjectPtr<Node> incNode;
			Name id = newNode->get_name_attribute(TXT("_id"));
			if (_document->get_includable(id, OUT_ incNode))
			{
				for_every(aConst, incNode->all_attributes())
				{
					if (aConst->get_name() != TXT("_id"))
					{
						if (!newNode->addChildrenOnLoadTo->get_attribute(aConst->get_name()))
						{
							newNode->addChildrenOnLoadTo->set_attribute(aConst->get_name(), aConst->get_as_string());
						}
					}
				}
				// move children to node being loaded
				while (incNode->has_children())
				{
					auto* c = cast_to_nonconst(incNode->first_child());
					RefCountObjectPtr<Node> cPtr(c);
					incNode->remove_node(c);
					newNode->addChildrenOnLoadTo->push_back_node(c);
				}
			}
			else
			{
				error(TXT("[%S at %i] can't read includable \"%S\""), _document->get_document_info()->loadedFromFile.to_char(), _atLine, id.to_char());
				newNode.clear();
				_error = true;
				return RefCountObjectPtr<Node>();
			}
		}

		if (ch == '>')
		{
			// skip >
			_io->read_char(&_atLine);

			bool okToReturn = false;
			if (readChildren)
			{
				String closureName(String(TXT("/")) + nodeName);
				while (true)
				{
					RefCountObjectPtr<Node> child;
					child = Node::load_xml(_io, _document, newNode.get(), REF_ _atLine, REF_ _error);
					if (!child.get())
					{
						break;
					}
					if (child->get_name() == closureName)
					{
						child.clear(); 
						okToReturn = true;
						break;
					}
					else if (child->get_name().get_left(1) == TXT("/"))
					{
						error(TXT("[%S at %i] node \"%S\" ended with closure \"%S\""), _document->get_document_info()->loadedFromFile.to_char(), child->atLine, closureName.to_char(), child->get_name().to_char());
						child.clear();
						_error = true;
						okToReturn = true;
						break;
					}
					newNode->addChildrenOnLoadTo->push_back_node(child.get());
				}
			}
			else
			{
				// checking closure will happen on return
				okToReturn = true;
			}

			if (okToReturn)
			{
				if (nodeName == TXT("_include"))
				{
					// no need to keep it uncommented, it may actually break stuff
					commentItOut = true;
				}

				if (nodeName == TXT("_includable"))
				{
					// change it so we won't try to include it once again
					newNode->content = TXT("_includable_stored");
					newNode->contentRaw = TXT("_includable_stored");
					if (_document->add_includable(newNode->get_name_attribute(TXT("_id")), newNode.get()))
					{
						// but comment it out here
						commentItOut = true;
					}
					else
					{
						_error = true;
					}
				}

				if (commentItOut)
				{
					nodeType = NodeType::NodeCommentedOut; // shouldn't be part of the document
					newNode->type = NodeType::NodeCommentedOut; // we already created newNode
				}

				return newNode;
			}
		}

		newNode.clear();
		error(TXT("[%S at %i] syntax error"), _document->get_document_info()->loadedFromFile.to_char(), _atLine);
		_error = true;
		return RefCountObjectPtr<Node>();
	}
	else
	{
		// text - translate all characters to normal
		String content(spaces ? TXT(" ") : TXT(""));

		while ((ch = _io->get_last_read_char()))
		{
			if (ch == '&')
			{
				tchar add = Document::decode_char(_io, &_atLine);
				if (add != 0)
				{
					content += add;
				}
			}
			else if (Document::is_separator(ch))
			{
				if (content.get_length() > 0)
				{
					if (_io->omit_spaces(true, &_atLine) > 0)
					{
						content += ' ';
						continue;
					}
					else if (ch != '\r')
					{
						content += ch;
					}
				}
			}
			else if (ch == '<')
			{
				break;
			}
			else
			{
				if (_io->has_last_read_char())
				{
					content += ch;
				}
			}

			_io->read_char(&_atLine);
		}

		return RefCountObjectPtr<Node>(new Node(_document, _atLine, content, NodeType::Text));
	}
}

bool Node::save_xml_children(Interface* const _io, bool _includeComments, int _indent) const
{
	if (Node const* node = firstSubNode.get())
	{
		bool allTextNodes = true;
		node = firstSubNode.get();
		while (node)
		{
			if (node->get_type() != NodeType::Text)
			{
				allTextNodes = false;
			}
			node = node->nextNode.get();
		}

		node = firstSubNode.get();
		// save nodes
		while (node)
		{
			bool skip = false;
			if (!_includeComments)
			{
				if (node->get_type() == NodeType::NodeCommentedOut ||
					node->get_type() == NodeType::Comment)
				{
					skip = true;
				}
			}
			// ignore saving empty text nodes
			if (!skip)
			{
				if (node->get_type() != NodeType::Text ||
					!node->get_text().is_empty())
				{
					if (!allTextNodes)
					{
						_io->write_text(TXT("\n"));
						Document::indent(get_document(), _io, _indent + 1);
					}
					if (!node->save_xml(_io, _includeComments, _indent + 1))
					{
						error(TXT("could not save"));
						return false;
					}
				}
			}
			node = node->nextNode.get();
		}

		if (!allTextNodes)
		{
			_io->write_text(TXT("\n"));
			Document::indent(get_document(), _io, _indent);
		}
	}
	return true;
}

bool Node::save_xml(Interface* const _io, bool _includeComments, int _indent) const
{
	if (!_includeComments)
	{
		if (type == NodeType::NodeCommentedOut ||
			type == NodeType::Comment)
		{
			return true;
		}
	}
	switch (type)
	{
	case NodeType::Root:
		{
			if (!save_xml_children(_io, _includeComments, _indent))
			{
				return false;
			}
		}
		break;
	case NodeType::Node:
	case NodeType::NodeCommentedOut:
		{
			_io->write_text(TXT("<"));
			if (type == NodeType::NodeCommentedOut)
			{
				_io->write_text(node_commented_out_prefix());
			}
			_io->write_text(contentRaw);
			if (Attribute const * attribute = firstAttribute)
			{
				while (attribute)
				{
					_io->write_text(TXT(" "));
					if (!attribute->save_xml(_io))
					{
						error(TXT("could not save"));
						return false;
					}
					attribute = attribute->nextAttribute;
				}
			}
			bool anyChildren = firstSubNode.get() != nullptr;
			if (!_includeComments)
			{
				auto* child = firstSubNode.get();
				while (child)
				{
					bool skip = false;
					if (child->get_type() == NodeType::NodeCommentedOut ||
						child->get_type() == NodeType::Comment)
					{
						skip = true;
					}
					if (!skip)
					{
						anyChildren = true;
					}
					child = child->next_sub_node();
				}
			}
			if (anyChildren)
			{
				_io->write_text(TXT(">"));

				if (!save_xml_children(_io, _includeComments, _indent))
				{
					return false;
				}

				_io->write_text(TXT("</"));
				_io->write_text(contentRaw);
				_io->write_text(TXT(">"));
			}
			else
			{
				_io->write_text(TXT("/>"));
			}
		}
		break;
	case NodeType::Comment:
		{
			_io->write_text(TXT("<!--"));
			_io->write_text(contentRaw);
			_io->write_text(TXT("-->"));
		}
		break;
	case NodeType::Text:
		{
			for (int i = 0; i < contentRaw.get_length(); ++i)
			{
				String characterEncoded;
				tchar chRaw = contentRaw[i];
				characterEncoded = Document::encode_char(chRaw);
				_io->write_text(characterEncoded);
			}
		}
		break;
	default:
		break;
	}

	return true;
}

void Node::push_back_node(Node* _node)
{
#ifdef DEBUG_XML_NODE_LIFESPAN
	output(TXT("[xml-node-lifespan] %p add node %p"), this, _node);
#endif
	an_assert(!_node->parentNode);
	an_assert(!_node->nextNode.get());
	_node->parentNode = this;
	Node* lastValid = firstSubNode.get();
	while (lastValid)
	{
		if (auto* n = lastValid->nextNode.get())
		{
			lastValid = n;
		}
		else
		{
			break;
		}
	}
	if (lastValid)
	{
#ifdef DEBUG_XML_NODE_LIFESPAN
		output(TXT("[xml-node-lifespan] %p connect last %p->next = %p"), this, lastValid, _node);
#endif
		lastValid->nextNode = _node;
	}
	else
	{
#ifdef DEBUG_XML_NODE_LIFESPAN
		output(TXT("[xml-node-lifespan] %p connect first %p"), this, _node);
#endif
		firstSubNode = _node;
	}
}

void Node::push_back_attribute(Attribute* _attribute)
{
	if (Attribute* attribute = find_attribute(_attribute->get_name().to_char()))
	{
		delete_attribute(attribute);
	}

	_attribute->parentNode = this;
	Attribute* lastValid = firstAttribute;
	while (lastValid && lastValid->nextAttribute)
	{
		lastValid = lastValid->nextAttribute;
	}
	if (lastValid)
	{
		lastValid->nextAttribute = _attribute;
	}
	else
	{
		firstAttribute = _attribute;
	}
}

void Node::remove_node(Node* _node)
{
#ifdef DEBUG_XML_NODE_LIFESPAN
	output(TXT("[xml-node-lifespan] %p remove node %p"), this, _node);
#endif
	an_assert(_node->parentNode == this);
	_node->parentNode = nullptr;
	Node* prev = nullptr;
	Node* curr = firstSubNode.get();
	while (curr)
	{
		if (curr == _node)
		{
			// store temporarily as we're moving around and clearing this node as well
			RefCountObjectPtr<Node> keepNode(_node);
			if (prev)
			{
#ifdef DEBUG_XML_NODE_LIFESPAN
				output(TXT("[xml-node-lifespan] %p reconnect prev %p->next = %p"), this, prev, curr->nextNode.get());
#endif
				prev->nextNode = curr->nextNode;
			}
			else
			{
#ifdef DEBUG_XML_NODE_LIFESPAN
				output(TXT("[xml-node-lifespan] %p reconnect first %p (was %p)"), this, curr->nextNode.get(), firstSubNode.get());
#endif
				firstSubNode = curr->nextNode;
			}
			_node->nextNode.clear();
			return;
		}
		prev = curr;
		curr = curr->nextNode.get();
	}
}

void Node::delete_attribute(Attribute* _attribute)
{
	Attribute* prev = nullptr;
	Attribute* curr = firstAttribute;
	while (curr)
	{
		if (curr == _attribute)
		{
			if (prev)
			{
				prev->nextAttribute = curr->nextAttribute;
			}
			else
			{
				firstAttribute = curr->nextAttribute;
			}
			delete _attribute;
			return;
		}
		prev = curr;
		curr = curr->nextAttribute; 
	}
}

bool Node::has_text() const
{
	Node* child = firstSubNode.get();
	while (child)
	{
		if (child->get_type() == NodeType::Text)
		{
			return true;
		}
		child = child->next_sub_node();
	}
	return false;
}

bool Node::has_children() const
{
	Node const * child = firstSubNode.get();
	while (child)
	{
		if (child->get_type() == NodeType::Node)
		{
			return true;
		}
		child = child->next_sub_node();
	}
	return false;
}

Node * const Node::first_child()
{
	Node* child = firstSubNode.get();
	while (child)
	{
		if (child->get_type() == NodeType::Node)
		{
			return child;
		}
		child = child->next_sub_node();
	}
	return nullptr;
}

Node const * const Node::first_child() const
{
	Node const * child = firstSubNode.get();
	while (child)
	{
		if (child->get_type() == NodeType::Node)
		{
			return child;
		}
		child = child->next_sub_node();
	}
	return nullptr;
}

Node * const Node::next_sibling()
{
	Node* sibling = next_sub_node();
	while (sibling)
	{
		if (sibling->get_type() == NodeType::Node)
		{
			return sibling;
		}
		sibling = sibling->next_sub_node();
	}
	return nullptr;
}

Node const * const Node::next_sibling() const
{
	Node const * sibling = next_sub_node();
	while (sibling)
	{
		if (sibling->get_type() == NodeType::Node)
		{
			return sibling;
		}
		sibling = sibling->next_sub_node();
	}
	return nullptr;
}

Node * const Node::first_child_named(tchar const * _childName, tchar const * _altChildName)
{
	Node* child = firstSubNode.get();
	while (child)
	{
		if (child->get_name() == _childName)
		{
			return child;
		}
		child = child->next_sub_node();
	}
	return _altChildName ? first_child_named(_altChildName) : nullptr;
}

Node const * const Node::first_child_named(tchar const * _childName, tchar const * _altChildName) const
{
	Node const * child = firstSubNode.get();
	while (child)
	{
		if (child->get_name() == _childName)
		{
			return child;
		}
		child = child->next_sub_node();
	}
	return _altChildName ? first_child_named(_altChildName) : nullptr;
}

int Node::get_children_named(Array<Node const*>& children, tchar const * _childName, tchar const * _altChildName) const
{
	Node const * child = firstSubNode.get();
	while (child)
	{
		if (child->get_name() == _childName ||
			(_altChildName && child->get_name() == _altChildName))
		{
			children.push_back(child);
		}
		child = child->next_sub_node();
	}
	return children.get_size();
}

Node * const Node::next_sibling_named(tchar const * _siblingName, tchar const * _altSiblingName)
{
	Node* sibling = next_sub_node();
	while (sibling)
	{
		if (sibling->get_name() == _siblingName)
		{
			return sibling;
		}
		sibling = sibling->next_sub_node();
	}
	return _altSiblingName ? next_sibling_named(_altSiblingName) : nullptr;
}

Node const * const Node::next_sibling_named(tchar const * _siblingName, tchar const * _altSiblingName) const
{
	Node const * sibling = next_sub_node();
	while (sibling)
	{
		if (sibling->get_name() == _siblingName)
		{
			return sibling;
		}
		sibling = sibling->next_sub_node();
	}
	return _altSiblingName ? next_sibling_named(_altSiblingName) : nullptr;
}

String Node::get_internal_text() const
{
	String result = String::empty();
	Node const * child = first_sub_node();
	while (child)
	{
		if (child->get_type() == NodeType::Text)
		{
			if (! result.is_empty())
			{
				result += TXT(" ");
			}
			result += child->get_text();
		}
		child = child->next_sub_node();
	}
	return result;
}

bool Node::is_empty() const
{
	Node const * child = first_sub_node();
	while (child)
	{
		if (child->get_type() == NodeType::Text ||
			child->get_type() == NodeType::Node)
		{
			return false;
		}
		child = child->next_sub_node();
	}

	return true;
}

String Node::get_internal_text_raw() const
{
	String result = String::empty();
	Node const * child = first_sub_node();
	while (child)
	{
		if (child->get_type() == NodeType::Text)
		{
			if (!result.is_empty())
			{
				result += TXT(" ");
			}
			result += child->get_text_raw();
		}
		child = child->next_sub_node();
	}
	return result;
}

bool Node::serialise(Serialiser & _serialiser, int _version)
{
	bool result = true;

	if (_serialiser.is_reading())
	{
		clear();
	}

	if (_version >= VER_LINE_NUMBERS)
	{
		result &= serialise_data(_serialiser, atLine);
	}
	result &= serialise_data(_serialiser, type);
	result &= serialise_data(_serialiser, contentRaw);

	if (_serialiser.is_reading())
	{
		content = contentRaw;
		raw_to_normal(content);
	}

	// store attributes
	int attrCount = 0;
	if (_serialiser.is_writing())
	{
		Attribute const * attr = first_attribute();
		while (attr)
		{
			++attrCount;
			attr = attr->nextAttribute;
		}
	}

	result &= serialise_data(_serialiser, attrCount);

	if (_serialiser.is_writing())
	{
		Attribute * attr = first_attribute();
		while (attr)
		{
			result &= serialise_data(_serialiser, attr->name);
			result &= serialise_data(_serialiser, attr->value);
			attr = attr->nextAttribute;
		}
	}
	else
	{
		while (attrCount > 0)
		{
			String name;
			String value;
			if (serialise_data(_serialiser, name) &&
				serialise_data(_serialiser, value))
			{
				set_attribute(name, value);
			}
			else
			{
				return false;
			}
			--attrCount;
		}
	}

	// store children
	int childCount = 0;
	if (_serialiser.is_writing())
	{
		Node const * child = first_sub_node();
		while (child)
		{
			if (child->type != NodeType::Comment &&
				child->type != NodeType::NodeCommentedOut)
			{
				++childCount;
			}
			child = child->nextNode.get();
		}
	}

	result &= serialise_data(_serialiser, childCount);

	if (_serialiser.is_writing())
	{
		Node * child = first_sub_node();
		while (child)
		{
			if (child->type != NodeType::Comment &&
				child->type != NodeType::NodeCommentedOut)
			{
				result &= child->serialise(_serialiser, _version);
			}
			child = child->nextNode.get();
		}
	}
	else
	{
		while (childCount > 0)
		{
			RefCountObjectPtr<Node> child(new Node(document.get()));
			if (child->serialise(_serialiser, _version))
			{
				add_node(child.get());
			}
			else
			{
				child.clear();
				result = false;
			}
			--childCount;
		}
	}

	return result;
}

String Node::to_string_slow(bool _includeComments) const
{
	IO::MemoryStorage ms;
	ms.set_type(IO::InterfaceType::Text);
	save_xml(&ms, _includeComments);
	ms.go_to(0);
	String asString;
	ms.read_into_string(asString);
	return asString;
}
