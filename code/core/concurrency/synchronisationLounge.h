#pragma once

namespace Concurrency
{
	struct SynchronisationLoungeImpl;

	class SynchronisationLounge
	{
	public:
		SynchronisationLounge();
		~SynchronisationLounge();

		bool rest(float _forTime = 0.0f); // wait for waking up (returns true if was woken up)
		bool rest_a_little(); // auto choose time to rest
		void wake_up_all(); // wake up all waiting

	private:
		SynchronisationLoungeImpl* impl;
	};

};
