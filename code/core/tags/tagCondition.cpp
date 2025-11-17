#include "tagCondition.h"

#include "..\containers\arrayStack.h"
#include "..\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

TagCondition TagCondition::s_empty;

TagCondition::TagCondition()
: element(nullptr)
{
}

TagCondition::TagCondition(TagCondition const & _other)
: element(nullptr)
{
	operator=(_other);
}

TagCondition::TagCondition(String const & _tagConditionAsString)
: element(nullptr)
{
	load_from_string(_tagConditionAsString);
}

TagCondition &TagCondition::operator=(TagCondition const & _other)
{
	clear();
	if (_other.element)
	{
		memory_leak_suspect;
		element = _other.element->create_copy();
		forget_memory_leak_suspect;
	}
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
	return *this;
}

TagCondition::~TagCondition()
{
	clear();
}

void TagCondition::clear()
{
	delete_and_clear(element);
}

void TagCondition::setup_with_tags(Tags const& _tags)
{
	clear();
	String tags;
	_tags.do_for_every_tag([&tags](Tag const& _tag)
		{
			if (!tags.is_empty())
			{
				tags += String::space();
			}
			tags += _tag.get_tag().to_string();
		});

	load_from_string(tags);
}

void TagCondition::combine_with(TagCondition const & _other, TagConditionOperator::Type _withOperator)
{
	if (element != nullptr && _other.element != nullptr)
	{
		Element* newElement = new Element(Element::get_type(_withOperator));
		newElement->add_arg(element);
		memory_leak_suspect;
		newElement->add_arg(_other.element->create_copy());
		forget_memory_leak_suspect;
		element = newElement;
	}
	else
	{
		if (_other.element != nullptr)
		{
			an_assert(element == nullptr);
			memory_leak_suspect;
			element = _other.element->create_copy();
			forget_memory_leak_suspect;
		}
	}
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
}
	
bool TagCondition::load_from_xml(IO::XML::Node const* _node)
{
	return load_from_string(_node->get_text());
}

bool TagCondition::load_from_xml(IO::XML::Attribute const* _attr)
{
	return load_from_string(_attr->get_value());
}

bool TagCondition::load_from_xml_attribute_or_node(IO::XML::Node const* _node, String const& _attributeName)
{
	return load_from_xml_attribute_or_node(_node, _attributeName.to_char());
}

bool TagCondition::load_from_xml_attribute_or_node(IO::XML::Node const* _node, tchar const* _attributeName)
{
	if (IO::XML::Attribute const* attributeNode = _node->get_attribute(_attributeName))
	{
		return load_from_string(attributeNode->get_value());
	}
	return load_from_string(_node->get_text());
}

bool TagCondition::load_from_xml_attribute_or_child_node(IO::XML::Node const * _node, tchar const * _childNodeAttributeName, TagConditionOperator::Type _defaultOperator)
{
	if (!_node)
	{
		return true;
	}
	if (IO::XML::Node const * childNode = _node->first_child_named(_childNodeAttributeName))
	{
		return load_from_string(childNode->get_text(), _defaultOperator);
	}
	else if (IO::XML::Attribute const * attributeNode = _node->get_attribute(_childNodeAttributeName))
	{
		return load_from_string(attributeNode->get_value(), _defaultOperator);
	}
	else
	{
		return true;
	}
}

bool TagCondition::load_from_xml_attribute(IO::XML::Node const * _node, tchar const * _attributeName, TagConditionOperator::Type _defaultOperator)
{
	if (!_node)
	{
		return true;
	}
	if (IO::XML::Attribute const * attributeNode = _node->get_attribute(_attributeName))
	{
		return load_from_string(attributeNode->get_value(), _defaultOperator);
	}
	else
	{
		return true;
	}
}

String TagCondition::to_string() const
{
	return element ? element->to_string().compress() : String(TXT(""));
}

bool TagCondition::load_from_string(String const & _input, TagConditionOperator::Type _defaultOperator)
{
	List<Token> tokens;
	Token::load_from_string(_input, tokens);
	element = Element::load(tokens, Element::get_type(_defaultOperator));
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
	return true; // might be empty
}

