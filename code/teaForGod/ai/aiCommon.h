#pragma once

#include "..\..\core\tags\tag.h"

namespace Framework
{
	namespace AI
	{
		class MindInstance;
		struct LatentTaskHandle;
	};
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		class Common
		{
		public:
			static void initialise_static();
			static void close_static();

			static Tags & sounds_talk() { an_assert(s_common); return s_common->soundsTalk; }

			static bool handle_scripted_behaviour(::Framework::AI::MindInstance* _mind, Name const& _scriptedBehaviour, ::Framework::AI::LatentTaskHandle& _currentTask); // returns true if doing scripted behaviour

			static void set_scripted_behaviour(REF_ Name & _scriptedBehaviourVar, Name const & _value);
			static void unset_scripted_behaviour(REF_ Name & _scriptedBehaviourVar, Name const& _unset); // will clear it
			static void unset_scripted_behaviour(REF_ Name & _scriptedBehaviourVar); // will clear it

		private:
			static Common* s_common;
			Tags soundsTalk;
		};
	};
};
