#pragma once

#include "..\..\core\globalSettings.h"
#include "..\..\core\concurrency\counter.h"

namespace Framework
{
	namespace Render
	{
#ifdef AN_RENDERER_STATS
		struct Stats
		{
		public:
			Concurrency::Counter renderedScenes;
			Concurrency::Counter renderedRoomProxies;
			Concurrency::Counter renderedEnvironments;
			Concurrency::Counter renderedPresenceLinkProxies;
			Concurrency::Counter renderedDoorInRoomProxies;

			Stats();

			void clear();
			void log();

			static Stats & get() { return s_stats; }

		private:
			static Stats s_stats;
		};
#endif

	};
};
