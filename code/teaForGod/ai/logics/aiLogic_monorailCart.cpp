#include "aiLogic_monorailCart.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

//

REGISTER_FOR_FAST_CAST(MonorailCart);

MonorailCart::MonorailCart(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

MonorailCart::~MonorailCart()
{
}

void MonorailCart::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

