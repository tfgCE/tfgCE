#pragma once

#include "..\globalDefinitions.h"

namespace Instance
{
	class DataHandler;

	/**
	 *	This is interface for class that works as source for instance data.
	 */
	interface_class IDataSource
	{
	public:
		virtual void handle_instance_data(REF_ DataHandler const & _dataHandler) const = 0;
	};

};
