#include "registeredGameScriptConditions.h"

#include "conditions/gsc_chance.h"
#include "conditions/gsc_door.h"
#include "conditions/gsc_input.h"
#include "conditions/gsc_logicOps.h"
#include "conditions/gsc_objectVar.h"
#include "conditions/gsc_objectVarsEqual.h"
#include "conditions/gsc_presence.h"
#include "conditions/gsc_random.h"
#include "conditions/gsc_relativeLocation.h"
#include "conditions/gsc_room.h"
#include "conditions/gsc_roomVar.h"
#include "conditions/gsc_timer.h"
#include "conditions/gsc_var.h"
#include "conditions/gsc_varsEqual.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

using namespace Framework;
using namespace GameScript;

//

RegisteredScriptConditions* RegisteredScriptConditions::s_conditions = nullptr;

void RegisteredScriptConditions::initialise_static()
{
	an_assert(s_conditions == nullptr);
	s_conditions = new RegisteredScriptConditions();

	// add conditions
	add(RegisteredScriptCondition(Name(TXT("and")), []() { return new Conditions::And(); }));
	add(RegisteredScriptCondition(Name(TXT("chance")), []() { return new Conditions::Chance(); }));
	add(RegisteredScriptCondition(Name(TXT("door")), []() { return new Conditions::Door(); }));
	add(RegisteredScriptCondition(Name(TXT("input")), []() { return new Conditions::Input(); }));
	add(RegisteredScriptCondition(Name(TXT("not")), []() { return new Conditions::Not(); }));
	add(RegisteredScriptCondition(Name(TXT("or")), []() { return new Conditions::Or(); }));
	add(RegisteredScriptCondition(Name(TXT("objectVar")), []() { return new Conditions::ObjectVar(); }));
	add(RegisteredScriptCondition(Name(TXT("objectVarsEqual")), []() { return new Conditions::ObjectVarsEqual(); }));
	add(RegisteredScriptCondition(Name(TXT("presence")), []() { return new Conditions::Presence(); }));
	add(RegisteredScriptCondition(Name(TXT("random")), []() { return new Conditions::Random(); }));
	add(RegisteredScriptCondition(Name(TXT("relativeLocation")), []() { return new Conditions::RelativeLocation(); }));
	add(RegisteredScriptCondition(Name(TXT("room")), []() { return new Conditions::Room(); }));
	add(RegisteredScriptCondition(Name(TXT("roomVar")), []() { return new Conditions::RoomVar(); }));
	add(RegisteredScriptCondition(Name(TXT("timer")), []() { return new Conditions::Timer(); }));
	add(RegisteredScriptCondition(Name(TXT("var")), []() { return new Conditions::Var(); }));
	add(RegisteredScriptCondition(Name(TXT("varsEqual")), []() { return new Conditions::VarsEqual(); }));
}

void RegisteredScriptConditions::close_static()
{
	delete_and_clear(s_conditions);
}

void RegisteredScriptConditions::add(RegisteredScriptCondition _tse)
{
	an_assert(s_conditions != nullptr);
	for_every(e, s_conditions->conditions)
	{
		if (e->name == _tse.name)
		{
			// replace
			*e = _tse;
			return;
		}
	}
	s_conditions->conditions.push_back(_tse);
}

ScriptCondition* RegisteredScriptConditions::create(tchar const* _tse)
{
	an_assert(s_conditions != nullptr);

	for_every(e, s_conditions->conditions)
	{
		if (e->name == _tse)
		{
			return e->create();
		}
	}

	error(TXT("not recognised game script condition \"%S\""), _tse);

	return nullptr;
}
