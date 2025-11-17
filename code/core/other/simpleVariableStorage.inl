#pragma once

void* SimpleVariableInfo::access_raw() const
{
	an_assert(storage != nullptr);
	return storage->access_raw(*this);
}

void const * SimpleVariableInfo::get_raw() const
{
	an_assert(storage != nullptr);
	return storage->get_raw(*this);
}

template <typename Class>
Class & SimpleVariableInfo::access() const
{
	an_assert(storage != nullptr);
	an_assert(typeID == ::type_id<Class>(), TXT("type id mismatch %S (%i) v %S (%i)"), RegisteredType::get_name_of(typeID), typeID, RegisteredType::get_name_of(::type_id<Class>()), ::type_id<Class>());
	return storage->access<Class>(*this);
}

template <typename Class>
Class const & SimpleVariableInfo::get() const
{
	an_assert(storage != nullptr);
	an_assert(typeID == ::type_id<Class>(), TXT("type id mismatch %S (%i) v %S (%i)"), RegisteredType::get_name_of(typeID), typeID, RegisteredType::get_name_of(::type_id<Class>()), ::type_id<Class>());
	return storage->access<Class>(*this);
}

template <typename Class>
void SimpleVariableInfo::look_up(SimpleVariableStorage & _storage)
{
	if (name.is_valid())
	{
		*this = _storage.find<Class>(name);
	}
}

//

void* SimpleVariableStorageBuffer::access_data(int _address)
{
	return &data[_address];
}

void const * SimpleVariableStorageBuffer::get_data(int _address) const
{
	return &data[_address];
}

//

void* SimpleVariableStorage::access_raw(Name const & _name, TypeId _id)
{
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == _id)
		{
			return access_data(variable->address);
		}
	}

	add(_name, _id);

	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == _id)
		{
			return access_data(variable->address);
		}
	}

	an_assert(false, TXT("should never get here!"));
	return nullptr;
}

void const * SimpleVariableStorage::get_raw(Name const & _name, TypeId _id) const
{
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == _id)
		{
			return get_data(variable->address);
		}
	}

	return nullptr;
}

void* SimpleVariableStorage::access_raw(SimpleVariableInfo const & _name)
{
	an_assert(_name.storage, TXT("no storage?"));
	an_assert(_name.storage == this, TXT("different storages"));
#ifdef AN_ASSERT
	an_assert(_name.storageRevision == revision, TXT("storage revision differs %p:%i v %p:%i"), _name.storage, _name.storageRevision, this, revision);
#endif
	return access_data(_name.address);
}

void const * SimpleVariableStorage::get_raw(SimpleVariableInfo const & _name) const
{
	an_assert(_name.storage, TXT("no storage?"));
	an_assert(_name.storage == this, TXT("different storages"));
#ifdef AN_ASSERT
	an_assert(_name.storageRevision == revision, TXT("storage revision differs %p:%i v %p:%i"), _name.storage, _name.storageRevision, this, revision);
#endif
	return get_data(_name.address);
}

template <typename Class>
inline Class & SimpleVariableStorage::access(Name const & _name)
{
	TypeId id = ::type_id<Class>();
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == id)
		{
			return *(plain_cast<Class>(access_data(variable->address)));
		}
	}
	return add<Class>(_name);
}

inline void * SimpleVariableStorage::access(Name const & _name, TypeId _id)
{
	SimpleVariableInfo const & info = find(_name, _id);
	return info.access_raw();
}

template <typename Class>
inline Class * SimpleVariableStorage::access_existing(Name const & _name)
{
	TypeId id = ::type_id<Class>();
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == id)
		{
			return (plain_cast<Class>(access_data(variable->address)));
		}
	}
	return nullptr;
}

inline void * SimpleVariableStorage::access_existing(Name const & _name, TypeId _id)
{
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == _id)
		{
			return access_data(variable->address);
		}
	}
	return nullptr;
}

