#pragma once

#include "..\types\name.h"
#include "..\types\string.h"
#include "..\memory\refCountObject.h"
#include "..\memory\safeObject.h"
#include "..\serialisation\serialiserBasics.h"

#include "xmlAttribute.h"
#include "xmlIterators.h"

#define error_loading_xml(node, msg, ...) { { ScopedOutputLock outputLock; output_colour(1,0,0,1); trace_func(TXT("[ERROR in XML] %S : "), node? node->get_location_info().to_char(): TXT("no info")); report_func(msg, ##__VA_ARGS__); output_colour(); } debug_on_error(); }
#define error_loading_xml_dev_ignore(node, msg, ...) { { ScopedOutputLock outputLock; output_colour(1,0,0,1); trace_func(TXT("[ERROR in XML] %S : "), node? node->get_location_info().to_char(): TXT("no info")); report_func(msg, ##__VA_ARGS__); output_colour(); } }
#define warn_loading_xml(node, msg, ...) { { ScopedOutputLock outputLock; output_colour(1,1,0,1); trace_func(TXT("[warning in XML] %S : "), node? node->get_location_info().to_char(): TXT("no info")); report_func(msg, ##__VA_ARGS__); output_colour(); } /*debug_on_warn();*/ }
#define warn_loading_xml_dev_ignore(node, msg, ...) { { ScopedOutputLock outputLock; output_colour(1,1,0,1); trace_func(TXT("[warning in XML] %S : "), node? node->get_location_info().to_char(): TXT("no info")); report_func(msg, ##__VA_ARGS__); output_colour(); } }

#define error_loading_xml_on_assert(cond, result, node, msg, ...) if (! (cond)) { { result = false; ScopedOutputLock outputLock; output_colour(1,0,0,1); trace_func(TXT("[ERROR in XML] %S : "), node? node->get_location_info().to_char(): TXT("no info")); report_func(msg, ##__VA_ARGS__); output_colour(); } debug_on_error(); }
#define warn_loading_xml_on_assert(cond, node, msg, ...) if (! (cond)) { { ScopedOutputLock outputLock; output_colour(1,1,0,1); trace_func(TXT("[warning in XML] %S : "), node? node->get_location_info().to_char(): TXT("no info")); report_func(msg, ##__VA_ARGS__); output_colour(); } /*debug_on_warn();*/ }

#define XML_LOAD(_node, what) what.load_from_xml(_node, TXT(#what));
#define XML_LOAD_ATTR_CHILD(_node, what) what.load_from_attribute_or_from_child(_node, TXT(#what));
#define XML_LOAD_BOOL_ATTR(_node, what) what = _node->get_bool_attribute(TXT(#what), what);
#define XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, what) what = _node->get_bool_attribute_or_from_child_presence(TXT(#what), what);
#define XML_LOAD_FLOAT_ATTR(_node, what) what = _node->get_float_attribute(TXT(#what), what);
#define XML_LOAD_FLOAT_ATTR_CHILD(_node, what) what = _node->get_float_attribute_or_from_child(TXT(#what), what);
#define XML_LOAD_INT_ATTR(_node, what) what = _node->get_int_attribute(TXT(#what), what);
#define XML_LOAD_INT_ATTR_CHILD(_node, what) what = _node->get_int_attribute_or_from_child(TXT(#what), what);
#define XML_LOAD_NAME_ATTR(_node, what) what = _node->get_name_attribute(TXT(#what), what);
#define XML_LOAD_NAME_ATTR_CHILD(_node, what) what = _node->get_name_attribute_or_from_child(TXT(#what), what);
#define XML_LOAD_STRING_ATTR(_node, what) what = _node->get_string_attribute(TXT(#what), what);
#define XML_LOAD_STRING_ATTR_CHILD(_node, what) what = _node->get_string_attribute_or_from_child(TXT(#what), what);

