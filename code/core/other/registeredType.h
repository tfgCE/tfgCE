#pragma once

#include "..\casts.h"

#include "..\globalDefinitions.h"

#include "..\serialisation\serialiserBasics.h"

/**
 *	This is used to provide fast translation of runtime class into integer value.
 *	Each type HAS to be defined.
 */

struct LogInfoContext;
struct String;
struct Name;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

typedef int TypeId;

bool serialise_type_id(Serialiser & _serialiser, TypeId & _data);

namespace RegisteredType
{
	typedef TypeId Id;

	inline int none() { return 0; }

	template <typename Class>
	int get_id() { error(TXT("type not registered")); AN_DEVELOPMENT_BREAK; return none(); } // remember to put DECLARE_REGISTERED_TYPE, see macro at the end of this file

	template <typename Class>
	int get_id_if_registered() { return none(); }

	void initialise_static();
	void close_static();

	bool access_sub_field(Name const & _subField, void* _variable, Id _id, void* & _outSubField, Id & _outSubId);
	bool get_sub_field(Name const & _subField, void const * _variable, Id _id, void const * & _outSubField, Id & _outSubId);

	tchar const * const get_name_of(Id _id);
	TypeId get_type_id_by_name(tchar const * _name);
	int get_size_of(Id _id);
	int get_alignment_of(Id _id);
	void get_size_and_alignment_of(Id _id, OUT_ int & _size, OUT_ int & _alignment);
	bool is_plain_data(Id _id);
	void const * get_initial_value(Id _id);

	// allows parsing string to TypeId for all defined types
	int parse_type(String const & _type);
	int parse_type(tchar const * const _type);

	void parse_value(String const & _value, Id _intoId, void * _into);
	template <typename Class>
	void parse_value(String const & _value, Class & _into) { parse_value(_value, get_id<Class>(), (plain_cast<Class>)(&_into)); }

	bool load_from_xml(IO::XML::Node const * _node, Id _intoId, void * _into);
	template <typename Class>
	bool load_from_xml(IO::XML::Node const * _node, Class & _into) { return load_from_xml(_node, get_id<Class>(), (plain_cast<Class>)(&_into)); }

	bool save_to_xml(IO::XML::Node * _node, Id _asId, void const * _value);
	template <typename Class>
	bool save_to_xml(IO::XML::Node * _node, Class const & _value) { return save_to_xml(_node, get_id<Class>(), (plain_cast<Class>)(&_value)); }

	void log_value(LogInfoContext & _log, Id _id, void const * _value, tchar const * _name = nullptr);
	template <typename Class>
	void log_value(LogInfoContext & _log, Class const & _value, tchar const * _name = nullptr) { log_value(_log, get_id<Class>(), (plain_cast<Class>)(&_value), _name); }

	void construct(Id _id, void * _value);
	template <typename Class>
	void construct(Class & _value) { return construct(get_id<Class>(), &_value); }

	void destruct(Id _id, void * _value);
	template <typename Class>
	void destruct(Class & _value) { return destruct(get_id<Class>(), &_value); }

	void copy(Id _id, void * _value, void const * _source);
	template <typename Class>
	void copy(Class & _value, Class const & _source) { return copy(get_id<Class>(), &_value, & _source); }

	void log_all(LogInfoContext& _log);
};

template <typename Class>
inline int type_id()
{
	return RegisteredType::get_id<Class>();
}

template <typename Class>
inline int type_id_if_registered()
{
	return RegisteredType::get_id_if_registered<Class>();
}

inline TypeId type_id_none()
{
	return RegisteredType::none();
}

inline bool access_sub_field(Name const & _subField, void* _variable, TypeId _id, void* & _outSubField, TypeId & _outSubId)
{
	return RegisteredType::access_sub_field(_subField, _variable, _id, _outSubField, _outSubId);
}

inline bool get_sub_field(Name const & _subField, void const * _variable, TypeId _id, void const * & _outSubField, TypeId & _outSubId)
{
	return RegisteredType::get_sub_field(_subField, _variable, _id, _outSubField, _outSubId);
}

template <typename Class>
inline void construct(Class & _value)
{
	RegisteredType::construct<Class>(_value);
}

template <typename Class>
inline void destruct(Class & _value)
{
	RegisteredType::destruct<Class>(_value);
}

template <typename Class>
inline void copy(Class & _value, Class const & _source)
{
	RegisteredType::copy<Class>(_value, _source);
}

//

// just declare
// if something is defined twice, you need in a source file for object file causing problems, to add include to header to whatever is thought to be already defined
#define DECLARE_REGISTERED_TYPE(type) \
	template <> \
	int RegisteredType::get_id<type>(); \
	template <> \
	inline int RegisteredType::get_id_if_registered<type>() { return get_id<type>(); }

DECLARE_REGISTERED_TYPE(bool);
DECLARE_REGISTERED_TYPE(float);
DECLARE_REGISTERED_TYPE(int);

// for enums
/*
	for enums of this form:
	namespace/strut SomeEnum
	{
		enum Type
		{
			value0,
			value1,
			...
			MAX
		};

		static tchar const * to_char(Type _type);
	};
 
	in .h file
		DECLARE_REGISTERED_TYPE(SomeEnum::Type);

	in *RegisteredTypes.cpp file
		DEFINE_TYPE_LOOK_UP(SomeEnum::Type);
		ADD_SPECIALISATIONS_FOR_ENUM_USING_TO_CHAR(SomeEnum);
*/