#include "testConfig.h"

#include "..\..\core\utils.h"
#include "..\..\core\memory\memory.h"
#include "..\..\core\io\xml.h"

using namespace Framework;

TestConfig* TestConfig::s_config = nullptr;

void TestConfig::initialise_static()
{
	if (!s_config)
	{
		s_config = new TestConfig();
	}
}

void TestConfig::close_static()
{
	delete_and_clear(s_config);
}

SimpleVariableStorage const & TestConfig::get_params()
{
	an_assert(s_config != nullptr);

	return s_config->params;
}

SimpleVariableStorage & TestConfig::access_params()
{
	an_assert(s_config != nullptr);

	return s_config->params;
}

bool TestConfig::load_from_xml(IO::XML::Node const * _node)
{
	an_assert(s_config != nullptr);

	bool result = true;

#ifndef BUILD_PUBLIC_RELEASE
	result &= s_config->params.load_from_xml_child_node(_node, TXT("params"));
#endif

	return result;
}

