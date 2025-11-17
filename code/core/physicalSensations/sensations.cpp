#include "sensations.h"

//

using namespace PhysicalSensations;

//

#define PASS_AS_TCHAR(what) if (_type == what) return TXT(#what);

tchar const* SingleSensation::to_char(SingleSensation::Type _type)
{
	PASS_AS_TCHAR(Unknown);
	// attacked
	PASS_AS_TCHAR(ShotLight);
	PASS_AS_TCHAR(Shot);
	PASS_AS_TCHAR(ShotHeavy);
	PASS_AS_TCHAR(ElectrocutionLight);
	PASS_AS_TCHAR(Electrocution);
	PASS_AS_TCHAR(SlashedLight);
	PASS_AS_TCHAR(Slashed);
	PASS_AS_TCHAR(Punched);
	PASS_AS_TCHAR(GrabbedSomething);
	PASS_AS_TCHAR(UsedSomething);
	PASS_AS_TCHAR(ReleasedSomething);
	PASS_AS_TCHAR(EnergyTaken);
	PASS_AS_TCHAR(ExplosionLight);
	PASS_AS_TCHAR(Explosion);
	// action
	PASS_AS_TCHAR(RecoilWeak);
	PASS_AS_TCHAR(RecoilMedium);
	PASS_AS_TCHAR(RecoilStrong);
	PASS_AS_TCHAR(RecoilVeryStrong);
	// other
	PASS_AS_TCHAR(Tingling);
	PASS_AS_TCHAR(PowerUp);

	error(TXT("unknown bhaptics SingleSensation (to_char %i)"), _type);
	return TXT("??");
}

//

tchar const* OngoingSensation::to_char(OngoingSensation::Type _type)
{
	PASS_AS_TCHAR(Unknown);
	// attacked
	PASS_AS_TCHAR(LiftObject);
	PASS_AS_TCHAR(LiftHeavyObject);
	PASS_AS_TCHAR(PushObject);
	PASS_AS_TCHAR(PushHeavyObject);
	PASS_AS_TCHAR(Oppression);
	PASS_AS_TCHAR(Wind);
	PASS_AS_TCHAR(Falling);
	PASS_AS_TCHAR(UpgradeReceived);
	PASS_AS_TCHAR(SlowSpeed);
	PASS_AS_TCHAR(EnergyCharging);

	error(TXT("unknown bhaptics OngoingSensation (to_char %i)"), _type);
	return TXT("??");
}
