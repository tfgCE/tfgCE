#include "instanceData.h"

#include "instanceDataHandler.h"

using namespace ::Instance;

Data::Data(IDataSource* _dataSource, DataSetup const * _setup)
: dataSource(_dataSource)
{
	data.set_size(_setup->dataSize, true);
	memory_zero(data.get_data(), _setup->dataSize);
	
	process_data_handler(dataSource, DataHandler::datum_constructor().temp_ref());
}

Data::~Data()
{
	process_data_handler(dataSource, DataHandler::datum_destructor().temp_ref());
}

void Data::process_data_handler(IDataSource const* _dataSource, REF_ DataHandler & _dataProcessor)
{
	_dataProcessor.link_to(this);
	_dataSource->handle_instance_data(_dataProcessor);
}

