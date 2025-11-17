#pragma once

#include "..\values.h"
#include "..\containers\map.h"
#include "..\types\name.h"
#include "..\io\file.h"
#include "..\io\xml.h"

#include <functional>

struct Tag;

typedef std::function< void(Tag const & _tag) > DoForEachTag;

struct Tag
{
public:
	inline Tag();
	inline Tag(Name const & _tag, float _relevance = 1.0f);
	inline Tag(Name const & _tag, int _relevance);

	inline Name const & get_tag() const { return tag; }
	inline float get_relevance() const { return relevance; }
	inline int get_relevance_as_int() const { return relevanceAsInt; }

	inline void setup(float _relevance) { relevance = _relevance; relevanceAsInt = (int)(round(abs(_relevance)) + 0.1f) * (_relevance > 0.0f? 1 : -1); }
	inline void setup(int _relevance) { relevance = (float)_relevance; relevanceAsInt = _relevance; }

public:
	bool serialise(Serialiser & _serialiser);

private:
	Name tag;
	float relevance;
	int relevanceAsInt; // separate as we may have rounding errors
};

#define USE_MORE_TAGS

struct Tags
{
public:
	static Tags none;

	inline Tags();

	inline void clear() { tags.clear(); }

	inline bool is_empty() const { return tags.is_empty(); }
	inline void base_on(Tags const * _other);

	inline Tags & remove_tags(Tags const & _tags);
	inline Tags & remove_tag(Name const & _tag);
	inline Tags & set_tag(Name const & _tag, float _relevance = 1.0f);
	inline Tags & set_tag(Name const & _tag, int _relevance);
	inline float get_tag(Name const & _tag, float _default = 0.0f) const;
	inline int get_tag_as_int(Name const & _tag) const;
	inline bool has_tag(Name const & _tag) const; // even if is set to 0

	void add_tags_by_relevance(Tags const & _other);
	void set_tags_from(Tags const & _other);

	inline bool is_same_as(Tags const & _other) const { return is_contained_within(_other) && _other.is_contained_within(*this); }
	inline bool is_contained_within(Tags const & _other) const; // check if this set is contained within tags in _other
	inline bool does_match_any_from(Tags const & _other) const; // check if this set has any tags that match tags in _other
	inline float calc_cross_relevance(Tags const & _other) const; // pairs of tags have relevance mutliplied and sum is returned

	bool load_from_xml(IO::XML::Node const * _node, String const & _attributeChildrenName);
	bool load_from_xml(IO::XML::Node const * _node, tchar const * _attributeChildrenName);
	bool load_from_xml(IO::XML::Node const * _node);
	// load from attribute of node or inner text of node
	bool load_from_xml_attribute_or_node(IO::XML::Node const * _node, String const & _attributeName);
	bool load_from_xml_attribute_or_node(IO::XML::Node const * _node, tchar const * _attributeName = TXT("tags"));
	bool load_from_xml_attribute_or_child_node(IO::XML::Node const * _node, String const & _attributeOrChildName);
	bool load_from_xml_attribute_or_child_node(IO::XML::Node const * _node, tchar const * _attributeOrChildName = TXT("tags"));
	bool load_from_string(String const & _tags);

	void do_for_every_tag(DoForEachTag _do_for_every_tag) const;

	String to_string_all(bool _withZeroRelevance = false) const; // including base tags
	String to_string(bool _withZeroRelevance = false) const;
	String to_string_with_minus() const; // zero is with minus

public:
	bool serialise(Serialiser & _serialiser);

private:
#ifdef AN_NAT_VIS
	String natVis;
	void update_for_nat_vis();
#endif
	static int const MAX_TAG_COUNT = 16;
	ArrayStatic<Tag, MAX_TAG_COUNT> tags;
#ifdef USE_MORE_TAGS
	Array<Tag> moreTags;
	bool useMoreTags = false;
#endif
	Tags const * baseTags;
};

#include "tag.inl"

#include "..\values.h"

template <>
inline Tags zero<Tags>()
{
	return Tags::none;
}

template <>
inline bool load_value_from_xml<Tags>(REF_ Tags& _a, IO::XML::Node const* _node, tchar const* _valueName)
{
	if (_node)
	{
		return _a.load_from_xml_attribute_or_child_node(_node, _valueName);
	}
	return false;
}

DECLARE_REGISTERED_TYPE(Tags);
