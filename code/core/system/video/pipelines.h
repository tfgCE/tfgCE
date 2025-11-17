#pragma once

#include "../../types/string.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace System
{
	class Pipelines
	{
	public:
		static void initialise_static();
		static void setup_static();
		static void close_static();

	public:
		static bool load_source_from(REF_ String & _source, IO::XML::Node const * _node, tchar const * _childName, tchar const * _errorWhenEmpty);
	};

};
