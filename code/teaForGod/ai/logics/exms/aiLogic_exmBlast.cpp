#include "aiLogic_exmBlast.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\playerSetup.h"
#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\gameplay\equipment\me_gun.h"

#include "..\..\..\..\core\containers\arrayStack.h"
#include "..\..\..\..\core\memory\safeObject.h"
#include "..\..\..\..\core\types\hand.h"

#include "..\..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\sound\soundScene.h"
#include "..\..\..\..\framework\world\room.h"
#include "..\..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// sound, temporary object
DEFINE_STATIC_NAME(blast);

//

REGISTER_FOR_FAST_CAST(EXMBlast);

EXMBlast::EXMBlast(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
	exmBlastData= fast_cast<EXMBlastData>(_logicData);
}

EXMBlast::~EXMBlast()
{
}

void EXMBlast::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && owner.get() && hand != Hand::MAX)
	{
		if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get()))
		{
			if (exmModule)
			{
				if (exmModule->mark_exm_active_blink())
				{
					if (auto* s = owner->get_sound())
					{
						s->play_sound(NAME(blast));
					}
					if (auto* to = owner->get_temporary_objects())
					{
						to->spawn(NAME(blast));// , pilgrimOwner->get_presence()->get_in_room(), pilgrimOwner->get_presence()->get_centre_of_presence_transform_WS());
					}
				}
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(EXMBlastData);

bool EXMBlastData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	/*
	XML_LOAD_ATTR_CHILD(_node, range);
	XML_LOAD_ATTR_CHILD(_node, damage);
	{
		String damageTypeStr = _node->get_string_attribute_or_from_child(TXT("damageType"));
		if (!damageTypeStr.is_empty())
		{
			damageType = DamageType::parse(damageTypeStr);
		}
	}
	armourPenetration.load_from_attribute_or_from_child(_node, TXT("armourPenetration"), false);
	XML_LOAD_FLOAT_ATTR_CHILD(_node, confussionTime);
	*/

	return result;
}

bool EXMBlastData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