#ifdef AN_NAT_VIS
void TagCondition::update_for_nat_vis()
{
	natVis = to_string();
	if (natVis.is_empty())
	{
		natVis = TXT("--");
	}
}
#endif

//

TagCondition::Element::Type TagCondition::Element::get_type(TagConditionOperator::Type _operator)
{
	if (_operator == TagConditionOperator::And)
	{
		return Type::And;
	}
	an_assert(_operator == TagConditionOperator::Or);
	return Type::Or;
}

TagCondition::Element* TagCondition::Element::create_copy() const
{
	Element* newElement = new Element(type);
	newElement->tag = tag;
	newElement->value = value;
	if (argumentA != nullptr)
	{
		newElement->add_arg(argumentA->create_copy());
	}
	if (argumentB != nullptr)
	{
		newElement->add_arg(argumentB->create_copy());
	}
	return newElement;
}

TagCondition::Element::Element(Type _type)
: type(_type)
, argumentA(nullptr)
, argumentB(nullptr)
, parent(nullptr)
{
}
			
TagCondition::Element::Element(Token const & _token)
: argumentA(nullptr)
, argumentB(nullptr)
, parent(nullptr)
{
	if (_token.get_type() == Token::Type::Add)
	{
		type = Type::Add;
	}
	else if (_token.get_type() == Token::Type::And)
	{
		type = Type::And;
	}
	else if (_token.get_type() == Token::Type::Or)
	{
		type = Type::Or;
	}
	else if (_token.get_type() == Token::Type::Not)
	{
		type = Type::Not;
	}
	else if (_token.get_type() == Token::Type::Tag)
	{
		type = Type::Tag;
		tag = _token.get_tag();
	}
	else if (_token.get_type() == Token::Type::Value)
	{
		type = Type::Value;
		value = _token.get_value();
	}
	else if (_token.get_type() == Token::Type::Equal)
	{
		type = Type::Equal;
	}
	else if (_token.get_type() == Token::Type::LessThan)
	{
		type = Type::LessThan;
	}
	else if (_token.get_type() == Token::Type::LessOrEqual)
	{
		type = Type::LessOrEqual;
	}
	else if (_token.get_type() == Token::Type::GreaterThan)
	{
		type = Type::GreaterThan;
	}
	else if (_token.get_type() == Token::Type::GreaterOrEqual)
	{
		type = Type::GreaterOrEqual;
	}
	else
	{
		error(TXT("can't decide what to put here"));
	}
}
			
TagCondition::Element::~Element()
{
	delete_and_clear(argumentA);
	delete_and_clear(argumentB);
}

TagCondition::Element* TagCondition::Element::load(List<Token> const & _tokens, Type _defaultOperator)
{
	int index = 0;
	int openBrackets = 0;
	bool error = false;
	Element* loadedElement = sub_load(_tokens, _defaultOperator, REF_ index, REF_ openBrackets, REF_ error, false);
	an_assert(!loadedElement || !loadedElement->parent);
	return loadedElement;
}
			
