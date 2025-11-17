#include "psOWO.h"

#include "..\mainConfig.h"

#include "..\splash\splashLogos.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
#define OWO_OUTPUT
#endif

//

using namespace PhysicalSensations;

//

// physical sensation system id
DEFINE_STATIC_NAME(owo);

//

enum OWOSensationID
{
	// based on official document
	// https://docs.google.com/document/d/1NOCX77HHMmR00X4oAhb3PN6dBTypEb9IM0PoVVomqkY/edit

	UnknownSensation	= -2,
	Stop				= -1 /* stop sensation currently playing */,

	Shot				= 4 /* shot with exit */,
	ShotLight			= 9 /* quick shot */,
	Cut					= 5 /* razor blade */,
	Stabbed				= 6 /* hack */,
	Punched				= 7 /* punch */,
	Explosion			= 20 /* severe abdominal wound */,
	Recoil				= 0 /* ball game */,
	Insects				= 10 /* spiders - tickling */,
	
	LiftObject			= 13 /* load heavy object */,
	LiftHeavyObject		= 12 /* load very heavy object */,
	PushObject			= 19 /* pushing heavy object */,
	PushHeavyObject		= 18 /* pushing very heavy object */,
	Oppression			= 22 /* oppression */,
	Wind				= 11 /* freefall */,
	Falling				= 11 /* freefall */,
	StrangeSensation	= 23 /* strange presence (dorsal) */,
	SlowSpeed			= 15 /* drive slow speed vibration */,

	// special
	NotUsedSensation	= UnknownSensation
};

enum OWOMuscleID
{
	// enum should grant numbers but we should explicitly define ids to be consistent with official document
	// https://docs.google.com/document/d/1NOCX77HHMmR00X4oAhb3PN6dBTypEb9IM0PoVVomqkY/edit

	UnknownMuscle	= -2,

	FirstMuscle		= 0,
	//

	// front up
	PectoralRight	= 0,
	PectoralLeft	= 1,
	// front down
	AbdominalRight	= 2,
	AbdominalLeft	= 3,
	// side
	ArmRight		= 4,
	Armleft			= 5,
	// back up
	DorsalRight		= 6,
	DorsalLeft		= 7,
	// back down
	LumbarRight		= 8,
	LumbarLeft		= 9,

	//
	LastMuscle		= 9,

};

int owo_get_sensation_priority(OWOSensationID sId)
{
	switch (sId)
	{
	case Explosion:			return 100 + 70;
	case Shot:				return 100 + 50;
	case ShotLight:			return 100 + 45;
	case Stabbed:			return 100 + 41;
	case Cut:				return 100 + 40;
	case Punched:			return 100 + 30;
	case Recoil:			return 100 + 20;
	case Insects:			return 100 + 10;

	case LiftHeavyObject:	return 11;
	case LiftObject:		return 10;
	case PushHeavyObject:	return 6;
	case PushObject:		return 5;
	case Oppression:		return 4;
	case Wind:				return 3; // or Falling
	case StrangeSensation:	return 2;
	case SlowSpeed:			return 1;

	default:				return 0;
	}
}

bool owo_is_right_muscle(OWOMuscleID mId)
{
	return mId == PectoralRight
		|| mId == AbdominalRight
		|| mId == ArmRight
		|| mId == DorsalRight
		|| mId == LumbarRight;
}

bool owo_is_left_muscle(OWOMuscleID mId)
{
	return ! owo_is_right_muscle(mId);
}

OWOMuscleID owo_fix_muscle_for_sensation(OWOMuscleID mId, OWOSensationID sId)
{
	if (sId == Explosion)
	{
		return owo_is_right_muscle(mId) ? AbdominalRight : AbdominalLeft;
	}
	if (sId == StrangeSensation)
	{
		return owo_is_right_muscle(mId) ? DorsalRight : DorsalLeft;
	}

	return mId;
}

