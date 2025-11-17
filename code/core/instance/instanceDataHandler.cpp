#include "instanceData.h"
#include "instanceDataHandler.h"

using namespace ::Instance;

DataHandler::DataHandler(DataHandlerMode::Type _mode)
: mode(_mode)
, instanceData(nullptr)
, prepareDataSetup(nullptr)
, buildDataSetup(nullptr)
{
}

void DataHandler::prepare_setup_and_datums(IDataSource* _dataSource, DataSetup* _instanceDataSetup)
{
	DataHandler dataProcessor(DataHandlerMode::PrepareSetupAndDatums);
	dataProcessor.prepareDataSetup = _instanceDataSetup;
	dataProcessor.prepareDataSetup->dataSize = 0;
	_dataSource->handle_instance_data(REF_ dataProcessor);
}

DataHandler DataHandler::datum_constructor()
{
	DataHandler dataProcessor(DataHandlerMode::ConstructDatums);
	return dataProcessor;
}

DataHandler DataHandler::datum_destructor()
{
	DataHandler dataProcessor(DataHandlerMode::DestructDatums);
	return dataProcessor;
}

void DataHandler::link_to(Data * _instanceData)
{
	an_assert(instanceData == nullptr, TXT("should not be linked to anything else"));
	instanceData = _instanceData;
}
