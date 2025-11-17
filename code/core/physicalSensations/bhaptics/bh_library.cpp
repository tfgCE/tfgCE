#include "bh_library.h"

#include "..\bhapticsJava\bhj_bhapticsModule.h"
#include "..\bhapticsWindows\bhw_bhapticsModule.h"

#include "..\..\concurrency\scopedSpinLock.h"

#include "..\..\io\json.h"
#include "..\..\io\xml.h"

//

using namespace an_bhaptics;

//

// single sensation ids B[ack] L[eft] R[ight]
DEFINE_STATIC_NAME(ExplosionLight);
DEFINE_STATIC_NAME(Explosion);
DEFINE_STATIC_NAME(ExplosionLightB);
DEFINE_STATIC_NAME(ExplosionB);
DEFINE_STATIC_NAME(HeadExplosionLight);
DEFINE_STATIC_NAME(HeadExplosion);
DEFINE_STATIC_NAME(ElectrocutionLight);
DEFINE_STATIC_NAME(Electrocution);
DEFINE_STATIC_NAME(ElectrocutionLightB);
DEFINE_STATIC_NAME(ElectrocutionB);
DEFINE_STATIC_NAME(HeadElectrocutionLight);
DEFINE_STATIC_NAME(HeadElectrocution);
DEFINE_STATIC_NAME(PowerUp);
DEFINE_STATIC_NAME(PowerUpB);
DEFINE_STATIC_NAME(ShotLight);
DEFINE_STATIC_NAME(Shot);
DEFINE_STATIC_NAME(ShotHeavy);
DEFINE_STATIC_NAME(ShotLightB);
DEFINE_STATIC_NAME(ShotB);
DEFINE_STATIC_NAME(ShotHeavyB);
DEFINE_STATIC_NAME(HeadShotLight);
DEFINE_STATIC_NAME(HeadShot);
DEFINE_STATIC_NAME(HeadShotHeavy);
DEFINE_STATIC_NAME(RecoilWeakL);
DEFINE_STATIC_NAME(RecoilWeakR);
DEFINE_STATIC_NAME(RecoilMediumL);
DEFINE_STATIC_NAME(RecoilMediumR);
DEFINE_STATIC_NAME(RecoilStrongL);
DEFINE_STATIC_NAME(RecoilStrongR);
DEFINE_STATIC_NAME(RecoilVeryStrongL);
DEFINE_STATIC_NAME(RecoilVeryStrongR);
DEFINE_STATIC_NAME(VestRecoilWeakL);
DEFINE_STATIC_NAME(VestRecoilWeakR);
DEFINE_STATIC_NAME(VestRecoilMediumL);
DEFINE_STATIC_NAME(VestRecoilMediumR);
DEFINE_STATIC_NAME(VestRecoilStrongL);
DEFINE_STATIC_NAME(VestRecoilStrongR);
DEFINE_STATIC_NAME(VestRecoilVeryStrongL);
DEFINE_STATIC_NAME(VestRecoilVeryStrongR);
DEFINE_STATIC_NAME(GrabL);
DEFINE_STATIC_NAME(GrabR);
DEFINE_STATIC_NAME(UseL);
DEFINE_STATIC_NAME(UseR);
DEFINE_STATIC_NAME(ReleaseL);
DEFINE_STATIC_NAME(ReleaseR);
DEFINE_STATIC_NAME(VestGrabL);
DEFINE_STATIC_NAME(VestGrabR);
DEFINE_STATIC_NAME(VestUseL);
DEFINE_STATIC_NAME(VestUseR);
DEFINE_STATIC_NAME(VestReleaseL);
DEFINE_STATIC_NAME(VestReleaseR);
DEFINE_STATIC_NAME(EnergyTakenL);
DEFINE_STATIC_NAME(EnergyTakenR);

// ongoing sensations ids
DEFINE_STATIC_NAME(UpgradeReceived);
DEFINE_STATIC_NAME(ElevatorRide);
DEFINE_STATIC_NAME(ChargingL);
DEFINE_STATIC_NAME(ChargingR);
DEFINE_STATIC_NAME(PullPushL);
DEFINE_STATIC_NAME(PullPushR);
DEFINE_STATIC_NAME(WindBlow);


//

Library* Library::s_library = nullptr;

void Library::initialise_static()
{
	an_assert(s_library == nullptr);
	s_library = new Library();
}

