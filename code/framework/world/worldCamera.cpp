#include "worldCamera.h"

#include "..\world\doorInRoom.h"
#include "..\world\pointOfInterestInstance.h"
#include "..\world\room.h"

#include "..\module\modulePresence.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(fov);

//

Camera::Camera()
: owner(nullptr)
, inRoom(nullptr)
{
}

void Camera::clear_placement()
{
	inRoom = nullptr;
	placement = Transform::identity;
	owner = nullptr;
	ownerRelativePlacement = Transform::identity;
	vrAnchor = Transform::identity;
}

void Camera::set_placement_based_on(IModulesOwner const * _owner, CameraPlacementBasedOn::Type _basedOn)
{
	if (ModulePresence const * presence = _owner->get_presence())
	{
		// place in centre of presence
		// fix owner relative presence by this translation
		// we don't use offset_placement/location here as we are at CENTRE OF PRESENCE
		inRoom = presence->get_in_room();
		placement = presence->get_placement();
		vrAnchor = presence->get_vr_anchor();
		an_assert(vrAnchor.get_scale() > 0.0f);
		Vector3 const presenceCentreOffset = presence->get_centre_of_presence_os().get_translation();
		placement.set_translation(placement.location_to_world(presenceCentreOffset));
		owner = _owner;
		ownerRelativePlacement = Transform(-presenceCentreOffset, Quat::identity);
		if (_basedOn == CameraPlacementBasedOn::Eyes)
		{
			Transform relativePlacement = presence->get_eyes_relative_look();
			relativePlacement.set_translation(relativePlacement.get_translation() - presenceCentreOffset);
			offset_placement(relativePlacement);
		}
		if (_basedOn == CameraPlacementBasedOn::OwnerPlacement)
		{
			// move back but maybe we will hit doors?
			offset_location(-presenceCentreOffset);
		}
	}
}

void Camera::set_placement_based_on(PointOfInterestInstance* _foundPOI)
{
	an_assert(_foundPOI);
	inRoom = _foundPOI->get_room();
	placement = _foundPOI->calculate_placement();
	vrAnchor = inRoom->get_vr_anchor(placement); // uhm, should not be used at all?
	an_assert(vrAnchor.get_scale() > 0.0f);
	owner = nullptr;
	ownerRelativePlacement = Transform::identity;

	set_fov(_foundPOI->get_poi()->parameters.get_value<float>(NAME(fov), fov));
}

void Camera::offset_placement(Transform const & _inLocalSpace, bool _keepWithinRoom)
{
	offset_location(_inLocalSpace.get_translation(), _keepWithinRoom);
	offset_orientation(_inLocalSpace.get_orientation());
}

void Camera::offset_location(Vector3 const & _inLocalSpace, bool _keepWithinRoom)
{
	Transform ownerAbsolutePlacement = placement.to_world(ownerRelativePlacement);
	Transform nextPlacement = placement;
#ifdef AN_DEVELOPMENT
	float offsetLength = _inLocalSpace.length();
	float offsetLengthT = placement.vector_to_world(_inLocalSpace).length();
	an_assert(abs(offsetLength - offsetLengthT) < 0.001f);
#endif
	nextPlacement.set_translation(placement.get_translation() + placement.vector_to_world(_inLocalSpace));

	if (!_keepWithinRoom)
	{
		Room * intoRoom;
		Transform intoRoomTransform;
		if (inRoom &&
			inRoom->move_through_doors(placement, nextPlacement, OUT_ intoRoom, OUT_ intoRoomTransform))
		{
			inRoom = intoRoom;
			nextPlacement = intoRoomTransform.to_local(nextPlacement);
			ownerAbsolutePlacement = intoRoomTransform.to_local(ownerAbsolutePlacement);
			vrAnchor = intoRoomTransform.to_local(vrAnchor);
			an_assert(vrAnchor.get_scale() > 0.0f);
		}
	}

	placement = nextPlacement;
	ownerRelativePlacement = placement.to_local(ownerAbsolutePlacement);
}

void Camera::offset_orientation(Quat const & _inLocalOrientationSpace)
{
	Transform ownerAbsolutePlacement = placement.to_world(ownerRelativePlacement);
	Transform nextPlacement = placement;
	nextPlacement.set_orientation(nextPlacement.get_orientation().to_world(_inLocalOrientationSpace));
	placement = nextPlacement;
	ownerRelativePlacement = placement.to_local(ownerAbsolutePlacement);
}
