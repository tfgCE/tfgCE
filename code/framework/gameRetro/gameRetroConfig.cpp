#include "gameRetroConfig.h"

#include "..\..\core\utils.h"
#include "..\..\core\debug\debug.h"
#include "..\..\core\memory\memory.h"
#include "..\..\core\io\xml.h"

#include "..\..\framework\module\moduleData.h"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(GameRetroConfig);

void GameRetroConfig::initialise_static()
{
	base *prev = s_config;
	s_config = new GameRetroConfig();
	if (prev)
	{
		if (GameRetroConfig* prevAs = fast_cast<GameRetroConfig>(prev))
		{
			*s_config = *prevAs;
		}
		else
		{
			*s_config = *prev;
		}
		delete prev;
	}
}

GameRetroConfig::GameRetroConfig()
{
}

bool GameRetroConfig::internal_load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::internal_load_from_xml(_node);

	for_every(node, _node->children_named(TXT("retroInput")))
	{
		result &= inputConfig.load_from_xml(node);
	}

	return result;
}

void GameRetroConfig::internal_save_to_xml(IO::XML::Node * _node, bool _userSpecific) const
{
	base::internal_save_to_xml(_node, _userSpecific);

	inputConfig.save_to_xml(_node, _userSpecific);
}
