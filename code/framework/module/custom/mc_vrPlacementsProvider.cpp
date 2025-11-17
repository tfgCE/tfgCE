#include "mc_vrPlacementsProvider.h"

#include "mc_lightSources.h"

#include "..\modules.h"
#include "..\moduleDataImpl.inl"

#include "..\..\appearance\controllers\ac_particlesUtils.h"
#include "..\..\collision\checkCollisionContext.h"
#include "..\..\collision\checkSegmentResult.h"
#include "..\..\game\gameUtils.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\object\temporaryObject.h"
#include "..\..\sound\soundSample.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace CustomModules;

//

DEFINE_STATIC_NAME(vrPlacementHead);
DEFINE_STATIC_NAME(vrPlacementHandLeft);
DEFINE_STATIC_NAME(vrPlacementHandRight);

//

REGISTER_FOR_FAST_CAST(CustomModules::VRPlacementsProvider);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new CustomModules::VRPlacementsProvider(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & CustomModules::VRPlacementsProvider::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("vrPlacementsProvider")), create_module);
}

CustomModules::VRPlacementsProvider::VRPlacementsProvider(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

CustomModules::VRPlacementsProvider::~VRPlacementsProvider()
{
}

void CustomModules::VRPlacementsProvider::reset()
{
	base::reset();

	head.clear();
	handLeft.clear();
	handRight.clear();
}

void CustomModules::VRPlacementsProvider::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	auto const & variables = get_owner()->get_variables();
	if (auto* v = variables.get_existing<Transform>(NAME(vrPlacementHead)))
	{
		head = *v;
	}
	if (auto* v = variables.get_existing<Transform>(NAME(vrPlacementHandLeft)))
	{
		handLeft= *v;
	}
	if (auto* v = variables.get_existing<Transform>(NAME(vrPlacementHandRight)))
	{
		handRight= *v;
	}
}
