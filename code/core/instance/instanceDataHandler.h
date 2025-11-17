#pragma once

#include "instanceData.h"
#include "instanceDatum.h"
#include "iInstanceDataSource.h"

namespace Instance
{
	class Data;

	template <typename DatumClass> struct Datum;

	namespace DataHandlerMode
	{
		enum Type
		{
			PrepareSetupAndDatums, // just get size of all datums
			ConstructDatums, // only construction of datums (if instance data is reused)
			DestructDatums, // destroying all datums
		};
	};

	/**
	 *	Processor is used to handle instance datums provided by various objects.
	 *	It might be used to add to instance, construct or destroy datums in Data.
	 */
	class DataHandler
	{
	public:
		static void prepare_setup_and_datums(IDataSource* _dataSource, DataSetup* _instanceDataSetup);

		template <typename DatumClass>
		void handle(Datum<DatumClass> const & _instanceDatum) const;

		DataHandler& temp_ref() { return *this; }

	protected: friend class Data;
		void link_to(Data * _instanceData);
		static DataHandler datum_constructor();
		static DataHandler datum_destructor();

	private:
		DataHandlerMode::Type mode;

		Data * instanceData; // works on this instance data

		DataSetup* prepareDataSetup;
		DataSetup const * buildDataSetup;

		DataHandler(DataHandlerMode::Type _mode);
	};

	#include "instanceDataHandler.inl"
};
