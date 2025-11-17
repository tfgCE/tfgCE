#include "meshGenElementNamedCheckpoint.h"

#include "meshGenGenerationContext.h"

#include "..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementNamedCheckpoint);

ElementNamedCheckpoint::~ElementNamedCheckpoint()
{
}

bool ElementNamedCheckpoint::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	start = _node->get_name_attribute(TXT("start"), start);
	end = _node->get_name_attribute(TXT("end"), end);

	return result;
}

bool ElementNamedCheckpoint::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementNamedCheckpoint::process(GenerationContext & _context, ElementInstance & _instance) const
{
	if (start.is_valid())
	{
		auto& nc = _context.access_named_checkpoint(start);
		nc.start.make(_context);
	}
	if (end.is_valid())
	{
		auto& nc = _context.access_named_checkpoint(end);
		nc.end.set_if_not_set();
		nc.end.access().make(_context);
	}
	return true;
}

#ifdef AN_DEVELOPMENT
void ElementNamedCheckpoint::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
}
#endif

