#include "libraryStoredReplacer.h"

#include "libraryPrepareContext.h"

using namespace Framework;

bool LibraryStoredReplacerEntry::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;
	result &= replace.load_from_xml(_node->first_child_named(TXT("replace")), TXT("type"), TXT("name"), _lc);
	result &= with.load_from_xml(_node->first_child_named(TXT("with")), TXT("type"), TXT("name"), _lc);
	return result;
}

bool LibraryStoredReplacerEntry::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		result &= replace.prepare_for_game(_library, _pfgContext);
		result &= with.prepare_for_game(_library, _pfgContext);
	}
	return result;
}

bool LibraryStoredReplacerEntry::apply_to(REF_ UsedLibraryStoredAny & _usedLibraryStoredAny) const
{
	if (_usedLibraryStoredAny.get() == replace.get())
	{
		_usedLibraryStoredAny = with;
		return true;
	}
	else
	{
		return false;
	}
}

//

bool LibraryStoredReplacer::load_from_xml_child_node(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _childNodeName)
{
	return load_from_xml(_node->first_child_named(_childNodeName), _lc);
}

bool LibraryStoredReplacer::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;
	if (_node)
	{
		for_every(replacerNode, _node->children_named(TXT("replacer")))
		{
			LibraryStoredReplacerEntry replacer;
			if (replacer.load_from_xml(replacerNode, _lc))
			{
				replacers.push_back(replacer);
			}
			else
			{
				error(TXT("problem loading replacer"));
				result = false;
			}
		}
	}
	return result;
}

bool LibraryStoredReplacer::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	for_every(replacer, replacers)
	{
		result &= replacer->prepare_for_game(_library, _pfgContext);
	}
	return result;
}

bool LibraryStoredReplacer::apply_to(REF_ UsedLibraryStoredAny & _usedLibraryStoredAny) const
{
	for_every(replacer, replacers)
	{
		if (replacer->apply_to(REF_ _usedLibraryStoredAny))
		{
			return true;
		}
	}
	return false;
}
