#pragma once

#include "..\globalDefinitions.h"
#include "..\fastCast.h"

#include "iWMPTool.h"

struct Name;
struct Vector3;

namespace WheresMyPoint
{
	struct Value;

	interface_class IOwner
	{
		FAST_CAST_DECLARE(IOwner);
		FAST_CAST_END();

	public:
		virtual ~IOwner() {}
	public:
		// more universal
		virtual bool store_value_for_wheres_my_point(Name const & _byName, Value const & _value) = 0;
		virtual bool restore_value_for_wheres_my_point(Name const & _byName, OUT_ Value & _value) const = 0;
		virtual bool store_convert_value_for_wheres_my_point(Name const & _byName, TypeId _to) = 0;
		virtual bool rename_value_forwheres_my_point(Name const& _from, Name const& _to) = 0;
		// globals, only if provided, if not, just keep with standard store/restore
		virtual bool store_global_value_for_wheres_my_point(Name const & _byName, Value const & _value) { return store_value_for_wheres_my_point(_byName, _value); }
		virtual bool restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ Value & _value) const { return restore_value_for_wheres_my_point(_byName, OUT_ _value); }

		// easier access
		template <typename Class>
		bool restore_value(Name const & _byName, OUT_ Class & _value) const
		{
			Value val;
			if (restore_value_for_wheres_my_point(_byName, val))
			{
				if (val.get_type() == type_id<Class>())
				{
					_value = val.get_as<Class>();
					return true;
				}
			}
			return false;
		}
		//
		inline bool restore_value(Name const & _byName, TypeId const & _type, OUT_ void * _value) const
		{
			Value val;
			if (restore_value_for_wheres_my_point(_byName, val))
			{
				if (val.get_type() == _type)
				{
					RegisteredType::copy(_type, _value, val.get_raw());
					return true;
				}
			}
			return false;
		}

		// easier access
		template <typename Class>
		Class get_value(Name const & _byName, Class const & _defaultValue) const
		{
			Value val;
			if (restore_value_for_wheres_my_point(_byName, val))
			{
				if (val.get_type() == type_id<Class>())
				{
					return val.get_as<Class>();
				}
			}
			return _defaultValue;
		}

		// specific functions
		virtual bool get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const { return false; }

		virtual IOwner* get_wmp_owner() { return nullptr; } // to allow nested owners to get higher owner
	};
};
