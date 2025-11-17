#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\other\remapValue.h"
#include "..\..\core\types\name.h"
#include "..\..\core\fastCast.h"

#include "..\modulesOwner\modulesOwner.h"

#include "..\debugSettings.h"

#include "aiObjectVariables.h"

class SimpleVariableStorage;

#ifdef AN_USE_AI_LOG
#define access_ai_log(logic) logic->access_log()
#define ai_state(logic, number, ...) { logic->set_state_info(number, __VA_ARGS__); }
#define ai_state_clear(logic, number) { logic->clear_state_info(number); }
#define ai_log(logic, ...) { logic->access_log().log(__VA_ARGS__); }
#define ai_log_no_colour(logic) { logic->access_log().set_colour(); }
#define ai_log_colour(logic, colour) { logic->access_log().set_colour(colour); }
#define ai_log_increase_indent(logic) { logic->access_log().increase_indent(); }
#define ai_log_decrease_indent(logic) { logic->access_log().decrease_indent(); }
#else
#define access_ai_log(logic)
#define ai_state(logic, number, ...)
#define ai_state_clear(logic, number)
#define ai_log(logic, ...)
#define ai_log_no_colour(logic)
#define ai_log_colour(logic, colour)
#define ai_log_increase_indent(logic)
#define ai_log_decrease_indent(logic)
#endif

namespace Framework
{
	namespace AI
	{
		class LogicData;
		class MindInstance;
		struct MindInstanceContext;

		/**
		 *	Main element of mind instance.
		 *	It might be something that does whole logic of object.
		 *	Or it might just run behavior/decision tree.
		 *	Or may start latent tasks (LogicWithLatentTask works like that).
		 *	Or be mix of it.
		 */
		class Logic
		{
			FAST_CAST_DECLARE(Logic);
			FAST_CAST_END();
		public:
			template <typename Class>
			static Class* get_logic_as(IModulesOwner* _imo);

			float adjust_time_depending_on_distance(Range const& _time, RemapValue<float, float> const& _distToMulTime); // thinking time

		public:
			Logic(MindInstance* _mind, LogicData const * _logicData);
			virtual ~Logic();

			MindInstance* get_mind() const { return mind; }
			Framework::IModulesOwner* get_imo() const;

			void update_rare_logic_advance(float _deltaTime);

			void set_advance_logic_rarely(Optional<Range> const & _rare);
			void set_advance_appearance_rarely(Optional<Range> const & _rare);
			void set_auto_rare_advance_if_not_visible(Optional<Range> const& _rare) { autoRareAdvanceIfNotVisible = _rare; }
			
		public: // to override_ by actual logics
			virtual void advance(float _deltaTime);

			virtual void learn_from(SimpleVariableStorage & _parameters);
			virtual void ready_to_activate(); // first thought in world, when placed but before initialised (async)

			virtual void end();

		public:
#ifdef AN_USE_AI_LOG
			LogInfoContext & access_log() { return log; }
			void set_state_info(int _idx, tchar const * const _text = nullptr, ...);
			void clear_state_info(int _idx);
#endif
			void debug_draw_state() const;
			virtual void debug_draw(Room* _room) const {}

			virtual void log_logic_info(LogInfoContext& _log) const;

		private:
			MindInstance* mind;
			Optional<Range> autoRareAdvanceIfNotVisible;
			Optional<Range> advanceLogicRarely;
			Optional<Range> advanceAppearanceRarely;
			LogicData const * logicData = nullptr;

#ifdef AN_USE_AI_LOG
			LogInfoContext log;
			Array<String> stateInfo;
#endif

#ifdef AN_DEVELOPMENT
		private:
			bool readiedToActivate = false;
			bool ended = false;
			bool requiresReadyToActivate = false;
		protected:
			void mark_requires_ready_to_activate() { requiresReadyToActivate = true; } // set it so we can catch it
#else
		protected:
			void mark_requires_ready_to_activate() {}
#endif
		};
	};
};

#include "aiLogic.inl"

DECLARE_REGISTERED_TYPE(Framework::AI::Logic*);