TagCondition::Element* TagCondition::Element::sub_load(List<Token> const & _tokens, Type _defaultOperator, REF_ int & _index, REF_ int & _openBrackets, REF_ bool & _error, bool _loadSingle)
{
	an_assert(_defaultOperator == Type::And || _defaultOperator == Type::Or);
	bool topLevel = _index == 0;
	Element* element = nullptr;
	for(; _index < _tokens.get_size();)
	{
		auto * token = &_tokens[_index];
		if (token->get_type() == Token::Type::OpenBracket)
		{
			++ _openBrackets;
			++ _index;
			Element* inBrackets = sub_load(_tokens, _defaultOperator, REF_ _index, REF_ _openBrackets, REF_ _error, false);
			if (inBrackets == nullptr)
			{
				error(TXT("could not load in brackets"));
				_error = true;
				break;
			}
			else
			{
				if (element != nullptr)
				{
					element->add_arg_def(inBrackets, _defaultOperator, REF_ _error);
				}
				element = inBrackets;
			}
			if (_loadSingle)
			{
				break;
			}
		}
		else if (token->get_type() == Token::Type::CloseBracket)
		{
			--_openBrackets;
			if (topLevel || _openBrackets < 0)
			{
				error(TXT("can't close bracket"));
				_error = true;
				break;
			}
			// advance to next, break, return composed element
			++ _index;
			break;
		}
		else if (token->get_type() == Token::Type::Not)
		{
			Element* notElement = new Element(*token);
			++_index;
			Element* argElement = sub_load(_tokens, _defaultOperator, REF_ _index, REF_ _openBrackets, REF_ _error, true);
			if (notElement && argElement)
			{
				notElement->argumentA = argElement;
				argElement->parent = notElement;
			}
			else
			{
				error(TXT("could not load argument"));
				_error = true;
			}
			if (element != nullptr)
			{
				element->add_arg_def(notElement, _defaultOperator, REF_ _error);
			}
			else
			{
				element = notElement;
			}
			if (_loadSingle)
			{
				break;
			}
		}
		else if (token->get_type() == Token::Type::Tag ||
				 token->get_type() == Token::Type::Value)
		{
			Element* tagElement = new Element(*token);
			if (element != nullptr)
			{
				element->add_arg_def(tagElement, _defaultOperator, REF_ _error);
			}
			element = tagElement;
			++_index;
			if (_loadSingle)
			{
				break;
			}
			if (_index < _tokens.get_size())
			{
				auto* token = &_tokens[_index];
				if (token->get_type() == Token::Type::Add ||
					token->get_type() == Token::Type::And ||
					token->get_type() == Token::Type::Or ||
					token->get_type() == Token::Type::Equal ||
					token->get_type() == Token::Type::LessThan ||
					token->get_type() == Token::Type::LessOrEqual ||
					token->get_type() == Token::Type::GreaterThan ||
					token->get_type() == Token::Type::GreaterOrEqual)
				{
					// skip to next if we follow with an operator
					continue;
				}
			}
		}
		else if (token->get_type() == Token::Type::Add ||
				 token->get_type() == Token::Type::And ||
				 token->get_type() == Token::Type::Or ||
				 token->get_type() == Token::Type::Equal ||
				 token->get_type() == Token::Type::LessThan ||
				 token->get_type() == Token::Type::LessOrEqual ||
				 token->get_type() == Token::Type::GreaterThan ||
				 token->get_type() == Token::Type::GreaterOrEqual)
		{
			Element* operatorElement = new Element(*token);
			if (!element)
			{
				error(TXT("can't start with and/or"));
				_error = true;
				break;
			}
			if (element->parent == nullptr)
			{
				operatorElement->add_arg_def(element, _defaultOperator, REF_ _error);
			}
			else
			{
				element->parent->replace_arg_with(element, operatorElement, REF_ _error);
			}
			element = operatorElement;
			++_index;
			if (!element->argumentA || element->argumentB)
			{
				error(TXT("did not expect that"));
				_error = true;
			}
			else
			{
				Element* argElement = sub_load(_tokens, _defaultOperator, REF_ _index, REF_ _openBrackets, REF_ _error, true);
				if (argElement)
				{
					element->argumentB = argElement;
				}
				else
				{
					error(TXT("could not load argument"));
					_error = true;
				}
			}
			if (token->get_type() == Token::Type::Equal ||
				token->get_type() == Token::Type::LessThan ||
				token->get_type() == Token::Type::LessOrEqual ||
				token->get_type() == Token::Type::GreaterThan ||
				token->get_type() == Token::Type::GreaterOrEqual)
			{
				if (element->argumentA && element->argumentB)
				{
					if (element->argumentA->get_type() != Type::Tag && element->argumentA->get_type() != Type::Value && element->argumentA->get_type() != Type::Add)
					{
						error(TXT("expected tag, value or sum (add) for left side"));
						_error = true;
					}
					if (element->argumentB->get_type() != Type::Tag && element->argumentB->get_type() != Type::Value && element->argumentB->get_type() != Type::Add)
					{
						error(TXT("expected tag, value or sum (add) for right side"));
						_error = true;
					}
				}
			}
		}
		else
		{
			// just skip it?
			if (element == nullptr)
			{
				_error = true;
				break;
			}
			++_index;
		}
		while (element && element->parent)
		{
			element = element->parent;
		}
	}
	if (element != nullptr && ! _error)
	{
		while (element->parent != nullptr)
		{
			element = element->parent;
		}
		return element;
	}
	else
	{
		return nullptr;
	}
}
			
