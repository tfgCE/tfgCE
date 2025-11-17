#include "mggc_dev.h"

#include "..\..\debug\previewGame.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool Dev::check(ElementInstance & _instance) const
{
#ifdef AN_DEVELOPMENT
	if (auto* g = Game::get())
	{
		return ! g->should_generate_dev_meshes(atLevel);
	}
	return true;
#else
	return false;
#endif
}

bool Dev::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

#ifdef AN_DEVELOPMENT
	atLevel = _node->get_int_attribute_or_from_child(TXT("atLevel"), atLevel);
#endif

	return result;
}
