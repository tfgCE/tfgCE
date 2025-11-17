#pragma once

#include <functional>

#include "..\..\core\types\name.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct LibraryLoadingContext;

	typedef std::function< bool(IO::XML::Node const * _node, LibraryLoadingContext & _lc) > RunTool;

	class LibraryTool
	{
	public:
		LibraryTool(Name const & _toolName, RunTool _runTool);

		bool should_run_from_xml(IO::XML::Node const * _node) const;
		bool run_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:
		Name toolName;
		RunTool runTool;
	};
};