#define MATCH_SINGLE_SENSATION_TYPE(TYPE) if (_type == SingleSensation::TYPE) return OWOSensationID::TYPE;
#define MATCH_SINGLE_SENSATION_TYPE_EX(TYPE, RETURN) if (_type == SingleSensation::TYPE) return OWOSensationID::RETURN;
OWOSensationID owo_get_sensation_id_for(SingleSensation::Type _type)
{
	MATCH_SINGLE_SENSATION_TYPE_EX(Unknown, UnknownSensation);
	MATCH_SINGLE_SENSATION_TYPE(ShotLight);
	MATCH_SINGLE_SENSATION_TYPE(Shot);
	MATCH_SINGLE_SENSATION_TYPE_EX(ShotHeavy, Shot);
	MATCH_SINGLE_SENSATION_TYPE_EX(ElectrocutionLight, Cut);
	MATCH_SINGLE_SENSATION_TYPE_EX(Electrocution, Cut);
	MATCH_SINGLE_SENSATION_TYPE_EX(SlashedLight, Stabbed);
	MATCH_SINGLE_SENSATION_TYPE_EX(Slashed, Stabbed);
	MATCH_SINGLE_SENSATION_TYPE(Punched);
	MATCH_SINGLE_SENSATION_TYPE_EX(GrabbedSomething, NotUsedSensation);
	MATCH_SINGLE_SENSATION_TYPE_EX(UsedSomething, Insects);
	MATCH_SINGLE_SENSATION_TYPE_EX(ReleasedSomething, NotUsedSensation);
	MATCH_SINGLE_SENSATION_TYPE_EX(EnergyTaken, NotUsedSensation);
	MATCH_SINGLE_SENSATION_TYPE_EX(ExplosionLight, Explosion);
	MATCH_SINGLE_SENSATION_TYPE(Explosion);
	MATCH_SINGLE_SENSATION_TYPE_EX(RecoilWeak, Recoil);
	MATCH_SINGLE_SENSATION_TYPE_EX(RecoilMedium, Recoil);
	MATCH_SINGLE_SENSATION_TYPE_EX(RecoilStrong, Recoil);
	MATCH_SINGLE_SENSATION_TYPE_EX(RecoilVeryStrong, Recoil);
	MATCH_SINGLE_SENSATION_TYPE_EX(Tingling, Insects);
	//MATCH_SINGLE_SENSATION_TYPE_EX(PowerUp, Insects);

	todo_implement(TXT("implement"));
	return OWOSensationID::UnknownSensation;
}

#define MATCH_ONGOING_SENSATION_TYPE(TYPE) if (_type == OngoingSensation::TYPE) return OWOSensationID::TYPE;
#define MATCH_ONGOING_SENSATION_TYPE_EX(TYPE, RETURN) if (_type == OngoingSensation::TYPE) return OWOSensationID::RETURN;
OWOSensationID owo_get_sensation_id_for(OngoingSensation::Type _type)
{
	MATCH_ONGOING_SENSATION_TYPE_EX(Unknown, UnknownSensation);
	MATCH_ONGOING_SENSATION_TYPE(LiftObject);
	MATCH_ONGOING_SENSATION_TYPE(LiftHeavyObject);
	MATCH_ONGOING_SENSATION_TYPE(PushObject);
	MATCH_ONGOING_SENSATION_TYPE(PushHeavyObject);
	MATCH_ONGOING_SENSATION_TYPE(Oppression);
	MATCH_ONGOING_SENSATION_TYPE(Wind);
	MATCH_ONGOING_SENSATION_TYPE(Falling);
	MATCH_ONGOING_SENSATION_TYPE_EX(UpgradeReceived, StrangeSensation);
	MATCH_ONGOING_SENSATION_TYPE(SlowSpeed);
	MATCH_ONGOING_SENSATION_TYPE_EX(EnergyCharging, NotUsedSensation);

	todo_implement(TXT("implement"));
	return OWOSensationID::UnknownSensation;
}

