#pragma once

struct LifetimeIndicator
{
	enum State
	{
		Alive = 0x370babe,	// elo babe
		Dead  = 0x15dead,	// is dead
	};
	int state = Alive;
	
	LifetimeIndicator(): state(Alive) {}
	~LifetimeIndicator() { state = Dead; }
	
	tchar const * to_char()
	{
		if (state == Alive) return TXT("alive");
		if (state == Dead) return TXT("dead");
		return TXT("mess - most likely dead");
	}
};
