#pragma once

#include "..\casts.h"
#include "..\concurrency\mrswLock.h"
#include "..\containers\map.h"
#include "..\other\registeredType.h"
#include "..\types\optional.h"
#include "..\types\string.h"

struct CustomParameterCachedValueBase;

class CustomParameters;

/**
 *	For bools if no value is given, it is considered to be true
 */
struct CustomParameter
{
public:
	~CustomParameter();
	static CustomParameter const & not_set() { return s_notSet; }
	
	inline void set(String const & _value);
	inline void set_as_param_name(Name const & _param);

	inline bool is_param_name() const { return asParamName.is_valid(); }
	inline Name const & get_as_param_name() const { an_assert(asParamName.is_valid()); return asParamName; }

	template <typename Class>
	inline Class get_as() const; // may get cached value

	template <typename Class>
	inline Class get_as(Class const & _defValue) const; // may get cached value

	template <typename Class>
	void parse_into_as(REF_ Class & _into) const; // always parses

	void touch() {}

	bool is_set() const { return ! is_not_set(); }

private:
	static CustomParameter s_notSet;

	String asString; // without param suffix
	mutable Concurrency::MRSWLock lock;
	mutable Map<TypeId, CustomParameterCachedValueBase*> cachedValues;

	Name asParamName; // if loaded with "Param" suffix

	static CustomParameter & accessible_not_set() { return s_notSet; }

	template <typename Class>
	void get_cached_or_parse(REF_ Class & _into) const;

	void clear_cached_values();

	bool is_not_set() const { return this == &s_notSet; }

	friend class CustomParameters;
};

/**
 *	These are any random parameters that are stored as string and parsed to any value as required.
 */
class CustomParameters
{
public:
	CustomParameters();
	~CustomParameters();

	void set(Name const & _parName, String const & _parValue = String::empty());
	bool is_set(Name const & _parName) const;
	CustomParameter const & get(Name const & _parName) const;

	bool load_from_xml(IO::XML::Node const * _node); // reads only from child nodes! although for some instances it might be better to do this by hand, as we may want to omit some nodes

protected:
	Map<Name, CustomParameter> parameters;

	CustomParameter & access(Name const & _parName); // creates new if doesn't exist

	void set(String const & _parName, String const & _parValue = String::empty());

private:
	//CustomParameters(CustomParameters const & _other);
	//CustomParameters& operator=(CustomParameters const& _other);
};

struct CustomParameterCachedValueBase
{
	TypeId type;
	CustomParameterCachedValueBase(TypeId const & _type) : type(_type) {}
	virtual ~CustomParameterCachedValueBase() {}
};

template <typename Class>
struct CustomParameterCachedValue
: public CustomParameterCachedValueBase
{
	Class value;
	CustomParameterCachedValue(Class const & _value) : CustomParameterCachedValueBase(type_id<Class>()), value(_value) {}
	virtual ~CustomParameterCachedValue() {}
};

#include "customParameters.inl"
