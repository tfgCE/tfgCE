#include "aiLogics.h"

using namespace Framework;
using namespace AI;

Logics* Logics::s_logics = nullptr;

void Logics::initialise_static()
{
	an_assert(s_logics == nullptr);
	s_logics = new Logics();
}

void Logics::close_static()
{
	an_assert(s_logics != nullptr);
	delete_and_clear(s_logics);
}

void Logics::register_ai_logic(tchar const * _name, CREATE_AI_LOGIC_FUNCTION(_create_ai_logic), CREATE_AI_LOGIC_DATA_FUNCTION(_create_ai_logic_data))
{
	an_assert(s_logics != nullptr);
	Name name = Name(_name);
	if (RegisteredLogic** logic = s_logics->registeredLogics.get_existing(name))
	{
		delete *logic;
	}
	s_logics->registeredLogics[name] = new RegisteredLogic(name, _create_ai_logic, _create_ai_logic_data);
}

void Logics::register_ai_logic_latent_task(tchar const * _name, LatentFunction _function)
{
	an_assert(s_logics != nullptr);
	Name name = Name(_name);
	s_logics->registeredLatentTasks[name] = _function;
}


Logic* Logics::create(Name const & _name, ::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
{
	an_assert(s_logics != nullptr);
	if (RegisteredLogic** logic = s_logics->registeredLogics.get_existing(_name))
	{
		return (*logic)->create_ai_logic(_mind, _logicData);
	}
	else
	{
		error(TXT("could not find logic \"%S\""), _name.to_char());
		return nullptr;
	}
}

LogicData* Logics::create_data(Name const & _name)
{
	an_assert(s_logics != nullptr);
	if (RegisteredLogic** logic = s_logics->registeredLogics.get_existing(_name))
	{
		return (*logic)->create_ai_logic_data();
	}
	else if (_name.is_valid())
	{
		error(TXT("could not find logic \"%S\""), _name.to_char());
		return nullptr;
	}
	else
	{
		return nullptr;
	}
}

LatentFunction Logics::get_latent_task(Name const & _name, LatentFunction _default)
{
	an_assert(s_logics != nullptr);
	if (LatentFunction* function = s_logics->registeredLatentTasks.get_existing(_name))
	{
		return *function;
	}
	else if (_default)
	{
		return _default;
	}
	else if (_name.is_valid())
	{
		error(TXT("could not find latent task \"%S\""), _name.to_char());
		return nullptr;
	}
	else
	{
		return nullptr;
	}
}

Logics::~Logics()
{
	for_every_ptr(logic, registeredLogics)
	{
		delete logic;
	}
	registeredLogics.clear();
	registeredLatentTasks.clear();
}
