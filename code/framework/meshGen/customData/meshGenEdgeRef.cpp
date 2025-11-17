#include "meshGenEdgeRef.h"

#include "meshGenEdge.h"
#include "meshGenSpline.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenParamImpl.inl"

#include "..\..\library\library.h"
#include "..\..\library\libraryName.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace CustomData;

//

Edge* EdgeRef::get(MeshGeneration::GenerationContext const & _context) const
{
	if (useEdge.is_set())
	{
		auto ue = useEdge; // temporary copy
		ue.fill_value_with(_context);
		if (ue.is_set())
		{
			if (CustomLibraryStoredData * found = Library::get_current()->get_custom_datas().find(ue.get(&_context, Framework::LibraryName::invalid())))
			{
				return fast_cast<Edge>(found->access_data());
			}
		}
	}
	return edge.get();
}

bool EdgeRef::load_from_xml(IO::XML::Node const * _node, tchar const * _useEdgeAttr, tchar const * _ownEdgeChild, LibraryLoadingContext & _lc)
{
	bool result = true;

	useEdge.load_from_xml(_node, _useEdgeAttr);
	_lc.load_group_into(useEdge);

	if (!useEdge.is_set())
	{
		edge = new Edge();
		if (IO::XML::Node const * node = _ownEdgeChild == nullptr? _node : _node->first_child_named(_ownEdgeChild))
		{
			result &= edge->load_from_xml(node, _lc);
		}
		else
		{
			error_loading_xml(_node, TXT("no edge defined (should have own edge or use exiting)"));
			result = false;
		}
	}
	else
	{
		// pointer will be set in prepare (or never if param is provided)
		edge = nullptr;
	}

	return result;
}

bool EdgeRef::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (useEdge.is_value_set())
	{
		IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
		{
			an_assert(!edge.is_set(), TXT("should be deleted by now!"));
			if (CustomLibraryStoredData * found = _library->get_custom_datas().find(useEdge.get()))
			{
				edge = fast_cast<Edge>(found->access_data());
			}
			if (! edge.is_set())
			{
				error(TXT("couldn't find cross section \"%S\""), useEdge.get().to_string().to_char());
				result = false;
			}
		}
	}
	else if (useEdge.is_set())
	{
		// it's ok, it is set as param
	}
	else
	{
		an_assert(edge.is_set());
		result &= edge->prepare_for_game(_library, _pfgContext);
	}

	return result;
}
