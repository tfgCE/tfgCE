#include "postProcessChainDefinition.h"

#include "postProcessChain.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.h"
#include "..\..\library\usedLibraryStored.inl"

using namespace Framework;

//

bool PostProcessChainDefinition::Element::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= postProcess.load_from_xml(_node, TXT("name"), _lc);
	if (!postProcess.is_name_valid())
	{
		error_loading_xml(_node, TXT("error loading post process name"));
		result = false;
	}

	result &= requires.load_from_xml_attribute_or_child_node(_node, TXT("requires"));
	result &= disallows.load_from_xml_attribute_or_child_node(_node, TXT("disallows"));

	return result;
}

bool PostProcessChainDefinition::Element::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= postProcess.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

//

REGISTER_FOR_FAST_CAST(PostProcessChainDefinition);
LIBRARY_STORED_DEFINE_TYPE(PostProcessChainDefinition, postProcessChainDefinition);

PostProcessChainDefinition::PostProcessChainDefinition(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
{
}

bool PostProcessChainDefinition::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(postProcessNode, _node->children_named(TXT("postProcess")))
	{
		Element element;
		if (element.load_from_xml(postProcessNode, _lc))
		{
			elements.push_back(element);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool PostProcessChainDefinition::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	for_every(element, elements)
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}
	return result;
}

void PostProcessChainDefinition::setup(PostProcessChain* _chain)
{
	_chain->clear();
	add_to(_chain);
}

void PostProcessChainDefinition::add_to(PostProcessChain* _chain)
{
	for_every(element, elements)
	{
		_chain->add(element->postProcess.get(), element->requires, element->disallows);
	}
}

