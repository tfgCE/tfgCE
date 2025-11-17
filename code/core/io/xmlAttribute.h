#pragma once

#include "..\types\string.h"

#include "xmlAttribute.h"

#include "xmlIterators.h"

namespace IO
{
	class Interface;

	namespace XML
	{
		class Attribute
		{
			friend class Node;

		public:
			Attribute(String const &);
			~Attribute();

			String const & get_name() const { return name; }
			String const & get_value() const { return value; }

			String const & get_as_string() const;
			Name const get_as_name() const;
			int get_as_int() const;
			bool get_as_bool() const;
			float get_as_float() const;

			void set_value(String const & _value) { value = _value; }

			Attribute * const next_attribute() { return nextAttribute; }
			Attribute const * const next_attribute() const { return nextAttribute; }

			Node const * get_parent_node() const { return parentNode; }

		private:
			String name;
			String value;

			Node* parentNode;
			Attribute* nextAttribute;

			static Attribute * const load_xml(Interface * const, REF_ int & _atLine, REF_ bool & _error);
			bool save_xml(Interface * const) const;
		};
	};
};

