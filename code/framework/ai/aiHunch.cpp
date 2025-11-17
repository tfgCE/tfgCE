#include "aiHunch.h"

#include "..\interfaces\aiObject.h"

#include "..\world\world.h"

using namespace Framework;
using namespace AI;

Hunch::Hunch()
{
}

void Hunch::on_get()
{
	base::on_get();
	name = Name::invalid();
}

void Hunch::on_release()
{
	base::on_release();
	params.clear();
	expireBy.clear();
	expired = false;
	consumed = false;
}

Param & Hunch::access_param(Name const & _name)
{
	return params.access(_name);
}

Param const * Hunch::get_param(Name const & _name) const
{
	return params.get(_name);
}

void Hunch::update_world_time(float _worldTime)
{
	if (expireBy.is_set() &&
		expireBy.get() < _worldTime)
	{
		expired = true;
	}
}
