#pragma once

#include "..\library\usedLibraryStored.h"

namespace Framework
{
	// include in cpp:
	//	#include "customLibraryDataLookup.inl"

	template<typename Class>
	struct CustomLibraryDataSingle
	{
	public:
		bool load_from_xml(IO::XML::Node const * _node, tchar const * const _attr, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		Class* get() const { return lookedUp; }

	private:
		UsedLibraryStored<CustomLibraryStoredData> use;
		Class* lookedUp = nullptr;
	};

	template<typename Class>
	struct CustomLibraryDataLookup
	{
	public:
		bool load_from_xml(IO::XML::Node const * _node, tchar const * const _childrenName, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		Class* get(Name const & _id) const;

	private:
		struct Element
		{
			Name id;
			CustomLibraryDataSingle<Class> data;
		};
		Array<Element> datas;
	};
};
