#include "registeredType.h"

#include "..\serialisation\serialiser.h"

#define AN_REGISTER_TYPES_OFFSET 0

#include "registeredTypeRegistering.h"

#include "..\debug\logInfoContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

RegisteredTypeInfo* RegisteredTypeInfo::firstRegisteredTypeInfo = nullptr;

//

void RegisteredType::initialise_static()
{
}

void RegisteredType::close_static()
{
}

//

int ::RegisteredType::parse_type(String const & _type) { return parse_type(_type.to_char()); }
int ::RegisteredType::parse_type(tchar const * const _type)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_name(_type))
	{
		return foundType->id;
	}
	error(TXT("not recognised type for type look up\"%S\""), _type);
	todo_important(TXT("not recognised type for type look up\"%S\""), _type);
	return 0;
}

// parsing

void ::RegisteredType::parse_value(String const & _value, Id _intoId, void * _into)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_intoId))
	{
		if (foundType->parse_value)
		{
			foundType->parse_value(_value, _into);
			return; // everything parsed fine
		}
		else
		{
			todo_important(TXT("unable to parse! (%S)"), get_name_of(_intoId));
		}
	}
	else
	{
		todo_important(TXT("unable to parse!"));
	}
}

// loading

bool ::RegisteredType::load_from_xml(IO::XML::Node const * _node, Id _intoId, void * _into)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_intoId))
	{
		if (foundType->load_from_xml)
		{
			return foundType->load_from_xml(_node, _into);
		}
		else
		{
			todo_important(TXT("unable to load from xml! (%S)"), get_name_of(_intoId));
		}
	}
	else
	{
		todo_important(TXT("unable to load from xml!"));
	}
	return false;
}

// saving

bool ::RegisteredType::save_to_xml(IO::XML::Node * _node, Id _asId, void const * _value)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_asId))
	{
		if (foundType->save_to_xml)
		{
			return foundType->save_to_xml(_node, _value);
		}
		else
		{
			todo_important(TXT("unable to save to xml! (%S)"), get_name_of(_asId));
		}
	}
	else
	{
		todo_important(TXT("unable to save to xml!"));
	}
	return false;
}

// log value

void ::RegisteredType::log_value(LogInfoContext & _log, Id _id, void const * _value, tchar const * _name)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		if (foundType->log_value)
		{
			return foundType->log_value(_log, _value, _name? _name : TXT("??"));
		}
	}
	if (_name)
	{
		_log.log(TXT("%S : log not implemented"), _name);
	}
	else
	{
		_log.log(TXT("log not implemented"), _name);
	}
}

// other functions

tchar const * const ::RegisteredType::get_name_of(::TypeId _id)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		return foundType->name;
	}
	if (_id == NONE)
	{
		return TXT("not registered");
	}
	if (_id == type_id_none())
	{
		return TXT("<none>");
	}
	todo_important(TXT("unable to find id by name!"));
	return TXT("unrecognised");
}

TypeId RegisteredType::get_type_id_by_name(tchar const * _name)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_name(_name))
	{
		return foundType->id;
	}
	todo_important(TXT("unable to find id by name \"%S\"!"), _name);
	return NONE;
}

int ::RegisteredType::get_size_of(::TypeId _id)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		return (int)foundType->size;
	}
	todo_important(TXT("unable to find size!"));
	return 0;
}

int ::RegisteredType::get_alignment_of(::TypeId _id)
{
	if (RegisteredTypeInfo const* foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		return (int)foundType->alignment;
	}
	todo_important(TXT("unable to find alignment!"));
	return 0;
}

void ::RegisteredType::get_size_and_alignment_of(Id _id, OUT_ int& _size, OUT_ int& _alignment)
{
	if (RegisteredTypeInfo const* foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		_size = (int)foundType->size;
		_alignment = (int)foundType->alignment;
		return;
	}
	todo_important(TXT("unable to find alignment!"));
	_size = 0;
	_alignment = 0;
}

