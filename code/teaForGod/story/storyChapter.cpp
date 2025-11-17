#include "storyChapter.h"

#include "storyScene.h"

#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace Story;

//

REGISTER_FOR_FAST_CAST(Chapter);
LIBRARY_STORED_DEFINE_TYPE(Chapter, storyChapter);

Chapter::Chapter(Framework::Library* _library, Framework::LibraryName const& _name)
:base(_library, _name)
{
}

Chapter::~Chapter()
{
}

bool Chapter::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("scene")))
	{
		Framework::UsedLibraryStored<Scene> scene;
		if (scene.load_from_xml(node, _lc) &&
			scene.is_name_valid())
		{
			scenes.push_back(scene);
		}
		else
		{
			error_loading_xml(node, TXT("couldn't read scene"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("atCleanStart")))
	{
		result &= atCleanStart.load_from_xml(node, _lc);
	}
	for_every(node, _node->children_named(TXT("atStart")))
	{
		result &= atStart.load_from_xml(node, _lc);
	}
	for_every(node, _node->children_named(TXT("atEnd")))
	{
		result &= atEnd.load_from_xml(node, _lc);
	}

	return result;
}

bool Chapter::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(scene, scenes)
	{
		result &= scene->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	return result;
}

void Chapter::load_data_on_demand_if_required()
{
	for_every_ref(scene, scenes)
	{
		scene->load_data_on_demand_if_required();
	}
}
