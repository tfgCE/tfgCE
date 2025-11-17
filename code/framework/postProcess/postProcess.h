#pragma once

#include "..\debugSettings.h"
#include "..\library\libraryStored.h"
#include "graph\postProcessGraph.h"

namespace Framework
{
#ifdef AN_LOG_POST_PROCESS
	class PostProcessDebugSettings
	{
	public:
		static bool log;
		static bool logRT;
		static bool logRTDetailed;
	};
#endif

	class PostProcess
	: public LibraryStored
	{
		FAST_CAST_DECLARE(PostProcess);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		PostProcess(Library * _library, LibraryName const & _name);

		PostProcessGraph* get_graph() { return graph; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~PostProcess();

	private:
		PostProcessGraph* graph;

	};

};
