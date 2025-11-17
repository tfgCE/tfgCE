#pragma once

#include "iWMPOwner.h"
#include "..\other\simpleVariableStorage.h"

namespace WheresMyPoint
{
	struct SimpleWMPOwner
	: public WheresMyPoint::IOwner
	{
		FAST_CAST_DECLARE(SimpleWMPOwner);
		FAST_CAST_BASE(WheresMyPoint::IOwner);
		FAST_CAST_END();

	public:
		SimpleWMPOwner() {}
		virtual ~SimpleWMPOwner() {}

		SimpleVariableStorage const& access_variables() { return variables; }
		SimpleVariableStorage const& get_variables() const { return variables; }

	public: // WheresMyPoint::IOwner
		override_ bool store_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value);
		override_ bool restore_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const;
		override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
		override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);

	private:
		SimpleVariableStorage variables;
	};
};
