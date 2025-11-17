#include "iVR.h"

#include "vrOffsets.h"

#include "oculus\oculus.h"
#include "oculusMobile\oculusMobile.h"
#include "openvr\openvr.h"
#include "openxr\openxr.h"
#include "wavevr\wavevr.h"

#include "..\mainConfig.h"

#include "..\debug\debugRenderer.h"
#include "..\debug\debugVisualiser.h"

#include "..\system\core.h"
#include "..\system\input.h"
#include "..\system\gamepad.h"
#include "..\system\video\shader.h"
#include "..\system\video\shaderProgram.h"
#include "..\system\video\video3d.h"
#include "..\system\video\renderTarget.h"
#include "..\system\video\fragmentShaderOutputSetup.h"

#include "..\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define AN_DEBUG_HEAD_SIDE_TOUCH
#endif
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define VERBOSE_UPDATE_VR
#endif

#ifdef AN_INSPECT_VR_PLAY_AREA
	#ifndef VERBOSE_UPDATE_VR
		#define VERBOSE_UPDATE_VR
	#endif
#endif

//#define VISUALISE_PLAY_AREA_FROM_RAW_BOUNDARY_POINTS

//

using namespace VR;

//

// input usage tags
DEFINE_STATIC_NAME(vr);
DEFINE_STATIC_NAME(handTracking); // handTracking for specific hand or any handTracking in general
DEFINE_STATIC_NAME(immobileVR); // if using immobileVR / sliding/smooth locomotion
DEFINE_STATIC_NAME(usingControllers); // using controllers

//

Transform RenderInfo::get_eye_offset_to_use(int _eyeIdx) const
{
	Transform result = eyeOffset[_eyeIdx];
	if (auto* vr = IVR::get())
	{
		result.set_translation(vr->get_scaling().general * result.get_translation());
	}
	return result;
}

//

VRFinger::Type VRFinger::parse(String const& _fingerName)
{
	if (_fingerName == TXT("thumb")) return Thumb;
	if (_fingerName == TXT("pointer")) return Pointer;
	if (_fingerName == TXT("middle")) return Middle;
	if (_fingerName == TXT("ring")) return Ring;
	if (_fingerName == TXT("pinky")) return Pinky;

	return VRFinger::None;
}

VRHandBoneIndex::Type VRFinger::get_tip_bone(VRFinger::Type _finger)
{
	if (_finger == VRFinger::Thumb) return VRHandBoneIndex::ThumbTip;
	if (_finger == VRFinger::Pointer) return VRHandBoneIndex::PointerTip;
	if (_finger == VRFinger::Middle) return VRHandBoneIndex::MiddleTip;
	if (_finger == VRFinger::Ring) return VRHandBoneIndex::RingTip;
	if (_finger == VRFinger::Pinky) return VRHandBoneIndex::PinkyTip;
	return VRHandBoneIndex::None;
}

VRHandBoneIndex::Type VRFinger::get_base_bone(VRFinger::Type _finger)
{
	if (_finger == VRFinger::Thumb) return VRHandBoneIndex::ThumbBase;
	if (_finger == VRFinger::Pointer) return VRHandBoneIndex::PointerBase;
	if (_finger == VRFinger::Middle) return VRHandBoneIndex::MiddleBase;
	if (_finger == VRFinger::Ring) return VRHandBoneIndex::RingBase;
	if (_finger == VRFinger::Pinky) return VRHandBoneIndex::PinkyBase;
	return VRHandBoneIndex::None;
}

//

VRHandPose::VRHandPose()
{
	SET_EXTRA_DEBUG_INFO(bonesPS, TXT("VRHandPose.bonesPS"));
	SET_EXTRA_DEBUG_INFO(fingers, TXT("VRHandPose.fingers"));
	clear();
}

void VRHandPose::clear()
{
	for_count(int, b, MAX_RAW_BONES)
	{
		rawBoneParents[b] = NONE;
		rawBonesLS[b].clear();
		rawBonesRS[b].clear();
	}
	rawPlacement.clear();
	rawHandTrackingRootPlacement.clear();

	placement.clear();
	bonesPS.set_size(VRHandBoneIndex::MAX);
	for_every(b, bonesPS)
	{
		b->clear();
	}
	fingers.set_size(VRFinger::MAX);
}

Transform VRHandPose::get_raw_bone_rs(Hand::Type _hand, int _index) const
{
	if (_index >= 0 && _index < MAX_RAW_BONES)
	{
		auto* vr = IVR::get();
		an_assert(vr);
		if (rawBonesRS[_index].is_set())
		{
			return rawBonesRS[_index].get();
		}
		else
		{
			auto const& refHandPose = vr->get_reference_pose_set().get_hand(_hand);
			Transform boneHS = Transform::identity;
			int index = _index;
			while (index != NONE)
			{
				if (rawBonesLS[index].is_set())
				{
					boneHS = rawBonesLS[index].get().to_world(boneHS);
				}
				index = refHandPose.rawBoneParents[index];
			}
			return boneHS;
		}
	}
	else
	{
		return Transform::identity;
	}
}

Transform VRHandPose::get_raw_bone_rs(Hand::Type _hand, VRHandBoneIndex::Type _index) const
{
	auto* vr = IVR::get();
	an_assert(vr);
	return get_raw_bone_rs(_hand, vr->translate_bone_index(_index));
}

Transform const & VRHandPose::get_bone_ps(VRHandBoneIndex::Type _index) const
{
	return _index >= 0 && _index < VRHandBoneIndex::MAX? bonesPS[_index].get(Transform::identity) : Transform::identity;
}

void VRHandPose::calculate_auto_and_add_offsets(Hand::Type _hand, bool _setup)
{
	auto* vr = IVR::get();
	an_assert(vr);
	// keep bonesPS

	if ((vr->is_using_hand_tracking(_hand) || _setup) && rawHandTrackingRootPlacement.is_set())
	{
		placement = rawHandTrackingRootPlacement.get().to_world(VR::Offsets::get_hand_tracking(_hand));
		Transform rootPlacement = rawHandTrackingRootPlacement.get();
		Transform addFingerOffset = VR::Offsets::get_finger_tracking(_hand);
		for_count(int, b, VRHandBoneIndex::MAX)
		{
			Transform boneVRS = rootPlacement.to_world(get_raw_bone_rs(_hand, (VRHandBoneIndex::Type)b));
			bonesPS[b] = (placement.get().to_local(boneVRS)).to_world(addFingerOffset);
		}
	}
	else if (!vr->is_using_hand_tracking(_hand) && rawPlacement.is_set())
	{
		placement = rawPlacement.get().to_world(VR::Offsets::get_hand(_hand));
	}
	else
	{
		placement.clear();
	}

	// fingers straight
	if (vr->is_using_hand_tracking(_hand) || _setup)
	{
		for_every(f, fingers)
		{
			VRFinger::Type fingerType = (VRFinger::Type)for_everys_index(f);
			VRHandBoneIndex::Type tipBone = VRFinger::get_tip_bone(fingerType);
			VRHandBoneIndex::Type baseBone = VRFinger::get_base_bone(fingerType);
			if (baseBone != VRHandBoneIndex::None && tipBone != VRHandBoneIndex::None &&
				bonesPS[baseBone].is_set() && bonesPS[tipBone].is_set())
			{
				f->baseToTip = bonesPS[tipBone].get().get_translation() - bonesPS[baseBone].get().get_translation();
				f->baseToTipNormal = f->baseToTip.normal();
				f->length = max(0.01f, f->baseToTip.length());

				if (!_setup)
				{
					auto const & refFinger = vr->get_reference_pose_set().get_hand(_hand).fingers[fingerType];

					f->straightLength = clamp(f->length / refFinger.length, 0.0f, 1.0f);
					f->straightRefAligned = Vector3::dot(f->baseToTip, refFinger.baseToTipNormal) / refFinger.length;
				}
			}
		}
	}
}

static float to_three_finger_states(float refAligned, float _folded, float _straight)
{
	if (refAligned < _folded)
	{
		return remap_value(refAligned, -1.0f, _folded, 1.0f, VRControls::HAND_TRACKING_FOLDED_TO_BUTTON_THRESHOLD);
	}
	if (refAligned > _straight)
	{
		return remap_value(refAligned, _straight, 1.0f, VRControls::HAND_TRACKING_STRAIGHT_TO_BUTTON_THRESHOLD, 0.0f);
	}
	return remap_value(refAligned, _folded, _straight, VRControls::HAND_TRACKING_FOLDED_TO_BUTTON_THRESHOLD, VRControls::HAND_TRACKING_STRAIGHT_TO_BUTTON_THRESHOLD);
}

Transform VRHandPose::get_hand_centre_offset(Hand::Type _hand)
{
	return Transform(Vector3((_hand == Hand::Right ? 1.0f : -1.0f) * (-0.02f), 0.06f, 0.0f), Quat::identity);
}

void VRHandPose::store_controls(Hand::Type _hand, VRPoseSet const& _pose, VRControls::Hand& controls)
{
	auto* vr = IVR::get();
	an_assert(vr);

	if (vr->is_using_hand_tracking(_hand))
	{
		// for controls, general state, override anything that comes not from hand tracking
		controls.thumb = to_three_finger_states(fingers[VRFinger::Thumb].straightRefAligned, 0.3f, 0.9f);
		controls.pointer = to_three_finger_states(fingers[VRFinger::Pointer].straightRefAligned, 0.0f, 0.8f);
		controls.middle = to_three_finger_states(fingers[VRFinger::Middle].straightRefAligned, 0.0f, 0.8f);
		controls.ring = to_three_finger_states(fingers[VRFinger::Ring].straightRefAligned, 0.0f, 0.8f);
		controls.pinky = to_three_finger_states(fingers[VRFinger::Pinky].straightRefAligned, 0.0f, 0.8f);
	}

	if (vr->is_using_hand_tracking() && placement.is_set())
	{
		float pureUpsideDown = -placement.get().get_axis(Axis::Up).z;
		controls.upsideDown = clamp(remap_value(pureUpsideDown, 0.5f, 1.0f, 0.0f, 1.1f), 0.0f, 1.0f);

		if (_pose.view.is_set())
		{
			float insideToHead = Vector3::dot(-placement.get().get_axis(Axis::Up), (_pose.view.get().get_translation() - placement.get().get_translation()).normal());
			controls.insideToHead = clamp(remap_value(insideToHead, 0.5f, 1.0f, 0.0f, 1.1f), 0.0f, 1.0f);
		}
		else
		{
			controls.insideToHead = 0.0f;
		}
	}
	else
	{
		controls.upsideDown = 0.0f;
		controls.insideToHead = 0.0f;
	}

	if (_pose.view.is_set() &&
		placement.is_set())
	{
		Transform headVRS = _pose.view.get().to_world(Transform(Vector3(0.0f, 0.0f, -0.015f), Quat::identity));

		Transform handHS = headVRS.to_local(placement.get().to_world(get_hand_centre_offset(_hand)));

		float generalScaling = IVR::get()->get_scaling().general;

		float radius = 0.07f * generalScaling;
		float minDistX = 0.10f * generalScaling;
		float maxDistX = 0.23f * generalScaling; // this works for oculus and vive wand
		Vector3 scaleLoc(1.0f, 0.5f, 1.0f);

		Vector3 modLoc = handHS.get_translation() * scaleLoc;
		if (sqr(modLoc.z) + sqr(modLoc.y) < sqr(radius) &&
			abs(modLoc.x) < maxDistX &&
			abs(modLoc.x) > minDistX)
		{
			controls.headSideTouch = 1.0f;
		}
		else
		{
			controls.headSideTouch = 0.0f;
		}

#ifdef AN_DEBUG_HEAD_SIDE_TOUCH
		debug_context(IVR::get());
		debug_draw_arrow(true, controls.headSideTouch.get() > 0.5f ? Colour::red : Colour::yellow, headVRS.to_world(handHS).get_translation(), headVRS.get_translation());
		debug_draw_arrow(true, Colour::green, headVRS.get_translation(), headVRS.get_translation() + headVRS.get_axis(Axis::Right) * maxDistX);
		debug_draw_arrow(true, Colour::green, headVRS.get_translation(), headVRS.get_translation() - headVRS.get_axis(Axis::Right) * maxDistX);
		float stepA = 20.0f;
		for (float a = 0.0f; a <= 360.0f; a += stepA)
		{
			float pa = a - stepA;
			Vector3 lp(0.0f, (sin_deg(pa) / scaleLoc.y) * radius, (cos_deg(pa) / scaleLoc.z) * radius);
			Vector3 lc(0.0f, (sin_deg(a) / scaleLoc.y) * radius, (cos_deg(a) / scaleLoc.z) * radius);
			for (float side = -1.0f; side <= 1.0f; side += 2.0f)
			{
				lc.x = lp.x = minDistX * side;
				debug_draw_line(true, Colour::green, headVRS.location_to_world(lp), headVRS.location_to_world(lc));
				Vector3 oc = lc;
				Vector3 op = lp;
				oc.x = op.x = maxDistX * side;
				debug_draw_line(true, Colour::green, headVRS.location_to_world(op), headVRS.location_to_world(oc));
				debug_draw_line(true, Colour::green, headVRS.location_to_world(oc), headVRS.location_to_world(lc));
			}
		}
		debug_pop_transform();
		debug_no_context();
#endif
	}
	else
	{
		controls.headSideTouch = 0.0f;
	}
}

//--

VRHandPose const & VRPoseSet::get_hand(Hand::Type _hand) const
{
	auto* vr = IVR::get();
	an_assert(vr);
	return hands[vr->get_hand(_hand)];
}

//--

void VRPoseSet::post_store_controls(VRControls& controls)
{
	if (MainConfig::global().is_joystick_input_swapped())
	{
		auto& handL = controls.hands[Hand::Left];
		auto& handR = controls.hands[Hand::Right];
		swap(handL.joystick, handR.joystick);
		swap(handL.buttons.joystickPress, handR.buttons.joystickPress);
	}

	for_count(int, i, Hand::MAX)
	{
		controls.hands[i].post_store_controls();
	}

	for_count(int, i, Hand::MAX)
	{
		controls.hands[i].do_auto_buttons();
	}
}

