#pragma once

#include "..\types\name.h"
#include "..\memory\refCountObject.h"
#include "tag.h"

namespace TagConditionOperator
{
	enum Type
	{
		And,
		Or
	};
};

class TagCondition
: public RefCountObject
{
public:
	TagCondition();
	TagCondition(TagCondition const & _other);
	explicit TagCondition(String const& _tagConditionAsString);
	virtual ~TagCondition();

	static TagCondition const& empty() { return s_empty; }

	TagCondition & operator=(TagCondition const & _other);

	void setup_with_tags(Tags const& _tags); // clear and will require all tags that are non zero

	inline bool check(Tags const & _tags) const;
	bool is_empty() const { return element == nullptr; }
	void clear();

	void combine_with(TagCondition const & _other, TagConditionOperator::Type _withOperator);
		
	bool load_from_xml(IO::XML::Node const* _node);
	bool load_from_xml(IO::XML::Attribute const* _attr);
	bool load_from_xml_attribute_or_node(IO::XML::Node const* _node, String const& _attributeName);
	bool load_from_xml_attribute_or_node(IO::XML::Node const* _node, tchar const* _attributeName = TXT("tags"));
	bool load_from_xml_attribute_or_child_node(IO::XML::Node const * _node, tchar const * _childNodeAttributeName, TagConditionOperator::Type _defaultOperator = TagConditionOperator::And);
	bool load_from_xml_attribute(IO::XML::Node const * _node, tchar const * _attributeName, TagConditionOperator::Type _defaultOperator = TagConditionOperator::And);
	bool load_from_string(String const & _input, TagConditionOperator::Type _defaultOperator = TagConditionOperator::And);

	String to_string() const;

private:
#ifdef AN_NAT_VIS
	String natVis;
	void update_for_nat_vis();
#endif

	struct Token
	{
	public:
		enum Type
		{
			Tag,
			Value,
			Equal,
			LessThan,
			LessOrEqual,
			GreaterThan,
			GreaterOrEqual,
			Add,
			And,
			Or,
			Not,
			OpenBracket,
			CloseBracket,
		};

		Token(Type _type);
		Token(String const & _tag);
		Token(float _value);
			
		Type get_type() const { return type; }
		Name const & get_tag() const { return tag; }
		float get_value() const { return value; }

		static void load_from_string(String const & _input, OUT_ List<Token>& _list);

	private:
		Type type;
		Name tag;
		float value = 1.0f;
	};

	struct Element
	{
	public:
		enum Type
		{
			Tag,
			Value,
			Add,
			And,
			Or,
			Not,
			Equal,
			LessThan,
			LessOrEqual,
			GreaterThan,
			GreaterOrEqual,
		};

		inline static Type get_type(TagConditionOperator::Type _operator);

		Element(Type _type);
		Element(Token const & _token);
		~Element();

		Element* create_copy() const;

		Type get_type() const { return type; }

		inline float check(Tags const & _tags) const;

		void add_arg(Element* _arg);
		void add_arg_err(Element* _arg, REF_ bool & _error);
		void add_arg_def(Element* _arg, Type _defaultOperator, REF_ bool & _error);

		static Element* load(List<Token> const & _tokens, Type _defaultOperator);
		
		String to_string() const;

	private:
		Type type;
		Name tag;
		float value = 1.0f;
		Element* argumentA;
		Element* argumentB;
		Element* parent;

		static Element* sub_load(List<Token> const & _tokens, Type _defaultOperator, REF_ int & _index, REF_ int & _openBrackets, REF_ bool & _error, bool _loadSingle);

		inline float check_for_arg(Element* _argument, Tags const & _tags) const;

		inline bool is_complete() const;

		void remove_from_parent();
		void replace_arg_with(Element* _old, Element* _new, REF_ bool & _error);
	};

	static TagCondition s_empty;
	Element* element;
};
	
#include "tagCondition.inl"

#include "..\values.h"

template <>
inline TagCondition zero<TagCondition>()
{
	return TagCondition();
}

template <>
inline bool load_value_from_xml<TagCondition>(REF_ TagCondition& _a, IO::XML::Node const* _node, tchar const* _valueName)
{
	if (_node)
	{
		return _a.load_from_xml_attribute_or_child_node(_node, _valueName);
	}
	return false;
}

DECLARE_REGISTERED_TYPE(TagCondition);

template <>
inline void set_to_default<TagCondition>(TagCondition& _object)
{
	_object = TagCondition();
}