#define XML_LOAD_STR(_node, what, strWhat) what.load_from_xml(_node, strWhat);
#define XML_LOAD_FLOAT_ATTR_STR(_node, what, strWhat) what = _node->get_float_attribute(strWhat, what);
#define XML_LOAD_FLOAT_ATTR_CHILD_STR(_node, what, strWhat) what = _node->get_float_attribute_or_from_child(strWhat, what);
#define XML_LOAD_INT_ATTR_STR(_node, what, strWhat) what = _node->get_int_attribute(strWhat, what);
#define XML_LOAD_INT_ATTR_CHILD_STR(_node, what, strWhat) what = _node->get_int_attribute_or_from_child(strWhat, what);
#define XML_LOAD_NAME_ATTR_STR(_node, what, strWhat) what = _node->get_name_attribute(strWhat, what);
#define XML_LOAD_NAME_ATTR_CHILD_STR(_node, what, strWhat) what = _node->get_name_attribute_or_from_child(strWhat, what);
#define XML_LOAD_STRING_ATTR_STR(_node, what, strWhat) what = _node->get_string_attribute(strWhat, what);
#define XML_LOAD_STRING_ATTR_CHILD_STR(_node, what, strWhat) what = _node->get_string_attribute_or_from_child(strWhat, what);

namespace IO
{
	class Interface;
	struct Digested;

	namespace XML
	{
		class Document;
		struct DocumentInfo;

		inline tchar const * node_commented_out_prefix() { return TXT("."); }

		namespace NodeType
		{
			enum Type
			{
				Node,
				Text,
				Comment,
				Root,
				NodeCommentedOut // check xml.h
			};
		};

