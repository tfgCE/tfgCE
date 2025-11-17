#pragma once

namespace Concurrency
{
	struct GateImpl;

	/*
	 *	"allow one" - allows one to pass through
	 *	"wait" - wait for allowance to go through (always have to wait)
	 */
	class Gate
	{
	public:
		Gate();
		~Gate();

		void allow_one();
		void wait();
		bool wait_for(float _time); // returns true if waited fine

	private:
		GateImpl* impl;
	};

};
