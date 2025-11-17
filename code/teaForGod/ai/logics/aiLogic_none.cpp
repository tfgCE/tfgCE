#include "aiLogic_none.h"

#include "..\..\utils.h"

#include "..\..\modules\custom\mc_powerSource.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(solarPanelDir)

//

REGISTER_FOR_FAST_CAST(None);

None::None(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	noneData = fast_cast<NoneData>(_logicData);
}

None::~None()
{
}

LATENT_FUNCTION(None::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM_NOT_USED(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	auto* self = fast_cast<None>(logic);

	LATENT_BEGIN_CODE();

	while (true)
	{
		if (self->noneData->noRareWait)
		{
			LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.5f));
		}
		else
		{
			LATENT_WAIT(Random::get_float(0.1f, 0.5f));
		}
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(NoneData);

bool NoneData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	noRareWait = _node->get_bool_attribute_or_from_child_presence(TXT("noRareWait"), noRareWait);

	return result;
}
