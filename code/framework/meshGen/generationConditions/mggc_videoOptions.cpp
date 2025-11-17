#include "mggc_videoOptions.h"

#include "..\..\..\core\mainConfig.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool VideoOptions::check(ElementInstance & _instance) const
{
	return condition.check(::MainConfig::global().get_video().options);
}

bool VideoOptions::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	condition.load_from_string(_node->get_text());

	return result;
}
