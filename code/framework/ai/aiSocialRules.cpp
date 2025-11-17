#include "aiSocialRules.h"

#include "..\..\core\containers\list.h"
#include "..\..\core\math\mathUtils.h"
#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace AI;

//

int SocialRelation::at_least(SocialRelation::Type _type)
{
	int type = _type;
	int result = 0;
	do
	{
		result |= type;
		type = type << 2;
	} while (type != Max);
	return type;
}

int SocialRelation::at_most(SocialRelation::Type _type)
{
	int type = _type;
	int result = 0;
	do
	{
		result |= type;
		type = type >> 2;
	} while (type != Min);

	return type;
}

SocialRelation::Type SocialRelation::parse_attr(String const & _attr)
{
	if (_attr == TXT("neutralTowards") ||
		_attr == TXT("neutral"))
	{
		return SocialRelation::Neutral;
	}
	if (_attr == TXT("loves"))
	{
		return SocialRelation::Loves;
	}
	if (_attr == TXT("hates"))
	{
		return SocialRelation::Hates;
	}
	error(TXT("could not recognise social relation \"%S\""), _attr.to_char());
	return SocialRelation::Neutral;
}
//

SocialRules* SocialRules::s_socialRules = nullptr;

void SocialRules::initialise_static()
{
	an_assert(s_socialRules == nullptr);
	s_socialRules = new SocialRules();
}

void SocialRules::close_static()
{
	an_assert(s_socialRules);
	delete_and_clear(s_socialRules);
}

bool SocialRules::is_faction_defined(Name const & _faction)
{
	an_assert(s_socialRules);
	return s_socialRules->factions.does_contain(_faction);
}

SocialRelation::Type SocialRules::get_relation(Name const & _of, Name const & _towards)
{
	an_assert(s_socialRules);
	return s_socialRules->get_relation_internal(_of, _towards);
}

bool SocialRules::load_rules_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	for_every(node, _node->children_named(TXT("rule")))
	{
		Rule rule;
		for_every(attr, node->all_attributes())
		{
			if (attr->get_name() == TXT("mutually"))
			{
				rule.mutually = attr->get_as_bool();
			}
			else if (attr->get_name() == TXT("faction"))
			{
				rule.faction = attr->get_as_name();
			}
			else if (attr->get_name() == TXT("except"))
			{
				String exceptions = attr->get_as_string();
				List<String> tokens;
				exceptions.split(String::comma(), tokens);
				for_every(token, tokens)
				{
					rule.exceptions.push_back(Name(token->trim()));
				}
			}
			else
			{
				rule.relation = SocialRelation::parse_attr(attr->get_name());
				String towards = attr->get_as_string();
				List<String> tokens;
				towards.split(String::comma(), tokens);
				for_every(token, tokens)
				{
					rule.towards.push_back(Name(token->trim()));
				}
			}
		}
		rules.push_back(rule);
	}
	result &= build_matrix();
	return result;
}

bool SocialRules::build_matrix()
{
	bool result = true;
	factions.clear();
	socialMatrix.clear();

	for_every(rule, rules)
	{
		factions.push_back_unique(rule->faction);
		for_every(towards, rule->towards)
		{
			factions.push_back_unique(*towards);
		}
		for_every(exception, rule->exceptions)
		{
			factions.push_back_unique(*exception);
		}
	}

	socialMatrix.set_size(sqr(factions.get_size()));
	for_every(sm, socialMatrix)
	{
		*sm = SocialRelation::Neutral;
	}

	// by default all factions like themselves
	for_every(f, factions)
	{
		access_relation_internal(*f, *f) = SocialRelation::Likes;
	}

	Array<Name> ofFaction;
	Array<Name> working;

	for_every(rule, rules)
	{
		if (!rule->relation.is_set())
		{
			continue;
		}
		ofFaction.clear();
		if (rule->faction.is_valid())
		{
			ofFaction.push_back(rule->faction);
		}
		else
		{
			ofFaction.add_from(factions);
		}
		working.clear();
		if (rule->towards.is_empty())
		{
			working.add_from(factions);
		}
		else
		{
			working.add_from(rule->towards);
		}
		for_every(exception, rule->exceptions)
		{
			working.remove_fast(*exception);
		}

		for_every(of, ofFaction)
		{
			for_every(w, working)
			{
				access_relation_internal(*of, *w) = rule->relation.get();
				if (rule->mutually)
				{
					access_relation_internal(*w, *of) = rule->relation.get();
				}
			}
		}
	}

	return result;
}

int SocialRules::get_faction_index(Name const & _faction) const
{
	for_every(faction, factions)
	{
		if (*faction == _faction)
		{
			return for_everys_index(faction);
		}
	}
	return NONE;
}

Optional<bool> SocialRules::check_is_enemy(IAIObject const* _aiOwner, IAIObject const* _ai, bool _isEnemy)
{
	an_assert(s_socialRules);
	if (s_socialRules->is_enemy_external)
	{
		return s_socialRules->is_enemy_external(_aiOwner, _ai, _isEnemy);
	}
	return Optional<bool>();
}

SocialRelation::Type SocialRules::get_relation_internal(Name const & _of, Name const & _towards) const
{
	int of = get_faction_index(_of);
	int towards = get_faction_index(_towards);
	SocialRelation::Type result = SocialRelation::Neutral;
	if (of != NONE && towards != NONE)
	{
		result = socialMatrix[of * factions.get_size() + towards];
	}
	return result;
}

SocialRelation::Type & SocialRules::access_relation_internal(Name const & _of, Name const & _towards)
{
	int of = get_faction_index(_of);
	int towards = get_faction_index(_towards);
	an_assert(of != NONE);
	an_assert(towards != NONE);
	return socialMatrix[of * factions.get_size() + towards];
}
