#include "mggc_lod.h"

#include "..\meshGenConfig.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\debug\previewGame.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool LOD::check(ElementInstance & _instance) const
{
	int lod = _instance.get_context().get_for_lod();
	if (auto* g = Game::get())
	{
		lod = max(lod, g->get_generate_at_lod());
	}

	lod *= MeshGeneration::Config::get().get_lod_index_multiplier();

	if (!atLevelRange.is_empty())
	{
		return atLevelRange.does_contain(lod);
	}
	return lod <= atMaxLevel;
}

bool LOD::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (_node->has_attribute_or_child(TXT("atLevel")))
	{
		REMOVE_AS_SOON_AS_POSSIBLE_ warn_loading_xml(_node, TXT("change \"atLevel\" to \"atMaxLevel\""));
	}
	atMaxLevel = _node->get_int_attribute_or_from_child(TXT("atLevel"), atMaxLevel);
	atMaxLevel = _node->get_int_attribute_or_from_child(TXT("atMaxLevel"), atMaxLevel);
	atLevelRange.load_from_xml(_node, TXT("atLevelRange"));

	return result;
}
