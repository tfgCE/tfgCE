#include "tag.h"

#include "..\other\parserUtils.h"

#include "..\serialisation\serialiser.h"

//

bool Tag::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, tag);
	result &= serialise_data(_serialiser, relevance);
	result &= serialise_data(_serialiser, relevanceAsInt);

	return result;
}


//

Tags Tags::none;

void Tags::add_tags_by_relevance(Tags const& _other)
{
	auto* src = &_other;
	while (src)
	{
#ifdef USE_MORE_TAGS
		if (src->useMoreTags)
		{
			for_every(tag, src->moreTags)
			{
				set_tag(tag->get_tag(), tag->get_relevance() + get_tag(tag->get_tag()));
			}
		}
		else
#endif
		{
			for_every(tag, src->tags)
			{
				set_tag(tag->get_tag(), tag->get_relevance() + get_tag(tag->get_tag()));
			}
		}
		src = src->baseTags;
	}
}

void Tags::set_tags_from(Tags const & _other)
{
	auto* src = &_other;
	while (src)
	{
#ifdef USE_MORE_TAGS
		if (src->useMoreTags)
		{
			for_every(tag, src->moreTags)
			{
				set_tag(tag->get_tag(), tag->get_relevance());
			}
		}
		else
#endif
		{
			for_every(tag, src->tags)
			{
				set_tag(tag->get_tag(), tag->get_relevance());
			}
		}
		src = src->baseTags;
	}
}

bool Tags::load_from_xml(IO::XML::Node const * _node)
{
	return load_from_string(_node->get_text());
}

bool Tags::load_from_xml(IO::XML::Node const * _node, String const & _attributeChildrenName)
{
	return load_from_xml(_node, _attributeChildrenName.to_char());
}

bool Tags::load_from_xml(IO::XML::Node const * _node, tchar const * _attributeChildrenName)
{
	if (!_node)
	{
		return false;
	}
	bool result = load_from_string(_node->get_string_attribute(_attributeChildrenName));
	for_every(tagsNode, _node->children_named(_attributeChildrenName))
	{
		result = load_from_string(tagsNode->get_text()) && result;
	}
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
	return result;
}

bool Tags::load_from_xml_attribute_or_node(IO::XML::Node const * _node, String const & _attributeName)
{
	return load_from_xml_attribute_or_node(_node, _attributeName.to_char());
}

bool Tags::load_from_xml_attribute_or_node(IO::XML::Node const * _node, tchar const * _attributeName)
{
	if (!_node)
	{
		return false;
	}
	bool result = load_from_string(_node->get_string_attribute(_attributeName));
	result &= load_from_string(_node->get_text());
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
	return result;
}

bool Tags::load_from_xml_attribute_or_child_node(IO::XML::Node const * _node, String const & _attributeOrChildName)
{
	return load_from_xml_attribute_or_child_node(_node, _attributeOrChildName.to_char());
}

bool Tags::load_from_xml_attribute_or_child_node(IO::XML::Node const * _node, tchar const * _attributeOrChildName)
{
	if (!_node)
	{
		return false;
	}
	bool result = true;
	if (_node->has_attribute(_attributeOrChildName))
	{
		result &= load_from_string(_node->get_string_attribute(_attributeOrChildName));
	}
	for_every(childNode, _node->children_named(_attributeOrChildName))
	{
		result &= load_from_string(childNode->get_text());
	}
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
	return result;
}

bool Tags::load_from_string(String const & _tags)
{
	bool result = true;
	List<String> tokens;
	_tags.replace(String(TXT("=")), String(TXT(" = "))).compress().split(String::space(), tokens);
	Name currentTag = Name::invalid();
	bool lastWasAssignment = false;
	bool removeTag = false;
	for_every(token, tokens)
	{
		if (*token == TXT("="))
		{
			if (! lastWasAssignment)
			{
				lastWasAssignment = true;
			}
			else
			{
				// TODO error
				result = false;
			}
		}
		else
		{
			if (lastWasAssignment)
			{
				if (currentTag.is_valid() && ! removeTag)
				{
					set_tag(currentTag, ParserUtils::parse_float(*token));
				}
				currentTag = Name::invalid();
			}
			else
			{
				if (token->has_prefix(TXT("-")))
				{
					currentTag = Name(&token->get_data()[1]);
					removeTag = true;
				}
				else
				{
					currentTag = Name(*token);
					removeTag = false;
				}
				if (currentTag.is_valid())
				{
					if (removeTag)
					{
						remove_tag(currentTag);
					}
					else
					{
						set_tag(currentTag);
					}
				}
			}
			lastWasAssignment = false;
		}
	}
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
	return result;
}

