#pragma once

#include "iInstanceDataSource.h"
#include "instanceDatum.h"

#include "..\containers\array.h"

namespace Instance
{
	struct DataBuilder;

	struct DataSetup
	{
		int dataSize; // size of all data in instance

		DataSetup()
		: dataSize(0)
		{
		}
	};

	/**
	 *	Data is data created for instance of "something" (might be graph, might be chain).
	 *	It should contain data that varies over lifetime of object it is instanced of.
	 */
	class Data
	{
	public:
		Data(IDataSource* _dataSource, DataSetup const * _setup);
		~Data();

		int8 * access_data() { return data.begin(); }

		template <typename DatumClass>
		inline DatumClass & operator [](REF_ Datum<DatumClass> const & _datum);

		template <typename DatumClass>
		inline DatumClass const & operator [](REF_ Datum<DatumClass> const & _datum) const;

	private:
		IDataSource* dataSource;
		Array<int8> data;

		void process_data_handler(IDataSource const * _dataSource, REF_ DataHandler & _dataHandler);
	};

	#include "instanceData.inl"
};