OWOMuscleID owo_get_muscle_id_for(Sensation const & _sensation)
{
	if (_sensation.hand.is_set())
	{
		if (_sensation.hand.get() == Hand::Left) return OWOMuscleID::Armleft;
		if (_sensation.hand.get() == Hand::Right) return OWOMuscleID::ArmRight;
	}

	bool up = false;
	bool front = true;
	bool right = false;
	if (_sensation.hitLocOS.is_set())
	{
		up = _sensation.hitLocOS.get().z >= 0.9f;
		if (_sensation.eyeHeightOS.is_set())
		{
			up = _sensation.hitLocOS.get().z >= _sensation.eyeHeightOS.get() * 0.6f;
		}
		front = _sensation.hitLocOS.get().y >= 0.0f;
		right = _sensation.hitLocOS.get().x >= 0.0f;
	}

	if (front)
	{
		if (right)
		{
			return up ? PectoralRight : AbdominalRight;
		}
		else
		{
			return up ? PectoralLeft : AbdominalLeft;
		}
	}
	else
	{
		if (right)
		{
			return up ? DorsalRight : LumbarRight;
		}
		else
		{
			return up ? DorsalLeft : LumbarLeft;
		}
	}
}

void owo_request_sensation(Network::ClientTCP& client, int _owoSensationId, int _owoMuscleId)
{
	if (_owoSensationId != UnknownSensation &&
		_owoMuscleId != UnknownMuscle)
	{
		char sensationRequest[256];
#ifdef AN_CLANG
		sprintf
#else
		sprintf_s
#endif
			(sensationRequest, "owo/%i/%i/eof", _owoSensationId, _owoMuscleId);
		client.send_raw_8bit_text(sensationRequest, false);
	}
}

//

REGISTER_FOR_FAST_CAST(OWO);

void OWO::splash_logo()
{
	Splash::Logos::add(TXT("owo"), SPLASH_LOGO_DEVICE);
}

Name const& OWO::system_id()
{
	return NAME(owo);
}

void OWO::start()
{
	if (!MainConfig::global().get_owo_address().is_empty() && MainConfig::global().get_owo_port() != 0)
	{
		Network::Address address(MainConfig::global().get_owo_address(), MainConfig::global().get_owo_port());

		if (address.is_ok())
		{
			RefCountObjectPtr<IPhysicalSensations> keepCurrent;
			keepCurrent = IPhysicalSensations::get();
		
			bool startAndConnect = true;
			
			if (auto* owo = fast_cast<OWO>(keepCurrent.get()))
			{
				if (owo->is_ok() &&
					owo->address == address)
				{
					startAndConnect = false;
				}
			}

			if (startAndConnect)
			{
				terminate();

				new OWO(address);
			}
		}
	}
}

OWO::OWO(Network::Address const& _address)
: address(_address)
, client(_address)
{
}

OWO::~OWO()
{
	isOk = false;
	client.close();
}

enum ImplIds
{
	ImplSensationActive,
	ImplSensationId,
	ImplMuscleId,
};

void OWO::async_init()
{
	if (initialising.acquire_if_not_locked())
	{
		isConnecting = true;
		client.connect();
		isConnecting = false;

		isOk = !client.has_connection_failed();
	}
}

void OWO::internal_start_sensation(REF_ SingleSensation & _sensation)
{
	_sensation.implInt[ImplSensationId] = owo_get_sensation_id_for(_sensation.type);
	_sensation.implInt[ImplMuscleId] = owo_get_muscle_id_for(_sensation);

	_sensation.implInt[ImplSensationActive] = true;

	owo_request_sensation(client, _sensation.implInt[ImplSensationId], _sensation.implInt[ImplMuscleId]);
}

void OWO::internal_start_sensation(REF_ OngoingSensation & _sensation)
{
	_sensation.implInt[ImplSensationId] = owo_get_sensation_id_for(_sensation.type);
	_sensation.implInt[ImplMuscleId] = owo_get_muscle_id_for(_sensation);

	_sensation.implInt[ImplSensationActive] = true;

	owo_request_sensation(client, _sensation.implInt[ImplSensationId], _sensation.implInt[ImplMuscleId]);
}

void OWO::internal_stop_sensation(Sensation::ID _id)
{
	for_every(s, ongoingSensations)
	{
		if (s->id == _id)
		{
			owo_request_sensation(client, OWOSensationID::Stop, s->implInt[ImplMuscleId]);
		}
	}
}

bool OWO::internal_stop_all_sensations()
{
	for_range(int, muscleId, OWOMuscleID::FirstMuscle, OWOMuscleID::LastMuscle)
	{
		owo_request_sensation(client, OWOSensationID::Stop, muscleId);
	}

	return true;
}
