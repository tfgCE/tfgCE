#pragma once

#include "..\io\xml.h"
#include "..\other\registeredType.h"

#include "optionalNP.h"

template <typename Element>
struct Optional
{
public:
	Optional();
	Optional(NotProvided const & _notProvided);
	Optional(Element const & _element);
	// operator = is default one - copies isSet and element

	inline bool is_set() const { return is_flag_set(flags, Flag::IsSet); }
	inline void clear();
	inline void unset_with(Element const & _element);
	inline void set(Element const & _element);
	inline void set(Optional<Element> const & _optional);
	inline void set_if_not_set();
	inline void set_if_not_set(Element const & _element);
	inline void set_if_not_set(Optional<Element> const & _optional);
	inline Element & access();
	inline Element const & get() const;
	inline Element const & get(Element const & _default) const;

	bool load_from_xml(IO::XML::Node const * _node);
	bool load_from_xml(IO::XML::Attribute const * _attr);
	bool load_from_xml(IO::XML::Node const * _node, tchar const * const _attrOrChildName);

	bool operator==(Optional<Element> const & _other) const { return flags == _other.flags && (flags == None || element == _other.element); }
	bool operator!=(Optional<Element> const & _other) const { return ! operator==(_other); }

	bool operator==(Element const & _element) const { return is_set() && element == _element; }
	bool operator!=(Element const & _element) const { return ! operator==(_element); }

public:
	bool serialise(Serialiser & _serialiser);

private:
	enum Flag
	{
		None = 0,
		IsSet = 1,
		HasAnyValueStored = 2,
	};
	int8 flags;
	Element element;
};

// no default, from node
template <typename Element>
bool load_optional_enum_from_xml(Optional<AN_NOT_CLANG_TYPENAME Element>& _element, IO::XML::Node const* _node, Element(*parse)(String const& _from))
{
	if (_node)
	{
		_element = parse(_node->get_text());
	}
	return true;
}

// no default, from attribute
template <typename Element>
bool load_optional_enum_from_xml(Optional<AN_NOT_CLANG_TYPENAME Element>& _element, IO::XML::Attribute const* _attr, Element(*parse)(String const& _from))
{
	if (_attr)
	{
		_element = parse(_attr->get_as_string());
	}
	return true;
}

// with default, from node
template <typename Element>
bool load_optional_enum_from_xml(Optional<AN_NOT_CLANG_TYPENAME Element>& _element, IO::XML::Node const* _node, Element (*parse)(String const & _from, Element _default), Element _default)
{
	if (_node)
	{
		_element = parse(_node->get_text(), _element.get(_default));
	}
	return true;
}

// with default, from attribute
template <typename Element>
bool load_optional_enum_from_xml(Optional<AN_NOT_CLANG_TYPENAME Element>& _element, IO::XML::Attribute const* _attr, Element (*parse)(String const & _from, Element _default), Element _default)
{
	if (_attr)
	{
		_element = parse(_attr->get_as_string(), _element.get(_default));
	}
	return true;
}

#include "..\defaults.h"

#include "optional.inl"

struct Vector3;

DECLARE_REGISTERED_TYPE(Optional<bool>);
DECLARE_REGISTERED_TYPE(Optional<float>);
DECLARE_REGISTERED_TYPE(Optional<Vector3>);
