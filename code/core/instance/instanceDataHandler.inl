template <typename DatumClass>
void DataHandler::handle(Datum<DatumClass> const & _instanceDatum) const
{
	if (mode == DataHandlerMode::ConstructDatums)
	{
		an_assert(instanceData != nullptr);
		_instanceDatum.construct(REF_ *instanceData);
	}
	else if (mode == DataHandlerMode::DestructDatums)
	{
		an_assert(instanceData != nullptr);
		_instanceDatum.destruct(REF_ *instanceData);
	}
	else if (mode == DataHandlerMode::PrepareSetupAndDatums)
	{
		an_assert(prepareDataSetup != nullptr);
		(cast_to_nonconst<>(&_instanceDatum))->add(REF_ *instanceData, REF_ prepareDataSetup->dataSize);
	}
}