void Library::close_static()
{
	an_assert(s_library);
	delete_and_clear(s_library);
}

void Library::register_for(bhapticsJava::BhapticsModule* bhm)
{
	an_assert(s_library);

	Concurrency::ScopedSpinLock lock(s_library->sensationsLock);

	for_every(s, s_library->sensations)
	{
		bhm->register_sensation(s->id, s->sensationJson);
	}
}

void Library::register_for(bhapticsWindows::BhapticsModule* bhm)
{
	an_assert(s_library);

	Concurrency::ScopedSpinLock lock(s_library->sensationsLock);

	for_every(s, s_library->sensations)
	{
		bhm->register_sensation(s->id, s->sensationJson);
	}
}

void Library::set(Name const& _id, String const& _sensationJson)
{
	an_assert(s_library);

	Concurrency::ScopedSpinLock lock(s_library->sensationsLock, true);

	for_every(s, s_library->sensations)
	{
		if (s->id == _id)
		{
			s->sensationJson = _sensationJson;
			return;
		}
	}

	Sensation s;
	s.id = _id;
	s.sensationJson = _sensationJson;
	s_library->sensations.push_back(s);
}

String Library::get(Name const& _id)
{
	an_assert(s_library);

	Concurrency::ScopedSpinLock lock(s_library->sensationsLock, true);

	for_every(s, s_library->sensations)
	{
		if (s->id == _id)
		{
			return s->sensationJson;
		}
	}

	return String::empty();
}

bool Library::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (s_library)
	{
		Concurrency::ScopedSpinLock lock(s_library->sensationsLock);

		Name id = _node->get_name_attribute(TXT("id"));
		Name copyOf = _node->get_name_attribute(TXT("copyOf"));
		Name mirrorOf = _node->get_name_attribute(TXT("mirrorOf"));
		String sensationJson = _node->get_text().trim(); // as created by bhaptics designer

		if (sensationJson.does_contain(TXT("\"project\"")))
		{
			// extract contents of project
			int openBracket = sensationJson.find_first_of('{');
			openBracket = sensationJson.find_first_of('{', openBracket + 1);
			int closeBracket = sensationJson.find_last_of('}');
			closeBracket = sensationJson.find_last_of('}', closeBracket - 1);
			int projectAt = sensationJson.find_first_of(String(TXT("\"project\"")));
			if (projectAt < openBracket && closeBracket > openBracket)
			{
				sensationJson = sensationJson.get_sub(openBracket, closeBracket - openBracket + 1);
			}
		}

		String renameFrom = _node->get_string_attribute(TXT("renameFrom"));
		if (!renameFrom.is_empty())
		{
			sensationJson = sensationJson.replace(renameFrom, id.to_string());
		}

		if (copyOf.is_valid())
		{
			sensationJson = get(copyOf);
			sensationJson = sensationJson.replace(copyOf.to_string(), id.to_string());
		}

		if (mirrorOf.is_valid())
		{
			sensationJson = get(mirrorOf);
			sensationJson = sensationJson.replace(mirrorOf.to_string(), id.to_string());
			for_every(node, _node->children_named(TXT("mirror")))
			{
				String left = node->get_string_attribute(TXT("left"));
				String right = node->get_string_attribute(TXT("right"));
				error_loading_xml_on_assert(!left.is_empty() && !right.is_empty(), result, node, TXT("requires both \"left\" and \"right\" defined"));
				if (!left.is_empty() && !right.is_empty())
				{
					String temp(TXT("##TEMP##"));
					sensationJson = sensationJson.replace(left, temp).replace(right, left).replace(temp, right);
				}
			}
		}

		error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("missing \"id\""));
		error_loading_xml_on_assert(!sensationJson.is_empty(), result, _node, TXT("no content provided for sensation json"));

		if (id.is_valid() && ! sensationJson.is_empty())
		{
			set(id, sensationJson);
		}
	}

	return result;
}

