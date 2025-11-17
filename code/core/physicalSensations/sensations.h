#pragma once

#include "..\math\math.h"
#include "..\types\hand.h"
#include "..\types\optional.h"

namespace PhysicalSensations
{
	struct Sensation
	{
		typedef int ID;

		ID id = NONE; 

		Optional<Vector3> dirOS; // dir at which the hit came 0,-1,0 means from front going backward
		Optional<Vector3> hitLocOS;
		Optional<float> eyeHeightOS;

		Optional<Hand::Type> hand;
		Optional<bool> head;

		int implInt[8]; // implementation related

		Sensation& with_dir_os(Vector3 const& _dirOS) { dirOS = _dirOS; return *this; }
		Sensation& with_hit_loc_os(Vector3 const& _hitLocOS) { hitLocOS = _hitLocOS; return *this; }
		Sensation& with_eye_height_os(float _eyeHeightOS) { eyeHeightOS = _eyeHeightOS; return *this; }
		Sensation& for_head(bool _head = true) { head = _head; return *this; }
		Sensation& for_hand(Hand::Type _hand) { hand = _hand; return *this; }
		Sensation& for_left_hand() { return for_hand(Hand::Left); }
		Sensation& for_right_hand() { return for_hand(Hand::Right); }
	};

	struct SingleSensation
	: public Sensation
	{
		enum Type
		{
			Unknown,
			// attacked
			ShotLight,
			Shot,
			ShotHeavy,
			ElectrocutionLight,
			Electrocution,
			SlashedLight,
			Slashed,
			Punched,
			GrabbedSomething,
			UsedSomething,
			ReleasedSomething,
			EnergyTaken,
			ExplosionLight,
			Explosion,
			// action
			RecoilWeak,
			RecoilMedium,
			RecoilStrong,
			RecoilVeryStrong,
			// other
			Tingling,
			PowerUp,

			MAX
		} type = Unknown;

		SingleSensation(Type _type) : type(_type) {}

		static tchar const* to_char(Type _type);
	};

	struct OngoingSensation
	: public Sensation
	{
		enum Type
		{
			Unknown,
			//
			LiftObject,
			LiftHeavyObject,
			PushObject,
			PushHeavyObject,
			Oppression,
			Wind,
			Falling,
			UpgradeReceived,
			SlowSpeed,
			EnergyCharging,

			MAX
		} type = Unknown;

		Optional<float> timeLeft;
		bool sustainable = true;

		OngoingSensation(Type _type, Optional<float> _duration = NP, bool _sustainable = true) : type(_type), timeLeft(_duration), sustainable(_sustainable) {}

		static tchar const* to_char(Type _type);
	};
};
