#include "voiceoverSystem.h"

#include "..\teaForGodTest.h"

#include "..\game\game.h"
#include "..\pilgrimage\pilgrimage.h"
#include "..\pilgrimage\pilgrimageInstance.h"

#include "..\..\core\debug\debug.h"
#include "..\..\core\system\sound\soundSystem.h"
#include "..\..\core\mainSettings.h"

#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\sound\soundSample.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define LOG_VOICEOVER_SYSTEM
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define TEST_VOICEOVER_SYSTEM
#ifdef TEST_VOICEOVER_SYSTEM
#include "..\..\core\system\input.h"
#include "..\..\framework\library\library.h"
#endif
#endif

//

using namespace TeaForGodEmperor;

//

// voiceover actors
DEFINE_STATIC_NAME(narrator);
DEFINE_STATIC_NAME(pilgrimOverlayInfo);

//

VoiceoverSystem* VoiceoverSystem::s_system = nullptr;

VoiceoverSystem::VoiceoverSystem(bool _additional)
{
	if (!_additional)
	{
		an_assert(!s_system, TXT("consider changing voiceover system into additional"));
		s_system = this;
	}
}

VoiceoverSystem::~VoiceoverSystem()
{
	silence();
	if (s_system == this)
	{
		s_system = nullptr;
	}
}

void VoiceoverSystem::pause()
{
	if (isPaused)
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(accessLock);
	isPaused = true;
	for_every(a, actors)
	{
		for_every(p, a->playbacks)
		{
			p->playback.pause();
		}
	}
}

void VoiceoverSystem::resume()
{
	if (! isPaused)
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(accessLock);
	isPaused = false;
	for_every(a, actors)
	{
		for_every(p, a->playbacks)
		{
			p->playback.resume_with_fade_in();
		}
	}
}

void VoiceoverSystem::silence(Name const& _actor, Optional<float> const& _fadeOut)
{
#ifdef LOG_VOICEOVER_SYSTEM
	output(TXT("[VoiceoverSystem%S] silence %S %.3f"), this == s_system ? TXT("-main") : TXT("-add"), _actor.to_char(), _fadeOut.get(0.0f));
#endif
	Concurrency::ScopedSpinLock lock(accessLock);
	isPaused = false;
	for_every(a, actors)
	{
		if (a->actor == _actor)
		{
			for_every(p, a->playbacks)
			{
				silence(*p, _fadeOut);
			}
		}
	}
}

void VoiceoverSystem::silence(Optional<float> const& _fadeOut, Optional<Name> const& _exceptOfActor)
{
#ifdef LOG_VOICEOVER_SYSTEM
	output(TXT("[VoiceoverSystem%S] silence %.3f %S%S"), this == s_system ? TXT("-main") : TXT("-add"), _fadeOut.get(0.0f), _exceptOfActor.is_set() ? TXT("except ") : TXT(""), _exceptOfActor.is_set()? _exceptOfActor.get().to_char() : TXT(""));
#endif
	Concurrency::ScopedSpinLock lock(accessLock);
	isPaused = false;
	for_every(a, actors)
	{
		if (_exceptOfActor.is_set() && a->actor == _exceptOfActor.get())
		{
			continue;
		}
		for_every(p, a->playbacks)
		{
			silence(*p, _fadeOut);
		}
	}
}

void VoiceoverSystem::silence(Optional<float> const& _fadeOut, Array<Name> const& _exceptOfActors)
{
#ifdef LOG_VOICEOVER_SYSTEM
	String except;
	for_every(e, _exceptOfActors)
	{
		except += String::space();
		except += e->to_string();
	}
	output(TXT("[VoiceoverSystem%S] silence %.3f %S"), this == s_system ? TXT("-main") : TXT("-add"), _fadeOut.get(0.0f), except.trim().to_char());
#endif
	Concurrency::ScopedSpinLock lock(accessLock);
	isPaused = false;
	for_every(a, actors)
	{
		if (_exceptOfActors.does_contain(a->actor))
		{
			continue;
		}
		for_every(p, a->playbacks)
		{
			silence(*p, _fadeOut);
		}
	}
}

