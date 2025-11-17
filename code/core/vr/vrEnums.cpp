#include "vrEnums.h"

#include "..\types\string.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace VR;

//

VRHandBoneIndex::Type VRHandBoneIndex::parse_tip(String const& _fingerName)
{
	if (_fingerName == TXT("thumb")) return ThumbTip;
	if (_fingerName == TXT("pointer")) return PointerTip;
	if (_fingerName == TXT("middle")) return MiddleTip;
	if (_fingerName == TXT("ring")) return RingTip;
	if (_fingerName == TXT("pinky")) return PinkyTip;

	return Wrist;
}

VRHandBoneIndex::Type VRHandBoneIndex::parse_base(String const& _fingerName)
{
	if (_fingerName == TXT("thumb")) return ThumbBase;
	if (_fingerName == TXT("pointer")) return PointerBase;
	if (_fingerName == TXT("middle")) return MiddleBase;
	if (_fingerName == TXT("ring")) return RingBase;
	if (_fingerName == TXT("pinky")) return PinkyBase;

	return Wrist;
}

//

