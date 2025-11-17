#include "meshGenModifiers.h"

#include "meshGenModifier_mirror.h"

#include "meshGenModifierUtils.h"

#include "..\meshGenValueDef.h"
#include "..\meshGenValueDefImpl.inl"
#include "..\meshGenUtils.h"

#include "..\..\..\core\collision\model.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

//

/**
 *	Allows:
 *		cut using cut axis and point
 */
class KaleidoscopeData
: public ElementModifierData
{
	FAST_CAST_DECLARE(KaleidoscopeData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	ValueDef<int> amount = 5;
	ValueDef<bool> mirrorElements = true; // if not mirrored, it will be just cut

	ValueDef<Transform> relPlacement;
};

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_kaleidoscope_data()
{
	return new KaleidoscopeData();
}

//

bool Modifiers::kaleidoscope(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (KaleidoscopeData const* data = fast_cast<KaleidoscopeData>(_data))
		{
			int amount = data->amount.get(_context);
			bool mirrorElements = data->mirrorElements.get(_context);
			Transform relPlacement = Transform::identity;
			if (data->relPlacement.is_set())
			{
				relPlacement = data->relPlacement.get(_context);
			}
			float angle = 360.0f / (float)amount;

			Vector3 cutDir = relPlacement.vector_to_world(-Vector3::xAxis.rotated_by_yaw(angle * 0.5f));
			Plane cutPlane = Plane(cutDir, Vector3::zero);

			Modifiers::Utils::cut_using_plane(_context, checkpoint, REF_ now, cutPlane);
			if (mirrorElements)
			{
				MeshMirrorData mirrorData;
				mirrorData.mirrorDir = relPlacement.get_axis(Axis::Right);

				result &= make_mirror(_context, _instigatorInstance, _subject, &mirrorData);
			}
			else
			{
				Vector3 secondCutDir = relPlacement.vector_to_world((Vector3::xAxis).rotated_by_yaw(-angle * 0.5f));
				Plane secondCutPlane = Plane(secondCutDir, Vector3::zero);
				Modifiers::Utils::cut_using_plane(_context, checkpoint, REF_ now, secondCutPlane);
			}

			Checkpoint postMirror(_context);

			for_count(int, iter, amount - 1)
			{
				Transform transform(Vector3::zero, Rotator3(0.0f, angle * (float)(iter + 1), 0.0f).to_quat());
				//transform = relPlacement.to_world(transform.to_world(relPlacement.inverted()));

				Matrix44 transformMat = transform.to_matrix();

				Checkpoint preCopy(_context);
				Modifiers::Utils::create_copy(_context, checkpoint, postMirror, Modifiers::Utils::Flags::Mesh);
				Checkpoint postCopy(_context);
				for (int i = preCopy.partsSoFarCount; i < postCopy.partsSoFarCount; ++i)
				{
					::Meshes::Mesh3DPart* part = _context.get_parts()[i].get();
					apply_transform_to_vertex_data(part->access_vertex_data(), part->get_number_of_vertices(), part->get_vertex_format(), relPlacement.inverted().to_matrix());
					apply_transform_to_vertex_data(part->access_vertex_data(), part->get_number_of_vertices(), part->get_vertex_format(), transformMat);
					apply_transform_to_vertex_data(part->access_vertex_data(), part->get_number_of_vertices(), part->get_vertex_format(), relPlacement.to_matrix());
				}
			}
		}
	}

	return result;
}

//

REGISTER_FOR_FAST_CAST(KaleidoscopeData);

bool KaleidoscopeData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	amount.load_from_xml_child_node(_node, TXT("amount"));
	mirrorElements.load_from_xml_child_node(_node, TXT("mirrorElements"));
	relPlacement.load_from_xml_child_node(_node, TXT("relPlacement"));
	relPlacement.load_from_xml_child_node(_node, TXT("placement"));

	return result;
}
