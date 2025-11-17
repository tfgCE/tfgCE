#include "libraryTool.h"

#include "..\..\core\io\xml.h"

using namespace Framework;

LibraryTool::LibraryTool(Name const & _toolName, RunTool _runTool)
: toolName(_toolName)
, runTool(_runTool)
{
}

bool LibraryTool::should_run_from_xml(IO::XML::Node const * _node) const
{
	return _node && _node->get_name() == toolName.to_string();
}

bool LibraryTool::run_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (runTool)
	{
		return runTool(_node, _lc);
	}
	else
	{
		error(TXT("no tool defined"));
		return false;
	}
}
