template <typename DatumClass>
DatumClass & Data::operator [](REF_ Datum<DatumClass> const & _datum)
{
	an_assert(_datum.dataAddress != NONE);
	an_assert(data.is_index_valid(_datum.dataAddress));
	an_assert(data.is_index_valid(_datum.dataAddress + _datum.get_size() - 1));
	return *(plain_cast<DatumClass>(&data[_datum.dataAddress]));
}

template <typename DatumClass>
DatumClass const & Data::operator [](REF_ Datum<DatumClass> const & _datum) const
{
	an_assert(_datum.dataAddress != NONE);
	an_assert(data.is_index_valid(_datum.dataAddress));
	an_assert(data.is_index_valid(_datum.dataAddress + _datum.get_size() - 1));
	return *(plain_cast<DatumClass const>(&data[_datum.dataAddress]));
}
