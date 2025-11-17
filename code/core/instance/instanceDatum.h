#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\debug\debug.h"

namespace Instance
{
	class Data;

	template <typename DatumClass>
	struct Datum
	{
	public:
		Datum();

		// building
		inline int get_size() const { return sizeof(DatumClass); }
		inline void add(REF_ Data & _data, REF_ int & _dataAddress);
		inline void construct(REF_ Data & _data) const;
		inline void destruct(REF_ Data & _data) const;

		// using
		inline DatumClass & access(REF_ Data & _data) const;

	private:
		int dataAddress; // address in data of Data

		friend class Data;
	};

};

#include "instanceDatum.inl"
