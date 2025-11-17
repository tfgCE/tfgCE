#include "ac_provideFingerTipPlacementMS.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\wheresMyPoint\iWMPTool.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\input.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

#ifdef AN_DEVELOPMENT
#define AN_DEBUG_DRAW_FINGER
#define AN_DEBUG_DRAW_FINGER_CONTROLS
#endif

//

DEFINE_STATIC_NAME(provideFingerTipPlacementMS);

//

REGISTER_FOR_FAST_CAST(ProvideFingerTipPlacementMS);

ProvideFingerTipPlacementMS::ProvideFingerTipPlacementMS(ProvideFingerTipPlacementMSData const * _data)
: base(_data)
, provideFingerTipPlacementMSData(_data)
{
	resultPlacementMSVar = provideFingerTipPlacementMSData->resultPlacementMSVar.get();

	Name handName = provideFingerTipPlacementMSData->hand.get(Name::invalid());
	Name fingerName = provideFingerTipPlacementMSData->finger.get(Name::invalid());

	hand = Hand::parse(handName.to_string());
	finger = VR::VRFinger::parse(fingerName.to_string());
	fingerTipVRHandBone = VR::VRHandBoneIndex::parse_tip(fingerName.to_string());
	fingerBaseVRHandBone = VR::VRHandBoneIndex::parse_base(fingerName.to_string());
	baseBone = provideFingerTipPlacementMSData->baseBone.get(Name::invalid());
	tipBone = provideFingerTipPlacementMSData->tipBone.get(Name::invalid());
	offsetLocationOS = provideFingerTipPlacementMSData->offsetLocationOS.get(Vector3::zero);
}

ProvideFingerTipPlacementMS::~ProvideFingerTipPlacementMS()
{
}

void ProvideFingerTipPlacementMS::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& varStorage = get_owner()->access_controllers_variable_storage();
	resultPlacementMSVar.look_up<Transform>(varStorage);

	if (Meshes::Skeleton const* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		baseBone.look_up(skeleton);
		tipBone.look_up(skeleton);
		if (baseBone.is_valid() && tipBone.is_valid())
		{
			meshBaseToTipLength = (skeleton->get_bones_default_placement_MS()[baseBone.get_index()].get_translation() - skeleton->get_bones_default_placement_MS()[tipBone.get_index()].get_translation()).length();
		}
		else
		{
			meshBaseToTipLength = 0.0f;
		}
	}

	update_ref();
}

void ProvideFingerTipPlacementMS::update_ref()
{
	if (vrRefBaseToTip.is_zero())
	{
		vrRefBaseToTip = Vector3::zero;
		vrRefBaseToTipLength = 0.0f;
		if (auto* vr = VR::IVR::get())
		{
			if (vr->is_using_hand_tracking(hand))
			{
				Transform placementRefOS = vr->get_reference_pose_set().get_hand(hand).bonesPS[fingerTipVRHandBone].get(Transform::identity);
				Transform placementRefBaseOS = vr->get_reference_pose_set().get_hand(hand).bonesPS[fingerBaseVRHandBone].get(Transform::identity);
				vrRefBaseToTip = (placementRefOS.get_translation() - placementRefBaseOS.get_translation());
				vrRefBaseToTipLength = vrRefBaseToTip.length();
			}
		}
	}
}

