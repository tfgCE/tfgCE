#pragma once

#include "..\globalDefinitions.h"

namespace Concurrency
{
	class ThreadSystemUtils
	{
	public:
		static int32 get_current_thread_system_id();
		static int32 get_number_of_cores(bool _allAvailable = false); // all available set to false is using not forced by config number of cores

		static void yield();
	};

};
