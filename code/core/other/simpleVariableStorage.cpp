#include "simpleVariableStorage.h"

#include "parserUtils.h"

#include "..\io\xml.h"
#include "..\serialisation\serialiser.h"
#include "..\tags\tag.h"
#include "..\tags\tagCondition.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

bool does_node_has_anything_else_besides_name(IO::XML::Node const * _node)
{
	for_every(attr, _node->all_attributes())
	{
		if (attr->get_name() != TXT("name"))
		{
			return true;
		}
	}
	return false;
}
//

bool SimpleVariableAddress::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, dataBuffer);
	result &= serialise_data(_serialiser, address);

	return result;
}

//

SimpleVariableInfo SimpleVariableInfo::s_invalid;

SimpleVariableInfo::SimpleVariableInfo(Name const & _name, TypeId _typeID, SimpleVariableAddress _address, SimpleVariableStorage* _storage)
: name(_name)
, typeID(_typeID)
, address(_address)
, storage(_storage)
#ifdef AN_ASSERT
, storageRevision(_storage ? _storage->revision : 0)
#endif
{
}

bool SimpleVariableInfo::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, name);
	result &= serialise_type_id(_serialiser, typeID);
	result &= serialise_data(_serialiser, address);

	return result;
}

bool SimpleVariableInfo::load_from_xml(IO::XML::Node const * _node, tchar const * _attrName)
{
	bool result = true;

	if (_node)
	{
		name = _node->get_name_attribute(_attrName, name);
		invalidate_value();
	}

	return result;
}

void SimpleVariableInfo::invalidate_variable()
{
	name = Name::invalid();
	invalidate_value();
}

void SimpleVariableInfo::invalidate_value()
{
	storage = nullptr;
	typeID = NONE;
	address = SimpleVariableAddress();
}

void SimpleVariableInfo::look_up(SimpleVariableStorage& _storage, TypeId _id)
{
	if (name.is_valid())
	{
		*this = _storage.find(name, _id);
	}
}

void SimpleVariableInfo::set_name(Name const & _name)
{
	if (name == _name)
	{
		return;
	}
	name = _name;
	if (name.is_valid() && typeID != type_id_none())
	{
		if (storage)
		{
			*this = storage->find(name, typeID);
		}
	}
	else
	{
		invalidate_value();
	}
}

//

int SimpleVariableStorageBuffer::allocate(int _size, int _align)
{
	an_assert(_size <= DATA_SIZE);
	int availableSpace = data.get_max_size() - data.get_size();
	int requiredSpace = _size + _align - 1; // alignment might be off by 1, we won't align to its full value
	if (availableSpace >= requiredSpace)
	{
		// align first
		if (_align != 0)
		{
			int alignementOff = data.get_size() % _align;
			if (alignementOff < 0)
			{
				warn_dev_investigate(TXT("alignementOff shouldn't be negative (%i), data size:%i, align:%i"), alignementOff, data.get_size(), _align);
				// if for some reason at this point we are unable to fit, bail out
				return NONE;
			}
			if (alignementOff != 0)
			{
				int offset = _align - alignementOff;
				if (offset > availableSpace)
				{
					warn_dev_investigate(TXT("offset won't fit (%i), data size:%i, available:%i, align:%i"), offset, data.get_size(), availableSpace, _align);
					// don't let the crash to happen
					return NONE;
				}
				data.grow_size(offset);
			}
			availableSpace = data.get_max_size() - data.get_size();
		}
		if (availableSpace >= _size)
		{
			int result = data.get_size();
			data.grow_size(_size);
			// clear memory, even if there's a constructor or anything else, it is just better to have the memory in the same state
			memory_zero(&data[result], _size);
			return result;
		}
	}
	return NONE;
}

bool SimpleVariableStorageBuffer::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_plain_data_array(_serialiser, data);

	return result;
}

//

SimpleVariableStorage::SimpleVariableStorage()
{
}

SimpleVariableStorage::~SimpleVariableStorage()
{
	clear();
}

SimpleVariableStorage::SimpleVariableStorage(SimpleVariableStorage const & _other)
{
	operator=(_other);
}

