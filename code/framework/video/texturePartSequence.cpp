#include "texturePartSequence.h"

#include "..\library\library.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

using namespace Framework;

bool TexturePartSequence::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	interval = _node->get_float_attribute(TXT("interval"), interval);

	for_every(node, _node->children_named(TXT("add")))
	{
		UsedLibraryStored<TexturePart> element;
		if (element.load_from_xml(node, TXT("texturePart"), _lc))
		{
			elements.push_back(element);
		}
		else
		{
			error_loading_xml(node, TXT("could not load texture part"));
			result = false;
		}
	}

	return result;
}

bool TexturePartSequence::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(node, _node->children_named(_childName))
	{
		result &= load_from_xml(node, _lc);
	}

	return result;
}

bool TexturePartSequence::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(element, elements)
	{
		result &= element->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}
