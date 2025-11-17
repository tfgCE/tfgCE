#include "gse_soundOverridePlayParams.h"

#include "..\..\game\game.h"
#include "..\..\game\playerSetup.h"
#include "..\..\music\musicPlayer.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\sound\soundScene.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool Elements::SoundOverridePlayParams::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute_or_from_child(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	sound = _node->get_name_attribute_or_from_child(TXT("sound"), sound);
	sample.load_from_xml(_node, TXT("sample"), _lc);
	playParams.load_from_xml(_node, _lc);

	clear = _node->get_bool_attribute_or_from_child_presence(TXT("clear"), clear);
	clearAll = _node->get_bool_attribute_or_from_child_presence(TXT("clearAll"), clearAll);
	allAttached = _node->get_bool_attribute_or_from_child_presence(TXT("allAttached"), allAttached);

	return result;
}

bool Elements::SoundOverridePlayParams::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= sample.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::SoundOverridePlayParams::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				apply_to(imo);
			}
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}

void Elements::SoundOverridePlayParams::apply_to(Framework::IModulesOwner* imo) const
{
	if (auto* s = imo->get_sound())
	{
		if (clearAll)
		{
			s->clear_play_params_overrides();
		}
		else if (clear)
		{
			if (sound.is_valid()) s->clear_play_params_overrides(sound);
			if (auto* ss = sample.get()) s->clear_play_params_overrides(ss->get_name());
		}
		else
		{
			if (sound.is_valid()) s->set_play_params_overrides(sound, playParams);
			if (auto* ss = sample.get()) s->set_play_params_overrides(ss->get_name(), playParams);
		}
	}

	if (allAttached)
	{
		if (auto* p = imo->get_presence())
		{
			Concurrency::ScopedSpinLock lock(p->access_attached_lock(), true);
			for_every_ptr(aimo, p->get_attached())
			{
				apply_to(aimo);
			}
		}
	}
}