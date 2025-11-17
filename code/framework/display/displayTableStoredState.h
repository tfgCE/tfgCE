#pragma once

namespace Framework
{
	struct DisplayTableStoredState
	{
		int rowTopIdx = 0;
		int rowSelectedIdx = 0;
		void* rowSelectedCustomData = nullptr;

		void clear()
		{
			*this = DisplayTableStoredState();
		}
	};
};