SimpleVariableStorage& SimpleVariableStorage::operator = (SimpleVariableStorage const & _other)
{
	clear();
	variables = _other.variables;
	auto otherVariable = _other.variables.begin_const();
#ifdef AN_ASSERT
	revision = _other.revision;
#endif
	for_every(variable, variables)
	{
		int size;
		int align;
		RegisteredType::get_size_and_alignment_of(variable->type_id(), OUT_ size, OUT_ align);

		// associate variables with current storage
		variable->storage = this;
		variable->address = allocate(size, align);
		RegisteredType::construct(variable->type_id(), variable->access_raw());
		RegisteredType::copy(variable->type_id(), variable->access_raw(), otherVariable->get_raw());
		++otherVariable;
	}

	return *this;
}

bool SimpleVariableStorage::operator == (SimpleVariableStorage const & _other) const
{
	if (variables.get_size() != _other.variables.get_size())
	{
		return false;
	}
	auto otherVariable = _other.variables.begin_const();
	for_every(variable, variables)
	{
		if (! memory_compare(variable->get_raw(), otherVariable->get_raw(), RegisteredType::get_size_of(variable->type_id())))
		{
			return false;
		}
		++otherVariable;
	}

	return true;
}

void SimpleVariableStorage::clear()
{
	for_every(variable, variables)
	{
		RegisteredType::destruct(variable->type_id(), variable->access_raw());
	}
	for_every_ptr(dataBuffer, dataBuffers)
	{
		delete dataBuffer;
	}
	dataBuffers.clear();
	variables.clear();
#ifdef AN_ASSERT
	++revision;
#endif
}

bool SimpleVariableStorage::load_variable_from_xml_as(IO::XML::Node const * _node, TypeId const & _id)
{
	bool result = true;

	if (!_node)
	{
		return true;
	}

	Name name(_node->get_name());
	if (name.is_valid())
	{
		if (_node->is_empty() && !does_node_has_anything_else_besides_name(_node))
		{
			mark_read_empty(name, _id);
		}
		SimpleVariableInfo const & added = find(name, _id);
		if (!RegisteredType::load_from_xml(_node, _id, added.access_raw()))
		{
			error_loading_xml(_node, TXT("error loading value for variable \"%S\" in simple variable storage"), name.to_char());
			result = false;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("there was no name for stored variable"));
		result = false;
	}

	return result;
}

bool SimpleVariableStorage::load_variable_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (!_node)
	{
		return true;
	}

	Name name = _node->get_name_attribute(TXT("name"));
	if (name.is_valid())
	{
		TypeId id = RegisteredType::parse_type(_node->get_name());
		if (_node->is_empty() && !does_node_has_anything_else_besides_name(_node))
		{
			mark_read_empty(name, id);
		}
		SimpleVariableInfo const & added = find(name, id);
		if (!RegisteredType::load_from_xml(_node, id, added.access_raw()))
		{
			error_loading_xml(_node, TXT("error loading value for variable \"%S\" in simple variable storage"), name.to_char());
			result = false;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("there was no name for stored variable"));
		result = false;
	}

	return result;
}

bool SimpleVariableStorage::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (!_node)
	{
		return true;
	}

	for_every(child, _node->all_children())
	{
		result &= load_variable_from_xml(child);
	}

	return result;
}

bool SimpleVariableStorage::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * const _childNodeName)
{
	bool result = true;

	for_every(node, _node->children_named(_childNodeName))
	{
		result &= load_from_xml(node);
	}

	return result;
}

bool SimpleVariableStorage::save_variable_to_xml_child_node(IO::XML::Node * _node, SimpleVariableInfo const & _variable) const
{
	auto* node = _node->add_node(RegisteredType::get_name_of(_variable.type_id()));
	node->set_attribute(TXT("name"), _variable.get_name().to_string());
	RegisteredType::save_to_xml(node, _variable.type_id(), _variable.get_raw());
	return true;
}

bool SimpleVariableStorage::save_to_xml(IO::XML::Node * _node) const
{
	bool result = true;
	for_every(variable, variables)
	{
		result &= save_variable_to_xml_child_node(_node, *variable);
	}
	return result;
}

bool SimpleVariableStorage::save_to_xml_child_node(IO::XML::Node * _node, tchar const * const _childNodeName) const
{
	if (variables.is_empty())
	{
		return true;
	}
	return save_to_xml(_node->add_node(_childNodeName));
}

