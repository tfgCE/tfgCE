#include "libraryStoredRenamer.h"

#include "libraryPrepareContext.h"
#include "libraryStoredReplacer.h"

using namespace Framework;

bool LibraryStoredRenamer::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(renameNode, _node->children_named(TXT("rename")))
	{
		String renameFrom = renameNode->get_string_attribute(TXT("from"));
		String renameTo = renameNode->get_string_attribute(TXT("to"));
		if (! renameFrom.is_empty())
		{
			renames.push_back(Rename(renameFrom, renameTo));
		}
		else
		{
			error_loading_xml(renameNode, TXT("rename node doesn't have \"from\""));
			result = false;
		}
	}

	return result;
}

bool LibraryStoredRenamer::load_from_xml_child_node(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _childNodeName)
{
	bool result = true;

	for_every(node, _node->children_named(_childNodeName))
	{
		result &= load_from_xml(node, _lc);
	}

	return result;
}

bool LibraryStoredRenamer::apply_to(REF_ LibraryName & _name) const
{
	bool changedAnything = false;
	String objectName = _name.to_string();
	for_every(rename, renames)
	{
		changedAnything |= objectName.replace_inline(rename->renameFrom, rename->renameTo);
	}
	if (changedAnything)
	{
		_name = LibraryName(objectName);
	}
	return changedAnything;
}

bool LibraryStoredRenamer::apply_to(REF_ UsedLibraryStoredAny & _object, Library* _library) const
{
	bool result = true;
	if (_object.get_name().is_valid())
	{
		LibraryName objectName = _object.get_name();
		if (apply_to(REF_ objectName))
		{
			if (_library)
			{
				result &= _object.set_and_find(LibraryName(objectName), _object.get_type(), _library);
			}
			else
			{
				_object.set(LibraryName(objectName), _object.get_type());
			}
		}
	}
	return result;
}

bool LibraryStoredRenamer::apply_to(REF_ LibraryStoredReplacer & _replacer, Library* _library) const
{
	bool result = true;

	for_every(replacer, _replacer.replacers)
	{
		result &= apply_to(REF_ replacer->replace, _library);
		result &= apply_to(REF_ replacer->with, _library);
	}

	return result;
}
