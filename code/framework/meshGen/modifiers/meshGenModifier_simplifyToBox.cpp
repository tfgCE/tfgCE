#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenSimplifyToBoxInfo.h"
#include "..\meshGenValueDef.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

//

/**
 */
class SimplifyToBoxData
: public ElementModifierData
{
	FAST_CAST_DECLARE(SimplifyToBoxData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	SimplifyToBoxInfo simplifyToBoxInfo;
};

//

bool Modifiers::simplify_to_box(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
{
	bool result = true;

	Checkpoint checkpoint(_context);

	if (_subject)
	{
		result &= _context.process(_subject, &_instigatorInstance);
	}
	else
	{
		checkpoint = _context.get_checkpoint(1); // of parent
	}

	Checkpoint now(_context);
	
	if (checkpoint != now)
	{
		if (SimplifyToBoxData const* data = fast_cast<SimplifyToBoxData>(_data))
		{
			data->simplifyToBoxInfo.process(_context, _instigatorInstance, checkpoint.partsSoFarCount, false);
		}
	}
	
	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_simplify_to_box_data()
{
	return new SimplifyToBoxData();
}

//

REGISTER_FOR_FAST_CAST(SimplifyToBoxData);

bool SimplifyToBoxData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= simplifyToBoxInfo.load_from_xml(_node, _lc);

	return result;
}