void ProvideFingerTipPlacementMS::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ProvideFingerTipPlacementMS::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(provideFingerTipPlacementMS_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	auto* vr = VR::IVR::get();
	if (!mesh || !vr || !vr->is_using_hand_tracking(hand))
	{
		return;
	}

	update_ref();

	todo_hack(TXT("get directly from VR, we already access VR at many times, so why bother here with controler/presence chain"));

	Meshes::Pose* poseLS = _context.access_pose_LS();

	Transform placementOS = vr->get_render_pose_set().get_hand(hand).bonesPS[fingerTipVRHandBone].get(Transform::identity);
	Transform vrPlacementBaseOS = vr->get_render_pose_set().get_hand(hand).bonesPS[fingerBaseVRHandBone].get(Transform::identity);
	Transform placementBaseOS = get_owner()->get_ms_to_os_transform().to_world(poseLS->get_bone_ms_from_ls(baseBone.get_index()));
	placementBaseOS.set_translation(placementBaseOS.get_translation() - offsetLocationOS);

	float straighten = 0.0f;
	if (finger == VR::VRFinger::Thumb)
	{
		float straightRefAligned = vr->get_render_pose_set().get_hand(hand).fingers[finger].straightRefAligned;
		straighten = clamp(Range(0.8f, 0.95f).get_pt_from_value(straightRefAligned), 0.0f, 1.0f);
	}
	else
	{
		float straight = vr->get_render_pose_set().get_hand(hand).fingers[finger].straightLength;
		straighten = clamp(Range(0.7f, 0.92f).get_pt_from_value(straight), 0.0f, 1.0f);
	}

#ifdef AN_DEBUG_DRAW_FINGER
	float debY = 0.0f;
#else
#ifdef AN_DEBUG_DRAW_FINGER_CONTROLS
	float debY = 0.0f;
#endif
#endif

#ifdef AN_DEBUG_DRAW_FINGER
	if (vr)
	{
		Transform h = vr->get_render_pose_set().hands[vr->get_hand(hand)].placement.get(Transform::identity);
		Transform f = h.to_world(vr->get_render_pose_set().get_hand(hand).bonesPS[fingerTipVRHandBone].get(Transform::identity));
		debug_draw_transform_size(true, f, 0.01f);
		debug_draw_text(true, Colour::white, f.get_translation(), Vector2(0.5f, debY), true, 0.1f, NP, TXT("sl:%.2f"), vr->get_render_pose_set().get_hand(hand).fingers[finger].straightLength);
		debY += 1.0f;
		debug_draw_text(true, Colour::white, f.get_translation(), Vector2(0.5f, debY), true, 0.1f, NP, TXT("sra:%.2f"), vr->get_render_pose_set().get_hand(hand).fingers[finger].straightRefAligned);
		debY += 1.0f;
		debug_draw_text(true, Colour::white, f.get_translation(), Vector2(0.5f, debY), true, 0.1f, NP, TXT("s:%.2f"), straighten);
		debY += 1.0f;
	}
#endif
#ifdef AN_DEBUG_DRAW_FINGER_CONTROLS
	if (vr)
	{
		Transform h = vr->get_render_pose_set().hands[vr->get_hand(hand)].placement.get(Transform::identity);
		Transform f = h.to_world(vr->get_render_pose_set().get_hand(hand).bonesPS[fingerTipVRHandBone].get(Transform::identity));
		auto const & vrHand = vr->get_controls().hands[vr->get_hand(hand)];
		auto const& buttons = vrHand.buttons;
		bool folded = false;
		bool straight = false;
		if (finger == VR::VRFinger::Thumb) { folded = buttons.thumbFolded; straight = buttons.thumbStraight; }
		if (finger == VR::VRFinger::Pointer) { folded = buttons.pointerFolded; straight = buttons.pointerStraight; }
		if (finger == VR::VRFinger::Middle) { folded = buttons.middleFolded; straight = buttons.middleStraight; }
		if (finger == VR::VRFinger::Ring) { folded = buttons.ringFolded; straight = buttons.ringStraight; }
		if (finger == VR::VRFinger::Pinky) { folded = buttons.pinkyFolded; straight = buttons.pinkyStraight; }
		float pinch = 0.0f;
		if (finger == VR::VRFinger::Pointer) { pinch = vrHand.pointerPinch.get(0.0f); }
		if (finger == VR::VRFinger::Middle) { pinch = vrHand.middlePinch.get(0.0f); }
		if (finger == VR::VRFinger::Ring) { pinch = vrHand.ringPinch.get(0.0f); }
		if (finger == VR::VRFinger::Pinky) { pinch = vrHand.pinkyPinch.get(0.0f); }
 		debug_draw_text(true, Colour::cyan, f.get_translation(), Vector2(0.5f, debY), true, 0.1f, NP, TXT("%c"), folded ? 'F' : (straight ? 'S' : '-'));
		debY += 1.0f;
 		debug_draw_text(true, Colour::green, f.get_translation(), Vector2(0.5f, debY), true, 0.1f, NP, TXT("%.2f"), pinch);
		debY += 1.0f;
		if (finger == VR::VRFinger::Middle)
		{
			debug_draw_text(true, Colour::red, f.get_translation(), Vector2(0.5f, debY), true, 0.1f, NP, TXT("ih:%.2f"), vrHand.insideToHead.get(0.0f));
			debY += 1.0f;
			debug_draw_text(true, Colour::red, f.get_translation(), Vector2(0.5f, debY), true, 0.1f, NP, TXT("ud:%.2f"), vrHand.upsideDown.get(0.0f));
			debY += 1.0f;
		}
}
#endif

	// make it match length of the fingers we have in our actual model
	if (straighten > 0.0f && vrRefBaseToTipLength != 0.0f && meshBaseToTipLength != 0.0f)
	{
		Vector3 t = placementOS.get_translation();
		Vector3 b = placementBaseOS.get_translation();
		Vector3 vb = vrPlacementBaseOS.get_translation();
#ifdef AN_DEBUG_DRAW_FINGER
		Transform prevPlacementOS = placementOS;
		if (vr)
		{
			Transform h = vr->get_render_pose_set().hands[vr->get_hand(hand)].placement.get(Transform::identity);
			debug_draw_transform_size_coloured(true, h.to_world(placementOS), 0.01f, Colour::blue, Colour::blue, Colour::blue);
			debug_draw_line(true, Colour::blue, h.to_world(vrPlacementBaseOS).get_translation(), h.to_world(placementOS).get_translation());
		}
#endif
		placementOS.set_translation(
			(1.0f - straighten) * (t) +
					 straighten * (b + (t - vb) * (meshBaseToTipLength / vrRefBaseToTipLength))); // force straight

#ifdef AN_DEBUG_DRAW_FINGER
		if (vr)
		{
			Transform h = vr->get_render_pose_set().hands[vr->get_hand(hand)].placement.get(Transform::identity);
			Transform f = h.to_world(placementOS);
			debug_draw_arrow(true, Colour::cyan, h.to_world(prevPlacementOS).get_translation(), h.to_world(placementOS).get_translation());
			debug_draw_transform_size_coloured(true, h.to_world(placementBaseOS), 0.01f, Colour::orange, Colour::orange, Colour::orange);
			debug_draw_transform_size_coloured(true, h.to_world(vrPlacementBaseOS), 0.01f, Colour::yellow, Colour::yellow, Colour::yellow);
			debug_draw_line(true, Colour::green, h.to_world(placementBaseOS).get_translation(), h.to_world(placementOS).get_translation());
			debug_draw_transform_size_coloured(true, h.to_world(placementOS), 0.01f, Colour::green, Colour::green, Colour::green);
			debug_draw_text(true, Colour::grey, f.get_translation(), Vector2(0.5, 2.0f), true, 0.1f, NP, TXT("m%.3f"), meshBaseToTipLength);
			debug_draw_text(true, Colour::grey, f.get_translation(), Vector2(0.5, 3.0f), true, 0.1f, NP, TXT("v%.3f"), vrRefBaseToTipLength);
			debug_draw_text(true, Colour::grey, f.get_translation(), Vector2(0.5, 4.0f), true, 0.1f, NP, TXT("c%.3f"), (placementOS.get_translation() - b).length());
		}
#endif
	}

	placementOS.set_translation(placementOS.get_translation() + offsetLocationOS);
	Transform placementMS = get_owner()->get_ms_to_os_transform().to_local(placementOS);

	if (get_active() == 1.0f)
	{
		resultPlacementMSVar.access<Transform>() = placementMS;
	}
	else
	{
		resultPlacementMSVar.access<Transform>() = Transform::lerp(get_active(), resultPlacementMSVar.get<Transform>(), placementMS);
	}
}

