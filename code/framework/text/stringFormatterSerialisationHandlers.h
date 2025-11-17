#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\name.h"

class Serialiser;

namespace Framework
{
	interface_class ICustomStringFormatter;
	struct ICustomStringFormatterPtr;
	
	interface_class ICustomStringFormatterParamSerialisationHandler // what a long name!
	: public RefCountObject
	{
	public:
		virtual bool get_type_for_serialisation(ICustomStringFormatter const * _custom, REF_ Name & _type) const = 0; // get type for _custom
		virtual bool does_serialise(Name const & _type) const = 0; // if serialise given type
		virtual bool serialise(Serialiser& _serialiser, ICustomStringFormatterPtr & _custom, Name const & _type) const = 0; // serialise, type is already serialised
	};

	struct StringFormatterSerialisationHandlers
	{
		static void initialise_static();
		static void close_static();
		static void add_serialisation_handler(ICustomStringFormatterParamSerialisationHandler* _handler);

		static bool serialise(Serialiser& _serialiser, ICustomStringFormatterPtr & _custom); // serialise, type is already serialised

	private:
		static StringFormatterSerialisationHandlers* stringFormatterSerialisationHandlers;
		static StringFormatterSerialisationHandlers* get() { return stringFormatterSerialisationHandlers; }
		
		Array<RefCountObjectPtr<ICustomStringFormatterParamSerialisationHandler>> handlers;
	};
};

#define SERIALISATION_HANDLER_GET_TYPE(Class) \
	if (fast_cast<Class>(_custom)) \
	{ \
		DEFINE_STATIC_NAME_STR(CN##Class, TXT(#Class)); \
		_type = NAME(CN##Class); \
		return true; \
	}

#define SERIALISATION_HANDLER_IF_TYPE(Class) \
	DEFINE_STATIC_NAME_STR(CN##Class, TXT(#Class)); \
	if (_type == NAME(CN##Class))

#define SERIALISATION_HANDLER_DOES_SERIALISE(Class) \
	SERIALISATION_HANDLER_IF_TYPE(Class) return true;

#define SERIALISATION_HANDLER_WRITING(Class, obj) \
	auto const * obj = _serialiser.is_writing()? (fast_cast<Class>(_custom.get() ? _custom.get()->get_actual() : nullptr)) : nullptr; \
	if (_serialiser.is_writing())

#define SERIALISATION_HANDLER_READING() \
	if (_serialiser.is_reading())

#define SERIALISATION_HANDLER_SERIALISE_PTR(Class) \
	SERIALISATION_HANDLER_IF_TYPE(Class) \
	{ \
		SERIALISATION_HANDLER_WRITING(Class, obj) \
		{ \
			result &= ##Class##Ptr::serialise_write_for(_serialiser, obj); \
		} \
		SERIALISATION_HANDLER_READING() \
		{ \
			Framework::CustomStringFormatterStoredPtr<##Class##Ptr>* ref = new Framework::CustomStringFormatterStoredPtr<##Class##Ptr>(); \
			result &= serialise_data(_serialiser, ref->stored); \
			_custom = ref; \
		} \
		return result; \
	}

#define SERIALISATION_HANDLER_SERIALISE_VALUE(Class) \
	SERIALISATION_HANDLER_IF_TYPE(Class) \
	{ \
		SERIALISATION_HANDLER_WRITING(Class, obj) \
		{ \
			Class value = obj? *obj : Class(); \
			result &= serialise_data(_serialiser, value); \
		} \
		SERIALISATION_HANDLER_READING() \
		{ \
			Framework::CustomStringFormatterStoredValue<Class>* ref = new Framework::CustomStringFormatterStoredValue<Class>(); \
			result &= serialise_data(_serialiser, ref->stored); \
			_custom = ref; \
		} \
		return result; \
	}