void SimpleVariableStorage::mark_read_empty(Name const & _name, TypeId _id, bool _readEmpty)
{
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == _id)
		{
			an_assert(variable->storage == this);
			variable->mark_read_empty(_readEmpty);
			return;
		}
	}

	add(_name, _id);
	mark_read_empty(_name, _id); // will find as we just added it
}

SimpleVariableInfo const & SimpleVariableStorage::find(Name const & _name, TypeId _id)
{
	for_every(variable, variables)
	{
		if (variable->name == _name &&
			variable->typeID == _id)
		{
			an_assert(variable->storage == this);
			return *variable;
		}
	}

	add(_name, _id);
	return find(_name, _id); // will find as we just added it
}

void SimpleVariableStorage::add(Name const & _name, TypeId _id)
{
	SimpleVariableAddress address;
#ifndef AN_RELEASE
#ifndef AN_PROFILER
	for_every(variable, variables)
	{
		an_assert(variable->name != _name || variable->typeID != _id);
	}
#endif
#endif

	int typeSize;
	int typeAlign;
	RegisteredType::get_size_and_alignment_of(_id, OUT_ typeSize, OUT_ typeAlign);

	bool replacedExisting = false;
	for_every(variable, variables)
	{
		if (!variable->name.is_valid() && 
			RegisteredType::get_size_of(variable->typeID) == typeSize && 
			RegisteredType::get_alignment_of(variable->typeID) == typeAlign)
		{
			// replace existing, we need first to remove what was there and then construct it again
			{
				void* ptr = access_data(variable->address);
				RegisteredType::destruct(variable->typeID, ptr);
			}
			address = variable->address;
			*variable = SimpleVariableInfo(_name, _id, address, this);
			{
				void* ptr = access_data(address);
				RegisteredType::construct(_id, ptr);
			}
			replacedExisting = true;
			break;
		}
	}

	if (!replacedExisting)
	{
		address = allocate(typeSize, typeAlign);
		variables.push_back(SimpleVariableInfo(_name, _id, address, this));
		void* ptr = access_data(address);
		RegisteredType::construct(_id, ptr);
	}

	an_assert(address.is_valid());
	void* ptr = access_data(address);
	if (RegisteredType::is_plain_data(_id))
	{
		if (void const * initialValue = RegisteredType::get_initial_value(_id))
		{
			memory_copy(ptr, initialValue, typeSize);
		}
		else
		{
			// allocate clears memory
		}
	}
	else
	{
		if (void const * initialValue = RegisteredType::get_initial_value(_id))
		{
			RegisteredType::copy(_id, ptr, initialValue);
		}
		else
		{
			// constructor should handle this
		}
	}
}

void SimpleVariableStorage::set(SimpleVariableInfo const& _var)
{
	an_assert(this != _var.storage);
	void* dest = access_raw(_var.name, _var.typeID);
	if (RegisteredType::is_plain_data(_var.typeID))
	{
		memory_copy(dest, _var.get_raw(), RegisteredType::get_size_of(_var.typeID));
	}
	else
	{
		RegisteredType::copy(_var.typeID, dest, _var.get_raw());
	}
}

void SimpleVariableStorage::set_from(SimpleVariableStorage const & _other)
{
	an_assert(this != &_other);
	for_every(variable, _other.variables)
	{
		void* dest = access_raw(variable->name, variable->typeID);
		if (RegisteredType::is_plain_data(variable->typeID))
		{
			memory_copy(dest, variable->get_raw(), RegisteredType::get_size_of(variable->typeID));
		}
		else
		{
			RegisteredType::copy(variable->typeID, dest, variable->get_raw());
		}
	}
}

void SimpleVariableStorage::set_missing_from(SimpleVariableStorage const & _other)
{
	for_every(variable, _other.variables)
	{
		if (! get_raw(variable->name, variable->typeID))
		{
			void* dest = access_raw(variable->name, variable->typeID);
			if (RegisteredType::is_plain_data(variable->typeID))
			{
				memory_copy(dest, variable->get_raw(), RegisteredType::get_size_of(variable->typeID));
			}
			else
			{
				RegisteredType::copy(variable->typeID, dest, variable->get_raw());
			}
		}
	}
}