void VRPoseSet::calculate_raw_from_as_read()
{
	auto* vr = IVR::get();
	an_assert(vr);

	if (rawViewAsRead.is_set())
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		rawView = vr->get_map_vr_space().to_local(vr->get_dev_in_vr_space().to_world(vr->get_immobile_origin_in_vr_space().to_local(vr->get_additional_offset_to_raw_as_read().to_world(rawViewAsRead.get()))));
#else
		rawView = vr->get_map_vr_space().to_local(vr->get_immobile_origin_in_vr_space().to_local(vr->get_additional_offset_to_raw_as_read().to_world(rawViewAsRead.get())));
#ifdef AN_INSPECT_VR_INVALID_ORIGIN_CONTINUOUS
		output(TXT("[vr_invalid_origin] view read at %S to %S (map %S %.1f')")
			, _trackingState.viewPose.get().get_translation().to_string().to_char()
			, rawView.get().get_translation().to_string().to_char()
			, vr->get_map_vr_space().get_translation().to_string().to_char()
			, vr->get_map_vr_space().get_orientation().to_rotator().yaw
		);
#endif
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		{
			Vector3 loc = rawView.get().get_translation();
			loc.z += vr->get_vertical_offset();
			rawView.access().set_translation(loc);
		}
#endif
	}
	else
	{
		rawView.clear();
	}

	for_count(int, i, Hands::Count)
	{
		auto& hand = hands[i];

		if (hand.rawPlacementAsRead.is_set())
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			hand.rawPlacement = vr->get_map_vr_space().to_local(vr->get_dev_in_vr_space().to_world(vr->get_immobile_origin_in_vr_space().to_local(vr->get_additional_offset_to_raw_as_read().to_world(hand.rawPlacementAsRead.get()))));
#else
			hand.rawPlacement = vr->get_map_vr_space().to_local(vr->get_immobile_origin_in_vr_space().to_local(vr->get_additional_offset_to_raw_as_read().to_world(hand.rawPlacementAsRead.get())));
#endif
		}
		else
		{
			hand.rawPlacement.clear();
		}

		if (hand.rawHandTrackingRootPlacementAsRead.is_set())
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			hand.rawHandTrackingRootPlacement = vr->get_map_vr_space().to_local(vr->get_dev_in_vr_space().to_world(vr->get_immobile_origin_in_vr_space().to_local(vr->get_additional_offset_to_raw_as_read().to_world(hand.rawHandTrackingRootPlacementAsRead.get()))));
#else
			hand.rawHandTrackingRootPlacement = vr->get_map_vr_space().to_local(vr->get_immobile_origin_in_vr_space().to_local(vr->get_additional_offset_to_raw_as_read().to_world(hand.rawHandTrackingRootPlacementAsRead.get())));
#endif
		}
		else
		{
			hand.rawHandTrackingRootPlacement.clear();
		}

#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (hand.rawPlacement.is_set())
		{
			Vector3 loc = hand.rawPlacement.get().get_translation();
			loc.z += vr->get_vertical_offset();
			hand.rawPlacement.access().set_translation(loc);
		}
		if (hand.rawHandTrackingRootPlacement.is_set())
		{
			Vector3 loc = hand.rawHandTrackingRootPlacement.get().get_translation();
			loc.z += vr->get_vertical_offset();
			hand.rawHandTrackingRootPlacement.access().set_translation(loc);
		}
#endif
	}
}

void VRPoseSet::calculate_auto()
{
	// first get right raw values
	calculate_raw_from_as_read();

	auto* vr = IVR::get();
	an_assert(vr);

	// store prev view, will be useful to update placements
	Optional<Transform> prevView = view;

	// store available placements
	// and clear placements to ignore scaling
	Optional<Transform> handPlacements[Hands::Count];
	for_count(int, h, Hands::Count)
	{
		handPlacements[h] = hands[h].placement;
		hands[h].placement.clear();
	}
	
	// calculate auto for hands to update placement
	if (vr->is_left_hand_available())
	{
		hands[vr->get_left_hand()].calculate_auto_and_add_offsets(Hand::Left);
	}
	if (vr->is_right_hand_available())
	{
		hands[vr->get_right_hand()].calculate_auto_and_add_offsets(Hand::Right);
	}

	// apply scaling and store as actual placements
	if (rawView.is_set())
	{
		view = rawView;
	}
	float horizontalScaling = vr->get_scaling().horizontal;
	float scaling = vr->get_scaling().general;
	if ((horizontalScaling != 1.0f || scaling != 1.0f) && view.is_set() && rawView.is_set() /* update only if we provided a new value */)
	{
		Vector3 scaleHorizontally(horizontalScaling, horizontalScaling, 1.0f);
		Vector3 viewLoc = view.get().get_translation();
		Vector3 scaledHorizontallyViewLoc = viewLoc * scaleHorizontally;
		view.access().set_translation(scaledHorizontallyViewLoc * scaling);
		for_count(int, h, Hands::Count)
		{
			if (hands[h].placement.is_set())
			{
				Vector3 handLoc = hands[h].placement.get().get_translation();
				Vector3 scaledHandLoc = (scaledHorizontallyViewLoc + (handLoc - viewLoc)) * scaling;
				hands[h].placement.access().set_translation(scaledHandLoc);
			}
		}
	}

	// move hands that were not read with view (flat)
	if (prevView.is_set() && view.is_set())
	{
		bool isRequired = false;
		for_count(int, h, Hands::Count)
		{
			if (!hands[h].placement.is_set() &&
				handPlacements[h].is_set())
			{
				isRequired = true;
				break;
			}
		}
		if (isRequired)
		{
			Transform flatPrevView = look_matrix(prevView.get().get_translation(), prevView.get().get_axis(Axis::Forward).normal_2d(), prevView.get().get_axis(Axis::Up)).to_transform();
			Transform flatView = look_matrix(view.get().get_translation(), view.get().get_axis(Axis::Forward).normal_2d(), view.get().get_axis(Axis::Up)).to_transform();
			for_count(int, h, Hands::Count)
			{
				if (!hands[h].placement.is_set() &&
					handPlacements[h].is_set())
				{
					hands[h].placement = flatView.to_world(flatPrevView.to_local(handPlacements[h].get()));
				}
			}
		}
	}
}

//

VRControls::VRControls()
{
	hands[0].handIndex = 0;
	hands[1].handIndex = 1;
}

Vector2 VRControls::get_dpad() const
{
	Vector2 dpad = Vector2::zero;
	for_count(int, i, Hands::Count)
	{
		dpad += hands[i].get_dpad();
	}
	dpad.x = clamp(dpad.x, -1.0f, 1.0f);
	dpad.y = clamp(dpad.y, -1.0f, 1.0f);
	return dpad;
}

bool VRControls::has_button_been_pressed(VR::Input::Button::WithSource _button) const
{
	bool result = false;

	for_count(int, i, Hands::Count)
	{
		Optional<bool> res = hands[i].has_button_been_pressed(_button);
		if (res.is_set())
		{
			result |= res.get() ? true : false; // ?!
		}
	}

	return result;
}

bool VRControls::has_button_been_released(VR::Input::Button::WithSource _button) const
{
	bool result = false;

	for_count(int, i, Hands::Count)
	{
		Optional<bool> res = hands[i].has_button_been_released(_button);
		if (res.is_set())
		{
			result |= res.get() ? true : false; // ?!
		}
	}

	return result;
}

bool VRControls::is_button_pressed(VR::Input::Button::WithSource _button) const
{
	bool result = false;

	for_count(int, i, Hands::Count)
	{
		Optional<bool> res = hands[i].is_button_pressed(_button);
		if (res.is_set())
		{
			result |= res.get() ? true : false; // ?!
		}
	}

	return result;
}

float VRControls::get_button_state(VR::Input::Button::WithSource _button) const
{
	float result = 0.0f;

	for_count(int, i, Hands::Count)
	{
		Optional<float> res = hands[i].get_button_state(_button);
		if (res.is_set())
		{
			result = max(result, res.get());
		}
	}

	return result;
}

bool VRControls::has_button_been_pressed(Array<VR::Input::Button::WithSource> const & _buttons) const
{
	bool result = false;

	for_count(int, i, Hands::Count)
	{
		result |= hands[i].has_button_been_pressed(_buttons);
	}

	return result;
}

bool VRControls::has_button_been_released(Array<VR::Input::Button::WithSource> const & _buttons) const
{
	bool result = false;

	for_count(int, i, Hands::Count)
	{
		result |= hands[i].has_button_been_released(_buttons);
	}

	return result;
}

bool VRControls::is_button_pressed(Array<VR::Input::Button::WithSource> const & _buttons) const
{
	bool result = false;

	for_count(int, i, Hands::Count)
	{
		result |= hands[i].is_button_pressed(_buttons);
	}

	return result;
}

float VRControls::get_button_state(Array<VR::Input::Button::WithSource> const & _buttons) const
{
	float result = 0.0f;

	for_count(int, i, Hands::Count)
	{
		result = max(result, hands[i].get_button_state(_buttons));
	}

	return result;
}

void VRControls::pre_advance()
{
	mouseMovement = Vector2::zero;
	for_count(int, i, Hands::Count)
	{
		hands[i].prevButtons = hands[i].buttons;
	}
	requestRecenter = false;
}

//

void VRControls::Hand::post_store_controls()
{
	if (MainConfig::global().is_joystick_input_blocked())
	{
		joystick = Vector2::zero;
		buttons.joystickLeft = false;
		buttons.joystickRight = false;
		buttons.joystickDown = false;
		buttons.joystickUp = false;
		buttons.joystickPress = false;
	}
}

void VRControls::Hand::do_auto_buttons()
{
	auto* vr = IVR::get();
	an_assert(vr);
	buttons.usesHandTracking = vr->is_using_hand_tracking();
	buttons.pointerPinch = pointerPinch.get(0.0f) > VRControls::PINCH_TO_BUTTON_THRESHOLD;
	buttons.middlePinch = middlePinch.get(0.0f) > VRControls::PINCH_TO_BUTTON_THRESHOLD;
	buttons.ringPinch = ringPinch.get(0.0f) > VRControls::PINCH_TO_BUTTON_THRESHOLD;
	buttons.pinkyPinch = pinkyPinch.get(0.0f) > VRControls::PINCH_TO_BUTTON_THRESHOLD;
	buttons.trigger = trigger.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.grip = grip.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.pose = pose.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.upsideDown = upsideDown.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.insideToHead = insideToHead.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.headSideTouch = headSideTouch.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.joystickLeft = joystick.x < -JOYSTICK_DIR_THRESHOLD;
	buttons.joystickRight = joystick.x > JOYSTICK_DIR_THRESHOLD;
	buttons.joystickDown = joystick.y < -JOYSTICK_DIR_THRESHOLD;
	buttons.joystickUp = joystick.y > JOYSTICK_DIR_THRESHOLD;
	buttons.thumb = thumb.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.pointer = pointer.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.middle = middle.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.ring = ring.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.pinky = pinky.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
	buttons.thumbFolded = thumb.get(0.0f) > VRControls::HAND_TRACKING_FOLDED_TO_BUTTON_THRESHOLD;
	buttons.pointerFolded = pointer.get(0.0f) > VRControls::HAND_TRACKING_FOLDED_TO_BUTTON_THRESHOLD;
	buttons.middleFolded = middle.get(0.0f) > VRControls::HAND_TRACKING_FOLDED_TO_BUTTON_THRESHOLD;
	buttons.ringFolded = ring.get(0.0f) > VRControls::HAND_TRACKING_FOLDED_TO_BUTTON_THRESHOLD;
	buttons.pinkyFolded = pinky.get(0.0f) > VRControls::HAND_TRACKING_FOLDED_TO_BUTTON_THRESHOLD;
	buttons.thumbStraight = thumb.get(1.0f) < VRControls::HAND_TRACKING_STRAIGHT_TO_BUTTON_THRESHOLD;
	buttons.pointerStraight = pointer.get(1.0f) < VRControls::HAND_TRACKING_STRAIGHT_TO_BUTTON_THRESHOLD;
	buttons.middleStraight = middle.get(1.0f) < VRControls::HAND_TRACKING_STRAIGHT_TO_BUTTON_THRESHOLD;
	buttons.ringStraight = ring.get(1.0f) < VRControls::HAND_TRACKING_STRAIGHT_TO_BUTTON_THRESHOLD;
	buttons.pinkyStraight = pinky.get(1.0f) < VRControls::HAND_TRACKING_STRAIGHT_TO_BUTTON_THRESHOLD;
}

void VRControls::Hand::reset()
{
	auto sourceCopy = source;
	auto handIndexCopy = handIndex;
	*this = Hand();
	source = sourceCopy;
	handIndex = handIndexCopy;
}

Vector2 VRControls::Hand::get_dpad() const
{
	Vector2 dpad = Vector2::zero;
	if (buttons.dpadLeft) dpad.x -= 1.0f;
	if (buttons.dpadRight) dpad.x += 1.0f;
	if (buttons.dpadDown) dpad.y -= 1.0f;
	if (buttons.dpadUp) dpad.y += 1.0f;
	return dpad;
}

