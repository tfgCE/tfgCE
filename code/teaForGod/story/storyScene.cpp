#include "storyScene.h"

#include "storyExecution.h"

#include "..\library\library.h"

#include "..\..\framework\gameScript\gameScript.h"
#include "..\..\framework\library\usedLibraryStored.inl"

//

using namespace TeaForGodEmperor;
using namespace Story;

//

REGISTER_FOR_FAST_CAST(Scene);
LIBRARY_STORED_DEFINE_TYPE(Scene, storyScene);

Scene::Scene(Framework::Library* _library, Framework::LibraryName const& _name)
:base(_library, _name)
{
}

Scene::~Scene()
{
}

void Scene::load_data_on_demand_if_required()
{
	if (auto* g = gameScript.get())
	{
		g->load_elements_on_demand_if_required();
	}
}

bool Scene::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= gameScript.load_from_xml(_node, TXT("gameScript"), _lc);

	for_every(node, _node->children_named(TXT("requirements")))
	{
		result &= requirements.load_from_xml(node, _lc);
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

bool Scene::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= gameScript.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

bool AlterExecution::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= setsStoryFacts.load_from_xml_attribute_or_child_node(_node, TXT("setsStoryFacts"));
	result &= clearStoryFacts.load_from_xml_attribute_or_child_node(_node, TXT("clearStoryFacts"));
	clearStoryFactsPrefixed = _node->get_string_attribute_or_from_child(TXT("clearStoryFactsPrefixed"), clearStoryFactsPrefixed);

	return result;
}

void AlterExecution::perform(Execution& _execution) const
{
	Concurrency::ScopedMRSWLockWrite lock(_execution.access_facts_lock());

	if (!clearStoryFactsPrefixed.is_empty())
	{
		Tags tagsToRemove;
		tchar const* prefix = clearStoryFactsPrefixed.to_char();
		_execution.get_facts().do_for_every_tag([&tagsToRemove, prefix](Tag const& _tag)
			{
				if (_tag.get_tag().to_string().has_prefix(prefix))
				{
					tagsToRemove.set_tag(_tag.get_tag());
				}
			});
		_execution.access_facts().remove_tags(tagsToRemove);
	}
	_execution.access_facts().remove_tags(clearStoryFacts);
	_execution.access_facts().set_tags_from(setsStoryFacts);
}

//

bool Requirements::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= storyFacts.load_from_xml_attribute_or_child_node(_node, TXT("storyFacts"));

	return result;
}

bool Requirements::check(Execution const& _execution) const
{
	Concurrency::ScopedMRSWLockRead lock(_execution.access_facts_lock());
	return storyFacts.check(_execution.get_facts());
}