void ProvideFingerTipPlacementMS::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(ProvideFingerTipPlacementMSData);

AppearanceControllerData* ProvideFingerTipPlacementMSData::create_data()
{
	return new ProvideFingerTipPlacementMSData();
}

void ProvideFingerTipPlacementMSData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(provideFingerTipPlacementMS), create_data);
}

ProvideFingerTipPlacementMSData::ProvideFingerTipPlacementMSData()
{
}

bool ProvideFingerTipPlacementMSData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= resultPlacementMSVar.load_from_xml(_node, TXT("resultPlacementMSVarID"));

	result &= hand.load_from_xml(_node, TXT("hand"));
	result &= finger.load_from_xml(_node, TXT("finger"));
	
	result &= baseBone.load_from_xml(_node, TXT("baseBone"));
	result &= tipBone.load_from_xml(_node, TXT("tipBone"));

	result &= offsetLocationOS.load_from_xml(_node, TXT("offsetLocationOS"));

	return result;
}

bool ProvideFingerTipPlacementMSData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ProvideFingerTipPlacementMSData::create_copy() const
{
	ProvideFingerTipPlacementMSData* copy = new ProvideFingerTipPlacementMSData();
	*copy = *this;
	return copy;
}

AppearanceController* ProvideFingerTipPlacementMSData::create_controller()
{
	return new ProvideFingerTipPlacementMS(this);
}

void ProvideFingerTipPlacementMSData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	resultPlacementMSVar.fill_value_with(_context);
	hand.fill_value_with(_context);
	finger.fill_value_with(_context);
	baseBone.fill_value_with(_context);
	tipBone.fill_value_with(_context);
	offsetLocationOS.fill_value_with(_context);
}
