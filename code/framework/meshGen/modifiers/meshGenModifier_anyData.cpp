#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"

#include "..\..\appearance\appearanceControllerData.h"
#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\collision\model.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"
#include "..\..\..\core\types\vectorPacked.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

//

class AnyMeshDataData
: public ElementModifierData
{
	FAST_CAST_DECLARE(AnyMeshDataData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	Name outParam;
};

//

bool Modifiers::any_mesh_data(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
	
	bool valid = false;

	if (checkpoint != now)
	{
		for (int i = checkpoint.partsSoFarCount; i < _context.get_parts().get_size(); ++i)
		{
			if (!_context.get_parts()[i]->is_empty())
			{
				valid = true;
			}
		}
	}

	if (auto* d = fast_cast<AnyMeshDataData>(_data))
	{
		if (d->outParam.is_valid())
		{
			_context.set_parameter(d->outParam, type_id<bool>(), &valid, 0); // in parent
		}
	}

	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_any_mesh_data_data()
{
	return new AnyMeshDataData();
}

//

REGISTER_FOR_FAST_CAST(AnyMeshDataData);

bool AnyMeshDataData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	outParam = _node->get_name_attribute_or_from_child(TXT("outParam"), outParam);

	return result;
}