#ifndef AN_CLANG
#define HANDLE(_id, _var) if (_button.type == _id) return _button.released? (!buttons.##_var && prevButtons.##_var) : (buttons.##_var && !prevButtons.##_var);
#else
#define HANDLE(_id, _var) if (_button.type == _id) return _button.released? (!buttons._var && prevButtons._var) : (buttons._var && !prevButtons._var);
#endif
Optional<bool> VRControls::Hand::has_button_been_pressed(VR::Input::Button::WithSource _button) const
{
	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;
	if ((_button.source & source) &&
		_button.hand.get(hand) == hand)
	{
		HANDLE(VR::Input::Button::InUse, inUse);
		HANDLE(VR::Input::Button::UsesHandTracking, usesHandTracking);
		HANDLE(VR::Input::Button::Primary, primary);
		HANDLE(VR::Input::Button::Secondary, secondary);
		HANDLE(VR::Input::Button::SystemMenu, systemMenu);
		HANDLE(VR::Input::Button::SystemGestureProcessing, systemGestureProcessing);
		HANDLE(VR::Input::Button::Grip, grip);
		HANDLE(VR::Input::Button::PointerPinch, pointerPinch);
		HANDLE(VR::Input::Button::MiddlePinch, middlePinch);
		HANDLE(VR::Input::Button::RingPinch, ringPinch);
		HANDLE(VR::Input::Button::PinkyPinch, pinkyPinch);
		HANDLE(VR::Input::Button::Trigger, trigger);
		HANDLE(VR::Input::Button::Pose, pose);
		HANDLE(VR::Input::Button::UpsideDown, upsideDown);
		HANDLE(VR::Input::Button::InsideToHead, insideToHead);
		HANDLE(VR::Input::Button::HeadSideTouch, headSideTouch);
		HANDLE(VR::Input::Button::Thumb, thumb);
		HANDLE(VR::Input::Button::Pointer, pointer);
		HANDLE(VR::Input::Button::Middle, middle);
		HANDLE(VR::Input::Button::Ring, ring);
		HANDLE(VR::Input::Button::Pinky, pinky);
		HANDLE(VR::Input::Button::ThumbFolded, thumbFolded);
		HANDLE(VR::Input::Button::PointerFolded, pointerFolded);
		HANDLE(VR::Input::Button::MiddleFolded, middleFolded);
		HANDLE(VR::Input::Button::RingFolded, ringFolded);
		HANDLE(VR::Input::Button::PinkyFolded, pinkyFolded);
		HANDLE(VR::Input::Button::ThumbStraight, thumbStraight);
		HANDLE(VR::Input::Button::PointerStraight, pointerStraight);
		HANDLE(VR::Input::Button::MiddleStraight, middleStraight);
		HANDLE(VR::Input::Button::RingStraight, ringStraight);
		HANDLE(VR::Input::Button::PinkyStraight, pinkyStraight);
		HANDLE(VR::Input::Button::Trackpad, trackpad);
		HANDLE(VR::Input::Button::TrackpadTouch, trackpadTouch);
		HANDLE(VR::Input::Button::TrackpadDirCentre, trackpadDirCentre);
		HANDLE(VR::Input::Button::TrackpadDirLeft, trackpadDirLeft);
		HANDLE(VR::Input::Button::TrackpadDirRight, trackpadDirRight);
		HANDLE(VR::Input::Button::TrackpadDirDown, trackpadDirDown);
		HANDLE(VR::Input::Button::TrackpadDirUp, trackpadDirUp);
		HANDLE(VR::Input::Button::JoystickLeft, joystickLeft);
		HANDLE(VR::Input::Button::JoystickRight, joystickRight);
		HANDLE(VR::Input::Button::JoystickDown, joystickDown);
		HANDLE(VR::Input::Button::JoystickUp, joystickUp);
		HANDLE(VR::Input::Button::JoystickPress, joystickPress);
		an_assert(false, TXT("implement_ button"));
		return NP;
	}
	else
	{
		return NP;
	}
}
#undef HANDLE

#ifndef AN_CLANG
#define HANDLE(_id, _var) if (_button.type == _id) return _button.released? (buttons.##_var && !prevButtons.##_var) : (!buttons.##_var && prevButtons.##_var);
#else
#define HANDLE(_id, _var) if (_button.type == _id) return _button.released? (buttons._var && !prevButtons._var) : (!buttons._var && prevButtons._var);
#endif
Optional<bool> VRControls::Hand::has_button_been_released(VR::Input::Button::WithSource _button) const
{
	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;
	if ((_button.source & source) &&
		_button.hand.get(hand) == hand)
	{
		HANDLE(VR::Input::Button::InUse, inUse);
		HANDLE(VR::Input::Button::UsesHandTracking, usesHandTracking);
		HANDLE(VR::Input::Button::Primary, primary);
		HANDLE(VR::Input::Button::Secondary, secondary);
		HANDLE(VR::Input::Button::SystemMenu, systemMenu);
		HANDLE(VR::Input::Button::SystemGestureProcessing, systemGestureProcessing);
		HANDLE(VR::Input::Button::Grip, grip);
		HANDLE(VR::Input::Button::PointerPinch, pointerPinch);
		HANDLE(VR::Input::Button::MiddlePinch, middlePinch);
		HANDLE(VR::Input::Button::RingPinch, ringPinch);
		HANDLE(VR::Input::Button::PinkyPinch, pinkyPinch);
		HANDLE(VR::Input::Button::Trigger, trigger);
		HANDLE(VR::Input::Button::Pose, pose);
		HANDLE(VR::Input::Button::UpsideDown, upsideDown);
		HANDLE(VR::Input::Button::InsideToHead, insideToHead);
		HANDLE(VR::Input::Button::HeadSideTouch, headSideTouch);
		HANDLE(VR::Input::Button::Thumb, thumb);
		HANDLE(VR::Input::Button::Pointer, pointer);
		HANDLE(VR::Input::Button::Middle, middle);
		HANDLE(VR::Input::Button::Ring, ring);
		HANDLE(VR::Input::Button::Pinky, pinky);
		HANDLE(VR::Input::Button::ThumbFolded, thumbFolded);
		HANDLE(VR::Input::Button::PointerFolded, pointerFolded);
		HANDLE(VR::Input::Button::MiddleFolded, middleFolded);
		HANDLE(VR::Input::Button::RingFolded, ringFolded);
		HANDLE(VR::Input::Button::PinkyFolded, pinkyFolded);
		HANDLE(VR::Input::Button::ThumbStraight, thumbStraight);
		HANDLE(VR::Input::Button::PointerStraight, pointerStraight);
		HANDLE(VR::Input::Button::MiddleStraight, middleStraight);
		HANDLE(VR::Input::Button::RingStraight, ringStraight);
		HANDLE(VR::Input::Button::PinkyStraight, pinkyStraight);
		HANDLE(VR::Input::Button::Trackpad, trackpad);
		HANDLE(VR::Input::Button::TrackpadTouch, trackpadTouch);
		HANDLE(VR::Input::Button::TrackpadDirCentre, trackpadDirCentre);
		HANDLE(VR::Input::Button::TrackpadDirLeft, trackpadDirLeft);
		HANDLE(VR::Input::Button::TrackpadDirRight, trackpadDirRight);
		HANDLE(VR::Input::Button::TrackpadDirDown, trackpadDirDown);
		HANDLE(VR::Input::Button::TrackpadDirUp, trackpadDirUp);
		HANDLE(VR::Input::Button::JoystickLeft, joystickLeft);
		HANDLE(VR::Input::Button::JoystickRight, joystickRight);
		HANDLE(VR::Input::Button::JoystickDown, joystickDown);
		HANDLE(VR::Input::Button::JoystickUp, joystickUp);
		HANDLE(VR::Input::Button::JoystickPress, joystickPress);
		an_assert(false, TXT("implement_ button"));
		return NP;
	}
	else
	{
		return NP;
	}
}
#undef HANDLE

bool VRControls::Hand::is_button_available(VR::Input::Button::WithSource _button) const
{
	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;
	if ((_button.source & source) &&
		_button.hand.get(hand) == hand)
	{
		return true;
	}
	else
	{
		return false;
	}
}

#ifndef AN_CLANG
#define HANDLE(_id, _var) if (_button.type == _id) return _button.released? !buttons.##_var : buttons.##_var;
#else
#define HANDLE(_id, _var) if (_button.type == _id) return _button.released? !buttons._var : buttons._var;
#endif
Optional<bool> VRControls::Hand::is_button_pressed(VR::Input::Button::WithSource _button) const
{
	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;
	if ((_button.source & source) &&
		_button.hand.get(hand) == hand)
	{
		HANDLE(VR::Input::Button::InUse, inUse);
		HANDLE(VR::Input::Button::UsesHandTracking, usesHandTracking);
		HANDLE(VR::Input::Button::Primary, primary);
		HANDLE(VR::Input::Button::Secondary, secondary);
		HANDLE(VR::Input::Button::SystemMenu, systemMenu);
		HANDLE(VR::Input::Button::SystemGestureProcessing, systemGestureProcessing);
		HANDLE(VR::Input::Button::Grip, grip);
		HANDLE(VR::Input::Button::PointerPinch, pointerPinch);
		HANDLE(VR::Input::Button::MiddlePinch, middlePinch);
		HANDLE(VR::Input::Button::RingPinch, ringPinch);
		HANDLE(VR::Input::Button::PinkyPinch, pinkyPinch);
		HANDLE(VR::Input::Button::Trigger, trigger);
		HANDLE(VR::Input::Button::Pose, pose);
		HANDLE(VR::Input::Button::UpsideDown, upsideDown);
		HANDLE(VR::Input::Button::InsideToHead, insideToHead);
		HANDLE(VR::Input::Button::HeadSideTouch, headSideTouch);
		HANDLE(VR::Input::Button::Thumb, thumb);
		HANDLE(VR::Input::Button::Pointer, pointer);
		HANDLE(VR::Input::Button::Middle, middle);
		HANDLE(VR::Input::Button::Ring, ring);
		HANDLE(VR::Input::Button::Pinky, pinky);
		HANDLE(VR::Input::Button::ThumbFolded, thumbFolded);
		HANDLE(VR::Input::Button::PointerFolded, pointerFolded);
		HANDLE(VR::Input::Button::MiddleFolded, middleFolded);
		HANDLE(VR::Input::Button::RingFolded, ringFolded);
		HANDLE(VR::Input::Button::PinkyFolded, pinkyFolded);
		HANDLE(VR::Input::Button::ThumbStraight, thumbStraight);
		HANDLE(VR::Input::Button::PointerStraight, pointerStraight);
		HANDLE(VR::Input::Button::MiddleStraight, middleStraight);
		HANDLE(VR::Input::Button::RingStraight, ringStraight);
		HANDLE(VR::Input::Button::PinkyStraight, pinkyStraight);
		HANDLE(VR::Input::Button::Trackpad, trackpad);
		HANDLE(VR::Input::Button::TrackpadTouch, trackpadTouch);
		HANDLE(VR::Input::Button::TrackpadDirCentre, trackpadDirCentre);
		HANDLE(VR::Input::Button::TrackpadDirLeft, trackpadDirLeft);
		HANDLE(VR::Input::Button::TrackpadDirRight, trackpadDirRight);
		HANDLE(VR::Input::Button::TrackpadDirDown, trackpadDirDown);
		HANDLE(VR::Input::Button::TrackpadDirUp, trackpadDirUp);
		HANDLE(VR::Input::Button::JoystickLeft, joystickLeft);
		HANDLE(VR::Input::Button::JoystickRight, joystickRight);
		HANDLE(VR::Input::Button::JoystickDown, joystickDown);
		HANDLE(VR::Input::Button::JoystickUp, joystickUp);
		HANDLE(VR::Input::Button::JoystickPress, joystickPress);
		an_assert(false, TXT("implement_ button"));
		return NP;
	}
	else
	{
		return NP;
	}
}
#undef HANDLE

#ifndef AN_CLANG
#define HANDLE_01(_id, _var) if (_button.type == _id) return _button.released? (buttons.##_var? 0.0f : 1.0f) : (buttons.##_var? 1.0f : 0.0f);
#else
#define HANDLE_01(_id, _var) if (_button.type == _id) return _button.released? (buttons._var? 0.0f : 1.0f) : (buttons._var? 1.0f : 0.0f);
#endif
#define HANDLE_CUSTOM(_id, _var, _value) if (_button.type == _id) return _button.released? 1.0f - _value : _value;

