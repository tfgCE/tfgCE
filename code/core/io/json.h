#pragma once

/**
 */

#include "..\containers\list.h"
#include "..\memory\refCountObject.h"
#include "..\types\optional.h"
#include "..\types\string.h"

namespace IO
{
	namespace JSON
	{
		class Node
		{
		public:
			Node() {}
			Node(tchar const * _name) : name(_name) {}
			Node(String const& _name) : name(_name) {}

			Node& access_sub(tchar const* _subName);
			void set(tchar const* _attribute, tchar const * _value);
			void set(tchar const* _attribute, String const & _value);
			void set(tchar const* _attribute, bool const _value);
			void set(tchar const* _attribute, int const _value);
			void set(tchar const* _attribute, float const _value);

			Node* add_array_element(String const& _name) { return add_array_element(_name.to_char()); }
			Node* add_array_element(tchar const* _name);

			List<Node> const* get_array_elements(String const& _name) const { return get_array_elements(_name.to_char()); }
			List<Node> const* get_array_elements(tchar const* _name) const;

			Node const * get_sub(tchar const* _subName) const;
			String const & get(tchar const* _attribute) const;
			bool get_as_bool(tchar const* _attribute, bool _defaultValue = false) const;
			int get_as_int(tchar const* _attribute, int _defaultValue = 0) const;
			float get_as_float(tchar const* _attribute, float _defaultValue = 0.0) const;

			List<Node> const& get_elements() const { return elements; }

			String to_string() const;
			bool parse(String const & _source);

		private:
			String name;

			String value;
			Optional<bool> valueBool;
			Optional<int> valueInt;
			Optional<float> valueFloat;
			Optional<String> valueNumber;
			bool isArray = false;
			List<Node> elements;

			List<Node> subValues; // it's either values above or sub values

			Node & clear_value();

			bool parse(tchar const *& _ch);
		};

		class Document
		: public Node
		, public RefCountObject
		{
		};
	};
};
