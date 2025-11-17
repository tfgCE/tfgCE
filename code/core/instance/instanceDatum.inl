#ifdef AN_CLANG
#include "..\memory\objectHelpers.h"
#include "..\casts.h"
#endif

template <typename DatumClass>
Instance::Datum<AN_NOT_CLANG_TYPENAME DatumClass>::Datum()
: dataAddress(NONE)
{
}

template <typename DatumClass>
void Instance::Datum<DatumClass>::add(REF_ Data & _data, REF_ int & _dataAddress)
{
	an_assert(dataAddress == NONE, TXT("datum has been already added?"));
	dataAddress = _dataAddress;
	_dataAddress += get_size();
}

template <typename DatumClass>
void Instance::Datum<DatumClass>::construct(REF_ Data & _data) const
{
	an_assert(dataAddress != NONE, TXT("datum should be added first to instance data"));
	ObjectHelpers<DatumClass>::call_constructor_on(plain_cast<DatumClass>(&_data[*this]));
}

template <typename DatumClass>
void Instance::Datum<DatumClass>::destruct(REF_ Data & _data) const
{
	an_assert(dataAddress != NONE, TXT("datum should be added first to instance data"));
	ObjectHelpers<DatumClass>::call_destructor_on(plain_cast<DatumClass>(&_data[*this]));
}

template <typename DatumClass>
DatumClass & Instance::Datum<DatumClass>::access(REF_ Data & _data) const
{
	an_assert(dataAddress != NONE, TXT("datum should be added first to instance data"));
	return *(plain_cast<DatumClass>(&_data[*this]));
}
