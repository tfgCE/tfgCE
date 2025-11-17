#pragma once

#include "latentFrame.h"

namespace Latent
{
	struct Block;

	struct Scope
	{
	public:
		Scope(Frame & _frame);
		~Scope();

		Block & append_block();

		bool is_last_block() const;

		template <typename Class>
		Class & next_var(tchar const * _name);

		template <typename Class>
		Class & next_existing_param(tchar const * _name);

		template <typename Class>
		Class const * next_optional_param(tchar const * _name);

		template <typename Class>
		void next_function_param(Class const & _value);

		void end_params();

		void mark_has_finished();
		bool has_finished() const;

		void set_current_state(int _state);
		int get_current_state() const;

		inline float get_delta_time() const { return frame.get_delta_time(); }
		inline void decrease_delta_time(float _by) { return frame.decrease_delta_time(_by); }

	private:
		Frame & frame;
		Block & block;

#ifdef AN_INSPECT_LATENT
	public:
		void add_info(tchar const * _info);
#endif

	public:
		int doOnceHack = 0; /* this is a hack for avoiding dangling else, it's always set to 1 and then immediately back to 0 */ \
	};

	#include "latentScope.inl"
};
