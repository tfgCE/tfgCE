#pragma once

#define ONE_RENDER_MODE

struct String;

namespace VR
{
	namespace Eye
	{
		enum Type
		{
			Left = 0,
			Right = 1,

			Count,

			OnMain = Right // consistent with vr systems
		};
	};


	namespace Hands
	{
		enum Type
		{
			Count = 2
		};
	};

	namespace VRHandBoneIndex
	{
		enum Type
		{
			Wrist,
			// base - where finger starts
			ThumbBase,
			PointerBase, // index
			MiddleBase,
			RingBase,
			PinkyBase,
			// middle part
			ThumbMid,
			PointerMid, // index
			MiddleMid,
			RingMid,
			PinkyMid,
			// tip - where finger ends (it should have the same orientation as a finger before it
			ThumbTip,
			PointerTip, // index
			MiddleTip,
			RingTip,
			PinkyTip,

			MAX,
			None = -1
		};

		Type parse_tip(String const& _fingerName);
		Type parse_base(String const& _fingerName);
	};

	namespace DecideHandsResult
	{
		enum Type
		{
			CantTellAtAll,
			UnableToDecide,
			PartiallyDecided,
			Decided,
		};
	};

	namespace RenderMode
	{
		// general abstract idea
		enum Type
		{
			Normal,
			HiRes,
		};
	};

	namespace InternalRenderMode
	{
		// modes are implemented now only for quest
		enum Type
		{
#ifdef ONE_RENDER_MODE
			Normal,
			HiRes = Normal, // menu etc
			
			COUNT = 1
#else
			Normal,
			HiRes, // menu etc
			
			COUNT
#endif
		};
	};

};
