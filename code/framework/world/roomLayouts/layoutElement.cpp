#include "layoutElement.h"

#include "objectLayout.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\library\usedLibraryStoredOrTagged.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

bool LayoutElement::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	chance = _node->get_float_attribute(TXT("chance"), chance);

	placement.load_from_xml_child_node(_node, TXT("placement"));

	offsetLocationRelative.load_from_xml_child_node(_node, TXT("offsetLocationRelative"));
	offsetLocationRelative.load_from_xml_child_node(_node, TXT("offsetLocation"));
	offsetRotation.load_from_xml_child_node(_node, TXT("offsetRotation"));

	if (_node->get_name() == TXT("mesh"))
	{
		mesh.load_from_xml(_node, _lc);
	}

	id = _node->get_name_attribute(TXT("id"), id);
	placeOnId = _node->get_name_attribute(TXT("placeOnId"), placeOnId);
	placeOnSocket = _node->get_name_attribute(TXT("placeOnSocket"), placeOnSocket);
	placeUsingSocket = _node->get_name_attribute(TXT("placeUsingSocket"), placeUsingSocket);

	for_every(node, _node->children_named(TXT("place")))
	{
		placeOnId = node->get_name_attribute(TXT("onId"), placeOnId);
		placeOnSocket = node->get_name_attribute(TXT("onSocket"), placeOnSocket);
		placeUsingSocket = node->get_name_attribute(TXT("usingSocket"), placeUsingSocket);
	}

	return result;
}

bool LayoutElement::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= mesh.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

Optional<Transform> LayoutElement::prepare_placement(Transform const& _mainPlacement, Array<LayoutElementMeshInstance> const& _alreadyPlacedMeshes, REF_ Random::Generator& _rg) const
{
	Transform resultPlacement = _mainPlacement;
	{
		if (placeOnId.is_valid())
		{
			bool isValid = false;
			for_every_reverse(m, _alreadyPlacedMeshes)
			{
				if (placeOnId == m->id)
				{
					resultPlacement = m->placement;
					if (placeOnSocket.is_valid())
					{
						int socketIdx = m->mesh->find_socket_index(placeOnSocket);
						if (socketIdx != NONE)
						{
							Transform socketMS = m->mesh->calculate_socket_using_ms(placeOnSocket);
							resultPlacement = resultPlacement.to_world(socketMS);
							isValid = true;
						}
					}
					break;
				}
			}
			if (!isValid)
			{
				return NP;
			}
		}
	}
	resultPlacement = resultPlacement.to_world(placement);
	if (offsetLocationRelative.has_something())
	{
		Vector3 offLoc;
		offLoc.x = offsetLocationRelative.x.is_empty()? 0.0f : _rg.get(offsetLocationRelative.x);
		offLoc.y = offsetLocationRelative.y.is_empty()? 0.0f : _rg.get(offsetLocationRelative.y);
		offLoc.z = offsetLocationRelative.z.is_empty()? 0.0f : _rg.get(offsetLocationRelative.z);
		resultPlacement = resultPlacement.to_world(Transform(offLoc, Quat::identity));
	}
	if (offsetRotation.has_something())
	{
		Rotator3 offRot;
		offRot.pitch = offsetRotation.pitch.is_empty()? 0.0f : _rg.get(offsetRotation.pitch);
		offRot.yaw   = offsetRotation.yaw.is_empty()  ? 0.0f : _rg.get(offsetRotation.yaw);
		offRot.roll  = offsetRotation.roll.is_empty() ? 0.0f : _rg.get(offsetRotation.roll);
		resultPlacement = resultPlacement.to_world(Transform(Vector3::zero, offRot.to_quat()));
	}
	return resultPlacement;
}

Optional<Transform> LayoutElement::alter_placement_with_place_using_socket(Transform const& _placement, Mesh const* _mesh) const
{
	an_assert(_mesh);
	Transform resultPlacement = _placement;
	if (placeUsingSocket.is_valid())
	{
		int socketIdx = _mesh->find_socket_index(placeUsingSocket);
		if (socketIdx == NONE)
		{
			return NP;
		}
		Transform socketMS = _mesh->calculate_socket_using_ms(socketIdx);
		Transform invertedSocketMS = socketMS.inverted();
		//invertedSocketMS.scale_translation(placement.get_scale());
		resultPlacement = resultPlacement.to_world(invertedSocketMS);
	}
	return resultPlacement;
}

Optional<Transform> LayoutElement::alter_placement_with_place_using_socket(Transform const& _placement, ObjectLayout const* _layout) const
{
	an_assert(_layout);
	Transform resultPlacement = _placement;
	if (placeUsingSocket.is_valid())
	{
		Optional<Transform> socketMS = _layout->find_socket_placement(placeUsingSocket);
		if (!socketMS.is_set())
		{
			return NP;
		}
		Transform invertedSocketMS = socketMS.get().inverted();
		//invertedSocketMS.scale_translation(placement.get_scale());
		resultPlacement = resultPlacement.to_world(invertedSocketMS);
	}
	return resultPlacement;
}
