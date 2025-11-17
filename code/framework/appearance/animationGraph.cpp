#include "animationGraph.h"

#include "..\module\moduleAppearance.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(Framework::AnimationGraph);
LIBRARY_STORED_DEFINE_TYPE(AnimationGraph, animationGraph);

AnimationGraph::AnimationGraph(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
{
}

AnimationGraph::~AnimationGraph()
{
}

void AnimationGraph::prepare_to_unload()
{
	base::prepare_to_unload();
}

#ifdef AN_DEVELOPMENT
void AnimationGraph::ready_for_reload()
{
	base::ready_for_reload();

	nodeSet.clear();
}
#endif

bool AnimationGraph::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	result &= inputNode.load_from_xml(_node);

	error_loading_xml_on_assert(inputNode.get_id().is_valid(), result, _node, TXT("no input node id provided \"inputNodeId\""));

	result &= nodeSet.load_from_xml(_node, _lc);

	return result;
}

bool AnimationGraph::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = LibraryStored::prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		if (!prepare_runtime())
		{
			error(TXT("could not prepare animation graph"));
			result = false;
		}
	}

	return result;
}

void AnimationGraph::log(LogInfoContext & _context) const
{
	base::log(_context);
}

void AnimationGraph::prepare_instance(AnimationGraphRuntime& _runtime, ModuleAppearance* _owner)
{
	_runtime = this->runtime; // copy
	_runtime.owner = _owner;
	_runtime.imoOwner = _owner->get_owner();
	nodeSet.prepare_instance(_runtime);
}

bool AnimationGraph::prepare_runtime()
{
	bool result = true;

	runtime = AnimationGraphRuntime(); // reset

	result &= nodeSet.prepare_runtime(runtime);

	result &= inputNode.prepare_runtime(nodeSet);

	return result;
}

void AnimationGraph::on_activate(AnimationGraphRuntime& _runtime)
{
	if (auto* node = inputNode.get_node())
	{
		node->on_activate(_runtime);
	}
}

void AnimationGraph::on_reactivate(AnimationGraphRuntime& _runtime)
{
	if (auto* node = inputNode.get_node())
	{
		node->on_reactivate(_runtime);
	}
}

void AnimationGraph::on_deactivate(AnimationGraphRuntime& _runtime)
{
	if (auto* node = inputNode.get_node())
	{
		node->on_deactivate(_runtime);
	}
}

void AnimationGraph::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output)
{
	if (auto* node = inputNode.get_node())
	{
		node->execute(_runtime, _context, _output);
	}
	else
	{
		error(TXT("no graph node!"));
	}
}
