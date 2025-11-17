#include "..\module\modulePresence.h"

template <typename Class>
Class* Framework::IModulesOwner::get_gameplay_for_attached_to()
{
	auto* goUp = this;
	while (goUp)
	{
		if (auto* g = goUp->get_gameplay_as<Class>())
		{
			return g;
		}
		if (auto* p = goUp->get_presence())
		{
			goUp = p->get_attached_to();
		}
		else
		{
			break;
		}
	}
	return nullptr;
}

template <typename Class>
Class* Framework::IModulesOwner::get_custom_for_attached_to()
{
	auto* goUp = this;
	while (goUp)
	{
		if (auto* c = goUp->get_custom<Class>())
		{
			return c;
		}
		if (auto* p = goUp->get_presence())
		{
			goUp = p->get_attached_to();
		}
		else
		{
			break;
		}
	}
	return nullptr;
}

template <typename Class>
Class* Framework::IModulesOwner::get_custom() const
{
	for_every_ptr(custom, customs)
	{
		if (Class* found = fast_cast<Class>(custom))
		{
			return found;
		}
	}
	return nullptr;
}

template <typename Class>
void Framework::IModulesOwner::for_all_custom(std::function<void(Class*)> _do) const
{
	for_every_ptr(custom, customs)
	{
		if (Class* found = fast_cast<Class>(custom))
		{
			_do(found);
		}
	}
}
