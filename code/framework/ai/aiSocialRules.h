#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"

#include <functional>

namespace Framework
{
	interface_class IAIObject;

	namespace AI
	{
		class MindInstance;

		namespace SocialRelation
		{
			// these work both as mask (for looking/checking) and as check which one is lower/higher
			// by default same faction likes each other
			enum Type
			{
				Hates		= 1,
				Dislikes	= 2,
				Neutral		= 4,
				Likes		= 8,
				Loves		= 16,

				Min = Hates,
				Max = Loves,
			};

			int at_least(SocialRelation::Type _type);
			int at_most(SocialRelation::Type _type);

			SocialRelation::Type parse_attr(String const & _attr);
		};

		class SocialRules
		{
		public:
			static void initialise_static();
			static void close_static();

			static bool load_from_xml(IO::XML::Node const * _node) { an_assert(s_socialRules); return s_socialRules->load_rules_from_xml(_node); }

			static SocialRelation::Type get_relation(Name const & _of, Name const & _towards);

			static bool is_faction_defined(Name const & _faction);

			static void use_for_is_enemy_external(std::function<Optional<bool>(IAIObject const* _aiOwner, IAIObject const* _ai, bool _isEnemy)> _is_enemy_external) { an_assert(s_socialRules); s_socialRules->is_enemy_external = _is_enemy_external; }
			static Optional<bool> check_is_enemy(IAIObject const* _aiOwner, IAIObject const* _ai, bool _isEnemy);

		private:
			static SocialRules* s_socialRules;

			struct Rule
			{
				Name faction;
				bool mutually = false;
				Array<Name> towards; // if not defined, towards all
				Array<Name> exceptions;
				Optional<SocialRelation::Type> relation;
			};

			Array<Rule> rules;
			Array<Name> factions;
			Array<SocialRelation::Type> socialMatrix;
			std::function<Optional<bool> (IAIObject const* _aiOwner, IAIObject const* _ai, bool _isEnemy)> is_enemy_external = nullptr;

			bool load_rules_from_xml(IO::XML::Node const * _node);

			bool build_matrix();

			int get_faction_index(Name const & _faction) const;

			SocialRelation::Type get_relation_internal(Name const & _of, Name const & _towards) const;
			SocialRelation::Type & access_relation_internal(Name const & _of, Name const & _towards);
		};
	};
};
