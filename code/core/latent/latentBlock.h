#pragma once

#include "..\types\name.h"
#include "..\memory\pooledObject.h"

namespace Latent
{
	class Frame;

	struct Block
	: public PooledObject<Block>
	{
	public:
		void set_state(int _state) { currentState = _state; }
		int get_state() const { return currentState; }

		void mark_has_finished() { hasFinished = true; }
		bool has_finished() const { return hasFinished; }

	private: friend class Frame;
		void start(REF_ Frame & _frame);
		void end(REF_ Frame & _frame);

	private:
		int variablesCountBefore; // count of variables before this block
		int currentState;
		bool hasFinished;
	};
};