		class Node
		: public RefCountObject
		{
			friend class Document;

			public:
				// create normal element (text is added through text_node or adding text)
				Node(Document const * _document, int _atLine, String const & _content, NodeType::Type const = NodeType::Node);
				~Node();

				DocumentInfo const * get_document_info() const { return documentInfo.get(); }
				Document * get_document() const { return document.get(); }

				String to_string_slow(bool _includeComments = true) const;

			public: // use document if possible
				bool save_single_node_to_xml(Interface* const, int _indent = 0) const;
				static RefCountObjectPtr<Node> load_single_node_from_xml(Interface* const, Document* _document, REF_ int& _atLine, REF_ bool& _error);

			public:
				String get_location_info() const;

				// access
				NodeType::Type const get_type() const { return type; }
				String get_text() const { return type == NodeType::Text || type == NodeType::Comment? content : ( type == NodeType::Node? get_internal_text() : String::empty() ); }
				String get_text_raw() const { return type == NodeType::Text || type == NodeType::Comment ? contentRaw : (type == NodeType::Node ? get_internal_text_raw() : String::empty()); } // remember about no break spaces
				String const & get_name() const { return type == NodeType::Node ? content : String::empty(); }
				String get_internal_text() const;
				String get_internal_text_raw() const;

				bool is_empty() const; // doesn't have any children nor text

				// clear
				void clear();

				// adding
				Node* const add_text(String const &);
				Node* const add_node(String const &);
				Node* const add_comment(String const &);
				Node* const add_text(tchar const * const _text) { return add_text(String(_text)); }
				Node* const add_node(tchar const * const _nodeName) { return add_node(String(_nodeName)); }
				Node* const add_comment(tchar const * const _nodeName) { return add_comment(String(_nodeName)); }
				void add_node(Node* _node);

				// removing
				void remove_node(Node* _node);
				void delete_attribute(Attribute* _attribute);

				// iterators
				AllAttributes const all_attributes() const { return AllAttributes(this); }
				AllChildren const all_children() const { return AllChildren(this); }
				NamedChildren const children_named(tchar const * _childName) const { return NamedChildren(this, String(_childName)); }
				NamedChildren const children_named(String const & _childName) const { return NamedChildren(this, _childName); }

				// accessing parent
				Node* get_parent() { return parentNode; }
				Node const * get_parent() const { return parentNode; }

				// checking what's inside
				bool has_children() const;
				bool has_text() const;

				// accessing children
				Node * const first_child();
				Node const * const first_child() const;
				Node * const first_child_named(tchar const * _childName, tchar const * _altChildName = nullptr);
				Node const * const first_child_named(tchar const * _childName, tchar const * _altChildName = nullptr) const;
				Node * const first_child_named(String const & _childName) { return first_child_named(_childName.to_char()); }
				Node const * const first_child_named(String const & _childName) const { return first_child_named(_childName.to_char()); }
				int get_children_named(Array<Node const*>& children, tchar const * _childName, tchar const * _altChildName = nullptr) const;

				// accessing siblings
				Node * const next_sibling();
				Node const * const next_sibling() const;
				Node * const next_sibling_named(tchar const * _siblingName, tchar const * _altSiblingName = nullptr);
				Node const * const next_sibling_named(tchar const * _siblingName, tchar const * _altSiblingName = nullptr) const;
				Node * const next_sibling_named(String const & _siblingName) { return next_sibling_named(_siblingName.to_char()); }
				Node const * const next_sibling_named(String const & _siblingName) const { return next_sibling_named(_siblingName.to_char()); }

				// attributes
				void set_attribute(tchar const * _attribute, String const & _value);
				void set_attribute(tchar const * _attribute, tchar const *_value) { set_attribute(_attribute, String(_value)); }
				void set_attribute(String const & _attribute, String const & _value) { set_attribute(_attribute.to_char(), _value); }
				bool has_attribute(tchar const * _attribute) const { return get_attribute(_attribute) != nullptr; }
				Attribute const * const get_attribute(tchar const * _attribute) const;
				Attribute const * const get_attribute(String const & _attribute) const { return get_attribute(_attribute.to_char()); }
				Attribute * const find_attribute(tchar const * _attribute);
				Attribute * const first_attribute() { return firstAttribute; }
				Attribute const * const first_attribute() const { return firstAttribute; }

				void set_bool_attribute(tchar const * _attribute, bool const _value);
				void set_int_attribute(tchar const * _attribute, int const _value);
				void set_float_attribute(tchar const * _attribute, float const _value);
				void set_bool_to_child(tchar const * _name, bool const _value);
				void set_int_to_child(tchar const * _name, int const _value);
				void set_float_to_child(tchar const * _name, float const _value);
				void set_string_to_child(tchar const * _name, String const & _value);
				void set_string_to_child(tchar const * _name, tchar const * _value);

				Name const get_name_attribute(tchar const * _attribute, Name const & _defVal = Name::invalid()) const;
				Name const get_name_attribute(String const & _attribute, Name const & _defVal = Name::invalid()) const { return get_name_attribute(_attribute.to_char(), _defVal); }
				Name const get_name_attribute_or_from_child(tchar const * _name, Name const & _defVal) const;
				String const & get_string_attribute(tchar const * _attribute, String const & _defVal = String::empty()) const;
				String const & get_string_attribute(String const & _attribute, String const & _defVal = String::empty()) const { return get_string_attribute(_attribute.to_char(), _defVal); }
				String get_string_attribute_or_from_child(tchar const * _name, String const & _defVal = String::empty()) const;
				String get_string_attribute_or_from_node(tchar const * _name, String const & _defVal = String::empty()) const;
				int get_int_attribute(tchar const * _attribute, int _defVal = 0) const;
				int get_int_attribute(String const & _attribute, int _defVal = 0) const { return get_int_attribute(_attribute.to_char(), _defVal); }
				int get_int_attribute_or_from_child(tchar const * _name, int _defVal = 0.0f) const;
				bool get_bool_attribute(tchar const * _attribute, bool _defVal = false) const;
				bool get_bool_attribute(String const & _attribute, bool _defVal = false) const { return get_bool_attribute(_attribute.to_char(), _defVal); }
				bool get_bool_attribute_or_from_child(tchar const * _name, bool _defVal = false) const;
				bool get_bool_attribute_or_from_child_presence(tchar const * _name, bool _defVal = false) const; // if child is present it is set to true
				float get_float_attribute(tchar const * _attribute, float _defVal = 0.0f) const;
				float get_float_attribute(String const & _attribute, float _defVal = 0.0f) const { return get_float_attribute(_attribute.to_char(), _defVal); }
				float get_float_attribute_or_from_child(tchar const * _name, float _defVal = 0.0f) const;

				Name const get_name_from_child(tchar const * _childName, Name const & _defVal = Name::invalid()) const;
				Name const get_name_from_child(String const & _childName, Name const & _defVal = Name::invalid()) const { return get_name_from_child(_childName.to_char(), _defVal); }
				String get_string_from_child(tchar const * _childName, String const & _defVal = String::empty()) const;
				String get_string_from_child(String const & _childName, String const & _defVal = String::empty()) const { return get_string_from_child(_childName.to_char(), _defVal); }
				int get_int_from_child(tchar const * _childName, int _defVal = 0) const;
				int get_int_from_child(String const & _childName, int _defVal = 0) const { return get_int_from_child(_childName.to_char(), _defVal); }
				bool get_bool_from_child(tchar const * _childName, bool _defVal = false) const;
				bool get_bool_from_child(String const & _childName, bool _defVal = false) const { return get_bool_from_child(_childName.to_char(), _defVal); }
				float get_float_from_child(tchar const * _childName, float _defVal = 0.0f) const;
				float get_float_from_child(String const & _childName, float _defVal = 0.0f) const { return get_float_from_child(_childName.to_char(), _defVal); }

				bool has_attribute_or_child(tchar const * _name) const;

				int get_int(int _defVal = 0) const;
				bool get_bool(bool _defVal = false) const;
				float get_float(float _defVal = 0.0f) const;
				Name get_name(Name const & _defVal) const;

				template <typename Class>
				inline Class get(Class const & _defVal) const;

				template <typename Class>
				inline Class get() const;

			private: friend class Document;
					 friend struct AllChildren;
					 friend struct AllChildren::Iterator;
				Node * const first_sub_node() { return firstSubNode.get(); }
				Node const * const first_sub_node() const { return firstSubNode.get(); }
				Node * const next_sub_node() { return nextNode.get(); }
				Node const * const next_sub_node() const { return nextNode.get(); }
			private:
				::SafePtr<Document> document; // for saving (ignored when kept for streaming)
				RefCountObjectPtr<DocumentInfo> documentInfo;
				int atLine;

				NodeType::Type type;
				String content;
				String contentRaw; // special characters are decoded, no break spaces &#160; are stored as 160 (not 32), therefore raw text has to be turned into normal spaces

				Node* parentNode;
				Node* addChildrenOnLoadTo; // for _section
				RefCountObjectPtr<Node> nextNode;
				RefCountObjectPtr<Node> firstSubNode; // below, renamed from child as child should be all sub nodes of type Node!
				Attribute* firstAttribute;

				Node(Document const * _document);

				bool save_xml(Interface* const, bool _includeComments = true, int _indent = 0) const;
				bool save_xml_children(Interface* const, bool _includeComments = true, int _indent = 0) const;
				static RefCountObjectPtr<Node> load_xml(Interface* const, Document* _document, Node* _soonBeParentNode, REF_ int& _atLine, REF_ bool& _error);

				bool serialise(Serialiser & _serialiser, int _version); // serialiser does ignore comments when writing

				void push_back_node(Node* _node);
				void push_back_attribute(Attribute* _attribute);

				friend struct IO::Digested;
		};

		#include "xmlNode.inl"
	};

};

DECLARE_SERIALISER_FOR(::IO::XML::NodeType::Type);

