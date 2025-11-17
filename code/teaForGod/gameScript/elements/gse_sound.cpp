#include "gse_sound.h"

#include "..\..\game\game.h"

#include "..\..\library\library.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\sound\soundScene.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool Elements::Sound::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	dullness.load_from_xml(_node, TXT("dullness"));
	dullnessBlendTime.load_from_xml(_node, TXT("dullnessBlendTime"));

	extraPlayback = _node->get_bool_attribute_or_from_child_presence(TXT("extraPlayback"), extraPlayback);
	extraPlaybackId = _node->get_name_attribute(TXT("extraPlaybackId"), extraPlaybackId);
	extraPlaybackStartAt.load_from_xml(_node, TXT("extraPlaybackStartAt"));
	stopExtraPlaybackId = _node->get_name_attribute(TXT("stopExtraPlaybackId"), stopExtraPlaybackId);
	if (extraPlaybackId.is_valid())
	{
		extraPlayback = true;
	}

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::Sound::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	bool extraPlaybackHandled = false;
	if (stopExtraPlaybackId.is_valid())
	{
		if (auto* game = Game::get_as<Game>())
		{
			if (auto* ss = game->access_sound_scene())
			{
				ss->stop_extra_playback(stopExtraPlaybackId, fadeOutOnStop);
				extraPlaybackHandled = true;
			}
		}
	}
	if (dullness.is_set())
	{
		if (auto* game = Game::get_as<Game>())
		{
			game->set_scripted_sound_dullness(dullness.get(), dullnessBlendTime.get(0.5f));
		}
	}

	if (extraPlayback)
	{
		if (playAllSamples)
		{
			for_every_ref(sam, sample.samples)
			{
				auto sp = sam->play();
				if (extraPlaybackStartAt.is_set())
				{
					sp.set_at(extraPlaybackStartAt.get());
				}
				if (auto* game = Game::get_as<Game>())
				{
					if (auto* ss = game->access_sound_scene())
					{
						ss->add_extra_playback(sp, extraPlaybackId, sam->get_fade_out_time());
						extraPlaybackHandled = true;
					}
				}
			}
		}
		else if (!sample.samples.is_empty())
		{
			int idx = Random::get_int(sample.samples.get_size());
			if (auto* sam = sample.samples[idx].get())
			{
				auto sp = sam->play();
				if (auto* game = Game::get_as<Game>())
				{
					if (auto* ss = game->access_sound_scene())
					{
						ss->add_extra_playback(sp, extraPlaybackId, sam->get_fade_out_time());
						extraPlaybackHandled = true;
					}
				}
			}
		}
	}
	if (!extraPlaybackHandled)
	{
		return base::execute(_execution, _flags);
	}
	
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
