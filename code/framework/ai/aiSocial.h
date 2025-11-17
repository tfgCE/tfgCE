#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\types\optional.h"

namespace Framework
{
	interface_class IAIObject;

	namespace AI
	{
		class MindInstance;

		class Social
		{
		public:
			// myFaction allows to override_ my faction, to pretend that we are of other faction, this is useful for objects that may have faction change (behaviour wise) but should be considered of their normal faction
			bool is_enemy(IAIObject const* _ai) const;
			bool is_enemy_or_owned_by_enemy(IAIObject const* _ai) const;

			void be_enemy(IAIObject const* _ai);
			void no_longer_be_enemy(IAIObject const* _ai);

			bool is_ally(IAIObject const* _ai) const;
			bool is_ally_or_owned_by_ally(IAIObject const* _ai) const;

			void be_ally(IAIObject const* _ai);
			void no_longer_be_ally(IAIObject const* _ai);

			bool is_never_against_allies() const { return neverAgainstAllies; }

			// NP resets to default, useful when switching sides
			void set_never_against_allies(Optional<bool> const & _neverAgainstAllies = NP);
			void set_sociopath(Optional<bool> const & _sociopath = NP);
			void set_endearing(Optional<bool> const & _endearing = NP);
			void set_faction(Optional<Name> const & _faction = NP);

			Name const & get_faction() const { return faction; }

		private: friend class MindInstance;
			Social(MindInstance* _mind);
			~Social();

		private:
			Name faction;
			MindInstance* mind = nullptr;
			bool neverAgainstAllies = false; // won't attack allies even if they attack us
			bool sociopath = false; // attacks anyone that has a mind and is an actor
			bool endearing = false; // everyone likes them, no one attacks
			Array<SafePtr<IAIObject>> enemies;
			Array<SafePtr<IAIObject>> allies;
		};
	};
};