bool ::RegisteredType::is_plain_data(::TypeId _id)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		return foundType->plainData;
	}
	todo_important(TXT("unable to find plain data info!"));
	return false;
}

void const * ::RegisteredType::get_initial_value(::TypeId _id)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		return foundType->initialValue;
	}
	todo_important(TXT("unable to find initial value!"));
	return nullptr;
}

void ::RegisteredType::construct(::TypeId _id, void * _value)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		if (foundType->constructor)
		{
			foundType->constructor(_value);
		}
		return;
	}
	todo_important(TXT("unable to find constructor!"));
}

void ::RegisteredType::destruct(::TypeId _id, void * _value)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		if (foundType->destructor)
		{
			foundType->destructor(_value);
		}
		return;
	}
	todo_important(TXT("unable to find destructor!"));
}

void ::RegisteredType::copy(::TypeId _id, void * _value, void const * _from)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		if (foundType->copy)
		{
			foundType->copy(_value, _from);
		}
		else
		{
			memory_copy(_value, _from, foundType->size);
		}
		return;
	}
	todo_important(TXT("unable to find copy!"));
}
// sub fields

bool RegisteredType::access_sub_field(Name const & _subField, void* _variable, Id _id, void* & _outSubField, Id & _outSubId)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		if (RegisteredSubField const * foundSubField = foundType->find_sub_field(_subField))
		{
			_outSubField = (int8*)_variable + foundSubField->offset;
			_outSubId = foundSubField->id;
			return true;
		}
		todo_important(TXT("unable to find sub field \"%S\" for registered type info \"%S\""), _subField.to_char(), get_name_of(_id));
		return false;
	}
	todo_important(TXT("unable to find registered type info %i (subfield: %S)"), _id, _subField.to_char());
	return false;
}

bool RegisteredType::get_sub_field(Name const & _subField, void const * _variable, Id _id, void const * & _outSubField, Id & _outSubId)
{
	if (RegisteredTypeInfo const * foundType = RegisteredTypeInfo::find_by_id(_id))
	{
		if (RegisteredSubField const * foundSubField = foundType->find_sub_field(_subField))
		{
			_outSubField = (int8*)_variable + foundSubField->offset;
			_outSubId = foundSubField->id;
			return true;
		}
		todo_important(TXT("unable to find sub field \"%S\" for registered type info \"%S\""), _subField.to_char(), get_name_of(_id));
		return false;
	}
	todo_important(TXT("unable to find registered type info %i (subfield: %S)"), _id, _subField.to_char());
	return false;
}

void RegisteredType::log_all(LogInfoContext& _log)
{
	struct Log
	{
		TypeId type;
		String info;
	};
	Array<Log> all;
	
	RegisteredTypeInfo* iter = RegisteredTypeInfo::firstRegisteredTypeInfo;
	while (iter)
	{
		int atIdx = 0;
		while (all.is_index_valid(atIdx) && all[atIdx].type < iter->id)
		{
			++atIdx;
		}
		Log newOne;
		newOne.type = iter->id;
		newOne.info = String::printf(TXT("%08i:%4i:%S"), iter->id, iter->size, iter->name);
		all.insert_at(atIdx, newOne);
		iter = iter->next;
	}

	for_every(log, all)
	{
		_log.log(log->info.to_char());
	}
}

//

bool serialise_type_id(Serialiser & _serialiser, TypeId & _data)
{
	bool result = true;

	Name typeName;
	if (_serialiser.is_writing())
	{
		typeName = Name(::RegisteredType::get_name_of(_data));
	}

	result &= serialise_data(_serialiser, typeName);

	if (_serialiser.is_reading())
	{
		_data = ::RegisteredType::get_type_id_by_name(typeName.to_char());
		if (_data == NONE)
		{
			result = false;
		}
	}

	return result;
}
