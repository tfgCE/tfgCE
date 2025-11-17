#pragma once

#include "..\..\core\casts.h"
#include "..\..\core\other\simpleVariableStorage.h"

struct Name;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct TestConfig
	{
	public:
		static SimpleVariableStorage & access_params();
		static SimpleVariableStorage const & get_params();

		static void initialise_static();
		static void close_static();

		static bool load_from_xml(IO::XML::Node const * _node);
	public:
		static TestConfig* s_config;

		SimpleVariableStorage params;
	};

};