#define FORCE_SENSATION_HAND(RETURN) \
	{ \
		if (_sensation.hand.get() == Hand::Left) sensationId = NAME(RETURN##L); \
		if (_sensation.hand.get() == Hand::Right) sensationId = NAME(RETURN##R); \
	}

#define FORCE_SENSATION_VEST_FRONT_BACK(RETURN) \
	{ \
		if (fromFrontVest) \
		{ \
			sensationId = NAME(RETURN); \
		} \
		else \
		{ \
			sensationId = NAME(RETURN##B); \
		} \
	}

#define SINGLE_SENSATION_PROVIDE_AS(POSITION, TYPE, RETURN) if (_sensation.type == PhysicalSensations::SingleSensation::TYPE) { _sensations.push_back(SensationToTrigger(NAME(RETURN), PositionType::POSITION)); }
#define SINGLE_SENSATION_PROVIDE(POSITION, TYPE) SINGLE_SENSATION_PROVIDE_AS(POSITION, TYPE, TYPE);
#define SINGLE_SENSATION_PROVIDE_HAND_AS(TYPE, RETURN) if (proposedForHand.is_set() && _sensation.type == PhysicalSensations::SingleSensation::TYPE) { Name sensationId; FORCE_SENSATION_HAND(RETURN); if (sensationId.is_valid()) _sensations.push_back(SensationToTrigger(sensationId, proposedForHand.get())); }
#define SINGLE_SENSATION_PROVIDE_HAND(TYPE) SINGLE_SENSATION_PROVIDE_HAND_AS(TYPE, TYPE);
#define SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK_AS(TYPE, RETURN) if (_sensation.type == PhysicalSensations::SingleSensation::TYPE) { Name sensationId; FORCE_SENSATION_VEST_FRONT_BACK(RETURN); if (sensationId.is_valid()) _sensations.push_back(SensationToTrigger(sensationId, PositionType::Vest)); }
#define SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK(TYPE) SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK_AS(TYPE, TYPE);
#define SINGLE_SENSATION_PROVIDE_BY_HAND_AS(POSITION, TYPE, RETURN) if (_sensation.type == PhysicalSensations::SingleSensation::TYPE) { Name sensationId; FORCE_SENSATION_HAND(RETURN); if (sensationId.is_valid()) _sensations.push_back(SensationToTrigger(sensationId, PositionType::POSITION)); }
#define IF_SINGLE_SENSATION(TYPE) if (_sensation.type == PhysicalSensations::SingleSensation::TYPE)
#define IF_SINGLE_SENSATION_2(TYPE1, TYPE2) if (_sensation.type == PhysicalSensations::SingleSensation::TYPE1 || _sensation.type == PhysicalSensations::SingleSensation::TYPE2)
#define IF_SINGLE_SENSATION_3(TYPE1, TYPE2, TYPE3) if (_sensation.type == PhysicalSensations::SingleSensation::TYPE1 || _sensation.type == PhysicalSensations::SingleSensation::TYPE2 || _sensation.type == PhysicalSensations::SingleSensation::TYPE3)
#define IF_SINGLE_SENSATION_4(TYPE1, TYPE2, TYPE3, TYPE4) if (_sensation.type == PhysicalSensations::SingleSensation::TYPE1 || _sensation.type == PhysicalSensations::SingleSensation::TYPE2 || _sensation.type == PhysicalSensations::SingleSensation::TYPE3 || _sensation.type == PhysicalSensations::SingleSensation::TYPE4)
#define MATCH_SINGLE_SENSATION_AS(TYPE, RETURN) if (_sensation.type == PhysicalSensations::SingleSensation::TYPE) sensationId = NAME(RETURN);
#define MATCH_SINGLE_SENSATION(TYPE) MATCH_SINGLE_SENSATION_AS(TYPE, TYPE);
#define MATCH_SINGLE_SENSATION_HAND_AS(TYPE, RETURN) if (_sensation.type == PhysicalSensations::SingleSensation::TYPE && _sensation.hand.is_set()) FORCE_SENSATION_HAND(RETURN);
#define MATCH_SINGLE_SENSATION_HAND(TYPE) MATCH_SINGLE_SENSATION_HAND_AS(TYPE, TYPE);

void Library::get_sensation_ids_for(PhysicalSensations::SingleSensation const& _sensation, AvailableDevices const& _availableDevices, OUT_ ArrayStatic<SensationToTrigger, 4>& _sensations)
{
	bool fromFront = true;
	bool fromFrontVest = true;
	{
		if (_sensation.dirOS.is_set())
		{
			fromFront = _sensation.dirOS.get().y < -0.2f;
			fromFrontVest = _sensation.dirOS.get().y < 0.0f;
		}
		else if (_sensation.hitLocOS.is_set())
		{
			fromFront = abs(_sensation.hitLocOS.get().x) < _sensation.hitLocOS.get().y * 2.0f;
			fromFrontVest = _sensation.hitLocOS.get().y >= 0.0f;
		}
	}

	Optional<PositionType::Type> proposedForHand;
	{
		if (_sensation.hand.is_set())
		{
			proposedForHand = _sensation.hand.get() == Hand::Left ? PositionType::ForearmL : PositionType::ForearmR;
			if (!_availableDevices.has(proposedForHand.get()))
			{
				proposedForHand.clear();
			}
		}
	}

	IF_SINGLE_SENSATION_2(Explosion, ExplosionLight)
	{
		{
			SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK(ExplosionLight);
			SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK(Explosion);
		}
		if (fromFront && _availableDevices.has(PositionType::Head))
		{
			SINGLE_SENSATION_PROVIDE_AS(Head, ExplosionLight, HeadExplosionLight);
			SINGLE_SENSATION_PROVIDE_AS(Head, Explosion, HeadExplosion);
		}
	}
	IF_SINGLE_SENSATION_3(ShotLight, Shot, ShotHeavy)
	{
		if (fromFront && _sensation.head.get(false) && _availableDevices.has(PositionType::Head))
		{
			SINGLE_SENSATION_PROVIDE_AS(Head, ShotLight, HeadShotLight);
			SINGLE_SENSATION_PROVIDE_AS(Head, Shot, HeadShot);
			SINGLE_SENSATION_PROVIDE_AS(Head, ShotHeavy, HeadShotHeavy);
		}
		else 
		{
			SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK(ShotLight);
			SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK(Shot);
			SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK(ShotHeavy);
		}
	}
	IF_SINGLE_SENSATION_2(Electrocution, ElectrocutionLight)
	{
		if (fromFront && _sensation.head.get(false) && _availableDevices.has(PositionType::Head))
		{
			SINGLE_SENSATION_PROVIDE_AS(Head, ElectrocutionLight, HeadElectrocutionLight);
			SINGLE_SENSATION_PROVIDE_AS(Head, Electrocution, HeadElectrocution);
		}
		else
		{
			SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK(ElectrocutionLight);
			SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK(Electrocution);
		}
	}

	IF_SINGLE_SENSATION_4(RecoilWeak, RecoilMedium, RecoilStrong, RecoilVeryStrong)
	{
		{
			SINGLE_SENSATION_PROVIDE_BY_HAND_AS(Vest, RecoilWeak, VestRecoilWeak);
			SINGLE_SENSATION_PROVIDE_BY_HAND_AS(Vest, RecoilMedium, VestRecoilMedium);
			SINGLE_SENSATION_PROVIDE_BY_HAND_AS(Vest, RecoilStrong, VestRecoilStrong);
			SINGLE_SENSATION_PROVIDE_BY_HAND_AS(Vest, RecoilVeryStrong, VestRecoilVeryStrong);
		}
		{
			SINGLE_SENSATION_PROVIDE_HAND(RecoilWeak);
			SINGLE_SENSATION_PROVIDE_HAND(RecoilMedium);
			SINGLE_SENSATION_PROVIDE_HAND(RecoilStrong);
			SINGLE_SENSATION_PROVIDE_HAND(RecoilVeryStrong);
		}
	}

	IF_SINGLE_SENSATION(PowerUp)
	{
		SINGLE_SENSATION_PROVIDE_VEST_FRONT_BACK(PowerUp);
	}

	SINGLE_SENSATION_PROVIDE_HAND(EnergyTaken);
	if (proposedForHand.is_set())
	{
		SINGLE_SENSATION_PROVIDE_HAND_AS(GrabbedSomething, Grab);
		SINGLE_SENSATION_PROVIDE_HAND_AS(UsedSomething, Use);
		SINGLE_SENSATION_PROVIDE_HAND_AS(ReleasedSomething, Release);
	}
	else
	{
		SINGLE_SENSATION_PROVIDE_BY_HAND_AS(Vest, GrabbedSomething, VestGrab);
		SINGLE_SENSATION_PROVIDE_BY_HAND_AS(Vest, UsedSomething, VestUse);
		SINGLE_SENSATION_PROVIDE_BY_HAND_AS(Vest, ReleasedSomething, VestRelease);
	}

	//MATCH_SINGLE_SENSATION_TYPE_EX(SlashedLight, Stabbed);
	//MATCH_SINGLE_SENSATION_TYPE_EX(Slashed, Stabbed);
	//MATCH_SINGLE_SENSATION_TYPE(Punched);
	//MATCH_SINGLE_SENSATION_TYPE_EX(Tingling, Insects);
}

#define ONGOING_SENSATION_PROVIDE_AS(POSITION, TYPE, RETURN) if (_sensation.type == PhysicalSensations::OngoingSensation::TYPE) { _sensations.push_back(SensationToTrigger(NAME(RETURN), PositionType::POSITION)); }
#define ONGOING_SENSATION_PROVIDE(POSITION, TYPE) ONGOING_SENSATION_PROVIDE_AS(POSITION, TYPE, TYPE);
#define ONGOING_SENSATION_PROVIDE_HAND_AS(TYPE, RETURN) if (proposedForHand.is_set() && _sensation.type == PhysicalSensations::OngoingSensation::TYPE) { Name sensationId; FORCE_SENSATION_HAND(RETURN); if (sensationId.is_valid()) _sensations.push_back(SensationToTrigger(sensationId, proposedForHand.get())); }
#define ONGOING_SENSATION_PROVIDE_HAND(TYPE) ONGOING_SENSATION_PROVIDE_HAND_AS(TYPE, TYPE);
#define IF_ONGOING_SENSATION(TYPE) if (_sensation.type == PhysicalSensations::OngoingSensation::TYPE)
#define IF_ONGOING_SENSATION_2(TYPE1, TYPE2) if (_sensation.type == PhysicalSensations::OngoingSensation::TYPE1 || _sensation.type == PhysicalSensations::OngoingSensation::TYPE2)
#define IF_ONGOING_SENSATION_3(TYPE1, TYPE2, TYPE3) if (_sensation.type == PhysicalSensations::OngoingSensation::TYPE1 || _sensation.type == PhysicalSensations::OngoingSensation::TYPE2 || _sensation.type == PhysicalSensations::OngoingSensation::TYPE3)
#define IF_ONGOING_SENSATION_4(TYPE1, TYPE2, TYPE3, TYPE4) if (_sensation.type == PhysicalSensations::OngoingSensation::TYPE1 || _sensation.type == PhysicalSensations::OngoingSensation::TYPE2 || _sensation.type == PhysicalSensations::OngoingSensation::TYPE3 || _sensation.type == PhysicalSensations::OngoingSensation::TYPE4)
#define MATCH_ONGOING_SENSATION_AS(TYPE, RETURN) if (_sensation.type == PhysicalSensations::OngoingSensation::TYPE) sensationId = NAME(RETURN);
#define MATCH_ONGOING_SENSATION(TYPE) MATCH_ONGOING_SENSATION_AS(TYPE, TYPE);
#define MATCH_ONGOING_SENSATION_HAND_AS(TYPE, RETURN) if (_sensation.type == PhysicalSensations::OngoingSensation::TYPE && _sensation.hand.is_set()) FORCE_SENSATION_HAND(RETURN);
#define MATCH_ONGOING_SENSATION_HAND(TYPE) MATCH_ONGOING_SENSATION_HAND_AS(TYPE, TYPE);

void Library::get_sensation_ids_for(PhysicalSensations::OngoingSensation const& _sensation, AvailableDevices const& _availableDevices, OUT_ ArrayStatic<SensationToTrigger, 4>& _sensations)
{
	Optional<PositionType::Type> proposedForHand;
	{
		if (_sensation.hand.is_set())
		{
			proposedForHand = _sensation.hand.get() == Hand::Left ? PositionType::ForearmL : PositionType::ForearmR;
			if (!_availableDevices.has(proposedForHand.get()))
			{
				proposedForHand.clear();
			}
		}
	}

	if (proposedForHand.is_set())
	{
		IF_ONGOING_SENSATION_4(LiftObject, LiftHeavyObject, PushObject, PushHeavyObject)
		{
			Name sensationId;
			//FORCE_SENSATION_HAND(PullPush);
			if (sensationId.is_valid()) _sensations.push_back(SensationToTrigger(sensationId, proposedForHand.get()));
		}
	}

	ONGOING_SENSATION_PROVIDE(Vest, UpgradeReceived);
	ONGOING_SENSATION_PROVIDE_AS(Vest, Wind, WindBlow);
	ONGOING_SENSATION_PROVIDE_AS(Vest, Falling, WindBlow);
	ONGOING_SENSATION_PROVIDE_AS(Vest, SlowSpeed, ElevatorRide);
	ONGOING_SENSATION_PROVIDE_HAND_AS(EnergyCharging, Charging);

	// ------- Oppression
}

#define CHANGE_SENSATION_TYPE(FROM, TO) if (_sensationId == NAME(FROM)) _sensationId = NAME(TO);

an_bhaptics::PositionType::Type Library::get_position_type_for(PhysicalSensations::Sensation const& _sensation, AvailableDevices const& _availableDevices)
{
	if (_sensation.hand.is_set())
	{
		an_bhaptics::PositionType::Type proposed = _sensation.hand.get() == Hand::Left ? PositionType::ForearmL : PositionType::ForearmR;
		if (_availableDevices.has(proposed))
		{
			return proposed;
		}
		proposed = _sensation.hand.get() == Hand::Left ? PositionType::HandL : PositionType::HandR;
		return proposed;
	}

	if (_sensation.head.is_set() &&
		_sensation.head.get())
	{
		an_bhaptics::PositionType::Type proposed = PositionType::Head;
		if (_availableDevices.has(proposed))
		{
			bool fromFront = true;

			if (_sensation.dirOS.is_set())
			{
				fromFront = _sensation.dirOS.get().y > 0.2f;
			}
			else if (_sensation.hitLocOS.is_set())
			{
				fromFront = abs(_sensation.hitLocOS.get().x) < _sensation.hitLocOS.get().y * 2.0f;
			}

			if (fromFront)
			{
				return proposed;
			}
		}
	}

	return PositionType::Vest;
}

void Library::update_sensation_id_or_position(REF_ Name& _sensationId, REF_ PositionType::Type& _position)
{
	if (_position == PositionType::Head)
	{
		CHANGE_SENSATION_TYPE(ElectrocutionLight, HeadElectrocutionLight) else
		CHANGE_SENSATION_TYPE(Electrocution, HeadElectrocution) else
		CHANGE_SENSATION_TYPE(ShotLight, HeadShotLight) else
		CHANGE_SENSATION_TYPE(Shot, HeadShot) else
		CHANGE_SENSATION_TYPE(ShotHeavy, HeadShotHeavy) else
		{
			_position = PositionType::Vest;
		}
	}
}

void Library::get_offsets_for_vest(PhysicalSensations::Sensation const& _sensation, Name const & _sensationId, OUT_ float& _offsetYaw, OUT_ float& _offsetZ)
{
	if (_sensation.hitLocOS.is_set())
	{
		_offsetYaw = -Rotator3::get_yaw(_sensation.hitLocOS.get());
		Range zRange = Range(0.7f, 1.6f);
		if (_sensation.eyeHeightOS.is_set())
		{
			zRange.min = _sensation.eyeHeightOS.get() * 0.4f;
			zRange.max = _sensation.eyeHeightOS.get() * 0.92f;
		}
		_offsetZ = zRange.get_pt_from_value(_sensation.hitLocOS.get().z) - 0.5f;
	}
	else if (_sensation.dirOS.is_set())
	{
		Vector3 dirNOs = _sensation.dirOS.get().normal();
		_offsetYaw = -Rotator3::get_yaw(-dirNOs);
		_offsetZ = dirNOs.z * 0.55f;
	}
	else
	{
		_offsetYaw = 0.0f;
		_offsetZ = 0.0f;
	}

	// clamp to sane values
	_offsetZ = clamp(_offsetZ, -0.45f, 0.45f);

	if (_sensationId == NAME(ElectrocutionLight) ||
		_sensationId == NAME(Electrocution) ||
		_sensationId == NAME(ExplosionLight) ||
		_sensationId == NAME(Explosion) ||
		_sensationId == NAME(WindBlow))
	{
		_offsetZ = 0.0f;
	}
	if (_sensationId == NAME(UpgradeReceived) ||
		_sensationId == NAME(ElevatorRide))
	{
		_offsetYaw = 0.0f;
		_offsetZ = 0.0f;
	}
}
