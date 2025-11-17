#include "loaderSequence.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;

//

void Sequence::add(ILoader* _loader, Optional<float> const& _time, std::function<bool()> _should_show, Optional<bool> const & _passDeactivationThrough)
{
	Element e;
	e.loader = _loader;
	e.timeLeft = _time;
	e.should_show = _should_show;
	e.passDeactivationThrough = _passDeactivationThrough.get(false);
	loaders.push_back(e);
}

void Sequence::add(std::function<void()> _perform)
{
	Element e;
	e.perform = _perform;
	loaders.push_back(e);
}

void Sequence::do_perform(int _leaveAmount)
{
	while (loaders.get_size() > _leaveAmount)
	{
		auto& l = loaders.get_first();
		if (l.perform)
		{
			l.perform();
			loaders.remove_at(0);
		}
		else
		{
			break;
		}
	}
}

void Sequence::update(float _deltaTime)
{
	do_perform();
	if (loaders.is_empty())
	{
		base::deactivate();
		return;
	}
	if (loaders.get_size() == 1 &&
		shouldDeactivate)
	{
		auto& l = loaders.get_first();
		if (l.loader)
		{
			l.loader->deactivate();
			if (!l.loader->is_active())
			{
				base::deactivate();
				return;
			}
		}
		else
		{
			base::deactivate();
			return;
		}
	}
	else if (loaders.get_size() > 1)
	{
		auto& l = loaders.get_first();
		if (shouldDeactivate && l.passDeactivationThrough && l.loader)
		{
			l.loader->deactivate();
		}
		// first one is activated in ::activate()
		if (l.timeLeft.is_set())
		{
			l.timeLeft = l.timeLeft.get() - _deltaTime;
			if (l.timeLeft.get() <= 0.0f)
			{
				if (l.loader)
				{
					l.loader->deactivate();
				}
			}
		}
		if (!l.loader || ! l.loader->is_active())
		{
			while (! loaders.is_empty())
			{
				loaders.remove_at(0);
				if (!loaders.is_empty())
				{
					do_perform();
					if (! loaders.is_empty() &&
						loaders.get_first().should_show &&
						!loaders.get_first().should_show())
					{
						continue;
					}
				}
				break;
			}
			if (! loaders.is_empty() &&
				loaders.get_first().loader)
			{
				loaders.get_first().loader->activate();
				validLoaderForDisplay = loaders.get_first().loader;
			}
		}
	}
	if (!loaders.is_empty())
	{
		auto& l = loaders.get_first();
		if (l.loader)
		{
			l.loader->update(_deltaTime);
		}
	}
}

void Sequence::display(System::Video3D * _v3d, bool _vr)
{
	if (validLoaderForDisplay)
	{
		validLoaderForDisplay->display(_v3d, _vr);
	}
}

bool Sequence::activate()
{
	bool wasActivated = base::activate();

	while (!loaders.is_empty())
	{
		do_perform();
		if (!loaders.is_empty() &&
			loaders.get_first().should_show &&
			!loaders.get_first().should_show())
		{
			loaders.remove_at(0);
			continue;
		}
		break;
	}
	if (!loaders.is_empty() &&
		loaders.get_first().loader)
	{
		loaders.get_first().loader->activate();
		validLoaderForDisplay = loaders.get_first().loader;
	}

	return wasActivated;
}

void Sequence::deactivate()
{
	shouldDeactivate = true;
}