#ifndef AN_CLANG
#define HANDLE_BUTTON_STATE(_id, _var) \
	if (_button.type == _id) \
	{ \
		if (_var.is_set()) \
		{ \
			return _button.released? 1.0f - _var.get() : _var.get(); \
		} \
		else \
		{ \
			return _button.released? (buttons.##_var? 0.0f : 1.0f) : (buttons.##_var? 1.0f : 0.0f); \
		} \
	}
#else
#define HANDLE_BUTTON_STATE(_id, _var) \
	if (_button.type == _id) \
	{ \
		if (_var.is_set()) \
		{ \
			return _button.released? 1.0f - _var.get() : _var.get(); \
		} \
		else \
		{ \
			return _button.released? (buttons._var? 0.0f : 1.0f) : (buttons._var? 1.0f : 0.0f); \
		} \
	}
#endif
#ifndef AN_CLANG
#define HANDLE_HAND_TRACKING(_idFolded, _varFolded, _idStraight, _varStraight) \
	if (_button.type == _idFolded) \
	{ \
		return _button.released? (buttons.##_varFolded? 0.0f : 1.0f) : (buttons.##_varFolded? 1.0f : 0.0f); \
	} \
	if (_button.type == _idStraight) \
	{ \
		return _button.released? (buttons.##_varStraight? 0.0f : 1.0f) : (buttons.##_varStraight? 1.0f : 0.0f); \
	}
#else
#define HANDLE_HAND_TRACKING(_idFolded, _varFolded, _idStraight, _varStraight) \
	if (_button.type == _idFolded) \
	{ \
		return _button.released? (buttons._varFolded? 0.0f : 1.0f) : (buttons._varFolded? 1.0f : 0.0f); \
	} \
	if (_button.type == _idStraight) \
	{ \
		return _button.released? (buttons._varStraight? 0.0f : 1.0f) : (buttons._varStraight? 1.0f : 0.0f); \
	}
#endif
Optional<float> VRControls::Hand::get_button_state(VR::Input::Button::WithSource _button) const
{
	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;
	if ((_button.source & source) &&
		_button.hand.get(hand) == hand)
	{
		HANDLE_01(VR::Input::Button::InUse, inUse);
		HANDLE_01(VR::Input::Button::UsesHandTracking, usesHandTracking);
		HANDLE_01(VR::Input::Button::Primary, primary);
		HANDLE_01(VR::Input::Button::Secondary, secondary);
		HANDLE_01(VR::Input::Button::SystemMenu, systemMenu);
		HANDLE_01(VR::Input::Button::SystemGestureProcessing, systemGestureProcessing);
		HANDLE_BUTTON_STATE(VR::Input::Button::PointerPinch, pointerPinch);
		HANDLE_BUTTON_STATE(VR::Input::Button::MiddlePinch, middlePinch);
		HANDLE_BUTTON_STATE(VR::Input::Button::RingPinch, ringPinch);
		HANDLE_BUTTON_STATE(VR::Input::Button::PinkyPinch, pinkyPinch);
		HANDLE_BUTTON_STATE(VR::Input::Button::Trigger, trigger);
		HANDLE_BUTTON_STATE(VR::Input::Button::Grip, grip);
		HANDLE_BUTTON_STATE(VR::Input::Button::Pose, pose);
		HANDLE_BUTTON_STATE(VR::Input::Button::UpsideDown, upsideDown);
		HANDLE_BUTTON_STATE(VR::Input::Button::InsideToHead, insideToHead);
		HANDLE_BUTTON_STATE(VR::Input::Button::HeadSideTouch, headSideTouch);
		{
			HANDLE_BUTTON_STATE(VR::Input::Button::Thumb, thumb);
			HANDLE_BUTTON_STATE(VR::Input::Button::Pointer, pointer);
			HANDLE_BUTTON_STATE(VR::Input::Button::Middle, middle);
			HANDLE_BUTTON_STATE(VR::Input::Button::Ring, ring);
			HANDLE_BUTTON_STATE(VR::Input::Button::Pinky, pinky);
		}
		{
			// gives hard values 0.0 or 1.0, nothing between
			HANDLE_HAND_TRACKING(VR::Input::Button::ThumbFolded, thumbFolded, VR::Input::Button::ThumbStraight, thumbStraight);
			HANDLE_HAND_TRACKING(VR::Input::Button::PointerFolded, pointerFolded, VR::Input::Button::PointerStraight, pointerStraight);
			HANDLE_HAND_TRACKING(VR::Input::Button::MiddleFolded, middleFolded, VR::Input::Button::MiddleStraight, middleStraight);
			HANDLE_HAND_TRACKING(VR::Input::Button::RingFolded, ringFolded, VR::Input::Button::RingStraight, ringStraight);
			HANDLE_HAND_TRACKING(VR::Input::Button::PinkyFolded, pinkyFolded, VR::Input::Button::PinkyStraight, pinkyStraight);
		}
		HANDLE_01(VR::Input::Button::Trackpad, trackpad);
		HANDLE_01(VR::Input::Button::TrackpadTouch, trackpadTouch);
		HANDLE_01(VR::Input::Button::TrackpadDirCentre, trackpadDirCentre);
		HANDLE_01(VR::Input::Button::TrackpadDirLeft, trackpadDirLeft);
		HANDLE_01(VR::Input::Button::TrackpadDirRight, trackpadDirRight);
		HANDLE_01(VR::Input::Button::TrackpadDirDown, trackpadDirDown);
		HANDLE_01(VR::Input::Button::TrackpadDirUp, trackpadDirUp);
		HANDLE_01(VR::Input::Button::JoystickLeft, joystickLeft);
		HANDLE_01(VR::Input::Button::JoystickRight, joystickRight);
		HANDLE_01(VR::Input::Button::JoystickDown, joystickDown);
		HANDLE_01(VR::Input::Button::JoystickUp, joystickUp);
		HANDLE_01(VR::Input::Button::JoystickPress, joystickPress);
		an_assert(false, TXT("implement_ button"));
		return NP;
	}
	else
	{
		return NP;
	}
}
#undef HANDLE_HAND_TRACKING
#undef HANDLE_BUTTON_STATE
#undef HANDLE_CUSTOM
#undef HANDLE_01

Vector2 VRControls::Hand::get_stick(VR::Input::Stick::WithSource _stick, Vector2 _deadZone) const
{
	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;
	if ((_stick.source & source) &&
		_stick.hand.get(hand) == hand)
	{
		if (_stick.type == VR::Input::Stick::DPad) return ::System::Gamepad::apply_dead_zone(get_dpad(), _deadZone);
		if (_stick.type == VR::Input::Stick::Joystick) return ::System::Gamepad::apply_dead_zone(joystick, _deadZone);
		if (_stick.type == VR::Input::Stick::Trackpad) return ::System::Gamepad::apply_dead_zone(trackpad, _deadZone);
		if (_stick.type == VR::Input::Stick::TrackpadTouch) return ::System::Gamepad::apply_dead_zone(trackpadTouch, _deadZone);
		if (_stick.type == VR::Input::Stick::TrackpadDir) return ::System::Gamepad::apply_dead_zone(trackpadDir, _deadZone);
		an_assert(false, TXT("implement_ stick"));
	}

	return Vector2::zero;
}

Vector2 VRControls::Hand::get_mouse_relative_location(VR::Input::Mouse::WithSource _mouse) const
{
	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;
	if ((_mouse.source & source) &&
		_mouse.hand.get(hand) == hand)
	{
		return trackpadTouchDelta;
	}

	return Vector2::zero;
}

Vector2 VRControls::Hand::get_mouse_relative_location(Array<VR::Input::Mouse::WithSource> const & _mouses) const
{
	Vector2 result = Vector2::zero;
	for_every(mouse, _mouses)
	{
		result += get_mouse_relative_location(*mouse);
	}
	return result;
}

bool VRControls::Hand::has_button_been_pressed(Array<VR::Input::Button::WithSource> const & _buttons, bool _allRequired) const
{
	bool result = false;

	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;

	if (_allRequired)
	{
		// if number of pressed + number of already pressed = number of buttons
		int numberOfPressed = 0;
		int numberOfAlreadyPressed = 0;
		for_every(button, _buttons)
		{
			if (!button->hand.is_set() || button->hand.get() == hand)
			{
				if (has_button_been_pressed(*button).get(false))
				{
					++numberOfPressed;
				}
				if (is_button_pressed(*button).get(false))
				{
					++numberOfAlreadyPressed;
				}
			}
		}
		result = ((numberOfPressed + numberOfAlreadyPressed) == _buttons.get_size());
	}
	else
	{
		for_every(button, _buttons)
		{
			if (!button->hand.is_set() || button->hand.get() == hand)
			{
				Optional<bool> res = has_button_been_pressed(*button);
				if (res.get(false))
				{
					result = true;
				}
			}
		}
	}

	return result;
}

bool VRControls::Hand::has_button_been_released(Array<VR::Input::Button::WithSource> const & _buttons, bool _allRequired) const
{
	bool result = false;

	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;

	if (_allRequired)
	{
		// if number of released + number of still pressed = number of buttons
		int numberOfReleased = 0;
		int numberOfStillPressed = 0;
		for_every(button, _buttons)
		{
			if (!button->hand.is_set() || button->hand.get() == hand)
			{
				if (has_button_been_released(*button).get(false))
				{
					++numberOfReleased;
				}
				if (is_button_pressed(*button).get(false))
				{
					++numberOfStillPressed;
				}
			}
		}
		result = ((numberOfReleased + numberOfStillPressed) == _buttons.get_size());
	}
	else
	{
		for_every(button, _buttons)
		{
			if (!button->hand.is_set() || button->hand.get() == hand)
			{
				Optional<bool> res = has_button_been_released(*button);
				if (res.get(false))
				{
					result = true;
				}
			}
		}
	}

	return result;
}

bool VRControls::Hand::is_button_pressed(Array<VR::Input::Button::WithSource> const & _buttons, bool _allRequired) const
{
	bool result = false;
	
	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;

	if (_allRequired)
	{
		result = true;
		for_every(button, _buttons)
		{
			if (!button->hand.is_set() || button->hand.get() == hand)
			{
				Optional<bool> res = is_button_pressed(*button);
				if (! res.get(false))
				{
					result = false;
					break;
				}
			}
		}
	}
	else
	{
		for_every(button, _buttons)
		{
			if (!button->hand.is_set() || button->hand.get() == hand)
			{
				Optional<bool> res = is_button_pressed(*button);
				if (res.get(false))
				{
					result = true;
					break;
				}
			}
		}
	}

	return result;
}

float VRControls::Hand::get_button_state(Array<VR::Input::Button::WithSource> const & _buttons, bool _allRequired) const
{
	float result = 0.0f;

	::Hand::Type hand = handIndex == IVR::get()->get_left_hand() ? ::Hand::Left : ::Hand::Right;

	if (_allRequired)
	{
		result = 1.0f;
		for_every(button, _buttons)
		{
			if (!button->hand.is_set() || button->hand.get() == hand)
			{
				Optional<float> res = get_button_state(*button);
				result *= res.get(0.0f);
			}
		}
	}
	else
	{
		for_every(button, _buttons)
		{
			if (!button->hand.is_set() || button->hand.get() == hand)
			{
				Optional<float> res = get_button_state(*button);
				result = max(result, res.get(0.0f));
			}
		}
	}

	return result;
}

Vector2 VRControls::Hand::get_stick(Array<VR::Input::Stick::WithSource> const & _sticks, Vector2 _deadZone) const
{
	Vector2 result = Vector2::zero;

	for_every(stick, _sticks)
	{
		result += get_stick(*stick, _deadZone);
	}

	return result;
}

//

float Pulse::get_current(float const& _what, Optional<float> const& _whatEnd) const
{
	float startValue = _what;
	float endValue = _whatEnd.get(startValue);

	float pt = length != 0.0f ? clamp(1.0f - timeLeft / length, 0.0f, 1.0f) : 1.0f;
	float currentValue = startValue * (1.0f - pt) + pt * endValue;

	if (timeLeft > 0.0f)
	{
		float useFadeOutTime = fadeOutTime.get(0.0f);
		if (useFadeOutTime > 0.0f)
		{
			currentValue *= clamp(timeLeft / useFadeOutTime, 0.0f, 1.0f);
		}
		return clamp(currentValue, 0.0f, 1.0f);
	}
	else
	{
		return 0.0f;
	}
}

float Pulse::get_current_frequency() const
{
	return get_current(frequency, frequencyEnd);
}

float Pulse::get_current_strength() const
{
	return get_current(strength, strengthEnd);
}

bool Pulse::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	if (!_node)
	{
		return result;
	}
	length = _node->get_float_attribute(TXT("length"), length);
	frequency = _node->get_float_attribute(TXT("frequency"), frequency);
	frequencyEnd.load_from_xml(_node, TXT("frequencyEnd"));
	frequencyEnd.load_from_xml(_node, TXT("endFrequency"));
	strength = _node->get_float_attribute(TXT("strength"), strength);
	strengthEnd.load_from_xml(_node, TXT("strengthEnd"));
	strengthEnd.load_from_xml(_node, TXT("endStrength"));
	warn_loading_xml_on_assert(frequency > 0.0f || _node->get_bool_attribute_or_from_child_presence(TXT("autoFrequencyFromStrength")), _node, TXT("frequency should be greater than zero, will be auto calculated basing on strength"));
	warn_loading_xml_on_assert(strength > 0.0f, _node, TXT("strength should be greater than zero"));
	fadeOutTime.load_from_xml(_node, TXT("fadeOut"));
	fadeOutTime.load_from_xml(_node, TXT("fadeOutTime"));
	make_valid();
	return result;
}

void Pulse::make_valid()
{
	if (frequency == 0.0f)
	{
		frequency = 1.0f - 0.7f * strength;
	}
	if (! frequencyEnd.is_set() && strengthEnd.is_set())
	{
		frequencyEnd = 1.0f - 0.7f * strengthEnd.get();
	}
}

bool Pulse::load_from_xml(IO::XML::Node const * _node, tchar const * _childName)
{
	bool result = true;
	if (!_node)
	{
		return result;
	}
	for_every(node, _node->children_named(_childName))
	{
		result &= load_from_xml(node);
	}
	return result;
}

//

int AvailableFunctionality::convert_foveated_rendering_config_to_level(float _config)
{
	return clamp(TypeConversions::Normal::f_i_closest((float)foveatedRenderingNominal * _config), 0, foveatedRenderingMax);
}

float AvailableFunctionality::convert_foveated_rendering_level_to_config(int _level)
{
	return foveatedRenderingNominal > 0 ? (float)_level / (float)foveatedRenderingNominal : 0;
}

//

IVR* IVR::s_current = nullptr;

void IVR::create()
{
	if (!MainConfig::global().should_allow_vr() && !MainConfig::global().should_force_vr())
	{
		return;
	}

	String forcedVR = MainConfig::global().get_forced_vr();
	if (!forcedVR.is_empty())
	{
		output(TXT("forcing \"%S\""), forcedVR.to_char());
#ifdef AN_WITH_OCULUS_MOBILE
		if (String::compare_icase(forcedVR, TXT("oculusMobile")) ||
			String::compare_icase(forcedVR, TXT("oculus mobile")))
		{
			new OculusMobile();
		}
		else
#endif
/*
#ifdef AN_WITH_WAVEVR
		if (String::compare_icase(forcedVR, TXT("wavevr")))
		{
			new WaveVR();
		}
		else
#endif
*/
#ifdef AN_WITH_OPENXR
		if (String::compare_icase(forcedVR, TXT("openxr")))
		{
			new OpenXR(false); // always as second round, to force it
		}
		else
#endif
#ifdef AN_WITH_OCULUS
		if (String::compare_icase(forcedVR, TXT("oculus")))
		{
			new Oculus();
		}
		else
#endif
#ifdef AN_WITH_OPENVR
		if (String::compare_icase(forcedVR, TXT("steamvr")) ||
			String::compare_icase(forcedVR, TXT("openvr")))
		{
			new OpenVR();
		}
		else
#endif
		{
			// default
#ifdef AN_WITH_OPENXR
			new OpenXR(false); // always as second round, to force it
#else
	#ifdef AN_WITH_OPENVR
			new OpenVR();
	#endif
#endif
		}
	}
	else
	{
#ifdef AN_WITH_OPENXR
		if (OpenXR::is_available() && (!IVR::get() || !IVR::get()->is_ok(true)))
		{
			terminate();
			output(TXT("choose open xr"));
			new OpenXR(true); // first choice
		}
#endif
#ifdef AN_WITH_OCULUS_MOBILE
		if (OculusMobile::is_available() && (!IVR::get() || !IVR::get()->is_ok(true)))
		{
			terminate();
			output(TXT("choose oculus mobile"));
			new OculusMobile();
		}
#endif
#ifdef AN_WITH_OCULUS
		if (Oculus::is_available() && (! IVR::get() || ! IVR::get()->is_ok(true)))
		{
			terminate();
			output(TXT("choose oculus"));
			new Oculus();
		}
#endif
#ifdef AN_WITH_OPENVR
		if (OpenVR::is_available() && (!IVR::get() || !IVR::get()->is_ok(true)))
		{
			terminate();
			output(TXT("choose open vr"));
			new OpenVR();
		}
#endif
#ifdef AN_WITH_WAVEVR
		if (WaveVR::is_available() && (!IVR::get() || !IVR::get()->is_ok(true)))
		{
			terminate();
			output(TXT("choose wave vr"));
			new WaveVR();
		}
#endif
		// second round
#ifdef AN_WITH_OPENXR
		if (OpenXR::is_available() && (!IVR::get() || !IVR::get()->is_ok(true)))
		{
			terminate();
			output(TXT("choose open xr [second round]"));
			new OpenXR(false);
		}
#endif
	}

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("is ok?"));
#endif

	if (s_current)
	{
		output(TXT("VR running in requested mode \"%S\""), VRMode::to_char(MainConfig::global().get_vr_mode()));
	}

	if (s_current && ! MainConfig::global().should_force_vr())
	{
		if (!s_current->is_ok(true))
		{
			output(TXT("not ok, terminate"));
			terminate();
		}
	}
}

void IVR::terminate()
{
	delete_and_clear(s_current);
}

void IVR::create_config_files()
{
#ifdef AN_WITH_OPENVR
	OpenVR::create_config_files();
#endif
}

IVR::IVR(Name const & _name)
: name(_name)
, renderIsOpen(false)
{
	an_assert(s_current == nullptr);
	s_current = this;

	renderMode = RenderMode::HiRes; // start hi-res
}

IVR::~IVR()
{
	::System::Core::on_quick_exit_no_longer(TXT("close VR"));
	an_assert(s_current == this);
	s_current = nullptr;

	if (auto* input = ::System::Input::get())
	{
		input->remove_usage_tag(NAME(vr));
	}
}

void IVR::init(::System::Video3D* _v3d)
{
	::System::Core::on_quick_exit(TXT("close VR"), [this]() { this->get_ready_to_exit();  this->terminate(); });

	enter_vr_mode();
	init_video(_v3d);

	expectedFrameTime = update_expected_frame_time();
	idealExpectedFrameTime = calculate_ideal_expected_frame_time();
	an_assert(expectedFrameTime > 0.0f);

	advance();
	set_controls_base_pose_from_view();
	decide_hands();

	an_assert(::System::Input::get(), TXT("should be created at this time"));
	if (auto* input = ::System::Input::get())
	{
		input->set_usage_tag(NAME(vr));
	}
}

bool IVR::begin_render(System::Video3D * _v3d)
{
	store_prev_render_pose_set();
	if (!renderIsOpen)
	{
		// update scaling first
		if (may_use_render_scaling())
		{
			MEASURE_PERFORMANCE(updateRenderScaling);
			update_render_scaling(REF_ renderScaling);
		}
		else
		{
			// force
			renderScaling = 1.0f;
		}
		{
			MEASURE_PERFORMANCE(useRenderTargetScaling);
			use_render_target_scaling(renderScaling);
		}
		bool opened = begin_render_internal(_v3d);
		renderIsOpen = opened;
		if (opened)
		{
			return true;
		}
	}
	return false;
}

void IVR::copy_render_to_output(System::Video3D * _v3d)
{
	bool copy = true;
#ifdef VR_API
	copy = false;
#endif
	if (copy)
	{
		for (int rtIdx = 0; rtIdx < Eye::Count; ++rtIdx)
		{
			VR::Eye::Type eye = (VR::Eye::Type)rtIdx;
			if (System::RenderTarget * rtOutput = get_output_render_target(eye))
			{
				if (System::RenderTarget * rtRender = get_render_render_target(eye))
				{
					if (rtRender != rtOutput)
					{
						rtRender->resolve_forced_unbind();
						rtOutput->bind();
						_v3d->setup_for_2d_display();
						_v3d->set_viewport(VectorInt2::zero, rtOutput->get_size());
						_v3d->set_near_far_plane(0.02f, 100.0f);

						_v3d->set_2d_projection_matrix_ortho();
						_v3d->access_model_view_matrix_stack().clear();

						rtRender->render(0, _v3d, Vector2::zero, rtOutput->get_size().to_vector2());
						rtOutput->unbind();
					}
				}
			}
		}
	}
}

void IVR::end_render(System::Video3D * _v3d)
{
	if (! renderIsOpen)
	{
		// no need to end render
		return;
	}

	bool result = renderIsOpen;
	renderIsOpen = false;

	_v3d->requires_send_shader_program();

	if (!useDirectToVRRendering)
	{
		for (int rtIdx = 0; rtIdx < Eye::Count; ++rtIdx)
		{
			VR::Eye::Type eye = (VR::Eye::Type)rtIdx;
			if (System::RenderTarget * rt = get_output_render_target(eye))
			{
				rt->resolve();
			}
		}
	}

	if (result)
	{
		MEASURE_PERFORMANCE_COLOURED(vrDisplayThroughImpl, Colour::zxMagentaBright);
		end_render_internal(_v3d);
	}

	if (useFallbackRendering)
	{
		fallback_rendering(_v3d);
	}
}

void IVR::fallback_rendering(System::Video3D * _v3d)
{
	// fallback rendering

	if (! useDirectToVRRendering)
	{
		MEASURE_PERFORMANCE_COLOURED(vrFallbackRendering, Colour::zxMagentaBright);

		// ready to render to main render target
		_v3d->setup_for_2d_display();

		_v3d->set_default_viewport();
		_v3d->set_near_far_plane(0.02f, 100.0f);

		_v3d->set_2d_projection_matrix_ortho();
		_v3d->access_model_view_matrix_stack().clear();

		// render both eyes to main buffer
		::System::RenderTarget::bind_none();

		Vector2 renderAt = Vector2::zero;
		for (int rtIdx = 0; rtIdx < Eye::Count; ++rtIdx)
		{
			VR::Eye::Type eye = (VR::Eye::Type)rtIdx;
			if (System::RenderTarget* rt = get_output_render_target(eye))
			{
				// get size to what we should render
				Vector2 renderSize = rt->get_size().to_vector2();
				float viewportSizeY = _v3d->get_viewport_size().to_vector2().y;
				renderSize.x = renderSize.x * viewportSizeY / renderSize.y;
				renderSize.y = viewportSizeY;

				rt->render(0, _v3d, renderAt, renderSize);

				renderAt.x += renderSize.x;
			}
		}
	}
}

void IVR::render_one_output_to_main(System::Video3D* _v3d, Eye::Type _eye)
{
	// if using direct to vr, disallow rendering (for msaa it would require solving etc)
	if (!useDirectToVRRendering)
	{
		render_one_output_to(_v3d, _eye, nullptr);
	}
}

void IVR::render_one_output_to(System::Video3D* _v3d, Eye::Type _eye, ::System::RenderTarget* _rt, Optional<RangeInt2> const& _at)
{
	{
		MEASURE_PERFORMANCE_COLOURED(vrOneOutputToMain, Colour::zxMagentaBright);

		if (System::RenderTarget* rt = get_output_render_target(_eye))
		{
			rt->resolve();

			// ready to render to main render target
			_v3d->setup_for_2d_display();

			VectorInt2 renderToSize = _rt? _rt->get_size() : _v3d->get_screen_size();
			RangeInt2 renderAtRect;
			if (_at.is_set())
			{
				renderAtRect = RangeInt2(RangeInt(_at.get().x.min, _at.get().x.max), RangeInt(_at.get().y.min, _at.get().y.max));
			}
			else
			{
				renderAtRect = RangeInt2::zero;
				renderAtRect.include(renderToSize);
			}
			_v3d->set_viewport(renderAtRect.get_as_at(), renderAtRect.get_as_size());
			_v3d->set_near_far_plane(0.02f, 100.0f);

			_v3d->set_2d_projection_matrix_ortho();
			_v3d->access_model_view_matrix_stack().clear();

			if (_rt)
			{
				_rt->bind();
			}
			else
			{
				// render both eyes to main buffer
				::System::RenderTarget::bind_none();
			}

			// get size to what we should render
			Vector2 renderSize = rt->get_size().to_vector2();
			Vector2 viewportSize = _v3d->get_viewport_size().to_vector2();
			renderSize.x = renderSize.x * viewportSize.y / renderSize.y;
			renderSize.y = viewportSize.y;
			if (renderSize.x < viewportSize.x)
			{
				float scale = viewportSize.x / renderSize.x;
				renderSize.x *= scale;
				renderSize.y *= scale;
			}

			Vector2 renderAt = (viewportSize - renderSize) * 0.5f;

			{
				auto const lensCentreOffset = get_render_info().lensCentreOffset[_eye] * Vector2(1.0f, -1.0f);
				// if we're off the centre, possible render size will get smaller
				Vector2 renderSizeCentred = renderSize * Vector2(1.0f - abs(lensCentreOffset.x), 1.0f - abs(lensCentreOffset.y));
				float scale = 1.0f;
				// calculate scale to fill whole viewport
				scale = max(scale, max(1.0f, viewportSize.x / renderSizeCentred.x));
				scale = max(scale, max(1.0f, viewportSize.y / renderSizeCentred.y));
				// change scale
				renderSize *= scale;
				// find where the lens centre is at target space (rt, viewport)
				Vector2 centreAtTS = viewportSize * 0.5f;
				Vector2 lensCentreAtTS = centreAtTS + (renderSize * 0.5f) * lensCentreOffset; // render size halved as lens centre offset reaches from -1 to 1
				// offset by render scale
				renderAt = lensCentreAtTS - renderSize * 0.5f;
			}

			rt->render(0, _v3d, renderAt, renderSize);

			::System::RenderTarget::bind_none();
		}
		else
		{
			// if nothing provided, clear
			if (_rt)
			{
				_rt->bind();
			}
			else
			{
				// render both eyes to main buffer
				::System::RenderTarget::bind_none();
			}

			_v3d->clear_colour(Colour::black);

			::System::RenderTarget::bind_none();
		}
	}
}

void IVR::set_controls_base_pose_from_view()
{
	if (controlsPoseSet.view.is_set())
	{
		controlsPoseSet.baseFull = controlsPoseSet.view.get();
		{
			Rotator3 baseRotation = controlsPoseSet.baseFull.get().get_orientation().to_rotator();
			baseRotation.pitch = 0.0f;
			baseRotation.roll = 0.0f;
			controlsPoseSet.baseFull.access().set_orientation(baseRotation.to_quat());
		}
		controlsPoseSet.baseFlat = controlsPoseSet.baseFull;
		{
			Vector3 baseLocation = controlsPoseSet.baseFlat.get().get_translation();
			baseLocation.z = 0.0f;
			controlsPoseSet.baseFlat.access().set_translation(baseLocation);
		}
		lastValidRenderPoseSet.baseFlat = renderPoseSet.baseFlat = controlsPoseSet.baseFlat;
		lastValidRenderPoseSet.baseFull = renderPoseSet.baseFull = controlsPoseSet.baseFull;
		prevRenderPoseSet.baseFull = renderPoseSet.baseFull;
		prevControlsPoseSet.baseFull = controlsPoseSet.baseFull;
		prevRenderPoseSet.baseFlat = renderPoseSet.baseFlat;
		prevControlsPoseSet.baseFlat = controlsPoseSet.baseFlat;
	}
}

void IVR::store_last_valid_render_pose_set()
{
	if (renderPoseSet.view.is_set())
	{
		lastValidRenderPoseSet.view = renderPoseSet.view;
	}
	for_count(int, i, Hands::Count)
	{
		if (renderPoseSet.hands[i].placement.is_set())
		{
			lastValidRenderPoseSet.hands[i].placement = renderPoseSet.hands[i].placement;
		}
		for_count(int, b, VRHandPose::MAX_RAW_BONES)
		{
			if (renderPoseSet.hands[i].rawBonesLS[b].is_set())
			{
				lastValidRenderPoseSet.hands[i].rawBonesLS[b] = renderPoseSet.hands[i].rawBonesLS[b];
			}
			else if (renderPoseSet.hands[i].rawBonesRS[b].is_set())
			{
				lastValidRenderPoseSet.hands[i].rawBonesLS[b].clear();
			}
			if (renderPoseSet.hands[i].rawBonesRS[b].is_set())
			{
				lastValidRenderPoseSet.hands[i].rawBonesRS[b] = renderPoseSet.hands[i].rawBonesRS[b];
			}
			else if (renderPoseSet.hands[i].rawBonesLS[b].is_set())
			{
				lastValidRenderPoseSet.hands[i].rawBonesRS[b].clear();
			}
		}
	}
	if (renderPoseSet.baseFull.is_set())
	{
		lastValidRenderPoseSet.baseFull = renderPoseSet.baseFull;
	}
	if (renderPoseSet.baseFlat.is_set())
	{
		lastValidRenderPoseSet.baseFlat = renderPoseSet.baseFlat;
	}
}

void IVR::decide_hands()
{
	// check if implementation handles
	auto decisionResult = decide_hands_by_system(OUT_ leftHand, OUT_ rightHand);
	if (decisionResult == DecideHandsResult::Decided)
	{
		handsRequireDecision = false;
	}
	else
	{
		handsRequireDecision = true;
		if (decisionResult == DecideHandsResult::PartiallyDecided)
		{
			// keep roles as they are, but we will be still looking
		}
		else
		{
			if ((!controlsPoseSet.hands[0].placement.is_set() &&
				 !controlsPoseSet.hands[1].placement.is_set()) ||
				!controlsPoseSet.view.is_set())
			{
				rightHand = NONE;
				leftHand = NONE;
				// just fill randomly as both hands are missing or head
				for_count(int, i, Hands::Count)
				{
					if (rightHand == NONE) rightHand = i;
					else if (leftHand == NONE) leftHand = i;
				}
			}
			else
			{
				// one hand is enough to determine both hands, if another kicks in, we will redo this

				float viewYaw = controlsPoseSet.view.get().get_orientation().to_rotator().yaw;

				float hand0Yaw = controlsPoseSet.hands[0].placement.is_set()? controlsPoseSet.hands[0].placement.get().get_orientation().to_rotator().yaw : viewYaw;
				float hand1Yaw = controlsPoseSet.hands[1].placement.is_set()? controlsPoseSet.hands[1].placement.get().get_orientation().to_rotator().yaw : viewYaw;

				float hand0YawOffset = Rotator3::normalise_axis(hand0Yaw - viewYaw);
				float hand1YawOffset = Rotator3::normalise_axis(hand1Yaw - viewYaw);

				if (hand0YawOffset > hand1YawOffset)
				{
					rightHand = 0;
					leftHand = 1;
				}
				else
				{
					rightHand = 1;
					leftHand = 0;
				}

				// if they are on different sides, they no longer require decision
				if (hand0YawOffset > 45.0f && hand1YawOffset < -45.0f)
				{
					handsRequireDecision = false;
				}
				if (hand0YawOffset < -45.0f && hand1YawOffset > 45.0f)
				{
					handsRequireDecision = false;
				}
			}
		}
	}

	update_input_tags();

	update_on_decided_hands(leftHand, rightHand);

	update_hands_controls_source();

	update_hands_available();
}

void IVR::advance()
{
	expectedFrameTime = update_expected_frame_time();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	an_assert(!handsRequireSettingInputTags, TXT("we should already set it"));
#endif

	float deltaTime = ::System::Core::get_raw_delta_time();
	handsRequireDecisionTimeLeft -= deltaTime;
	if (handsRequireDecisionTimeLeft <= 0.0f)
	{
		if (handsRequireDecision)
		{
			decide_hands();
			handsRequireDecisionTimeLeft = 1.0f;
		}
		else
		{
			todo_note(TXT("check if hands may require deciding?"));
			handsRequireDecisionTimeLeft = 10.0f;
		}
	}
	for_count(int, i, Hands::Count)
	{
		pulses[i].timeLeft = max(0.0f, pulses[i].timeLeft - deltaTime);
	}
	if (resetProcessingLevelsTimeLeft.is_set())
	{
		resetProcessingLevelsTimeLeft = resetProcessingLevelsTimeLeft.get() - deltaTime;
		if (resetProcessingLevelsTimeLeft.get() <= 0.0f)
		{
			set_processing_levels();
			an_assert(!resetProcessingLevelsTimeLeft.is_set());
		}
	}
}

void IVR::game_sync()
{
	// do nothing
}

void IVR::pre_advance_controls()
{
	prevControllersIteration = controllersIteration;
	controls.pre_advance();
}

bool IVR::may_use_render_scaling() const
{
	if (eyeRenderRenderTargets[Eye::Left].get() ||
		eyeRenderRenderTargets[Eye::Right].get())
	{
		// we can use scaling (see below)
		return true;
	}
	else
	{
		return false;
	}
}

void IVR::use_render_target_scaling(float _scale)
{
	if (auto * rt = eyeRenderRenderTargets[Eye::Left].get())
	{
		rt->use_scaling(_scale);
	}
	if (auto * rt = eyeRenderRenderTargets[Eye::Right].get())
	{
		rt->use_scaling(_scale);
	}
	// output should remain
}

void IVR::create_render_targets_for_output(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution)
{
	eyeRenderRenderTargets[Eye::Left] = nullptr;
	eyeRenderRenderTargets[Eye::Right] = nullptr;
	eyeOutputRenderTargets[Eye::Left] = nullptr;
	eyeOutputRenderTargets[Eye::Right] = nullptr;

	useDirectToVRRendering = false;
	noOutputRenderTarget = false;

	::System::VideoConfig::general_set_post_process_allowed(true);

	create_internal_render_targets(_source, _fallbackEyeResolution, OUT_ useDirectToVRRendering, OUT_ noOutputRenderTarget);
	an_assert(!useDirectToVRRendering || get_available_functionality().directToVR, TXT("should allow \"direct to vr\" to use it, implement get_available_functionality?"));

	if (useDirectToVRRendering)
	{
		// we won't be using post process anyway
		::System::VideoConfig::general_set_post_process_allowed(false);
	}
	else
	{
		for (int i = 0; i < Eye::Count; ++i)
		{
			VectorInt2 resolution = get_render_info().eyeResolution[i];
			if (resolution.is_zero())
			{
				resolution = _fallbackEyeResolution * get_render_info().renderScale[i];
			}
			resolution *= MainConfig::global().get_video().vrResolutionCoef * MainConfig::global().get_video().overall_vrResolutionCoef;
			VectorInt2 fullResolution = resolution;
			resolution = apply_aspect_ratio_coef(resolution, MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef);

			::System::VideoConfig::general_set_post_process_allowed_for(resolution);

			Optional<::System::FoveatedRenderingSetup> foveatedRenderingSetup;

			if (::System::FoveatedRenderingSetup::should_use())
			{
				foveatedRenderingSetup.set_if_not_set();
				foveatedRenderingSetup.access().setup((Eye::Type)i, get_render_info().lensCentreOffset[i], resolution.to_vector2(), 1.0f);
			}

			{
				::System::RenderTargetSetup rtEyeSetup = ::System::RenderTargetSetup(
					resolution,
					MainConfig::global().get_video().get_vr_msaa_samples(),
					::System::DepthStencilFormat::defaultFormat);
				rtEyeSetup.set_aspect_ratio_coef(MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef);
				rtEyeSetup.copy_output_textures_from(_source);
				// force linear filtering
				for_count(int, n, rtEyeSetup.get_output_texture_count())
				{
					auto& otd = rtEyeSetup.access_output_texture_definition(n);
					otd.set_filtering_mag(::System::TextureFiltering::linear);
				}
				rtEyeSetup.for_eye_idx(i);
				rtEyeSetup.use_foveated_rendering(foveatedRenderingSetup);
				RENDER_TARGET_CREATE_INFO(TXT("IVR::create_render_targets_for_output, ! useDirectToVRRendering, eye setup %i, render render target"), i); 
				eyeRenderRenderTargets[i] = new ::System::RenderTarget(rtEyeSetup);
			}

			// for VR we should always create separate render target to have mip maps on it? I guess it's better
			if (noOutputRenderTarget)
			{
				eyeOutputRenderTargets[i] = eyeRenderRenderTargets[i];
			}
			else
			{
				::System::RenderTargetSetup rtEyeSetup = ::System::RenderTargetSetup(
					fullResolution,
					0);
				todo_note(TXT("maybe just rgb?"));
				rtEyeSetup.copy_output_textures_from(_source);
				rtEyeSetup.for_eye_idx(i);
				//rtEyeSetup.force_mip_maps();
				//rtEyeSetup.limit_mip_maps(1);
				// no foveated for output
				RENDER_TARGET_CREATE_INFO(TXT("IVR::create_render_targets_for_output, ! useDirectToVRRendering, eye setup %i, output render target"), i);
				eyeOutputRenderTargets[i] = new ::System::RenderTarget(rtEyeSetup);
			}
		}
	}

	on_render_targets_created();
}

void IVR::update_foveated_rendering_for_render_targets()
{
	if (!useDirectToVRRendering)
	{
		for (int i = 0; i < Eye::Count; ++i)
		{
			update_foveated_rendering_for_render_target(eyeRenderRenderTargets[i].get(), (Eye::Type)i);
		}
	}
}

void IVR::update_foveated_rendering_for_render_target(::System::RenderTarget* _rt, Eye::Type _eye)
{
	if (_rt)
	{
		::System::FoveatedRenderingSetup foveatedRenderingSetup;

		foveatedRenderingSetup.setup(_eye, get_render_info().lensCentreOffset[_eye], _rt->get_size().to_vector2(), renderScaling);

		// if doesn't use, won't modify anything
		_rt->update_foveated_rendering(foveatedRenderingSetup);
	}
}

void IVR::set_play_area_rect(Vector3 const & _halfForward, Vector3 const & _halfRight)
{
#ifdef VERBOSE_UPDATE_VR
	output(TXT("[IVR] set_play_area_rect"));
	output(TXT("[IVR]    half right:   %S"), _halfRight.to_string().to_char());
	output(TXT("[IVR]    half forward: %S"), _halfForward.to_string().to_char());
#endif

	{	// as provided
		rawWholePlayAreaRectHalfForward = _halfForward;
		rawWholePlayAreaRectHalfRight = _halfRight;
	}

	update_play_area_rect();
}

void IVR::update_scaling()
{
	auto const& mc = MainConfig::global();
	set_scaling(Scaling(mc.get_vr_scaling(), mc.should_be_immobile_vr()? 1.0f : mc.get_vr_horizontal_scaling()));
}

void IVR::update_play_area_rect()
{
	auto const& mc = MainConfig::global();

#ifdef VERBOSE_UPDATE_VR
	output(TXT("[IVR] update_play_area_rect"));
#endif

	if (mc.should_be_immobile_vr())
	{
#ifdef VERBOSE_UPDATE_VR
		output(TXT("[IVR] setup play area for immobile vr"));
		output(TXT("[IVR] immobile vr size: %.3fx%.3f"), mc.get_immobile_vr_size().x, mc.get_immobile_vr_size().y);
		output(TXT("[IVR] vr scaling: %.3f"), mc.get_vr_scaling());
#endif

		wholePlayAreaRectHalfForward = Vector3::yAxis * mc.get_immobile_vr_size().y * 0.5f;
		wholePlayAreaRectHalfRight = Vector3::xAxis * mc.get_immobile_vr_size().x * 0.5f;

		// as used
		playAreaRectHalfForward = wholePlayAreaRectHalfForward * (mc.get_vr_scaling());
		playAreaRectHalfRight = wholePlayAreaRectHalfRight * (mc.get_vr_scaling());
	}
	else
	{
#ifdef VERBOSE_UPDATE_VR
		output(TXT("[IVR] setup play area with values from vr system"));
#endif

		wholePlayAreaRectHalfForward = rawWholePlayAreaRectHalfForward;
		wholePlayAreaRectHalfRight = rawWholePlayAreaRectHalfRight;

		// adjust by config override
		if (mc.get_vr_map_space_size().is_set())
		{
#ifdef VERBOSE_UPDATE_VR
			output(TXT("[IVR] using space size from main config %S"), mc.get_vr_map_space_size().get().to_string().to_char());
#endif
			if (wholePlayAreaRectHalfForward.is_zero() || wholePlayAreaRectHalfRight.is_zero())
			{
				wholePlayAreaRectHalfForward = Vector3::yAxis;
				wholePlayAreaRectHalfRight = Vector3::xAxis;
			}
			wholePlayAreaRectHalfForward = wholePlayAreaRectHalfForward.normal() * mc.get_vr_map_space_size().get().y * 0.5f;
			wholePlayAreaRectHalfRight = wholePlayAreaRectHalfRight.normal() * mc.get_vr_map_space_size().get().x * 0.5f;

#ifdef VERBOSE_UPDATE_VR
			output(TXT("[IVR] whole half play area [from main config] %.3fx%.3f"), wholePlayAreaRectHalfRight.length(), wholePlayAreaRectHalfForward.length());
#endif
		}

#ifdef VERBOSE_UPDATE_VR
		output(TXT("[IVR] whole half play area [before orthogonal] %.3fx%.3f"), wholePlayAreaRectHalfRight.length(), wholePlayAreaRectHalfForward.length());
#endif

		// keep them orthogonal
		{
			Vector3 forwardNormal = wholePlayAreaRectHalfForward.normal();
			wholePlayAreaRectHalfRight -= forwardNormal * Vector3::dot(forwardNormal, wholePlayAreaRectHalfRight);
		}

#ifdef VERBOSE_UPDATE_VR
		output(TXT("[IVR] whole play area [for use] %.3fx%.3f"), wholePlayAreaRectHalfRight.length() * 2.0f, wholePlayAreaRectHalfForward.length() * 2.0f);
		output(TXT("[IVR] from main config:"));
		output(TXT("[IVR]   margin: %S"), mc.get_vr_map_margin().to_string().to_char());
		output(TXT("[IVR]   horizontal scaling: %.3f"), mc.get_vr_horizontal_scaling());
		output(TXT("[IVR]   scaling: %.3f"), mc.get_vr_scaling());
#endif

		// as used
		playAreaRectHalfForward = wholePlayAreaRectHalfForward.normal() * (wholePlayAreaRectHalfForward.length() - mc.get_vr_map_margin().y) * (mc.get_vr_horizontal_scaling() * mc.get_vr_scaling());
		playAreaRectHalfRight = wholePlayAreaRectHalfRight.normal() * (wholePlayAreaRectHalfRight.length() - mc.get_vr_map_margin().x) * (mc.get_vr_horizontal_scaling() * mc.get_vr_scaling());
	}

#ifdef VERBOSE_UPDATE_VR
	output(TXT("[IVR] half play area rect %.3f x %.3f"), playAreaRectHalfRight.length(), playAreaRectHalfForward.length());
#endif

	playAreaZone.clear();
	playAreaZone.add(playAreaRectHalfForward.to_vector2() - playAreaRectHalfRight.to_vector2());
	playAreaZone.add(playAreaRectHalfForward.to_vector2() + playAreaRectHalfRight.to_vector2());
	playAreaZone.add(-playAreaRectHalfForward.to_vector2() + playAreaRectHalfRight.to_vector2());
	playAreaZone.add(-playAreaRectHalfForward.to_vector2() - playAreaRectHalfRight.to_vector2());
	playAreaZone.build();

}

void IVR::output_play_area()
{
	auto const& mc = MainConfig::global();

	output(TXT("[IVR] whole play area [for use] %.3fx%.3f"), wholePlayAreaRectHalfRight.length() * 2.0f, wholePlayAreaRectHalfForward.length() * 2.0f);
	if (!mc.should_be_immobile_vr())
	{
		output(TXT("[IVR] using origin: %S, rotated %.1f'"), mapVRSpaceOrigin.get_translation().to_string().to_char(), mapVRSpaceOrigin.get_orientation().to_rotator().yaw);
	}
	output(TXT("[IVR] from main config:"));
	if (!mc.should_be_immobile_vr())
	{
		output(TXT("[IVR]   margin: %S"), mc.get_vr_map_margin().to_string().to_char());
	}
	output(TXT("[IVR]   horizontal scaling: %.3f"), mc.get_vr_horizontal_scaling());
	output(TXT("[IVR]   scaling: %.3f"), mc.get_vr_scaling());
	output(TXT("[IVR] play area rect %.3f x %.3f"), playAreaRectHalfRight.length() * 2.0f, playAreaRectHalfForward.length() * 2.0f);
	output(TXT("[IVR]   at: %S, rotated %.1f'"), mapVRSpace.get_translation().to_string().to_char(), mapVRSpace.get_orientation().to_rotator().yaw);
}

#ifdef AN_DEVELOPMENT
void IVR::debug_render_whole_play_area_rect(Optional<Colour> const & _colour, Transform const & _placement, bool _drawOffset)
{
#ifdef AN_DEBUG_RENDERER
	// use whole map vr without auto orientation adjustment (auto rotation shouldn't alter translation)
	Transform mapVRSpaceAlt = mapVRSpace;
	todo_note(TXT("this is bad"));
	mapVRSpaceAlt.set_orientation(mapVRSpaceManual.get_orientation());
	debug_push_transform(_placement);
	debug_push_transform(mapVRSpaceAlt);
	debug_draw_line(true, _colour.get(Colour::yellow), Vector3::zero, Vector3::zero + wholePlayAreaRectHalfForward);
	debug_draw_line(true, _colour.get(Colour::yellow), Vector3::zero + wholePlayAreaRectHalfForward - wholePlayAreaRectHalfRight, Vector3::zero + wholePlayAreaRectHalfForward + wholePlayAreaRectHalfRight);
	debug_draw_line(true, _colour.get(Colour::yellow), Vector3::zero - wholePlayAreaRectHalfForward - wholePlayAreaRectHalfRight, Vector3::zero - wholePlayAreaRectHalfForward + wholePlayAreaRectHalfRight);
	debug_draw_line(true, _colour.get(Colour::yellow), Vector3::zero - wholePlayAreaRectHalfForward - wholePlayAreaRectHalfRight, Vector3::zero + wholePlayAreaRectHalfForward - wholePlayAreaRectHalfRight);
	debug_draw_line(true, _colour.get(Colour::yellow), Vector3::zero - wholePlayAreaRectHalfForward + wholePlayAreaRectHalfRight, Vector3::zero + wholePlayAreaRectHalfForward + wholePlayAreaRectHalfRight);
	debug_draw_arrow(true, _colour.get(Colour::yellow), Vector3::zero, -mapVRSpaceAlt.get_translation());
	debug_pop_transform();
	debug_pop_transform();
#endif
}

void IVR::debug_render_play_area_rect(Optional<Colour> const & _colour, Transform const & _placement)
{
#ifdef AN_DEBUG_RENDERER
	debug_push_transform(_placement);
	debug_draw_line(true, _colour.get(Colour::green), Vector3::zero, Vector3::zero + playAreaRectHalfForward);
	debug_draw_line(true, _colour.get(Colour::green), Vector3::zero + playAreaRectHalfForward - playAreaRectHalfRight, Vector3::zero + playAreaRectHalfForward + playAreaRectHalfRight);
	debug_draw_line(true, _colour.get(Colour::green), Vector3::zero - playAreaRectHalfForward - playAreaRectHalfRight, Vector3::zero - playAreaRectHalfForward + playAreaRectHalfRight);
	debug_draw_line(true, _colour.get(Colour::green), Vector3::zero - playAreaRectHalfForward - playAreaRectHalfRight, Vector3::zero + playAreaRectHalfForward - playAreaRectHalfRight);
	debug_draw_line(true, _colour.get(Colour::green), Vector3::zero - playAreaRectHalfForward + playAreaRectHalfRight, Vector3::zero + playAreaRectHalfForward + playAreaRectHalfRight);
	debug_pop_transform();
#endif
}
#endif

void IVR::trigger_pulse(int _handIndex, float _length, float _frequency, Optional<float> const& _frequencyEnd, float _strength, Optional<float> const& _strengthEnd, Optional<float> _fadeOutTime)
{
	if (_handIndex == NONE)
	{
		return;
	}
	an_assert(_handIndex >= 0 && _handIndex <= Hands::Count);
	auto& pulse = pulses[_handIndex];
	if (_strength < pulse.get_current_strength())
	{
		todo_note(TXT("now we just skip it"));
		return;
	}
	pulse.timeLeft = _length;
	pulse.length = _length;
	pulse.fadeOutTime = _fadeOutTime.get(_length);
	pulse.frequency = _frequency;
	pulse.frequencyEnd = _frequencyEnd;
	pulse.strength = _strength;
	pulse.strengthEnd = _strengthEnd;
}

void IVR::trigger_pulse(int _handIndex, Pulse const & _pulse)
{
	trigger_pulse(_handIndex, _pulse.length, _pulse.frequency, _pulse.frequencyEnd, _pulse.strength, _pulse.strengthEnd, _pulse.fadeOutTime);
}

void IVR::rotate_map_vr_space_auto()
{
#ifdef AN_INSPECT_VR_INVALID_ORIGIN
	output(TXT("[vr_invalid_origin] rotate mapVRSpaceAuto"));
#endif

	// turn right!
	mapVRSpaceAuto = mapVRSpaceAuto.to_world(look_matrix(Vector3::zero, Vector3::xAxis, Vector3::zAxis).to_transform());
	update_map_vr_space();
}

void IVR::set_map_vr_space_origin(Transform const& _origin)
{
#ifdef AN_INSPECT_VR_INVALID_ORIGIN
	output(TXT("[vr_invalid_origin] set mapVRSpaceOrigin"));
#endif

	mapVRSpaceOrigin = _origin;
	update_map_vr_space();
}

Transform IVR::internal_calculate_map_vr_space(Vector3 const& _manualOffset, float _manualRotate, OUT_ Transform& _mapVRSpaceManual) const
{
	_mapVRSpaceManual = Transform::identity;
	_mapVRSpaceManual = _mapVRSpaceManual.to_world(Transform(_manualOffset, Quat::identity));
	_mapVRSpaceManual = _mapVRSpaceManual.to_world(Transform(Vector3::zero, Rotator3(0.0f, _manualRotate, 0.0f).to_quat()));

	return _mapVRSpaceManual.to_world(mapVRSpaceOrigin.to_world(mapVRSpaceAuto));
}

Transform IVR::calculate_map_vr_space(Vector3 const& _manualOffset, float _manualRotate) const
{
	Transform temp;
	return internal_calculate_map_vr_space(_manualOffset, _manualRotate, OUT_ temp);
}

void IVR::update_map_vr_space()
{
	auto& mc = MainConfig::global();
	if (mc.should_be_immobile_vr())
	{
		mapVRSpace = Transform::identity;
	}
	else
	{
		mapVRSpace = internal_calculate_map_vr_space( mc.get_vr_map_space_offset(), mc.get_vr_map_space_rotate(), OUT_ mapVRSpaceManual);
#ifdef AN_INSPECT_VR_INVALID_ORIGIN
		output(TXT("[vr_invalid_origin] mapVRSpace = %S %.1f'")
			, mapVRSpace.get_translation().to_string().to_char()
			, mapVRSpace.get_orientation().to_rotator().yaw
		);
		output(TXT("[vr_invalid_origin] mapVRSpaceManual = %S %.1f'")
			, mapVRSpaceManual.get_translation().to_string().to_char()
			, mapVRSpaceManual.get_orientation().to_rotator().yaw
		);
		output(TXT("[vr_invalid_origin] of mc.get_vr_map_space_offset() = %S %.1f'")
			, mc.get_vr_map_space_offset().to_string().to_char()
			, mc.get_vr_map_space_rotate()
		);
		output(TXT("[vr_invalid_origin] of mapVRSpaceOrigin = %S %.1f'")
			, mapVRSpaceOrigin.get_translation().to_string().to_char()
			, mapVRSpaceOrigin.get_orientation().to_rotator().yaw
		);
		output(TXT("[vr_invalid_origin] of mapVRSpaceAuto = %S %.1f'")
			, mapVRSpaceAuto.get_translation().to_string().to_char()
			, mapVRSpaceAuto.get_orientation().to_rotator().yaw
		);
#endif
	}
}

void IVR::offset_additional_offset_to_raw_as_read(Transform const& _newRelativeOrigin)
{
	additionalOffsetToRawAsRead = additionalOffsetToRawAsRead.to_world(_newRelativeOrigin);
}

void IVR::reset_immobile_origin_in_vr_space_if_pending()
{
	if (pendingResetImmobileOriginInVRSpace)
	{
		reset_immobile_origin_in_vr_space();
	}
}

void IVR::reset_immobile_origin_in_vr_space()
{
	pendingResetImmobileOriginInVRSpace = false;
	if (!MainConfig::global().should_be_immobile_vr())
	{
		immobileOriginInVRSpace = Transform::identity;
		output(TXT("[vr] updated origin in vr space to %.3f x %.3f, %.3f"), immobileOriginInVRSpace.get_translation().x, immobileOriginInVRSpace.get_translation().y, immobileOriginInVRSpace.get_orientation().to_rotator().yaw);
	}
	else
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		devInVRSpace = Transform::identity;
#endif			
		Transform viewVR = get_additional_offset_to_raw_as_read().to_world(controlsPoseSet.rawViewAsRead.get(Transform::identity));
		Transform immobileOrigin = look_matrix(viewVR.get_translation() * Vector3::xy, calculate_flat_forward_from(viewVR), Vector3::zAxis).to_transform();
		immobileOriginInVRSpace = immobileOrigin;
		output(TXT("[vr] updated origin in vr space to %.3f x %.3f, %.3f"), immobileOriginInVRSpace.get_translation().x, immobileOriginInVRSpace.get_translation().y, immobileOriginInVRSpace.get_orientation().to_rotator().yaw);

		controlsPoseSet.calculate_auto();

		// make other match this one
		prevControlsPoseSet = controlsPoseSet;
		prevRenderPoseSet = controlsPoseSet;
		renderPoseSet = controlsPoseSet;

		// reset base
		set_controls_base_pose_from_view();
	}
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
Transform const& IVR::get_dev_in_vr_space() const
{
	if (MainConfig::global().should_be_immobile_vr())
	{
		return Transform::identity;
	}
	else
	{
		return devInVRSpace;
	}
}

void IVR::move_relative_dev_in_vr_space(Vector3 const & _moveBy, Rotator3 const & _rotateBy)
{
	if (! MainConfig::global().should_be_immobile_vr() &&
		controlsPoseSet.view.is_set())
	{
		// _poseSet.rawView = owner->get_map_vr_space().to_local(owner->get_dev_in_vr_space().to_world(owner->get_immobile_origin_in_vr_space().to_local(get_transform_from(_trackingState.ViewPose.ThePose))));

		float horizontalScaling = get_scaling().horizontal;
		float scaling = get_scaling().general;

		float useScale = horizontalScaling * scaling;

		Transform viewInMapVR = controlsPoseSet.view.get();
		viewInMapVR.set_translation(viewInMapVR.get_translation() / useScale); // descale as we want to read the actual translation pre scale
		Transform viewDevVR = get_map_vr_space().to_world(viewInMapVR);
		Transform viewPureVR = devInVRSpace.to_local(viewDevVR); // pure - using sitting origin
		Rotator3 viewRot = viewPureVR.get_orientation().to_rotator();
		Rotator3 viewRotFlat = Rotator3(0.0f, viewRot.yaw, 0.0f);

		// we have to accommodate the fact that the view might be off the centre but we want to turn around where view is
		devInVRSpace.set_translation(devInVRSpace.get_translation() + devInVRSpace.vector_to_world((viewRotFlat.to_quat()).to_world(_moveBy)));
		devInVRSpace.set_translation(devInVRSpace.get_translation() + devInVRSpace.vector_to_world(viewPureVR.get_translation()));
		devInVRSpace.set_orientation(devInVRSpace.get_orientation().to_world(_rotateBy.to_quat()));
		devInVRSpace.set_translation(devInVRSpace.get_translation() + devInVRSpace.vector_to_world(-viewPureVR.get_translation()));
	}
}
#endif

void IVR::store_prev_controls_pose_set()
{
	prevControlsPoseSet = controlsPoseSet;
}

void IVR::store_prev_render_pose_set()
{
	prevRenderPoseSet = renderPoseSet;
}

void IVR::advance_turns()
{
	if (!controlsPoseSet.view.is_set())
	{
		return;
	}

	Vector3 currentViewDirVR = controlsPoseSet.view.get().get_axis(Axis::Forward);
	Rotator3 currentViewRotVR = currentViewDirVR.to_rotator();
	if (abs(currentViewRotVR.pitch) < 70.0f) // otherwise it doesn't make much sense
	{
		if (turnCount.lastViewYawVR.is_set())
		{
			turnCount.deg += Rotator3::normalise_axis(currentViewRotVR.yaw - turnCount.lastViewYawVR.get());
			float const treshold = 5.0f;
			while (turnCount.deg > (180.0f + treshold))
			{
				turnCount.deg -= 360.0f;
				++turnCount.count;
			}
			while (turnCount.deg < -(180.0f + treshold))
			{
				turnCount.deg += 360.0f;
				--turnCount.count;
			}
		}
		turnCount.lastViewYawVR = currentViewRotVR.yaw;
	}
}

bool IVR::is_using_hand_tracking(Optional<Hand::Type> const& _hand) const
{
	if (_hand.is_set())
	{
		int handIdx = get_hand(_hand.get());
		if (handIdx != NONE)
		{
			return usingHandTracking[handIdx];
		}
		return false;
	}
	return usingHandTracking[0] || usingHandTracking[1];
}

bool IVR::set_using_hand_tracking(int _handIdx, bool _usingHandTracking)
{
	if (usingHandTracking[_handIdx] == _usingHandTracking)
	{
		return false;
	}
	output(TXT("%S hand tracking for %i (%S) hand"), _usingHandTracking? TXT("using") : TXT("not using"), _handIdx, _handIdx == leftHand ? TXT("left") : TXT("right"));
	usingHandTracking[_handIdx] = _usingHandTracking;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	handsRequireSettingInputTags = true;
#endif
	return true;
}

void IVR::update_input_tags()
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	handsRequireSettingInputTags = false;
#endif
	an_assert(::System::Input::get(), TXT("should be created at this time"));
	if (auto* input = ::System::Input::get())
	{
		for_count(int, iHand, Hand::MAX)
		{
			Hand::Type hand = (Hand::Type)iHand;
			int handIdx = get_hand(hand);
			if (handIdx != NONE && usingHandTracking[handIdx])
			{
				input->set_usage_tag(NAME(handTracking), hand);
				input->remove_usage_tag(NAME(usingControllers), hand);
			}
			else
			{
				input->remove_usage_tag(NAME(handTracking), hand);
				input->set_usage_tag(NAME(usingControllers), hand);
			}
		}
		// general options
		if (usingHandTracking[0] || usingHandTracking[1])
		{
			input->set_usage_tag(NAME(handTracking));
		}
		else
		{
			input->remove_usage_tag(NAME(handTracking));
		}
		if (usingHandTracking[0] && usingHandTracking[1])
		{
			input->remove_usage_tag(NAME(usingControllers));
		}
		else
		{
			input->set_usage_tag(NAME(usingControllers));
		}
		if (MainConfig::global().should_be_immobile_vr())
		{
			input->set_usage_tag(NAME(immobileVR));
		}
		else
		{
			input->remove_usage_tag(NAME(immobileVR));
		}
	}
}