void TagCondition::Element::add_arg(Element* _arg)
{
	bool tempError = false;
	add_arg_def(_arg, Type::Or /* not important */, REF_ tempError);
}

void TagCondition::Element::add_arg_err(Element* _arg, REF_ bool & _error)
{
	add_arg_def(_arg, Type::Or /* not important */, REF_ _error);
}

bool TagCondition::Element::is_complete() const
{
	return type == Type::Tag ||
		  (type == Type::Not && argumentA) ||
		  ((type == Type::Add || type == Type::And || type == Type::Or || type == Type::Equal || type == Type::LessThan || type == Type::LessOrEqual || type == Type::GreaterThan || type == Type::GreaterOrEqual) && argumentA && argumentB);
}

void TagCondition::Element::add_arg_def(Element* _arg, Type _defaultOperator, REF_ bool & _error)
{
	if (_arg != nullptr)
	{
		if (is_complete() ||
			(type != Type::Add &&
			 type != Type::And &&
			 type != Type::Or &&
			 type != Type::Not &&
			 type != Type::Equal &&
			 type != Type::LessThan &&
			 type != Type::LessOrEqual &&
			 type != Type::GreaterThan &&
			 type != Type::GreaterOrEqual))
		{
			Element* operatorElement = new Element(_defaultOperator);
			if (parent == nullptr)
			{
				operatorElement->add_arg_def(this, _defaultOperator, REF_ _error);
			}
			else
			{
				parent->replace_arg_with(this, operatorElement, REF_ _error);
			}
			operatorElement->add_arg_def(_arg, _defaultOperator, REF_ _error);
		}
		else
		{
			_arg->parent = this;
			if (argumentA == nullptr)
			{
				argumentA = _arg;
			}
			else if (argumentB == nullptr && type != Type::Not)
			{
				argumentB = _arg;
			}
			else
			{
				error(TXT("could not match arg"));
				_error = true;
			}
		}
	}
}
		
void TagCondition::Element::remove_from_parent()
{
	if (parent != nullptr)
	{
		if (parent->argumentA == this)
		{
			parent->argumentA = nullptr;
		}
		else if (parent->argumentB == this)
		{
			parent->argumentB = nullptr;
		}
		parent = nullptr;
	}
}

void TagCondition::Element::replace_arg_with(Element* _old, Element* _new, REF_ bool & _error)
{
	if (_new != nullptr)
	{
		if (argumentA == _old)
		{
			_old->remove_from_parent();
			_new->add_arg_err(_old, REF_ _error);
			argumentA = _new;
			_new->parent = this;
		}
		else if (argumentB == _old)
		{
			_old->remove_from_parent();
			_new->add_arg_err(_old, REF_ _error);
			argumentB = _new;
			_new->parent = this;
		}
		else
		{
			an_assert(false);
		}
	}
}

String TagCondition::Element::to_string() const
{
	if (type == Tag)
	{
		return tag.to_string();
	}
	String result;

	if (argumentA)
	{
		result = result + String(TXT("(")) + argumentA->to_string() + String(TXT(")"));
	}
	switch (type)
	{
	case Add: result = result + TXT(" + "); break;
	case And: result = result + TXT(" and "); break;
	case Or: result = result + TXT(" or "); break;
	case Equal: result = result + TXT(" == "); break;
	case LessThan: result = result + TXT(" < "); break;
	case LessOrEqual: result = result + TXT(" <= "); break;
	case GreaterThan: result = result + TXT(" > "); break;
	case GreaterOrEqual: result = result + TXT(" >= "); break;
	case Not: result = String(TXT("not ")) + result; break;
	case Tag: break;
	case Value: result = result + String::printf(TXT("%.3f"), value); break;
	}
	if (argumentB)
	{
		an_assert(type != Not);
		result = result + String(TXT("(")) + argumentB->to_string() + String(TXT(")"));
	}
	return result;
}

