#pragma once

#include "meshGenValueDef.h"

namespace WheresMyPoint
{
	interface_class IOwner;
	interface_class ITool;
};

namespace Framework
{
	/**
	 *	Wrapper to make it easier to use outside mesh generation
	 *
	 *	It's called mesh gen param as it is used with mesh generation to extract parameter values from mesh generation's context
	 *
	 *	Should be used to modify values during mesh generation
	 */
	template<typename Class>
	struct MeshGenParam
	: public MeshGeneration::ValueDef<Class>
	{
		typedef MeshGeneration::ValueDef<Class> base;

		MeshGenParam() {}
		MeshGenParam(Class const & _defValue): base(_defValue) {}

		MeshGenParam& operator=(Class const & _value) { AN_CLANG_BASE value.set(_value); return *this; }

		Class const & get() const { return AN_CLANG_BASE get_value(); }
		Class const & get(Class const & _defaultValue) const { return AN_CLANG_BASE is_set()? AN_CLANG_BASE get_value() : _defaultValue; }
		Class const & get(SimpleVariableStorage const & _storage) const;
		Class const & get(SimpleVariableStorage const & _storage, Class const & _defaultValue, ShouldAllowToFail _shouldAllowToFail = DisallowToFail) const;
		Class const & get(SimpleVariableStorage const & _storage, MeshGenParam<Class> const & _general) const; // first will try param then general's param, then value, then general's value
		Class get(WheresMyPoint::IOwner const * _wmpOwner, Class const & _defaultValue, ShouldAllowToFail _shouldAllowToFail = DisallowToFail) const;
		bool process_for_wheres_my_point(WheresMyPoint::ITool const * _tool, WheresMyPoint::IOwner const* _wmpOwner, REF_ Class& _outValue, ShouldAllowToFail _shouldAllowToFail = DisallowToFail) const;
		// find allows to fail
		Class const * find(SimpleVariableStorage const & _storage) const;
		Class const * find(SimpleVariableStorage const & _storage, MeshGenParam<Class> const & _general) const;
		Class & access() { return AN_CLANG_BASE value.access(); }
		void update_if_set(Class & _what) const { if (AN_CLANG_BASE is_set()) { _what = get(); } }

		inline void fill_value_with(MeshGeneration::GenerationContext const & _context, ShouldAllowToFail _allowToFail = DisallowToFail);
		inline void fill_value_with(SimpleVariableStorage const & _storage, ShouldAllowToFail _allowToFail = DisallowToFail);

		void if_value_set(std::function<void()> _do) { if (AN_CLANG_BASE value.is_set()) { _do(); } }
	};

};
