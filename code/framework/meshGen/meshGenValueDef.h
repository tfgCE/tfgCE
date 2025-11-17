#pragma once

#include "..\..\core\types\optional.h"
#include "..\..\core\other\simpleVariableStorage.h"

class SimpleVariableStorage;

namespace Framework
{
	struct LibraryName;

	namespace MeshGeneration
	{
		struct GenerationContext;

		template<typename Class>
		struct ValueDef
		{
			ValueDef() {}
			ValueDef(Class const & _defValue) { value = _defValue; }

			void clear();

			Class get(GenerationContext const & _context) const;
			Class get_with_default(GenerationContext const & _context, Class const & _default) const;
			void set(Class const & _value) { value = _value; }
			void set_if_not_set(Class const & _value) { if (! value.is_set()) value = _value; }
			void set_param(Optional<Name> const & _valueParam) { valueParam = _valueParam; }

			bool is_value_set() const { return value.is_set(); }
			Class const & get_value() const { an_assert(is_value_set()); return value.get(); }

			bool load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr); // will auto add *Param to _valueAttr (bone -> boneParam)
			bool load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr, tchar const * _valueParamAttr);
			bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _valueChildAttr); // will auto add *Param to _valueChild
			bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _valueChild, tchar const * _valueParamAttr);

			bool is_set() const { return value.is_set() || valueParam.is_set(); }

			Optional<Name> const & get_value_param() const { return valueParam; }

		protected:
			Optional<Class> value;
			Optional<Name> valueParam;
		};

	};
};
