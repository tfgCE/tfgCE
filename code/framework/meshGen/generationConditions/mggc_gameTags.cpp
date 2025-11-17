#include "mggc_gameTags.h"

#include "..\..\game\game.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool GameTags::check(ElementInstance & _instance) const
{
	if (auto* g = Game::get())
	{
		return condition.check(g->get_game_tags());
	}

	return false;
}

bool GameTags::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	condition.load_from_string(_node->get_text());

	return result;
}
