#include "meshGenElementDoWheresMyPoint.h"

#include "meshGenGenerationContext.h"

#include "..\framework.h"

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementDoWheresMyPoint);

ElementDoWheresMyPoint::ElementDoWheresMyPoint()
{
	useStack = false;
}

ElementDoWheresMyPoint::~ElementDoWheresMyPoint()
{
}

bool ElementDoWheresMyPoint::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	forPreviewGameOnly = _node->get_bool_attribute(TXT("forPreviewGameOnly"), forPreviewGameOnly);

	result &= wheresMyPoint.load_from_xml(_node);

	return result;
}

bool ElementDoWheresMyPoint::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementDoWheresMyPoint::process(GenerationContext & _context, ElementInstance & _instance) const
{
	if (forPreviewGameOnly && ! Framework::is_preview_game())
	{
		// skip!
		return true;
	}
	WheresMyPoint::Value tempValue;
	WheresMyPoint::Context context(&_instance);
	context.set_random_generator(_instance.access_context().access_random_generator());
	if (wheresMyPoint.update(tempValue, context))
	{
		return true;
	}
	else
	{
		error_generating_mesh(_instance, TXT("error processing \"doWheresMyPoint\" element"));
		return false;
	}
}

