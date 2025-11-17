#include "meshGenModifiers.h"

#include "meshGenModifierUtils.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenValueDef.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

//

float cut_threshold() { return 0.001f; }

//

/**
 *	Allows:
 *		cut using cut axis and point
 */
class MeshCutData
: public ElementModifierData
{
	FAST_CAST_DECLARE(MeshCutData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	ValueDef<Vector3> cutDir = ValueDef<Vector3>(Vector3::zero);
	ValueDef<Vector3> cutPoint = ValueDef<Vector3>(Vector3::zero);
};

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_cut_data()
{
	return new MeshCutData();
}

//

bool Modifiers::cut(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (MeshCutData const * data = fast_cast<MeshCutData>(_data))
		{
			Plane cutPlane = Plane(data->cutDir.get(_context).normal(), data->cutPoint.get(_context));

			// cut existing using plane
			Modifiers::Utils::cut_using_plane(_context, checkpoint, REF_ now, cutPlane);
		}
	}

	return result;
}

//

REGISTER_FOR_FAST_CAST(MeshCutData);

bool MeshCutData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	cutDir.load_from_xml_child_node(_node, TXT("dir"));
	cutPoint.load_from_xml_child_node(_node, TXT("point"));

	if (cutDir.is_value_set())
	{
		if (cutDir.get_value().is_zero())
		{
			cutDir.set(Vector3::xAxis);
		}
	}

	return result;
}
