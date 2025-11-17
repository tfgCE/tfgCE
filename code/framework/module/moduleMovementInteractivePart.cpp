#include "moduleMovementInteractivePart.h"

#include "registeredModule.h"
#include "modules.h"
#include "moduleDataImpl.inl"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(ModuleMovementInteractivePart);

ModuleMovementInteractivePart::ModuleMovementInteractivePart(IModulesOwner* _owner)
: base(_owner)
{
}

void ModuleMovementInteractivePart::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		moduleMovementInteractivePartData = fast_cast<ModuleMovementInteractivePartData>(_moduleData);
	}
}

void ModuleMovementInteractivePart::update_base_object()
{
	auto * presence = get_owner()->get_presence();
	IModulesOwner* newBaseObject = nullptr;
	if (auto * a = presence->get_attached_to()) { newBaseObject = a; }
	if (auto * a = presence->get_based_on()) { newBaseObject = a; }
	if (baseObject.get() != newBaseObject)
	{
		mark_requires_movement();
	}
	baseObject = newBaseObject;
	if (newBaseObject)
	{
		if (auto * a = newBaseObject->get_appearance())
		{
			if (auto * m = a->get_mesh())
			{
				if (m->get_skeleton())
				{
					// keep updating for skeletal mesh
					mark_requires_movement(true);
				}
			}
		}
	}
}

void ModuleMovementInteractivePart::activate()
{
	auto * presence = get_owner()->get_presence();
	relativeInitialPlacementAttachedToBoneIdx = NONE;
	relativeInitialPlacementAttachedToSocketIdx = NONE;
	relativeInitialPlacement = Transform::identity;

	update_base_object();
	initialPlacementWS = presence->get_placement();
	if (!baseObject.is_set())
	{
		placementType = InWorldSpace;
	}
	else
	{
		placementType = RelativeToBaseObject;
		relativeInitialPlacement = baseObject->get_presence()->get_placement().to_local(presence->get_placement());
		if (presence->is_attached())
		{
			placementType = RelativeToAttachmentPlacement;
			int boneIdx;
			Transform relativePlacement;
			if (presence->get_attached_to_bone_info(OUT_ boneIdx, OUT_ relativePlacement))
			{
				relativeInitialPlacementAttachedToBoneIdx = boneIdx;
				relativeInitialPlacement = relativePlacement;
			}
			else
			{
				int socketIdx;
				if (presence->get_attached_to_socket_info(OUT_ socketIdx, OUT_ relativePlacement))
				{
					relativeInitialPlacementAttachedToSocketIdx = boneIdx;
					relativeInitialPlacement = relativePlacement;
				}
			}
		}
	}

	if (placementType == RelativeToAttachmentPlacement &&
		relativeInitialPlacementAttachedToBoneIdx == NONE &&
		relativeInitialPlacementAttachedToSocketIdx == NONE)
	{
		placementType = RelativeToBaseObject;
	}
}

void ModuleMovementInteractivePart::update_base_placement()
{
	basePlacementValid = false;

	auto * presence = get_owner()->get_presence();

	basePlacement.clear();
	switch (placementType)
	{
	case InWorldSpace:
		break;
	case RelativeToAttachmentPlacement:
		if (relativeInitialPlacementAttachedToBoneIdx != NONE)
		{
			if (auto* at = presence->get_attached_to())
			{
				if (auto * a = at->get_appearance())
				{
					if (auto * pa = presence->get_path_to_attached_to())
					{
						if (!pa->is_active())
						{
							return;
						}
						basePlacement = pa->from_target_to_owner(at->get_presence()->get_placement().to_world(a->get_ms_to_os_transform().to_world(a->get_final_pose_MS()->get_bone(relativeInitialPlacementAttachedToBoneIdx)).to_world(relativeInitialPlacement)));
					}
				}
			}
		}
		else if (relativeInitialPlacementAttachedToSocketIdx != NONE)
		{
			if (auto* at = presence->get_attached_to())
			{
				if (auto * a = at->get_appearance())
				{
					if (auto * pa = presence->get_path_to_attached_to())
					{
						if (!pa->is_active())
						{
							return;
						}
						basePlacement = pa->from_target_to_owner(at->get_presence()->get_placement().to_world(a->calculate_socket_os(relativeInitialPlacementAttachedToSocketIdx).to_world(relativeInitialPlacement)));
					}
				}
			}
		}
		else
		{
			// if should cease when not attached, let it be as it is
			an_assert(presence->should_cease_to_exist_when_not_attached(), TXT("no attachment placement!"));
			return;
		}
		break;
	case RelativeToBaseObject:
	case RelativeToBaseObjectUsingSocket:
	case RelativeToBaseObjectUsingParentSocket:
		if (presence->is_attached())
		{
			if (auto* at = presence->get_attached_to())
			{
				if (auto * pa = presence->get_path_to_attached_to())
				{
					if (!pa->is_active())
					{
						return;
					}
					basePlacement = pa->from_target_to_owner(at->get_presence()->get_placement()).to_world(relativeInitialPlacement);
				}
			}
		}
		else if (presence->get_based_on())
		{
			basePlacement = presence->get_based_on()->get_presence()->get_placement().to_world(relativeInitialPlacement);
		}
		else
		{
			// if should cease when not attached, let it be not relative
			an_assert(presence->should_cease_to_exist_when_not_attached(), TXT("not relative to anything and should not cease when not attached!"));
			return;
		}
		break;
	case UsingOwnSocketRelativeToInitialPlacementWS:
	case RelativeToInitialPlacementWS:
		basePlacement = initialPlacementWS;
		break;
	}

	basePlacementValid = true;
}

//

REGISTER_FOR_FAST_CAST(ModuleMovementInteractivePartData);

ModuleMovementInteractivePartData::ModuleMovementInteractivePartData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleMovementInteractivePartData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	return base::read_parameter_from(_attr, _lc);
}

bool ModuleMovementInteractivePartData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	return base::read_parameter_from(_node, _lc);
}
