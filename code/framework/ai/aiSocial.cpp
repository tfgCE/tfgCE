#include "aiSocial.h"

#include "aiMindInstance.h"
#include "aiSocialData.h"
#include "aiSocialRules.h"

#include "..\interfaces\aiObject.h"
#include "..\module\moduleAI.h"
#include "..\modulesOwner\modulesOwner.h"
#include "..\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AI;

//

// object tags
DEFINE_STATIC_NAME(dontAttack);

//

Social::Social(MindInstance* _mind)
: mind(_mind)
{
	if (auto* mindData = mind->get_mind())
	{
		if (auto* data = mindData->get_social_data())
		{
			set_never_against_allies();
			set_sociopath();
			set_endearing();
			set_faction();
		}
	}
}

Social::~Social()
{
}

void Social::be_enemy(IAIObject const* _ai)
{
	for_every_ref(enemy, enemies)
	{
		if (enemy == _ai)
		{
			return;
		}
	}
	enemies.push_back(SafePtr<IAIObject>(_ai));
}

void Social::no_longer_be_enemy(IAIObject const* _ai)
{
	for_every_ref(enemy, enemies)
	{
		if (enemy == _ai)
		{
			enemies.remove_fast_at(for_everys_index(enemy));
			return;
		}
	}
}

void Social::be_ally(IAIObject const* _ai)
{
	for_every_ref(ally, allies)
	{
		if (ally == _ai)
		{
			return;
		}
	}
	enemies.push_back(SafePtr<IAIObject>(_ai));
}

void Social::no_longer_be_ally(IAIObject const* _ai)
{
	for_every_ref(ally, enemies)
	{
		if (ally == _ai)
		{
			allies.remove_fast_at(for_everys_index(ally));
			return;
		}
	}
}

void Social::set_never_against_allies(Optional<bool> const & _neverAgainstAllies)
{
	if (_neverAgainstAllies.is_set())
	{
		neverAgainstAllies = _neverAgainstAllies.get();
	}
	else if (auto* mindData = mind->get_mind())
	{
		if (auto* data = mindData->get_social_data())
		{
			neverAgainstAllies = data->neverAgainstAllies;
		}
	}
}

void Social::set_sociopath(Optional<bool> const & _sociopath)
{
	if (_sociopath.is_set())
	{
		sociopath = _sociopath.get();
	}
	else if (auto* mindData = mind->get_mind())
	{
		if (auto* data = mindData->get_social_data())
		{
			sociopath = data->sociopath;
		}
	}
}

void Social::set_endearing(Optional<bool> const & _endearing)
{
	if (_endearing.is_set())
	{
		endearing = _endearing.get();
	}
	else if (auto* mindData = mind->get_mind())
	{
		if (auto* data = mindData->get_social_data())
		{
			endearing = data->endearing;
		}
	}
}

void Social::set_faction(Optional<Name> const & _faction)
{
	if (_faction.is_set())
	{
		faction = _faction.get();
	}
	else if (auto* mindData = mind->get_mind())
	{
		if (auto* data = mindData->get_social_data())
		{
			faction = data->faction;
		}
	}
}

bool Social::is_enemy_or_owned_by_enemy(IAIObject const* _ai) const
{
	while (_ai)
	{
		if (is_enemy(_ai))
		{
			return true;
		}
		auto* aiInstigator = _ai->ai_instigator();
		if (_ai == aiInstigator)
		{
			break;
		}
		_ai = aiInstigator;
	}
	return false;
}

bool Social::is_enemy(IAIObject const* _ai) const
{
	if (!_ai)
	{
		return false;
	}
	// ally before enemy, friendship power
	for_every_ref(ally, allies)
	{
		if (ally == _ai)
		{
			return false;
		}
	}
	for_every_ref(enemy, enemies)
	{
		if (enemy == _ai)
		{
			return true;
		}
	}
	bool result = false;
	if (auto* asObject = _ai->get_ai_object_as_object())
	{
		if (asObject->get_tags().get_tag(NAME(dontAttack)))
		{
			return false;
		}
		if (!asObject->is_sub_object())
		{
			if (sociopath && _ai != mind->get_owner_as_modules_owner())
			{
				result = true;
			}
			else if (endearing && _ai != mind->get_owner_as_modules_owner())
			{
				result = false;
			}
			else if (auto* actorAI = asObject->get_ai())
			{
				Name myFaction = get_faction();
				if (actorAI->get_mind())
				{
					Name otherFaction = actorAI->get_mind()->get_social().get_faction();
					if (SocialRules::get_relation(myFaction, otherFaction) < SocialRelation::Neutral)
					{
						result = true;
					}
				}
				else
				{
					error_stop(TXT("object %S does not have mind"), asObject->get_name().to_char());
				}
			}
		}
	}
	return SocialRules::check_is_enemy(mind->get_owner_as_modules_owner(), _ai, result).get(result);
}

bool Social::is_ally_or_owned_by_ally(IAIObject const* _ai) const
{
	while (_ai)
	{
		if (is_ally(_ai))
		{
			return true;
		}
		auto* aiInstigator = _ai->ai_instigator();
		if (_ai == aiInstigator)
		{
			break;
		}
		_ai = aiInstigator;
	}
	return false;
}

bool Social::is_ally(IAIObject const* _ai) const
{
	if (!_ai)
	{
		return false;
	}
	// ally before enemy, friendship power
	for_every_ref(ally, allies)
	{
		if (ally == _ai)
		{
			return true;
		}
	}
	for_every_ref(enemy, enemies)
	{
		if (enemy == _ai)
		{
			return false;
		}
	}

	if (auto* asObject = _ai->get_ai_object_as_object())
	{
		if (!asObject->is_sub_object())
		{
			if (sociopath && _ai != mind->get_owner_as_modules_owner())
			{
				return false;
			}
			else if (endearing && _ai != mind->get_owner_as_modules_owner())
			{
				return true;
			}
			else if (auto* actorAI = asObject->get_ai())
			{
				Name myFaction = get_faction();
				if (actorAI->get_mind())
				{
					Name otherFaction = actorAI->get_mind()->get_social().get_faction();
					if (SocialRules::get_relation(myFaction, otherFaction) > SocialRelation::Neutral)
					{
						return true;
					}
				}
				else
				{
					error_stop(TXT("object %S does not have mind"), asObject->get_name().to_char());
				}
			}
		}
	}
	return false; // neutral or enemy
}