#include "registeredGameScriptConditions.h"

#include "conditions/gsc_compareCellCoord.h"
#include "conditions/gsc_gameDefinition.h"
#include "conditions/gsc_gameDirector.h"
#include "conditions/gsc_gameModifiers.h"
#include "conditions/gsc_gameplaySpecial.h"
#include "conditions/gsc_gameSettings.h"
#include "conditions/gsc_health.h"
#include "conditions/gsc_isDemo.h"
#include "conditions/gsc_lastMissionResult.h"
#include "conditions/gsc_missionState.h"
#include "conditions/gsc_movementScripted.h"
#include "conditions/gsc_musicPlayer.h"
#include "conditions/gsc_overlayInfoSystem.h"
#include "conditions/gsc_persistence.h"
#include "conditions/gsc_pilgrim.h"
#include "conditions/gsc_pilgrimage.h"
#include "conditions/gsc_pilgrimOverlayInfo.h"
#include "conditions/gsc_playerPreferences.h"
#include "conditions/gsc_story.h"
#include "conditions/gsc_unloader.h"
#include "conditions/gsc_voiceoverSpeaking.h"

#include "..\pilgrimage\pilgrimage.h"

#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

#include "..\..\framework\gameScript\registeredGameScriptConditions.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;

//

void RegisteredScriptConditions::initialise_static()
{
	// add conditions
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("compareCellCoord")), []() { return new Conditions::CompareCellCoord(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("gameDefinition")), []() { return new Conditions::GameDefinition(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("gameDirector")), []() { return new Conditions::GameDirector(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("gameModifiers")), []() { return new Conditions::GameModifiers(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("gameplaySpecial")), []() { return new Conditions::GameplaySpecial(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("gameSettings")), []() { return new Conditions::GameSettings(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("health")), []() { return new Conditions::Health(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("isDemo")), []() { return new Conditions::IsDemo(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("lastMissionResult")), []() { return new Conditions::LastMissionResult(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("missionState")), []() { return new Conditions::MissionState(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("movementScripted")), []() { return new Conditions::MovementScripted(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("music")), []() { return new Conditions::MusicPlayer(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("musicPlayer")), []() { return new Conditions::MusicPlayer(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("overlayInfoSystem")), []() { return new Conditions::OverlayInfoSystem(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("persistence")), []() { return new Conditions::Persistence(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("pilgrim")), []() { return new Conditions::Pilgrim(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("pilgrimage")), []() { return new Conditions::Pilgrimage(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("pilgrimOverlayInfo")), []() { return new Conditions::PilgrimOverlayInfo(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("playerPreferences")), []() { return new Conditions::PlayerPreferences(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("story")), []() { return new Conditions::Story(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("unloader")), []() { return new Conditions::Unloader(); }));
	Framework::GameScript::RegisteredScriptConditions::add(Framework::GameScript::RegisteredScriptCondition(Name(TXT("voiceoverSpeaking")), []() { return new Conditions::VoiceoverSpeaking(); }));
}

void RegisteredScriptConditions::close_static()
{
}
