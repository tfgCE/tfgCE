#pragma once

#include "iWMPOwner.h"

#include "..\types\optional.h"

namespace WheresMyPoint
{
	template <typename Class>
	struct Param
	{
		Param() {}

		Class get(WheresMyPoint::IOwner* _parametersProvider, ShouldAllowToFail _allowToFailSilently = DisallowToFail) const;
		Class get_with_default(WheresMyPoint::IOwner* _parametersProvider, Class const & _defaultValue, ShouldAllowToFail _allowToFailSilently = DisallowToFail) const;

		void clear() { valueParam.clear(); }
		bool is_set() const { return value.is_set() || valueParam.is_set(); }

		bool load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr); // will auto add *Param to _valueAttr (bone -> boneParam)
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr, tchar const * _valueParamAttr);

		Optional<Name> const & get_value_param() const { return valueParam; }

	protected:
		Optional<Class> value;
		Optional<Name> valueParam;
	};

	#include "wmpParam.inl"
};