void Tags::do_for_every_tag(DoForEachTag _do_for_every_tag) const
{
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every(tag, moreTags)
		{
			_do_for_every_tag(*tag);
		}
	}
	else
#endif
	{
		for_every(tag, tags)
		{
			_do_for_every_tag(*tag);
		}
	}

	if (baseTags)
	{
		baseTags->do_for_every_tag(_do_for_every_tag);
	}
}

String Tags::to_string_all(bool _withZeroRelevance) const
{
	String result;

	auto* t = this;
	while (t)
	{
		String add = t->to_string(_withZeroRelevance);
		if (!add.is_empty())
		{
			if (!result.is_empty())
			{
				result += ' ';
			}
			result += add;
		}
		t = t->baseTags;
	}

	return result;
}

String Tags::to_string(bool _withZeroRelevance) const
{
	String result;
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every(tag, moreTags)
		{
			if (tag->get_relevance() != 0.0f || _withZeroRelevance)
			{
				if (!result.is_empty())
				{
					result += ' ';
				}
				result += tag->get_tag().to_char();
				if (tag->get_relevance() == 0.0f)
				{
					result += TXT("=0");
				}
				else if (tag->get_relevance() != 1.0f)
				{
					result += '=';
					result += ParserUtils::float_to_string_auto_decimals(tag->get_relevance(), 3).to_char();
				}
			}
		}
	}
	else
#endif
	{
		for_every(tag, tags)
		{
			if (tag->get_relevance() != 0.0f || _withZeroRelevance)
			{
				if (!result.is_empty())
				{
					result += ' ';
				}
				result += tag->get_tag().to_char();
				if (tag->get_relevance() == 0.0f)
				{
					result += TXT("=0");
				}
				else if (tag->get_relevance() != 1.0f)
				{
					result += '=';
					result += ParserUtils::float_to_string_auto_decimals(tag->get_relevance(), 3).to_char();
				}
			}
		}
	}
	return result;
}

String Tags::to_string_with_minus() const
{
	String result;
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every(tag, moreTags)
		{
			if (!result.is_empty())
			{
				result += ' ';
			}
			if (tag->get_relevance() == 0.0f)
			{
				result += TXT("-");
			}
			result += tag->get_tag().to_char();
			if (tag->get_relevance() != 0.0f && tag->get_relevance() != 1.0f)
			{
				result += '=';
				result += ParserUtils::float_to_string_auto_decimals(tag->get_relevance(), 3).to_char();
			}
		}
	}
	else
#endif
	{
		for_every(tag, tags)
		{
			if (!result.is_empty())
			{
				result += ' ';
			}
			if (tag->get_relevance() == 0.0f)
			{
				result += TXT("-");
			}
			result += tag->get_tag().to_char();
			if (tag->get_relevance() != 0.0f && tag->get_relevance() != 1.0f)
			{
				result += '=';
				result += ParserUtils::float_to_string_auto_decimals(tag->get_relevance(), 3).to_char();
			}
		}
	}
	return result;
}

#ifdef AN_NAT_VIS
void Tags::update_for_nat_vis()
{
	natVis = to_string(true);
	if (natVis.is_empty())
	{
		natVis = TXT("--");
	}
}
#endif

bool Tags::serialise(Serialiser & _serialiser)
{
	bool result = true;

	an_assert(!baseTags, TXT("not supported!"));
	
#ifdef USE_MORE_TAGS
	result &= serialise_data(_serialiser, useMoreTags);
	if (useMoreTags)
	{
		result &= serialise_data(_serialiser, moreTags);
	}
	else
#endif
	{
		result &= serialise_data(_serialiser, tags);
	}

	return result;
}