template <typename Class>
inline Class const * SimpleVariableStorage::get_existing(Name const & _name) const
{
	TypeId id = ::type_id<Class>();
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == id)
		{
			return (plain_cast<Class>(get_data(variable->address)));
		}
	}
	return nullptr;
}

inline void const * SimpleVariableStorage::get_existing(Name const & _name, TypeId _id) const
{
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == _id)
		{
			return get_data(variable->address);
		}
	}
	return nullptr;
}

template <typename Class>
inline Class const & SimpleVariableStorage::get_value(Name const & _name, Class const & _fallbackValue) const
{
	TypeId id = ::type_id<Class>();
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == id)
		{
			return *(plain_cast<Class>(get_data(variable->address)));
		}
	}
	return _fallbackValue;
}

inline bool SimpleVariableStorage::get_existing_of_any_type_id(Name const & _name, OUT_ TypeId & _id, void const * & _value) const
{
	for_every(variable, variables)
	{
		if (variable->get_name() == _name)
		{
			_id = variable->type_id();
			_value = variable->get_raw();
			return true;
		}
	}
	return false;
}

inline bool SimpleVariableStorage::get_existing_type_id(Name const & _name, OUT_ TypeId & _id) const
{
	for_every(variable, variables)
	{
		if (variable->get_name() == _name)
		{
			_id = variable->type_id();
			return true;
		}
	}
	return false;
}

template <typename Class>
Class & SimpleVariableStorage::add(Name const & _name)
{
	add(_name, ::type_id<Class>());
	return access<Class>(_name);
}

template <typename Class>
inline Class & SimpleVariableStorage::access(SimpleVariableInfo const & _name)
{
	an_assert(_name.storage, TXT("no storage?"));
	an_assert(_name.storage == this, TXT("different storages"));
#ifdef AN_ASSERT
	an_assert(_name.storageRevision == revision, TXT("storage revision differs %p:%i v %p:%i"), _name.storage, _name.storageRevision, this, revision);
#endif
	an_assert(_name.typeID == ::type_id<Class>());
	return *(plain_cast<Class>(access_data(_name.address)));
}

template <typename Class>
SimpleVariableInfo const & SimpleVariableStorage::find(Name const & _name)
{
	TypeId id = ::type_id<Class>();
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == id)
		{
			an_assert(variable->storage == this);
			return *variable;
		}
	}
	add<Class>(_name);
	// it is added, find it again
	return find<Class>(_name);
}

inline SimpleVariableInfo const * SimpleVariableStorage::find_existing(Name const & _name, TypeId _id) const
{
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == _id)
		{
			an_assert(variable->storage == this);
			return variable;
		}
	}
	return nullptr;
}

inline SimpleVariableInfo const * SimpleVariableStorage::find_any_existing(Name const & _name) const
{
	for_every(variable, variables)
	{
		if (variable->name == _name)
		{
			an_assert(variable->storage == this);
			return variable;
		}
	}
	return nullptr;
}

template <typename Class>
inline SimpleVariableInfo const * SimpleVariableStorage::find_existing(Name const & _name) const
{
	TypeId id = ::type_id<Class>();
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == id)
		{
			an_assert(variable->storage == this);
			return variable;
		}
	}
	return nullptr;
}

template <typename Class>
inline void SimpleVariableStorage::invalidate(Name const & _name)
{
	invalidate(_name, ::type_id<Class>());
}

inline void SimpleVariableStorage::invalidate(Name const & _name, TypeId const & _id)
{
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == _id)
		{
			// does not call destructor and constructor again, just sits there
			variable->name = Name::invalid();
		}
	}
}

void* SimpleVariableStorage::access_data(SimpleVariableAddress _address)
{
	return dataBuffers[_address.dataBuffer]->access_data(_address.address);
}

void const * SimpleVariableStorage::get_data(SimpleVariableAddress _address) const
{
	return dataBuffers[_address.dataBuffer]->get_data(_address.address);
}
