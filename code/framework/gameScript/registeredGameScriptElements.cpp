#include "registeredGameScriptElements.h"

#include "elements/gse_aiMessage.h"
#include "elements/gse_appearance.h"
#include "elements/gse_blendObjectVar.h"
#include "elements/gse_blendRoomVar.h"
#include "elements/gse_ceaseToExist.h"
#include "elements/gse_changeScript.h"
#include "elements/gse_collision.h"
#include "elements/gse_controller.h"
#include "elements/gse_copyObjectVar.h"
#include "elements/gse_copyVar.h"
#include "elements/gse_derandomizeVar.h"
#include "elements/gse_devPause.h"
#include "elements/gse_door.h"
#include "elements/gse_endScript.h"
#include "elements/gse_find.h"
#include "elements/gse_getObjectVar.h"
#include "elements/gse_goToLabel.h"
#include "elements/gse_goToRandomLabel.h"
#include "elements/gse_goSubLabel.h"
#include "elements/gse_info.h"
#include "elements/gse_label.h"
#include "elements/gse_lightningSpawner.h"
#include "elements/gse_lightSource.h"
#include "elements/gse_locomotion.h"
#include "elements/gse_movementControl.h"
#include "elements/gse_movementScripted.h"
#include "elements/gse_output.h"
#include "elements/gse_physicalSensation.h"
#include "elements/gse_presence.h"
#include "elements/gse_presenceForceDisplacementVRAnchor.h"
#include "elements/gse_resetObjectVar.h"
#include "elements/gse_returnSub.h"
#include "elements/gse_roomAppearance.h"
#include "elements/gse_setObjectVar.h"
#include "elements/gse_setPresencePath.h"
#include "elements/gse_setRoomVar.h"
#include "elements/gse_sound.h"
#include "elements/gse_startSubScript.h"
#include "elements/gse_stopSubScript.h"
#include "elements/gse_system.h"
#include "elements/gse_tagObjectVar.h"
#include "elements/gse_teleport.h"
#include "elements/gse_teleportAllUsingVRAnchor.h"
#include "elements/gse_temporaryObject.h"
#include "elements/gse_timer.h"
#include "elements/gse_trap.h"
#include "elements/gse_trapSub.h"
#include "elements/gse_triggerTrap.h"
#include "elements/gse_var.h"
#include "elements/gse_wait.h"
#include "elements/gse_waitForCondition.h"
#include "elements/gse_waitForObjectCease.h"
#include "elements/gse_yield.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

using namespace Framework;
using namespace GameScript;

//

RegisteredScriptElements* RegisteredScriptElements::s_elements = nullptr;

