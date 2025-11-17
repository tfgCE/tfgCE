#pragma once

#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\containers\array.h"

namespace Framework
{
	class LibraryStored;

	struct LibraryLoadSummary
	{
		// setup
	public:
		LibraryLoadSummary& store_loaded() { storeLoaded = true; return *this; }

		// library interface for storing
	public:
		void loaded(LibraryStored* _loaded);

		// output
	public:
		Array<LibraryStored*> const& get_loaded() const { return loadedObjects; }

	private:
		bool storeLoaded = false;

		// should be used for small things
		Concurrency::SpinLock lock = Concurrency::SpinLock(TXT("Framework.LibraryLoadSummary.lock")); // only used when loading and adding
		Array<LibraryStored*> loadedObjects;
	};

};
