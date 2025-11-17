#pragma once

#include "..\..\core\io\xml.h"
#include "..\..\core\values.h"

namespace Framework
{
	struct LibraryLoadingContext;

	struct LibraryName
	{
	public:
		static void initialise_static();
		static void close_static();

		static tchar const * separator() { return TXT(":"); } // has to be one character long

		String to_string() const { return group.to_string() + separator() + name.to_string(); }

		LibraryName();
		LibraryName(String const & _name);
		LibraryName(String const & _group, String const & _name);
		LibraryName(Name const & _group, Name const & _name);

		static LibraryName from_string(String const & _full);
		static LibraryName from_string_with_group(String const & _full, Name const & _group);
		static LibraryName from_xml(IO::XML::Node const *_node, String const & _attrName, LibraryLoadingContext & _lc);
		static LibraryName const & invalid() { return c_invalid; }

		bool is_valid() const;
		bool load_from_xml(IO::XML::Node const * _node, String const & _attrName, LibraryLoadingContext & _lc);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrName, LibraryLoadingContext & _lc);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrName = nullptr);
		bool load_from_xml(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, LibraryLoadingContext & _lc);
		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _attrName, LibraryLoadingContext & _lc);
		bool setup_with_string(String const & _name, Name _group = Name::invalid());
		bool setup_with_string(String const & _libraryName, LibraryLoadingContext & _lc);

		inline bool operator == (LibraryName const & _other) const { return group == _other.group && name == _other.name; }
		inline bool operator != (LibraryName const & _other) const { return group != _other.group || name != _other.name; }

		Name const & get_group() const { return group; }
		Name const & get_name() const { return name; }

		void set_group(Name const & _group) { group = _group; }
		void set_name(Name const & _name) { name = _name; }

	public:
		bool serialise(Serialiser & _serialiser);

	private:
		static LibraryName c_invalid;

		Name group;
		Name name;

		bool setup_with_string_loading(String const & _name, LibraryLoadingContext & _lc);

		bool check_if_valid_characters_used() const;

	public:
		static inline int compare(void const* _a, void const* _b)
		{
			LibraryName const& a = *plain_cast<LibraryName>(_a);
			LibraryName const& b = *plain_cast<LibraryName>(_b);
			int groupRes = String::compare_tchar_icase_sort(a.group.to_char(), b.group.to_char());
			if (groupRes != 0)
			{
				return groupRes;
			}
			return String::compare_tchar_icase_sort(a.name.to_char(), b.name.to_char());;
		}
	};

};

// specialisations

template <>
inline Framework::LibraryName zero<Framework::LibraryName>()
{
	return Framework::LibraryName::invalid();
}

template <>
inline bool load_value_from_xml<Framework::LibraryName>(REF_ Framework::LibraryName & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto attr = _node->get_attribute(_valueName))
		{
			_a.setup_with_string(_node->get_string_attribute(_valueName, _a.to_string()));
			return true;
		}
		if (auto child = _node->first_child_named(_valueName))
		{
			auto text = child->get_text();
			if (!text.is_empty())
			{
				_a.setup_with_string(text);
				return true;
			}
		}
	}
	return false;
}

//

DECLARE_REGISTERED_TYPE(Framework::LibraryName);
