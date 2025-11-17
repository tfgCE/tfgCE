#pragma once

#include "..\casts.h"
#include "..\containers\map.h"
#include "..\containers\array.h"
#include "..\containers\arrayStatic.h"
#include "..\other\registeredType.h"
#include "..\types\name.h"

#include <functional>

class SimpleVariableStorage;

struct LogInfoContext;

PLAIN_ struct SimpleVariableAddress
{
	int dataBuffer = NONE;
	int address = NONE; // in buffer
	SimpleVariableAddress() {}
	SimpleVariableAddress(int _dataBuffer, int _address) : dataBuffer(_dataBuffer), address(_address) {}

	bool is_valid() const { return dataBuffer != NONE && address != NONE; }

public:
	bool serialise(Serialiser & _serialiser);
};

PLAIN_ struct SimpleVariableInfo
{
public:
	static SimpleVariableInfo const & invalid() { return s_invalid; }

	SimpleVariableInfo() {}
	SimpleVariableInfo(Name const & _name) : name(_name) {}

	inline void* access_raw() const;
	inline void const * get_raw() const;

	template <typename Class>
	Class & access() const;

	template <typename Class>
	Class const & get() const;

	Name const & get_name() const { return name; }
	TypeId const & type_id() const { return typeID; }
	void set_name(Name const & _name);

	void mark_read_empty(bool _readEmpty = true) { readEmpty = _readEmpty; }
	bool was_read_empty() const { return readEmpty; }

	bool is_valid() const { return storage != nullptr; }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrName = TXT("id"));

	void invalidate_variable(); // will clear name too
	void invalidate_value(); // will remove content only, keeps name
	template <typename Class>
	void look_up(SimpleVariableStorage & _storage);
	void look_up(SimpleVariableStorage & _storage, TypeId _id);

public:
	bool serialise(Serialiser & _serialiser);

private:
	static SimpleVariableInfo s_invalid;

	GS_ Name name;
	GS_ TypeId typeID;
	GS_ SimpleVariableAddress address;
	bool readEmpty = false; // this is an information that it was read empty from xml
	SimpleVariableStorage* storage = nullptr;
#ifdef AN_ASSERT
	int storageRevision; // in case it was cleared
#endif
	SimpleVariableInfo(Name const & _name, TypeId _typeID, SimpleVariableAddress _address, SimpleVariableStorage* _storage);

	friend class SimpleVariableStorage;
};

struct SimpleVariableStorageBuffer
: public PooledObject<SimpleVariableStorageBuffer>
{
public:
	SimpleVariableStorageBuffer()
	{
		SET_EXTRA_DEBUG_INFO(data, TXT("SimpleVariableStorageBuffer.data"));
	}

	static int get_buffer_size() { return DATA_SIZE; }

	int allocate(int _size, int _align); // returns allocated address or NONE if no place available
	inline void* access_data(int _address);
	inline void const * get_data(int _address) const;

public:
	bool serialise(Serialiser & _serialiser);

private:
	static const int DATA_SIZE = 16384;

	ArrayStatic<int8, DATA_SIZE> data;
};

/**
 *	Simple variable storage - stores in buffers that are not relocated which grants that variables occupy same space for the lifetime.
 *	Calls constructors and destructors if provided.
 *	Allows invalidating to remove variable - hole might be replace by another variable (of same type).
 */
class SimpleVariableStorage
{
public:
	SimpleVariableStorage();
	~SimpleVariableStorage();

	SimpleVariableStorage(SimpleVariableStorage const & _other);
	SimpleVariableStorage& operator = (SimpleVariableStorage const & _other);
	bool operator == (SimpleVariableStorage const & _other) const;

	Array<SimpleVariableInfo> const & get_all() const { return variables; }

	void clear();
	bool is_empty() const { return variables.is_empty(); }

	void set(SimpleVariableInfo const & _var);
	void set_from(SimpleVariableStorage const & _other);
	void set_missing_from(SimpleVariableStorage const & _other);

	inline void * access_raw(Name const & _name, TypeId _id);
	
	inline void * access_raw(SimpleVariableInfo const & _name);

	inline void const * get_raw(Name const & _name, TypeId _id) const;

	inline void const * get_raw(SimpleVariableInfo const & _name) const;

	template <typename Class>
	inline Class & access(Name const & _name); // adds variable if not present

	inline void * access(Name const & _name, TypeId _id);

	template <typename Class>
	inline Class * access_existing(Name const & _name);

	inline void * access_existing(Name const & _name, TypeId _id);

	template <typename Class>
	inline Class & access(SimpleVariableInfo const & _name);

	template <typename Class>
	inline Class const * get_existing(Name const & _name) const;

	inline void const * get_existing(Name const & _name, TypeId _id) const;

	template <typename Class>
	inline Class const & get_value(Name const & _name, Class const & _fallbackValue) const;

	inline bool get_existing_of_any_type_id(Name const & _name, OUT_ TypeId & _id, void const * & _value) const;
	inline bool get_existing_type_id(Name const & _name, OUT_ TypeId & _id) const;

	template <typename Class>
	SimpleVariableInfo const & find(Name const & _name); // adds variable if not present

	template <typename Class>
	SimpleVariableInfo const * find_existing(Name const & _name) const; // doesn't add!

	inline SimpleVariableInfo const * find_existing(Name const & _name, TypeId _id) const; // doesn't add!
	
	inline SimpleVariableInfo const * find_any_existing(Name const & _name) const; // doesn't add!

	bool convert_existing(Name const & _name, TypeId _to); // doesn't add!
	bool rename_existing(Name const & _from, Name const & _to); // will remove _to

	SimpleVariableInfo const & find(Name const & _name, TypeId _id); // adds if not present
	
	void mark_read_empty(Name const & _name, TypeId _id, bool _readEmpty = true);

	template <typename Class>
	inline void invalidate(Name const & _name);

	inline void invalidate(Name const & _name, TypeId const & _id);

	bool load_variable_from_xml_as(IO::XML::Node const * _node, TypeId const & _id); // loads in form <name>value</name> instead <type name="name>value</type>
	bool load_variable_from_xml(IO::XML::Node const * _node); // load variable from this node
	bool load_from_xml(IO::XML::Node const * _node); // each child's name (node name) is type, it should also have "name" attribute to identify variable
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * const _childNodeName);
	bool save_variable_to_xml_child_node(IO::XML::Node * _node, SimpleVariableInfo const & _variable) const;
	bool save_to_xml(IO::XML::Node * _node) const;
	bool save_to_xml_child_node(IO::XML::Node * _node, tchar const * const _childNodeName) const;

	void log(LogInfoContext & _log) const;

	void do_for_every(std::function<void(SimpleVariableInfo & _info)> _do);
	void do_for_every(std::function<void(SimpleVariableInfo const & _info)> _do) const;

public:
	bool serialise(Serialiser & _serialiser);

protected:
#ifdef AN_ASSERT
	friend struct SimpleVariableInfo;
	int revision = 0;
#endif

	GS_ Array<SimpleVariableStorageBuffer*> dataBuffers;
	GS_ Array<SimpleVariableInfo> variables;

	template <typename Class>
	inline Class & add(Name const & _name);

	void add(Name const & _name, TypeId _type);

	SimpleVariableAddress allocate(int _size, int _align);
	inline void* access_data(SimpleVariableAddress _address);
	inline void const * get_data(SimpleVariableAddress _address) const;
};

#include "simpleVariableStorage.inl"

DECLARE_REGISTERED_TYPE(SimpleVariableInfo);