//

TagCondition::Token::Token(Type _type)
: type(_type)
{
	type = _type;
}
			
TagCondition::Token::Token(String const & _tag)
{
	type = Token::Type::Tag;
	tag = Name(_tag);
}
			
TagCondition::Token::Token(float _value)
{
	type = Token::Type::Value;
	value = _value;
}

void TagCondition::Token::load_from_string(String const & _input, OUT_ List<Token> & _tokens)
{
	List<String> inTokens;
	_input.replace(String(TXT("(")), String(TXT(" ( ")))
		  .replace(String(TXT(")")), String(TXT(" ) ")))
		  .replace(String(TXT("!")), String(TXT(" ! ")))
		  .replace(String(TXT("||")), String(TXT(" || ")))
		  .replace(String(TXT("&&")), String(TXT(" && ")))
		  .replace(String(TXT(">=")), String(TXT(" >= ")))
		  .replace(String(TXT("<=")), String(TXT(" <= ")))
		  .replace(String(TXT(">")), String(TXT(" > ")))
		  .replace(String(TXT("<")), String(TXT(" < ")))
		  .replace(String(TXT("=")), String(TXT(" = ")))
		  .replace(String(TXT("+")), String(TXT(" + ")))
		  .compress()
		  .replace(String(TXT("= =")), String(TXT("==")))
		  .replace(String(TXT("> =")), String(TXT(">=")))
		  .replace(String(TXT("< =")), String(TXT("<=")))
		  .compress()
		  .trim()
		  .split(String::space(), inTokens);
	if (inTokens.get_size() == 1 &&
		inTokens.get_first().is_empty())
	{
		// no tokens
		return;
	}
	for_every(token, inTokens)
	{
		if (*token == TXT("=") ||
			*token == TXT("=="))
		{
			_tokens.push_back(Token(Token::Type::Equal));
		}
		else if (*token == TXT("("))
		{
			_tokens.push_back(Token(Token::Type::OpenBracket));
		}
		else if (*token == TXT(")"))
		{
			_tokens.push_back(Token(Token::Type::CloseBracket));
		}
		else if (*token == TXT("<"))
		{
			_tokens.push_back(Token(Token::Type::LessThan));
		}
		else if (*token == TXT("<="))
		{
			_tokens.push_back(Token(Token::Type::LessOrEqual));
		}
		else if (*token == TXT(">"))
		{
			_tokens.push_back(Token(Token::Type::GreaterThan));
		}
		else if (*token == TXT(">="))
		{
			_tokens.push_back(Token(Token::Type::GreaterOrEqual));
		}
		else if (String::compare_icase(*token, TXT("+")))
		{
			_tokens.push_back(Token(Token::Type::Add));
		}
		else if (String::compare_icase(*token, TXT("and")) ||
				 String::compare_icase(*token, TXT("&&")))
		{
			_tokens.push_back(Token(Token::Type::And));
		}
		else if (String::compare_icase(*token, TXT("or")) ||
				 String::compare_icase(*token, TXT("||")))
		{
			_tokens.push_back(Token(Token::Type::Or));
		}
		else if (String::compare_icase(*token, TXT("not")) ||
				 String::compare_icase(*token, TXT("no")) ||
				 String::compare_icase(*token, TXT("!")))
		{
			_tokens.push_back(Token(Token::Type::Not));
		}
		else
		{
			// value or tag?
			bool numerical = false;
			bool nonNumerical = false;
			for_every(ch, token->get_data())
			{
				if (*ch == 0)
				{
					break;
				}
				if ((*ch >= '0' && *ch <= '9') ||
					*ch == '.' ||
					*ch == '-')
				{
					numerical = true;
					continue;
				}
				nonNumerical = true;
				break;
			}
			if (nonNumerical || !numerical)
			{
				_tokens.push_back(Token(*token));
			}
			else
			{
				_tokens.push_back(Token(ParserUtils::parse_float(*token)));
			}
		}
	}
}