VectorInt2 IVR::get_render_size(Eye::Type _eye)
{
	if (useDirectToVRRendering)
	{
		return get_direct_to_vr_render_size(_eye);
	}
	else if (_eye < Eye::Count)
	{
		if (auto* rt = eyeRenderRenderTargets[_eye].get())
		{
			return rt->get_size();
		}
	}
	return VectorInt2::zero;
}

VectorInt2 IVR::get_render_full_size_for_aspect_ratio_coef(Eye::Type _eye)
{
	return get_full_for_aspect_ratio_coef(get_render_size(_eye), get_render_aspect_ratio_coef(_eye));
}

float IVR::get_render_aspect_ratio_coef(Eye::Type _eye)
{
	if (useDirectToVRRendering)
	{
		return get_direct_to_vr_render_aspect_ratio_coef(_eye);
	}
	else if (_eye < Eye::Count)
	{
		if (auto* rt = eyeRenderRenderTargets[_eye].get())
		{
			return rt->get_setup().get_aspect_ratio_coef();
		}
	}
	return 1.0f;
}

void IVR::set_processing_levels(Optional<float> const& _cpuLevel, Optional<float> const& _gpuLevel, float _temporaryForTime)
{
	set_processing_levels(_cpuLevel, _gpuLevel, true /* temporarily*/);
	resetProcessingLevelsTimeLeft = _temporaryForTime;
}

