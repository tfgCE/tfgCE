#include "stats.h"

#include "..\..\core\debug\debug.h"

using namespace Framework::Render;

#ifdef AN_RENDERER_STATS

Stats Stats::s_stats;

Stats::Stats()
{
	clear();
}

void Stats::clear()
{
	renderedScenes = 0;
	renderedRoomProxies = 0;
	renderedEnvironments = 0;
	renderedPresenceLinkProxies = 0;
	renderedDoorInRoomProxies = 0;
}

void Stats::log()
{
	ScopedOutputLock outputLock;
	output(TXT("renderer stats"));
	output(TXT("  %5i : scenes"), renderedScenes.get());
	output(TXT("  %5i : room proxies"), renderedRoomProxies.get());
	output(TXT("  %5i : environments"), renderedEnvironments.get());
	output(TXT("  %5i : presence link proxies"), renderedPresenceLinkProxies.get());
	output(TXT("  %5i : door in room proxies"), renderedDoorInRoomProxies.get());
}

#endif