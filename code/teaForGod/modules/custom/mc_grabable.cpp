#include "mc_grabable.h"

#include "..\gameplay\modulePilgrimHand.h"

#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\modules.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(grabableAxisSocket);
DEFINE_STATIC_NAME(grabableDialSocket);

//

REGISTER_FOR_FAST_CAST(Grabable);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new Grabable(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & Grabable::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("grabable")), create_module);
}

Grabable::Grabable(Framework::IModulesOwner* _owner)
: base(_owner)
{
	SET_EXTRA_DEBUG_INFO(grabbedBy, TXT("Grabable.grabbedBy"));
}

Grabable::~Grabable()
{
}

void Grabable::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	grabableAxisSocket = _moduleData->get_parameter<Name>(this, NAME(grabableAxisSocket), grabableAxisSocket);
	grabableDialSocket = _moduleData->get_parameter<Name>(this, NAME(grabableDialSocket), grabableDialSocket);

	if (!grabableAxisSocket.is_valid() && !grabableDialSocket.is_valid())
	{
		error(TXT("missing grabable axis and grabable dial"));
	}
}

bool Grabable::set_can_be_grabbed(bool _canBeGrabbed)
{
	Concurrency::ScopedSpinLock lock(canBeGrabbedLock);
	bool result = canBeGrabbed;
	canBeGrabbed = _canBeGrabbed;
	startingPlacementRS.clear(); // always clear
	startingControlRS.clear();
	if (auto * controller = get_owner()->get_controller())
	{
		controller->clear_requested_placement();
	}
	return result;
}

void Grabable::set_grabbed(Framework::IModulesOwner* _by, bool _grabbed)
{
	Concurrency::ScopedSpinLock lock(grabbedByLock);
	if (_grabbed)
	{
		grabbedBy.push_back(SafePtr<::Framework::IModulesOwner>(_by));
	}
	else
	{
		grabbedBy.remove(SafePtr<::Framework::IModulesOwner>(_by));
		// to end
		physicalSensationPullPush.update(grabbedBy);
	}
}

void Grabable::process_control(Transform const& _controlWS)
{
	auto* presence = get_owner()->get_presence();
	an_assert(presence);

	// reference frame in our world space (_controlWS is in the same)
	Transform referenceFrameWS = Transform::identity;

	if (referenceFrame == ReferenceFrame::Attachment)
	{
		if (auto* attachedTo = presence->get_attached_to())
		{
			// get placement of an object we're attached to, and get it in our world space
			referenceFrameWS = presence->get_path_to_attached_to()->from_target_to_owner(attachedTo->get_presence()->get_placement());

			// check if there is a socket we're attached to
			if (presence->get_attached_to_socket().is_valid())
			{
				referenceFrameWS = referenceFrameWS.to_world(attachedTo->get_appearance()->calculate_socket_os(presence->get_attached_to_socket().get_index()));
			}
			else
			{
				int attachedToBone;
				Transform attachedRelativePlacement;
				// check if there is a bone we're attached to
				if (presence->get_attached_to_bone_info(OUT_ attachedToBone, OUT_ attachedRelativePlacement))
				{
					Transform attachedToPlacementMS = attachedTo->get_appearance()->get_final_pose_MS()->get_bone(attachedToBone).to_world(attachedRelativePlacement);
					referenceFrameWS = referenceFrameWS.to_world(attachedTo->get_appearance()->get_ms_to_os_transform().to_world(attachedToPlacementMS));
				}
			}
		}
	}

	if (!startingPlacementRS.is_set() || !startingControlRS.is_set())
	{
		startingPlacementRS = referenceFrameWS.to_local(presence->get_placement());
		startingControlRS = referenceFrameWS.to_local(_controlWS);
	}

	if (auto* controller = get_owner()->get_controller())
	{
		// we switch calculations to reference frame to have relative placements done there
		Transform controlRS = referenceFrameWS.to_local(_controlWS);
		Transform startingPlacementByNewControlRS = controlRS.to_world(startingControlRS.get().to_local(startingPlacementRS.get()));
#ifdef AN_DEVELOPMENT
		debug_filter(grabable);
		debug_context(get_owner()->get_presence()->get_in_room());
		debug_push_transform(referenceFrameWS);
		debug_draw_transform_coloured(true, startingControlRS.get(), Colour::grey, Colour::grey, Colour::grey);
		debug_draw_transform_coloured(true, controlRS, Colour::green, Colour::green, Colour::green);
		debug_draw_transform_coloured(true, startingPlacementRS.get(), Colour::blue, Colour::blue, Colour::blue);
		debug_draw_transform_coloured(true, startingPlacementByNewControlRS, Colour::red, Colour::red, Colour::red);
		debug_pop_transform();
		debug_no_context();
		debug_no_filter();
#endif

		controller->set_requested_placement(referenceFrameWS.to_world(startingPlacementByNewControlRS));
	}

	physicalSensationPullPush.update(grabbedBy);
}

//

void Grabable::PhysicalSensationPullPush::update(ArrayStatic<SafePtr<::Framework::IModulesOwner>, 8> const& grabbedBy)
{
	return;

	::Framework::IModulesOwner* pullPushFor = nullptr;

	for_every_ref(gby, grabbedBy)
	{
		if (Framework::GameUtils::is_controlled_by_local_player(gby))
		{
			if (gby->get_presence()->get_velocity_linear().length() > 0.1f)
			{
				pullPushFor = gby;
				break;
			}
		}
	}

	if (pullerPusher &&
		pullerPusher != pullPushFor)
	{
		// to change the pullerPusher
		pullPushFor = nullptr;
	}
	bool shouldPullPush = pullPushFor != nullptr;
	if (shouldPullPush)
	{
		timeSinceMoving = 0.0f;
	}
	else
	{
		timeSinceMoving += System::Core::get_delta_time();
	}
	shouldPullPush |= (pullPushFor && timeSinceMoving < 0.1f); // to fix when stops for one frame?

	{
		if (shouldPullPush && !physSens.is_set())
		{
			if (auto* mph = pullPushFor->get_gameplay_as<ModulePilgrimHand>())
			{
				PhysicalSensations::OngoingSensation s(PhysicalSensations::OngoingSensation::PushObject);
				s.for_hand(mph->get_hand());
				physSens = PhysicalSensations::start_sensation(s);
				pullerPusher = pullPushFor;
			}
		}
		else if (physSens.is_set() && !shouldPullPush)
		{
			PhysicalSensations::stop_sensation(physSens.get());
			physSens.clear();
		}
	}
}