void IVR::set_processing_levels(Optional<float> const& _cpuLevel, Optional<float> const& _gpuLevel, bool _temporarily)
{
	resetProcessingLevelsTimeLeft.clear();
}

void IVR::act_on_possibly_invalid_boundary()
{
	if (may_have_invalid_boundary())
	{
		if (!rawBoundaryPoints.is_empty())
		{
			Range2 r = Range2::empty;
			for_every(p, rawBoundaryPoints)
			{
				r.include(*p);
			}

			// use fewer points
			Array<Vector2> usePoints;
			Vector2 lastP = rawBoundaryPoints.get_first();
			usePoints.push_back(lastP);
			for_every(p, rawBoundaryPoints)
			{
				if ((*p - lastP).length() >= 0.05f)
				{
					usePoints.push_back(*p);
					lastP = *p;
				}
			}

			Vector2 c = Vector2::zero;
			for_every(p, usePoints)
			{
				c += *p;
			}
			c = c / (float)usePoints.get_size();

			struct TryPlayArea
			{
				float match = -1.0f;
				float yaw = 0;
				Range2 rect; // aligned to yaw

				void setup(float _yaw, Range2 const& _rect)
				{
					yaw = _yaw;
					rect = _rect;
					Vector2 size(rect.x.length(), rect.y.length());
					match = size.x * size.y;
					float preferRegular = 1.0f - (abs(size.x - size.y) / (max(size.x, size.y)));
					preferRegular = remap_value(preferRegular, 0.2f, 0.8f, 0.5f, 1.0f, true);
					match *= preferRegular;
					match = max(match, 0.0f);
				}

				static Optional<Vector2> intersect(Array<Segment2> const& against, Vector2 const& _start, Vector2 const _end)
				{
					Segment2 segment(_start, _end);
					Optional<float> t;
					for_every(s, against)
					{
						Optional<float> st = segment.calc_intersect_with(*s);
						if (st.is_set())
						{
							if (!t.is_set() ||
								t.get() >= st.get())
							{
								t = st;
							}
						}
					}

					if (t.is_set())
					{
						return segment.get_at_t(t.get());
					}

					return NP;
				}

				static bool test(Array<Segment2> const& against, Range2 const& _rect, Optional<VectorInt2> const & _extendingInDir = NP)
				{
					for_count(int, edgeIdx, 6)
					{
						/*
						 *		+-1-+-5-+
						 *		|   |   |
						 *		|   0   |
						 *		|   |   |
						 *		2   +   4
						 *		|       |
						 *		|       |
						 *		|       |
						 *		+---3---+
						 * 
						 * 
						 * 
						 * 
						 */

						Segment2 segment;

						// if we're extending in one dir, ignore directions that should remain the same
						if (_extendingInDir.is_set())
						{
							if (_extendingInDir.get().y > 0)
							{
								if (edgeIdx == 3)
								{
									continue;
								}
							}
							if (_extendingInDir.get().y < 0)
							{
								if (edgeIdx == 0 || edgeIdx == 1 || edgeIdx == 5)
								{
									continue;
								}
							}
							if (_extendingInDir.get().x < 0)
							{
								if (edgeIdx == 0 || edgeIdx == 4 || edgeIdx == 5)
								{
									continue;
								}
							}
							if (_extendingInDir.get().x > 0)
							{
								if (edgeIdx == 0 || edgeIdx == 1 || edgeIdx == 2)
								{
									continue;
								}
							}
						}

						switch (edgeIdx)
						{
						case 0: segment = Segment2(Vector2::zero, Vector2(_rect.x.centre(), _rect.y.max)); break;
						case 1: segment = Segment2(Vector2(_rect.x.centre(), _rect.y.max), Vector2(_rect.x.min, _rect.y.max)); break;
						case 2: segment = Segment2(Vector2(_rect.x.min, _rect.y.max), Vector2(_rect.x.min, _rect.y.min)); break;
						case 3: segment = Segment2(Vector2(_rect.x.min, _rect.y.min), Vector2(_rect.x.max, _rect.y.min)); break;
						case 4: segment = Segment2(Vector2(_rect.x.max, _rect.y.min), Vector2(_rect.x.max, _rect.y.max)); break;
						case 5: segment = Segment2(Vector2(_rect.x.max, _rect.y.max), Vector2(_rect.x.centre(), _rect.y.max)); break;
						default: segment = Segment2(Vector2::zero, Vector2(_rect.x.centre(), _rect.y.max)); break; // repeat 0
						}

						for_every(s, against)
						{
							if (segment.does_intersect_with(*s))
							{
								return false;
							}
						}
					}

					return true;
				}

				static void visualise(Array<Segment2> const& against, Range2 const& _rect, Range const & xRangeAt0, Array<Range> const & yRanges, DebugVisualiser* dv)
				{
					dv->start_gathering_data();
					dv->clear();

					for_every(s, against)
					{
						dv->add_line(s->get_start(), s->get_end(), Colour::blue);
					}

					dv->add_line(Vector2(-100.0f, 0.0f), Vector2(100.0f, 0.0f), Colour::grey);
					dv->add_line(Vector2(0.0f, -100.0f), Vector2(0.0f, 100.0f), Colour::grey);

					if (!yRanges.is_empty())
					{
						float useCellSize = xRangeAt0.length() / (float)yRanges.get_size();

						for_count(int, cellX, yRanges.get_size())
						{
							Range xRange;
							xRange.min = xRangeAt0.min + (float)cellX * useCellSize;
							xRange.max = xRangeAt0.min + (float)(cellX+1) * useCellSize;
							Range2 r;
							r.x = xRange;
							r.y = yRanges[cellX];
							dv->add_range2(r, Colour::c64Orange);
						}
					}

					dv->add_range2(_rect, test(against, _rect)? Colour::green :  Colour::red);

					dv->end_gathering_data();
					dv->show_and_wait_for_key();
				}

				static void visualise(Array<Segment2> const& against, Range2 const& _rect, DebugVisualiser* dv)
				{
					dv->start_gathering_data();
					dv->clear();

					for_every(s, against)
					{
						dv->add_line(s->get_start(), s->get_end(), Colour::blue);
					}

					dv->add_line(Vector2(-100.0f, 0.0f), Vector2(100.0f, 0.0f), Colour::grey);
					dv->add_line(Vector2(0.0f, -100.0f), Vector2(0.0f, 100.0f), Colour::grey);

					dv->add_range2(_rect, test(against, _rect)? Colour::green :  Colour::red);

					dv->end_gathering_data();
					dv->show_and_wait_for_key();
				}
			};

			TryPlayArea best;

			Array<Vector2> rotatedP;
			Array<Segment2> edges;
			rotatedP.make_space_for(usePoints.get_size());
			edges.make_space_for(usePoints.get_size());
			Array<Range> yRanges;

			float yawStep = 5.0f;
			float distStep = 0.01f;
			float maxPlayAreaHalfSize = 5.0f;
			float minPlayAreaHalfSize = 0.1f;
			float cellSize = 0.05f;

#ifdef VISUALISE_PLAY_AREA_FROM_RAW_BOUNDARY_POINTS
			DebugVisualiserPtr dv(new DebugVisualiser(String::printf(TXT("play area setup"))));
			dv->activate();
#endif

			for (float yaw = -45.0f; yaw <= 45.0f; yaw += yawStep)
			{
				rotatedP.clear();
				for_every(p, usePoints)
				{
					rotatedP.push_back((*p - c).rotated_by_angle(-yaw));
				}

				edges.clear();
				Vector2 lastP = rotatedP.get_last();
				for_every(p, rotatedP)
				{
					edges.push_back(Segment2(lastP, *p));
					lastP = *p;
				}

				Range xRangeAt0 = Range(-minPlayAreaHalfSize, minPlayAreaHalfSize);
				yRanges.clear();

				{
					Optional<Vector2> at;
					at = TryPlayArea::intersect(edges, Vector2::zero, Vector2(-maxPlayAreaHalfSize, 0.0f));
					if (at.is_set())
					{
						xRangeAt0.min = at.get().x + 0.01f;
					}
					at = TryPlayArea::intersect(edges, Vector2::zero, Vector2(maxPlayAreaHalfSize, 0.0f));
					if (at.is_set())
					{
						xRangeAt0.max = at.get().x - 0.01f;
					}
				}

				int cellXCount = TypeConversions::Normal::f_i_closest(xRangeAt0.length() / cellSize);
				cellXCount = max(1, cellXCount);

				float useCellSize = xRangeAt0.length() / (float)cellXCount;

				yRanges.clear();
				yRanges.make_space_for(cellXCount);
				for_count(int, cellX, cellXCount)
				{
					Range xRange;
					xRange.min = xRangeAt0.min + (float)cellX * useCellSize;
					xRange.max = xRange.min + useCellSize;

					Range yRange(-minPlayAreaHalfSize, minPlayAreaHalfSize);

					for_count(int, goUp, 2)
					{
						float curDist = 1.0f;
						float curDistStep = curDist;
						while (true)
						{
							Range2 testRect(xRange, goUp? Range(-minPlayAreaHalfSize, curDist) : Range(-curDist, minPlayAreaHalfSize));
							if (TryPlayArea::test(edges, testRect, VectorInt2(0, goUp? 1 : -1)))
							{
								if (goUp)
								{
									yRange.max = max(yRange.max, curDist);
								}
								else
								{
									yRange.min = min(yRange.min, -curDist);
								}
								curDist += curDistStep;
							}
							else
							{
								curDistStep *= 0.5f;
								curDist -= curDistStep;
								if (curDistStep < distStep)
								{
									break;
								}
							}
						}
					}
					yRanges.push_back(yRange);
				}

				for(int leftCellX = 0; leftCellX < cellXCount; ++leftCellX)
				{
					float leftX = xRangeAt0.min + (float)leftCellX * useCellSize;
					for(int rightCellX = cellXCount - 1; rightCellX >= leftCellX; --rightCellX)
					{
						float rightX = xRangeAt0.min + (float)(rightCellX + 1) * useCellSize;
						Range yRange = Range(-10000.0f, 10000.0f);
						Range const* y = &yRanges[leftCellX];
						for (int x = leftCellX; x <= rightCellX; ++x, ++y)
						{
							yRange.max = min(yRange.max, y->max);
							yRange.min = max(yRange.min, y->min);
						}

						Range2 testRect(Range(leftX, rightX), yRange);
						if (TryPlayArea::test(edges, testRect))
						{
							TryPlayArea newCandidate;
							newCandidate.setup(yaw, testRect);
							if (best.match < newCandidate.match)
							{
								best = newCandidate;
#ifdef VISUALISE_PLAY_AREA_FROM_RAW_BOUNDARY_POINTS
								TryPlayArea::visualise(edges, best.rect, xRangeAt0, yRanges, dv.get());
#endif
							}
						}
					}
				}
			}

			if (best.match > 0.0f)
			{
				output(TXT("[IVR} prepared play area from raw boundary points"));
				output(TXT("[IVR} yaw % .3f : rect %S"), best.yaw, best.rect.to_string().to_char());
				// we may avoid reading it again
				if (best.rect.y.length() > best.rect.x.length())
				{
					output(TXT("!@# rotate"));
					// rotate
					best.yaw += 90.0f;
					swap(best.rect.y, best.rect.x);
					// former y, we're now facing right
					swap(best.rect.x.min, best.rect.x.max);
					best.rect.x.min = -best.rect.x.min;
					best.rect.x.max = -best.rect.x.max;
					output(TXT("[IVR} rotated 90 right"));
					output(TXT("[IVR} yaw % .3f : rect %S"), best.yaw, best.rect.to_string().to_char());
				}
				Transform centre(c.to_vector3(), Rotator3(0.0f, best.yaw, 0.0f).to_quat());
				centre.set_translation(centre.location_to_world(Vector3(best.rect.x.centre(), best.rect.y.centre(), 0.0f)));
				set_map_vr_space_origin(centre);
				set_play_area_rect(Vector3::yAxis * (best.rect.y.length() * 0.5f), Vector3::xAxis * (best.rect.x.length() * 0.5f));

				REMOVE_AS_SOON_AS_POSSIBLE_ // this should work, but keep the message for time being
				//invalidBoundaryReplacementAvailable = true;
			}

#ifdef VISUALISE_PLAY_AREA_FROM_RAW_BOUNDARY_POINTS
			dv->deactivate();
#endif
		}
	}
}