bool SimpleVariableStorage::serialise(Serialiser & _serialiser)
{
	bool result = true;

	if (_serialiser.is_writing())
	{
		for_every(variable, variables)
		{
			if (!RegisteredType::is_plain_data(variable->type_id()))
			{
				error_stop(TXT("can't serialise non plain data!"));
				result = false;
			}
		}
	}

	int dataBuffersCount = dataBuffers.get_size();
	result &= serialise_data(_serialiser, dataBuffersCount);
	if (_serialiser.is_reading())
	{
		an_assert(dataBuffers.is_empty());
		for_count(int, i, dataBuffersCount)
		{
			dataBuffers.push_back(new SimpleVariableStorageBuffer());
		}
	}
	for_every_ptr(dataBuffer, dataBuffers)
	{
		result &= serialise_data(_serialiser, *dataBuffer);
	}
	result &= serialise_data(_serialiser, variables);

	// restore pointers when reading
	if (_serialiser.is_reading())
	{
		for_every(variable, variables)
		{
			variable->storage = this;
#ifdef AN_ASSERT
			variable->storageRevision = revision;
#endif
		}
	}

	return result;
}

void SimpleVariableStorage::log(LogInfoContext & _log) const
{
	for_every(variable, variables)
	{
		if (variable->get_name().is_valid())
		{
			RegisteredType::log_value(_log, variable->type_id(), variable->get_raw(), variable->get_name().to_char());
		}
	}
}

void SimpleVariableStorage::do_for_every(std::function<void(SimpleVariableInfo & _info)> _do)
{
	for_every(variable, variables)
	{
		_do(*variable);
	}
}

void SimpleVariableStorage::do_for_every(std::function<void(SimpleVariableInfo const & _info)> _do) const
{
	for_every(variable, variables)
	{
		_do(*variable);
	}
}

SimpleVariableAddress SimpleVariableStorage::allocate(int _size, int _align)
{
	int dataBufferIdx = 0;
	for_every_ptr(dataBuffer, dataBuffers)
	{
		int allocatedAt = dataBuffer->allocate(_size, _align);
		if (allocatedAt != NONE)
		{
			return SimpleVariableAddress(dataBufferIdx, allocatedAt);
		}
		++dataBufferIdx;
	}
	SimpleVariableStorageBuffer* dataBuffer = new SimpleVariableStorageBuffer();
	dataBuffers.push_back(dataBuffer);
	return SimpleVariableAddress(dataBufferIdx, dataBuffer->allocate(_size, _align));
}

bool SimpleVariableStorage::convert_existing(Name const& _name, TypeId _to)
{
	int existing = 0;
	bool existsTo = false;
	for_every(variable, variables)
	{
		if (variable->name == _name)
		{
			++existing;
			if (variable->typeID == _to)
			{
				existsTo = true;
			}
		}
	}

	if (existing > 1 && existsTo)
	{
		// remove the one that exists and replace it with something else
		invalidate(_name, _to);
	}

	if (auto* info = find_any_existing(_name))
	{
		if (info->type_id() == _to)
		{
			// a job well done!
			return true;
		}
		if (info->type_id() == type_id<Name>())
		{
			Name value = get_value<Name>(_name, Name::invalid());
			invalidate<Name>(_name); // act as removed
			if (_to == type_id<Tags>())
			{
				auto& t = access<Tags>(_name);
				t.clear();
				t.set_tag(value);
				return true;
			}
			if (_to == type_id<TagCondition>())
			{
				auto& t = access<TagCondition>(_name);
				t.load_from_string(value.to_string());
				return true;
			}
		}
		error(TXT("unsupported conversion from \"%S\" to \"%S\", implement?"), RegisteredType::get_name_of(info->type_id()), RegisteredType::get_name_of(_to));
		return false;
	}
	error(TXT("variable to convert not found \"%S\""), _name.to_char());
	return false;
}

bool SimpleVariableStorage::rename_existing(Name const& _from, Name const& _to)
{
	Optional<TypeId> fromType;
	bool renamed = false;
	for_every(variable, variables)
	{
		if (variable->name == _to)
		{
			variable->invalidate_variable();
		}
		else if (variable->name == _from)
		{
			variable->name = _to;
			renamed = true;
		}
	}
	return renamed;
}
