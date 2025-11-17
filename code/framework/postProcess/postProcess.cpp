#include "postProcess.h"

#include "..\..\core\graph\graphImplementation.h"

using namespace Framework;

//

#ifdef AN_LOG_POST_PROCESS
bool PostProcessDebugSettings::log = false;
bool PostProcessDebugSettings::logRT = false;
bool PostProcessDebugSettings::logRTDetailed = false;
#endif

//

REGISTER_FOR_FAST_CAST(PostProcess);
LIBRARY_STORED_DEFINE_TYPE(PostProcess, postProcess);

PostProcess::PostProcess(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, graph(nullptr)
{

}

PostProcess::~PostProcess()
{
	delete_and_clear(graph);
}

bool PostProcess::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);
	if (!graph)
	{
		graph = new PostProcessGraph();
	}

	// load graph
	result &= graph->load_from_xml(_node->first_child_named(TXT("graph")), _lc);

	return result;
}

bool PostProcess::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		result &= base::prepare_for_game(_library, _pfgContext);
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::ShaderProgramsLoaded)
	{
		if (graph)
		{
			result &= graph->load_shader_programs(_library);
		}
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::ConnectNodesInPostProcess)
	{
		if (graph)
		{
			result &= graph->ready_to_use();
		}
	}
	return result;
}


