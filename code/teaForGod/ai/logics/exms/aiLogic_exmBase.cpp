#include "aiLogic_exmBase.h"

#include "..\..\..\game\playerSetup.h"
#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\gameplay\equipment\me_gun.h"

#include "..\..\..\..\core\types\hand.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(exmHand);

//

REGISTER_FOR_FAST_CAST(EXMBase);

EXMBase::EXMBase(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMBase::~EXMBase()
{
}

void EXMBase::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
	hand = (Hand::Type)_parameters.get_value<int>(NAME(exmHand), hand);
}

void EXMBase::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (!owner.get() || !pilgrimOwner.get() || !pilgrimModule)
	{
		owner = get_mind()->get_owner_as_modules_owner();
		if (owner.get())
		{
			exmModule = owner->get_gameplay_as<ModuleEXM>();
			if (auto * instigator = owner->get_top_instigator())
			{
				pilgrimOwner = instigator;
				pilgrimModule = instigator->get_gameplay_as<ModulePilgrim>();
			}
		}
	}
}