void RegisteredScriptElements::initialise_static()
{
	an_assert(s_elements == nullptr);
	s_elements = new RegisteredScriptElements();

	// add elements
	add(RegisteredScriptElement(Name(TXT("aiMessage")), []() { return new Elements::AIMessage(); }));
	add(RegisteredScriptElement(Name(TXT("appearance")), []() { return new Elements::Appearance(); }));
	add(RegisteredScriptElement(Name(TXT("blendObjectVar")), []() { return new Elements::BlendObjectVar(); }));
	add(RegisteredScriptElement(Name(TXT("blendRoomVar")), []() { return new Elements::BlendRoomVar(); }));
	add(RegisteredScriptElement(Name(TXT("ceaseToExist")), []() { return new Elements::CeaseToExist(); }));
	add(RegisteredScriptElement(Name(TXT("changeScript")), []() { return new Elements::ChangeScript(); }));
	add(RegisteredScriptElement(Name(TXT("collision")), []() { return new Elements::Collision(); }));
	add(RegisteredScriptElement(Name(TXT("controller")), []() { return new Elements::Controller(); }));
	add(RegisteredScriptElement(Name(TXT("copyObjectVar")), []() { return new Elements::CopyObjectVar(); }));
	add(RegisteredScriptElement(Name(TXT("copyVar")), []() { return new Elements::CopyVar(); }));
	add(RegisteredScriptElement(Name(TXT("derandomizeVar")), []() { return new Elements::DerandomizeVar(); }));
	add(RegisteredScriptElement(Name(TXT("devPause")), []() { return new Elements::DevPause(); }));
	add(RegisteredScriptElement(Name(TXT("door")), []() { return new Elements::Door(); }));
	add(RegisteredScriptElement(Name(TXT("endScript")), []() { return new Elements::EndScript(); }));
	add(RegisteredScriptElement(Name(TXT("find")), []() { return new Elements::Find(); }));
	add(RegisteredScriptElement(Name(TXT("getObjectVar")), []() { return new Elements::GetObjectVar(); }));
	add(RegisteredScriptElement(Name(TXT("goToLabel")), []() { return new Elements::GoToLabel(); }));
	add(RegisteredScriptElement(Name(TXT("goToRandomLabel")), []() { return new Elements::GoToRandomLabel(); }));
	add(RegisteredScriptElement(Name(TXT("goSubLabel")), []() { return new Elements::GoSubLabel(); }));
	add(RegisteredScriptElement(Name(TXT("info")), []() { return new Elements::Info(); }));
	add(RegisteredScriptElement(Name(TXT("label")), []() { return new Elements::Label(); }));
	add(RegisteredScriptElement(Name(TXT("lightningSpawner")), []() { return new Elements::LightningSpawner(); }));
	add(RegisteredScriptElement(Name(TXT("lightSource")), []() { return new Elements::LightSource(); }));
	add(RegisteredScriptElement(Name(TXT("locomotion")), []() { return new Elements::Locomotion(); }));
	add(RegisteredScriptElement(Name(TXT("movementControl")), []() { return new Elements::MovementControl(); }));
	add(RegisteredScriptElement(Name(TXT("movementScripted")), []() { return new Elements::MovementScripted(); }));
	add(RegisteredScriptElement(Name(TXT("output")), []() { return new Elements::Output(); }));
	add(RegisteredScriptElement(Name(TXT("physicalSensation")), []() { return new Elements::PhysicalSensation(); }));
	add(RegisteredScriptElement(Name(TXT("presence")), []() { return new Elements::Presence(); }));
	add(RegisteredScriptElement(Name(TXT("presenceForceDisplacementVRAnchor")), []() { return new Elements::PresenceForceDisplacementVRAnchor(); }));
	add(RegisteredScriptElement(Name(TXT("resetObjectVar")), []() { return new Elements::ResetObjectVar(); }));
	add(RegisteredScriptElement(Name(TXT("returnSub")), []() { return new Elements::ReturnSub(); }));
	add(RegisteredScriptElement(Name(TXT("roomAppearance")), []() { return new Elements::RoomAppearance(); }));
	add(RegisteredScriptElement(Name(TXT("setObjectVar")), []() { return new Elements::SetObjectVar(); }));
	add(RegisteredScriptElement(Name(TXT("setPresencePath")), []() { return new Elements::SetPresencePath(); }));
	add(RegisteredScriptElement(Name(TXT("setRoomVar")), []() { return new Elements::SetRoomVar(); }));
	add(RegisteredScriptElement(Name(TXT("sound")), []() { return new Elements::Sound(); }));
	add(RegisteredScriptElement(Name(TXT("startSubScript")), []() { return new Elements::StartSubScript(); }));
	add(RegisteredScriptElement(Name(TXT("stopSubScript")), []() { return new Elements::StopSubScript(); }));
	add(RegisteredScriptElement(Name(TXT("system")), []() { return new Elements::System(); }));
	add(RegisteredScriptElement(Name(TXT("tagObjectVar")), []() { return new Elements::TagObjectVar(); }));
	add(RegisteredScriptElement(Name(TXT("teleport")), []() { return new Elements::Teleport(); }));
	add(RegisteredScriptElement(Name(TXT("teleportAllUsingVRAnchor")), []() { return new Elements::TeleportAllUsingVRAnchor(); }));
	add(RegisteredScriptElement(Name(TXT("temporaryObject")), []() { return new Elements::TemporaryObject(); }));
	add(RegisteredScriptElement(Name(TXT("timer")), []() { return new Elements::Timer(); }));
	add(RegisteredScriptElement(Name(TXT("trap")), []() { return new Elements::Trap(); }));
	add(RegisteredScriptElement(Name(TXT("trapSub")), []() { return new Elements::TrapSub(); }));
	add(RegisteredScriptElement(Name(TXT("triggerTrap")), []() { return new Elements::TriggerTrap(); }));
	add(RegisteredScriptElement(Name(TXT("var")), []() { return new Elements::Var(); }));
	add(RegisteredScriptElement(Name(TXT("wait")), []() { return new Elements::Wait(); }));
	add(RegisteredScriptElement(Name(TXT("waitForCondition")), []() { return new Elements::WaitForCondition(); }));
	add(RegisteredScriptElement(Name(TXT("waitForObjectToCease")), []() { return new Elements::WaitForObjectToCease(); }));
	add(RegisteredScriptElement(Name(TXT("yield")), []() { return new Elements::GSEYield(); }));
}

void RegisteredScriptElements::close_static()
{
	delete_and_clear(s_elements);
}

void RegisteredScriptElements::add(RegisteredScriptElement _tse)
{
	an_assert(s_elements != nullptr);
	for_every(e, s_elements->elements)
	{
		if (e->name == _tse.name)
		{
			// replace
			*e = _tse;
			return;
		}
	}
	s_elements->elements.push_back(_tse);
}

ScriptElement * RegisteredScriptElements::create(tchar const* _tse)
{
	an_assert(s_elements != nullptr);

	for_every(e, s_elements->elements)
	{
		if (e->name == _tse)
		{
			return e->create();
		}
	}

	error(TXT("not recognised game script element \"%S\""), _tse);

	return nullptr;
}
