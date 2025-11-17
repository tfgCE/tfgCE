#pragma once

#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\world\regionGeneratorInfo.h"

#include "..\ai\aiManagerInfo.h"

namespace TeaForGodEmperor
{
	namespace RegionGenerators
	{
		class Base
		: public Framework::RegionGeneratorInfo
		{
			FAST_CAST_DECLARE(Base);
			FAST_CAST_BASE(Framework::RegionGeneratorInfo);
			FAST_CAST_END();

			typedef RegionGeneratorInfo base;
		public:
			static void initialise_static();
			static void close_static();

			Base();
			virtual ~Base();

			AI::ManagerInfo const & get_ai_managers() const { return aiManagers; }

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			implement_ bool post_generate(Framework::Region* _region) const;

		private:
			AI::ManagerInfo aiManagers;
		};
	};

};