float VoiceoverSystem::get_time_to_end_speaking(Name const& _actor, Framework::Sample* _line) const
{
	Concurrency::ScopedSpinLock lock(accessLock);
	for_every(a, actors)
	{
		if (a->actor == _actor)
		{
			for_every(p, a->playbacks)
			{
				if (!_line || p->line == _line)
				{
					if (p->line && p->playback.is_playing())
					{
						if (p->playback.get_sample())
						{
							return max(0.0f, p->playback.get_sample()->get_length() - p->playback.get_at());
						}
					}
					if (!p->line)
					{
						return max(0.0f, p->timeLeft);
					}
				}
			}
		}
	}
	return 0.0f;
}

bool VoiceoverSystem::is_speaking(Name const& _actor, Framework::Sample* _line) const
{
	Concurrency::ScopedSpinLock lock(accessLock);
	for_every(a, actors)
	{
		if (a->actor == _actor)
		{
			for_every(p, a->playbacks)
			{
				if ((!_line || p->line == _line) &&
					((p->line && p->playback.is_playing() &&
					  ! p->playback.is_fading_out_or_faded_out()) ||
					 (! p->line && p->timeLeft > 0.0f)))
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool VoiceoverSystem::is_speaking(Name const& _actor, String const& _lineText) const
{
	Concurrency::ScopedSpinLock lock(accessLock);
	for_every(a, actors)
	{
		if (a->actor == _actor)
		{
			for_every(p, a->playbacks)
			{
				if (p->lineText == _lineText &&
					! p->line && p->timeLeft > 0.0f)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool VoiceoverSystem::is_anyone_speaking() const
{
	Concurrency::ScopedSpinLock lock(accessLock);
	for_every(a, actors)
	{
		for_every(p, a->playbacks)
		{
			if ((p->line && p->playback.is_playing() &&
				!p->playback.is_fading_out_or_faded_out()) ||
				(!p->line && p->timeLeft > 0.0f))
			{
				return true;
			}
		}
	}
	return false;
}

bool VoiceoverSystem::is_any_subtitle_active() const
{
	Concurrency::ScopedSpinLock lock(accessLock);
	for_every(a, actors)
	{
		for_every(p, a->playbacks)
		{
			if ((p->line && p->playback.is_playing() &&
				!p->playback.is_fading_out_or_faded_out()) ||
				(!p->line && p->timeLeft > 0.0f))
			{
				if (p->autoSubtitles &&
					(!p->line || p->lineTextCache.get_range(p->lineText).does_contain(p->at)))
				{
					return true;
				}
			}
		}
	}
	return false;
}

void VoiceoverSystem::speak(Name const& _actor, Optional<int> const& _audioDuck, Framework::Sample* _line, bool _autoSubtitles, Optional<float> const& _volume)
{
	if (!_line)
	{
		return;
	}
	speak(_actor, _line, String::empty(), _audioDuck.get(1), _autoSubtitles, _volume);
}

void VoiceoverSystem::speak(Name const& _actor, Optional<int> const& _audioDuck, String const& _lineText, Optional<float> const& _volume)
{
	speak(_actor, nullptr, _lineText, _audioDuck.get(1), true, _volume);
}

void VoiceoverSystem::speak(Name const& _actor, Framework::Sample* _line, String const& _lineText, int _audioDuck, bool _autoSubtitles, Optional<float> const& _volume)
{
	if (!PlayerPreferences::should_play_narrative_voiceovers() &&
		_actor == NAME(narrator))
	{
#ifdef LOG_VOICEOVER_SYSTEM
		output(TXT("[VoiceoverSystem%S] skip speaking (%S) %S"), this == s_system ? TXT("-main") : TXT("-add"), _actor.to_char(), _line ? _line->get_name().to_string().to_char() : _lineText.to_char());
#endif
		return;
	}

	float volume = 1.0f;
	if (!PlayerPreferences::should_play_pilgrim_overlay_voiceovers() &&
		_actor == NAME(pilgrimOverlayInfo))
	{
		// we still want to "play" it to make the subtitles visible
		volume = 0.0f;
	}

	volume = _volume.get(1.0f) * volume;

	if (_actor == NAME(pilgrimOverlayInfo) &&
		is_speaking(NAME(narrator)))
	{
		// make it more quiet as we don't want to overlap with narrator too much
		volume *= 0.3f;
	}

#ifdef LOG_VOICEOVER_SYSTEM
	output(TXT("[VoiceoverSystem%S] speak (%S) %S"), this == s_system ? TXT("-main") : TXT("-add"), _actor.to_char(), _line ? _line->get_name().to_string().to_char() : _lineText.to_char());
#endif

	float fadeTime = 0.0f;
	if (_line)
	{
		fadeTime = _line->get_fade_in_time();
	}
	Concurrency::ScopedSpinLock lock(accessLock);
	Actor* actor = nullptr;
	for_every(a, actors)
	{
		if (a->actor == _actor)
		{
			actor = a;
			break;
		}
	}
	if (!actor)
	{
		actors.push_back(Actor());
		actor = &actors.get_last();
		actor->actor = _actor;
	}
	if (_actor.is_valid())
	{
		for_every(p, actor->playbacks)
		{
			silence(*p, 0.1f);
		}
	}
	{
		actor->playbacks.push_front(Playback());
		Playback & p = actor->playbacks.get_first();
		p.line = _line;
		p.lineText = ! _lineText.is_empty()? _lineText : (_line? _line->get_text() : String::empty());
		p.audioDuck = _audioDuck;
		p.autoSubtitles = _autoSubtitles;
		if (_line)
		{
			p.playback = _line->play(fadeTime, ::Sound::PlaybackSetup().with_own_fading());
			p.playback.set_volume(volume); // make it silent
			p.at = p.playback.get_at();
		}
		else
		{
			p.timeLeft = max(1.2f, 0.3f + 0.06f * (float)(p.lineText.get_length()));
			p.at = 0.0f;
		}
		if (subtitleSystem && _autoSubtitles)
		{
			p.subtitleLine = p.lineTextCache.get(p.lineText, p.at, &p.clearSubtitles);
			if (!p.subtitleLine.is_empty())
			{
				if (p.clearSubtitles)
				{
					subtitleSystem->remove_all();
				}
				p.subtitleId = subtitleSystem->add(p.subtitleLine, NP, p.line? Optional<bool>() : true);
			}
		}
	}
}

void VoiceoverSystem::update(float _deltaTime)
{
	if (isPaused)
	{
		return;
	}
#ifdef TEST_VOICEOVER_SYSTEM
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::G))
	{
		if (auto* line = Framework::Library::get_current()->get_samples().find(Framework::LibraryName(String(TXT("voiceovers")), String(TXT("cds1 welcome to a place")))))
		{
			speak(Name(TXT("ai")), NP, line);
		}
	}
	if (::System::Input::get()->has_key_been_pressed(::System::Key::H))
	{
		if (auto* line = Framework::Library::get_current()->get_samples().find(Framework::LibraryName(String(TXT("voiceovers")), String(TXT("cds1 how am i supposed")))))
		{
			speak(Name(TXT("narrator")), NP, line);
		}
	}
#endif
#endif
#endif

	Concurrency::ScopedSpinLock lock(accessLock);

	// update playbacks
	{
		for_every(a, actors)
		{
			for_every(p, a->playbacks)
			{
				if (p->line)
				{
					p->playback.update_pitch();
					p->playback.update_volume();
					p->playback.advance_fading(_deltaTime);
					if (p->playback.is_playing())
					{
						p->at = p->playback.get_at();
					}
				}
				else
				{
					p->at += _deltaTime;
					p->timeLeft -= _deltaTime;
				}

				if (subtitleSystem && p->autoSubtitles)
				{
					auto const& subtitleLineToShow= p->lineTextCache.get(p->lineText, p->at, &p->clearSubtitles);
					if (p->subtitleLine != subtitleLineToShow)
					{
						if (p->clearSubtitles)
						{
							subtitleSystem->remove_all();
						}
						p->subtitleLine = subtitleLineToShow;
						auto prevId = p->subtitleId;
						auto newId = subtitleSystem->add(p->subtitleLine, prevId, p->line ? Optional<bool>() : true);
						remove_subtitle(*p, newId);
						p->subtitleId = newId;
					}
				}
			}
			// remove last one only if stopped playing
			if (!a->playbacks.is_empty())
			{
				auto& p = a->playbacks.get_last();
				if ((p.line && !p.playback.is_playing()) || (! p.line && p.timeLeft <= 0.0f))
				{
					remove_subtitle(p, 0.5f);
					a->playbacks.pop_back();
				}
			}
		}
	}

	update_audio_duck();
}

void VoiceoverSystem::update_audio_duck()
{
	int audioDuckNow = 0;

	//if (!isPaused)
	{
		for_every(a, actors)
		{
			for_every(p, a->playbacks)
			{
				if (p->line)
				{
					if (auto* s = p->line->get_sample())
					{
						if (p->at < s->get_length() - MainSettings::global().get_audio_duck_fade_time())
						{
							audioDuckNow = max(audioDuckNow, p->audioDuck);
						}
					}
				}
				else
				{
					audioDuckNow = max(audioDuckNow, p->audioDuck);
				}
			}
		}
	}

	if (audioDuckCurrent ^ audioDuckNow)
	{
		audioDuckCurrent = audioDuckNow;

		if (auto* ss = ::System::Sound::get())
		{
			if (auto* game = Game::get_as<Game>())
			{
				TagCondition const& tc = game->get_audio_duck_on_voiceover_channel_tag_condition();
				if (tc.is_empty())
				{
					warn(TXT("won't audio duck with no valid condition for channels - would duck everything"));
				}
				else
				{
					ss->audio_duck(audioDuckCurrent, tc);
				}
			}
		}
	}
}

void VoiceoverSystem::log(LogInfoContext& _log) const
{
	Concurrency::ScopedSpinLock lock(accessLock);
	{
		_log.set_colour();
		if (audioDuckCurrent)
		{
			_log.set_colour(Colour::orange);
			_log.log(TXT("AUDIO DUCKING ON (%i)"), audioDuckCurrent);
		}
		else
		{
			_log.log(TXT("audio ducking off"));
		}
		_log.set_colour();
		for_every(a, actors)
		{
			if (a->actor.is_valid())
			{
				_log.log(TXT("[%S]"), a->actor.to_char());
			}
			else
			{
				_log.log(TXT("/general voice actor channel/"));
			}
			LOG_INDENT(_log);
			for_every(p, a->playbacks)
			{
				_log.log(TXT("%5.1fs  (%03.0f%%)  %S"),
					p->playback.get_at(),
					p->playback.get_sample() && p->playback.get_sample()->get_length() > 0.0f ? 100.0f * (p->playback.get_at() / p->playback.get_sample()->get_length()) : 0.0f,
					p->subtitleLine.to_char()
				);
			}
		}
	}
}

void VoiceoverSystem::silence(Playback& _p, Optional<float> const & _fadeOut)
{
	if (_p.line)
	{
		_p.playback.fade_out(_fadeOut.get(_p.line->get_fade_out_time()));
	}
	remove_subtitle(_p);
}

void VoiceoverSystem::remove_subtitle(Playback& _p, float _delay)
{
	if (_p.subtitleId != NONE && subtitleSystem)
	{
		subtitleSystem->remove(_p.subtitleId, _delay);
		_p.subtitleId = NONE;
	}
}

void VoiceoverSystem::remove_subtitle(Playback& _p, SubtitleId const & _ifRemovedId)
{
	if (_p.subtitleId != NONE && subtitleSystem)
	{
		subtitleSystem->remove_if_removed(_p.subtitleId, _ifRemovedId);
		_p.subtitleId = NONE;
	}
}

void VoiceoverSystem::reset()
{
	Concurrency::ScopedSpinLock lock(accessLock);

	// update playbacks
	{
		for_every(a, actors)
		{
			for_every(p, a->playbacks)
			{
				if (p->line)
				{
					p->playback.stop();
				}

				if (subtitleSystem)
				{
					remove_subtitle(*p);
				}
			}
		}
	}

	actors.clear();

	update_audio_duck();
}
