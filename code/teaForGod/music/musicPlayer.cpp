#include "musicPlayer.h"

#include "..\game\gameDirector.h"

#include "..\modules\custom\mc_emissiveControl.h"
#include "..\modules\custom\mc_musicAccidental.h"

#include "..\pilgrimage\pilgrimage.h"
#include "..\pilgrimage\pilgrimageInstance.h"

#include "..\ai\aiRayCasts.h"

#include "..\utils\dullPlayback.h"

#include "..\..\core\debug\debug.h"

#include "..\..\framework\game\bullshotSystem.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\module\moduleSound.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\object.h"
#include "..\..\framework\sound\soundSample.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef INVESTIGATE_SOUND_SYSTEM
	#define LOG_MUSIC_PLAYER
#else
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		#define LOG_MUSIC_PLAYER
	#endif
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifndef LOG_MUSIC_PLAYER
		#define LOG_MUSIC_PLAYER
	#endif
#endif

#ifdef LOG_MUSIC_PLAYER
	//#define LOG_MUSIC_PLAYER_EXTENSIVE
#endif

//

using namespace TeaForGodEmperor;

//

// room parameters
DEFINE_STATIC_NAME(musicTrack);
DEFINE_STATIC_NAME(musicTrackIncidental);

// sample parameters (overriden by music definition)
DEFINE_STATIC_NAME(musicTrackFadeTime);
DEFINE_STATIC_NAME(musicTrackFadeInTime);
DEFINE_STATIC_NAME(musicTrackFadeOutTime);
DEFINE_STATIC_NAME(musicTrackBarCount);

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsNoMusic, TXT("no music"));
DEFINE_STATIC_NAME_STR(bsAllowAccidentalMusic, TXT("allow accidental music"));

//

MusicPlayer* MusicPlayer::s_player = nullptr;

MusicPlayer::MusicPlayer(bool _additional)
{
	if (!_additional)
	{
		an_assert(!s_player, TXT("consider changing music player into additional"));
		s_player = this;
	}
}

MusicPlayer::~MusicPlayer()
{
	stop();
	if (s_player == this)
	{
		s_player = nullptr;
	}
}

void MusicPlayer::reset()
{
	request_none();
	request_combat_auto_stop();
	stop(0.0f);
	set_dull(0.0f, 0.0f);
	block_requests();
}

void MusicPlayer::pause()
{
	if (isPaused)
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(accessLock);
	isPaused = true;
	currentTrackPlayback.pause();
	currentTrackPlaybackOverlay.pause();
	currentTrackPlaybackIncidentalOverlay.pause();
	for_every(p, previousPlaybacks)
	{
		p->pause();
	}
	for_every(p, bumperPlaybacks)
	{
		p->pause();
	}
	for_every(p, accidentalPlaybacks)
	{
		p->pause();
	}
	for_every(p, decoupledPlaybacks)
	{
		p->pause();
	}
}

void MusicPlayer::resume()
{
	if (! isPaused)
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(accessLock);
	isPaused = false;
	currentTrackPlayback.resume_with_fade_in();
	currentTrackPlaybackOverlay.resume_with_fade_in();
	currentTrackPlaybackOverlay.resume_with_fade_in();
	for_every(p, previousPlaybacks)
	{
		p->resume_with_fade_in();
	}
	for_every(p, bumperPlaybacks)
	{
		p->resume_with_fade_in();
	}
	for_every(p, accidentalPlaybacks)
	{
		p->resume_with_fade_in();
	}
	for_every(p, decoupledPlaybacks)
	{
		p->resume_with_fade_in();
	}
}

void MusicPlayer::stop(float _fadeOut)
{
	Concurrency::ScopedSpinLock lock(accessLock);
#ifdef LOG_MUSIC_PLAYER
	output(TXT("[MusicPlayer%S] stop %.3f"), this == s_player? TXT("-main") : TXT("-add"), _fadeOut);
#endif
	if (_fadeOut > 0.0f)
	{
		currentTrackPlayback.fade_out(_fadeOut);
		currentTrackPlaybackOverlay.fade_out(_fadeOut);
		currentTrackPlaybackIncidentalOverlay.fade_out(_fadeOut);
		for_every(p, previousPlaybacks)
		{
			p->fade_out(_fadeOut);
		}
		for_every(p, bumperPlaybacks)
		{
			p->fade_out(_fadeOut);
		}
		for_every(p, accidentalPlaybacks)
		{
			p->fade_out(_fadeOut);
		}
		for_every(p, decoupledPlaybacks)
		{
			p->fade_out(_fadeOut);
		}
	}
	else
	{
		currentTrackPlayback.stop();
		currentTrackPlaybackOverlay.stop();
		currentTrackPlaybackIncidentalOverlay.stop();
		for_every(p, previousPlaybacks)
		{
			p->stop();
		}
		for_every(p, bumperPlaybacks)
		{
			p->stop();
		}
		for_every(p, accidentalPlaybacks)
		{
			p->stop();
		}
		for_every(p, decoupledPlaybacks)
		{
			p->stop();
		}
	}

	//

	decouple_all_and_clear();
}

void MusicPlayer::fade_out(float _fadeOut)
{
	Concurrency::ScopedSpinLock lock(accessLock);
#ifdef LOG_MUSIC_PLAYER
	output(TXT("[MusicPlayer%S] fade out %.3f"), this == s_player ? TXT("-main") : TXT("-add"), _fadeOut);
#endif
	if (_fadeOut > 0.0f)
	{
		currentTrackPlayback.fade_out(_fadeOut);
		currentTrackPlaybackOverlay.fade_out(_fadeOut);
		currentTrackPlaybackIncidentalOverlay.fade_out(_fadeOut);
		for_every(p, previousPlaybacks)
		{
			p->fade_out(_fadeOut);
		}
		for_every(p, bumperPlaybacks)
		{
			p->fade_out(_fadeOut);
		}
		for_every(p, accidentalPlaybacks)
		{
			p->fade_out(_fadeOut);
		}
	}
	else
	{
		currentTrackPlayback.stop();
		currentTrackPlaybackOverlay.stop();
		currentTrackPlaybackIncidentalOverlay.stop();
		for_every(p, previousPlaybacks)
		{
			p->stop();
		}
		for_every(p, bumperPlaybacks)
		{
			p->stop();
		}
		for_every(p, accidentalPlaybacks)
		{
			p->stop();
		}
	}

	//

	decouple_all_and_clear();
}

void MusicPlayer::decouple_all_and_clear()
{
#ifdef LOG_MUSIC_PLAYER
	output(TXT("[MusicPlayer%S] decouple_all_and_clear"), this == s_player ? TXT("-main") : TXT("-add"));
#endif

	decoupledPlaybacks.push_back(currentTrackPlayback);
	currentTrackPlayback.decouple();
	currentTrackPlaybackAt.clear(); // will get updated if required
	decoupledPlaybacks.push_back(currentTrackPlaybackOverlay);
	currentTrackPlaybackOverlay.decouple();
	decoupledPlaybacks.push_back(currentTrackPlaybackIncidentalOverlay);
	currentTrackPlaybackIncidentalOverlay.decouple();
	for_every(p, previousPlaybacks)
	{
		decoupledPlaybacks.push_back(*p);
		p->decouple();
	}
	previousPlaybacks.clear();
	for_every(p, bumperPlaybacks)
	{
		decoupledPlaybacks.push_back(*p);
		p->decouple();
	}
	bumperPlaybacks.clear();
	for_every(p, accidentalPlaybacks)
	{
		decoupledPlaybacks.push_back(*p);
		p->decouple();
	}
	accidentalPlaybacks.clear();
}

void MusicPlayer::set_dull(float _dull, float _dullBlendTime)
{
	dullTarget = _dull;
	dullBlendTime = _dullBlendTime;
}

void MusicPlayer::block_requests(bool _block)
{
	if (auto* mp = get())
	{
#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] %Sblock requests"), _block ? TXT("") : TXT("un"));
#endif
		mp->requestsBlocked = _block;
	}
}

void MusicPlayer::request_none()
{
	if (auto* mp = get())
	{
		Concurrency::ScopedSpinLock lock(mp->requestLock);

		if (mp->requestsBlocked)
		{
			return;
		}
#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_none"));
#endif

		mp->requestedTrackType = MusicTrack::None;
		mp->trackLockedFor = 0.0f;
	}
}

void MusicPlayer::request_ambient()
{
	if (auto* mp = get())
	{
		Concurrency::ScopedSpinLock lock(mp->requestLock);

		if (mp->requestsBlocked)
		{
			return;
		}

#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_ambient"));
#endif

		mp->requestedTrackType = MusicTrack::Ambient;
		mp->trackLockedFor = 0.0f;
	}
}

void MusicPlayer::request_incidental(Optional<Name> const & _incidentalMusicTrack, Optional<bool> const& _incidentalAsOverlay)
{
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_setting_active(NAME(bsNoMusic)))
	{
		return;
	}
#endif

	if (auto* mp = get())
	{
		Concurrency::ScopedSpinLock lock(mp->requestLock);

		if (mp->requestsBlocked)
		{
			return;
		}

#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_incidental \"%S\""), _incidentalMusicTrack.is_set()? _incidentalMusicTrack.get().to_char() : TXT("<not set>"));
#endif

		if (_incidentalAsOverlay.get(false))
		{
			mp->requestedIncidentalOverlay = true;
		}
		else
		{
			mp->requestedTrackType = MusicTrack::Incidental;
			mp->trackLockedFor = 0.0f;
		}
		mp->requestedIncidentalMusicTrack = _incidentalMusicTrack;
		++mp->requestedIncidentalMusicTrackIdx;
	}
}

void MusicPlayer::request_music_track(Optional<Name> const& _musicTrack)
{
	if (auto* mp = get())
	{
		Concurrency::ScopedSpinLock lock(mp->requestLock);

		if (mp->requestsBlocked)
		{
			return;
		}

#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_music_track \"%S\""), _musicTrack.is_set()? _musicTrack.get().to_char() : TXT("[not set]"));
#endif

		mp->requestedMusicTrack = _musicTrack;
		mp->trackLockedFor = 0.0f;
	}
}

bool MusicPlayer::is_playing_combat()
{
	if (auto* mp = get())
	{
		if (mp->currentTrackType == MusicTrack::Combat)
		{
			return true;
		}
	}

	return false;
}

Name const &  MusicPlayer::get_current_track()
{
	if (auto* mp = get())
	{
		return mp->currentTrackId;
	}

	return Name::invalid();
}

void MusicPlayer::request_combat_auto(Optional<float> const& _highIntensityTime, Optional<float> const& _lowIntensityTime, Optional<float> const& _delay)
{
	if (auto* mp = get())
	{
		Concurrency::ScopedSpinLock lock(mp->requestLock);

		if (mp->requestsBlocked)
		{
			return;
		}

#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_combat_auto"));
#endif

		mp->trackLockedFor = 0.0f;
		if (mp->autoCombat.highIntensityTimeLeft == 0.0f && mp->autoCombat.lowIntensityTimeLeft == 0.0f)
		{
			mp->autoCombat.delay = max(mp->autoCombat.delay, _delay.get(mp->autoCombatDefinition.combat.delay));
		}
		// else it should be as it is (most likely zero)
		mp->autoCombat.highIntensityTimeLeft = max(mp->autoCombat.highIntensityTimeLeft, _highIntensityTime.get(mp->autoCombatDefinition.combat.highIntensityTime));
		mp->autoCombat.lowIntensityTimeLeft = max(mp->autoCombat.lowIntensityTimeLeft, _lowIntensityTime.get(mp->autoCombatDefinition.combat.lowIntensityTime));
	}
}

void MusicPlayer::request_combat_auto_indicate_presence(Optional<float> const& _lowIntensityTime, Optional<float> const& _delay)
{
	if (auto* mp = get())
	{
		if (mp->requestsBlocked)
		{
			return;
		}

#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_combat_auto_indicate_presence"));
#endif
		request_combat_auto(0.0f, _lowIntensityTime.get(mp->autoCombatDefinition.indicatePresence.lowIntensityTime), _delay.get(mp->autoCombatDefinition.indicatePresence.delay));
	}
}

void MusicPlayer::request_combat_auto_bump_high(Optional<float> const& _highIntensityTime)
{
	if (auto* mp = get())
	{
		Concurrency::ScopedSpinLock lock(mp->requestLock);

		if (mp->requestsBlocked)
		{
			return;
		}

#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_combat_auto_bump_high"));
#endif

		if (mp->autoCombat.lowIntensityTimeLeft > 0.0f)
		{
			mp->autoCombat.highIntensityTimeLeft = max(mp->autoCombat.highIntensityTimeLeft, _highIntensityTime.get(mp->autoCombatDefinition.bumpHigh.highIntensityTime));
			mp->autoCombat.delay = min(mp->autoCombat.delay, mp->autoCombatDefinition.bumpHigh.delay);
		}
	}
}

void MusicPlayer::request_combat_auto_bump_low(Optional<float> const& _lowIntensityTime)
{
	if (auto* mp = get())
	{
		Concurrency::ScopedSpinLock lock(mp->requestLock);

		if (mp->requestsBlocked)
		{
			return;
		}

#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_combat_auto_bump_low"));
#endif

		if (mp->autoCombat.lowIntensityTimeLeft > 0.0f)
		{
			mp->autoCombat.lowIntensityTimeLeft = max(mp->autoCombat.lowIntensityTimeLeft, _lowIntensityTime.get(mp->autoCombatDefinition.bumpLow.lowIntensityTime));
			mp->autoCombat.delay = min(mp->autoCombat.delay, mp->autoCombatDefinition.bumpLow.delay);
		}
	}
}

void MusicPlayer::request_combat_auto_stop()
{
	if (auto* mp = get())
	{
		Concurrency::ScopedSpinLock lock(mp->requestLock);

		if (mp->requestsBlocked)
		{
			return;
		}

#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_combat_auto_stop"));
#endif

		mp->autoCombat.delay = 0.0f;
		mp->autoCombat.highIntensityTimeLeft = 0.0f;
		mp->autoCombat.lowIntensityTimeLeft = 0.0f;
	}
}

void MusicPlayer::request_combat_auto_calm_down()
{
	if (auto* mp = get())
	{
		Concurrency::ScopedSpinLock lock(mp->requestLock);

		if (mp->requestsBlocked)
		{
			return;
		}

#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer-main] request_combat_auto_calm_down"));
#endif

		if (!mp->currentHighIntensity)
		{
			// maybe hasn't yet started (because of a delay)
			mp->autoCombat.highIntensityTimeLeft = 0.0f;
		}
		mp->autoCombat.highIntensityTimeLeft = min(mp->autoCombat.highIntensityTimeLeft, mp->autoCombatDefinition.calmDown.highIntensityTime);
		mp->autoCombat.lowIntensityTimeLeft = min(mp->autoCombat.lowIntensityTimeLeft, mp->autoCombatDefinition.calmDown.lowIntensityTime);
	}
}

void MusicPlayer::play(Framework::Sample* _music, Optional<float> const& _fadeTime)
{
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_setting_active(NAME(bsNoMusic)))
	{
		return;
	}
#endif

	Concurrency::ScopedSpinLock lock(accessLock);
#ifdef LOG_MUSIC_PLAYER
	output(TXT("[MusicPlayer%S] play %S"), this == s_player ? TXT("-main") : TXT("-add"), _music? _music->get_name().to_string().to_char() : TXT("--"));
#endif

	float fadeTime = _fadeTime.get(2.0f);
	if (!_fadeTime.is_set() && _music)
	{
		fadeTime = _music->get_fade_in_time();
	}

	if (currentTrackPlayback.is_playing())
	{
#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer%S] fade out current (to play new) %.3f"), this == s_player ? TXT("-main") : TXT("-add"), fadeTime);
#endif
		currentTrackPlayback.fade_out(fadeTime);
		previousPlaybacks.push_front(currentTrackPlayback);
		currentTrackPlayback.decouple();
	}

	auto& requestedSample = _music;
	if (requestedSample && requestedSample->get_sample())
	{
		currentSamplePlayed = requestedSample;
		currentTrackPlayback = requestedSample->play(fadeTime, ::Sound::PlaybackSetup().with_own_fading(), NP, true);
		{
			DullPlayback dullPlayback(dull);
			dullPlayback.handle(currentTrackPlayback);
		}
		currentTrackPlayback.resume();
	}

	currentTrackPlaybackAt.clear(); // will get updated if required
	queuedTrackPlayback = ChangeTrackPlayback();
}

PilgrimageInstance* MusicPlayer::get_pilgrimage_instance(Framework::Room * inRoom)
{
	return PilgrimageInstance::get_based_on_room(inRoom);
}

void MusicPlayer::update(Framework::IModulesOwner* _imo, float _deltaTime)
{
	if (isPaused)
	{
		return;
	}

	// do not check bsNoMusic here as we may still want accidental

	Concurrency::ScopedSpinLock lock(accessLock);

	MEASURE_PERFORMANCE_COLOURED(musicPlayerUpdate, Colour::zxGreen);

	Optional<Name> shouldBePlayingTrack;

	if (!currentTrackPlaybackOverlay.is_playing())
	{
		currentSamplePlayedOverlay = nullptr;
	}

	if (!currentTrackPlaybackIncidentalOverlay.is_playing())
	{
		currentSamplePlayedIncidentalOverlay = nullptr;
	}

	if (!currentTrackPlayback.is_playing())
	{
		currentSamplePlayed = nullptr;
	}

	if (currentTrackPlayback.is_playing())
	{
		if (auto* s = currentTrackPlayback.get_sample())
		{
			if (!s->is_looped())
			{
				if (currentTrackPlayback.get_at() > s->get_length() - fadeOutTimeForNonLooped.get(2.0f) - 0.1f)
				{
#ifdef LOG_MUSIC_PLAYER
					output(TXT("[MusicPlayer%S] getting close to end of non looped track %.3f -> %.3f - %.3f"), this == s_player ? TXT("-main") : TXT("-add"),
						currentTrackPlayback.get_at(),
						s->get_length(),
						fadeOutTimeForNonLooped.get(2.0f));
#endif
					// if incidental, get back to the previous requested music track and to ambient
					if (currentTrackType == MusicTrack::Incidental)
					{
						// change non looped to ambient
						requestedTrackType = MusicTrack::Ambient;
						requestedIncidentalMusicTrack.clear();
					}
					else
					{
						requestedMusicTrack.clear();
					}
				}
			}
		}
	}

	if (requestedTrackType != MusicTrack::None &&
		requestedTrackType != MusicTrack::Ambient &&
		currentTrackType == requestedTrackType)
	{
		if (!currentTrackPlayback.is_playing() ||
			currentTrackPlayback.is_fading_out_or_faded_out())
		{
#ifdef LOG_MUSIC_PLAYER
			output(TXT("[MusicPlayer%S] not playing anything, revert to ambient"), this == s_player ? TXT("-main") : TXT("-add"));
#endif
			// revert to ambient
			requestedTrackType = MusicTrack::Ambient;
		}
	}
	
	Framework::Room* imoInRoom = nullptr;
	Transform imoCentreInRoom = Transform::identity;
	Framework::ModulePresence* imoPresence = nullptr;
	if (_imo)
	{
		if (auto* p = _imo->get_presence())
		{
			if (auto* r = p->get_in_room())
			{
				imoPresence = p;
				imoInRoom = r;
				imoCentreInRoom = p->get_centre_of_presence_transform_WS();
			}
		}
	}

	bool allowAccidental = true;
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_setting_active(NAME(bsNoMusic)))
	{
		requestedIncidentalOverlay = false;
		requestedIncidentalMusicTrack.clear();
		if (! Framework::BullshotSystem::is_setting_active(NAME(bsAllowAccidentalMusic)))
		{
			allowAccidental = false;
		}
	}
#endif
	if (currentTrackPlaybackIncidentalOverlay.is_playing())
	{
		allowAccidental = false;
	}
	if (auto* gd = GameDirector::get_active())
	{
		if (gd->is_accidental_music_blocked())
		{
			allowAccidental = false;
		}
	}
	// accidental
	if (allowAccidental)
	{
		MEASURE_PERFORMANCE_COLOURED(accidental, Colour::zxBlue);
		accidentalTimeLeft = max(0.0f, accidentalTimeLeft - _deltaTime);
		if (accidentalTimeLeft <= 0.0f)
		{
			if (imoInRoom && imoPresence)
			{
				for_every_ptr(o, imoInRoom->get_objects())
				{
					if (auto* ma = o->get_custom<CustomModules::MusicAccidental>())
					{
						for_every(p, ma->access_music())
						{
							if (accidentalCooldowns.is_available(p->id))
							{
								if (p->cooldownCheckTS.get_time_since() > 0.0f)
								{
									p->reset_cooldown_check_ts();

									if (p->cooldownIndividualTS.get_time_since() > 0.0f)
									{
										bool isOk = true;
										if (o->get_presence()->get_in_room() != imoInRoom)
										{
											an_assert(false, TXT("we're looking in the same room, right?"));
											isOk = false;
										}

										if (isOk && p->maxDistance.is_set())
										{
											float distance = (o->get_presence()->get_centre_of_presence_WS() - imoCentreInRoom.get_translation()).length();
											if (distance > p->maxDistance.get())
											{
												isOk = false;
											}
										}

										if (isOk && p->movingCloser.is_set())
										{
											float movingTowardsDot = Vector3::dot(o->get_presence()->get_velocity_linear(), imoCentreInRoom.get_translation() - o->get_presence()->get_centre_of_presence_WS());
											bool movingTowards = movingTowardsDot >= 0.0f;
											if (movingTowards ^ p->movingCloser.get())
											{
												isOk = false;
											}
										}
											
										if (isOk && p->requiresVisualContact)
										{
											if (!AI::check_clear_ray_cast(AI::CastInfo().at_centre(), imoCentreInRoom.get_translation(), _imo, o, o->get_presence()->get_in_room(), o->get_presence()->get_placement()))
											{
												isOk = false;
											}
										}

										if (isOk)
										{
											accidentalCooldowns.set(p->id, Random::get(p->cooldown));

											p->reset_cooldown_individual_ts();

											if (p->sound.is_valid())
											{
												if (auto* s = o->get_sound())
												{
													s->play_sound(p->sound);
												}
											}

											if (p->emissive.is_valid())
											{
												if (auto* e = o->get_custom<CustomModules::EmissiveControl>())
												{
													e->emissive_activate(p->emissive);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			accidentalTimeLeft = Random::get_float(0.5f, 2.0f);
		}
	}

	if (requestedIncidentalOverlay)
	{
		MEASURE_PERFORMANCE_COLOURED(requestedIncidentalOverlay, Colour::zxBlue);
		Optional<Name> shouldBePlayingIncidentalTrack = requestedIncidentalMusicTrack;

		if (!shouldBePlayingIncidentalTrack.is_set() &&
			imoInRoom)
		{
			Name musicTrack;
			Name musicTrackVar = NAME(musicTrackIncidental);
			musicTrack = imoInRoom->get_value<Name>(musicTrackVar, musicTrack,
				false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);
			shouldBePlayingIncidentalTrack = musicTrack;
		}

		// just play on incidental overlay, don't change anything else
		MusicDefinition const* md = nullptr;
		if (auto* pi = get_pilgrimage_instance(imoInRoom))
		{
			if (auto* p = pi->get_pilgrimage())
			{
				md = &p->get_music_definition();
			}
		}

		if (md)
		{
			if (shouldBePlayingIncidentalTrack.is_set())
			{
				if (auto* playTrack = md->get_track(shouldBePlayingIncidentalTrack.get(), MusicTrack::Incidental))
				{
					if (auto* requestedSampleIncidentalOverlay = playTrack->sample.get())
					{
						handle_requested_sample_incidental_overlay(md, requestedSampleIncidentalOverlay);
					}
				}
			}
		}

		// consume
		requestedIncidentalOverlay = false;
		requestedIncidentalMusicTrack.clear();
	}

	trackLockedFor = max(0.0f, trackLockedFor - _deltaTime);
	if (trackLockedFor <= 0.0f)
	{
		MusicDefinition const* currentlyRequestedMusicDefinition = nullptr;

		MEASURE_PERFORMANCE_COLOURED(trackNotLocked, Colour::zxBlue);
#ifdef AN_ALLOW_BULLSHOTS
		if (Framework::BullshotSystem::is_setting_active(NAME(bsNoMusic)))
		{
			// just end up here to avoid setting shouldBePlayingTrack
		}
		else
#endif
		if (requestedMusicTrack.is_set() && requestedTrackType != MusicTrack::Incidental)
		{
			shouldBePlayingTrack = requestedMusicTrack.get();
		}
		else if (imoInRoom)
		{
 			if (imoInRoom == checkedForRoom && requestedTrackType == currentTrackType && requestedIncidentalMusicTrackIdx == currentIncidentalMusicTrackIdx)
			{
				shouldBePlayingTrack = trackForCheckedRoom;
			}
			else
			{
#ifdef LOG_MUSIC_PLAYER
#ifdef LOG_MUSIC_PLAYER_EXTENSIVE
				output(TXT("[MusicPlayer%S] check for new should be playing"), this == s_player ? TXT("-main") : TXT("-add"));
#endif
#endif
				checkedForRoom = imoInRoom;
				Name musicTrack;
				if (requestedTrackType == MusicTrack::Incidental && requestedIncidentalMusicTrack.is_set())
				{
#ifdef LOG_MUSIC_PLAYER
					output(TXT("[MusicPlayer%S] get incidental \"%S\""), this == s_player ? TXT("-main") : TXT("-add"), requestedIncidentalMusicTrack.get().to_char());
#endif
					musicTrack = requestedIncidentalMusicTrack.get();
				}
				else
				{
					Name musicTrackVar = NAME(musicTrack);
					if (requestedTrackType == MusicTrack::Incidental)
					{
						musicTrackVar = NAME(musicTrackIncidental);
					}
					if (requestedTrackType != MusicTrack::None)
					{
						musicTrack = imoInRoom->get_value<Name>(musicTrackVar, musicTrack,
							false /* we do not want origin room as it may be something completely different interior/exterior wise, better to depend on region */);
					}
				}
				shouldBePlayingTrack = musicTrack;
				trackForCheckedRoom = shouldBePlayingTrack;

				if (auto* pi = get_pilgrimage_instance(imoInRoom))
				{
					if (auto* p = pi->get_pilgrimage())
					{
#ifdef LOG_MUSIC_PLAYER
						//output(TXT("[MusicPlayer%S] using pilgrimage \"%S\" for next track \"%S\""), this == s_player ? TXT("-main") : TXT("-add"), p->get_name().to_string().to_char(), shouldBePlayingTrack.get().to_char());
#endif
						currentlyRequestedMusicDefinition = &p->get_music_definition();
					}
				}				
			}
		}

		MusicTrack::Type currentlyRequestedTrackType = requestedTrackType;
		bool currentlyRequestedHighIntensityCombat = false;

		{
			autoCombat.delay = max(0.0f, autoCombat.delay - _deltaTime);
			if (autoCombat.delay <= 0.0f)
			{
				autoCombat.highIntensityTimeLeft = max(0.0f, autoCombat.highIntensityTimeLeft - _deltaTime);
				autoCombat.lowIntensityTimeLeft = max(0.0f, autoCombat.lowIntensityTimeLeft - _deltaTime);
				if (autoCombat.highIntensityTimeLeft > 0.0f ||
					autoCombat.lowIntensityTimeLeft > 0.0f)
				{
					currentlyRequestedTrackType = MusicTrack::Combat;
				}
				if (autoCombat.highIntensityTimeLeft > 0.0f)
				{
					currentlyRequestedHighIntensityCombat = true;
				}
			}
		}

		if (shouldBePlayingTrack.is_set())
		{
			if (!currentTrackPlayback.is_playing() ||
				currentTrackId != shouldBePlayingTrack.get() ||
				currentTrackType != currentlyRequestedTrackType ||
				(currentlyRequestedMusicDefinition && currentlyRequestedMusicDefinition != currentMusicDefinition) || // if we change pilgrimage but we remain playing the same track (for example: ambient)
				currentHighIntensity != currentlyRequestedHighIntensityCombat)
			{
				if (!currentlyRequestedMusicDefinition)
				{
					if (auto* pi = get_pilgrimage_instance(imoInRoom))
					{
						if (auto* p = pi->get_pilgrimage())
						{
#ifdef LOG_MUSIC_PLAYER
							//output(TXT("[MusicPlayer%S] using pilgrimage \"%S\" for next track \"%S\""), this == s_player ? TXT("-main") : TXT("-add"), p->get_name().to_string().to_char(), shouldBePlayingTrack.get().to_char());
#endif
							currentlyRequestedMusicDefinition = &p->get_music_definition();
						}
					}
				}

				if (currentlyRequestedMusicDefinition)
				{
					if (!currentTrack ||
						currentTrackId != shouldBePlayingTrack.get() ||
						currentTrackType != currentlyRequestedTrackType ||
						(currentlyRequestedMusicDefinition && currentlyRequestedMusicDefinition != currentMusicDefinition)) // if we change pilgrimage but we remain playing the same track (for example: ambient))
					{
						currentTrack = currentlyRequestedMusicDefinition->get_track(shouldBePlayingTrack.get(), currentlyRequestedTrackType);
						if (!currentTrack && currentlyRequestedTrackType == MusicTrack::Combat)
						{
							// force no combat, will revert to ambient soon
							request_combat_auto_stop();
							currentlyRequestedTrackType = MusicTrack::Ambient;
							currentTrack = currentlyRequestedMusicDefinition->get_track(shouldBePlayingTrack.get(), currentlyRequestedTrackType);
						}
					}
				}

				// check again, maybe we dropped combat to ambient
				if (!currentTrackPlayback.is_playing() ||
					currentTrackId != shouldBePlayingTrack.get() ||
					currentTrackType != currentlyRequestedTrackType ||
					currentHighIntensity != currentlyRequestedHighIntensityCombat ||
					(currentlyRequestedMusicDefinition && currentlyRequestedMusicDefinition != currentMusicDefinition)) // if we change pilgrimage but we remain playing the same track (for example: ambient))
				{
#ifdef LOG_MUSIC_PLAYER
					if (currentTrackId != shouldBePlayingTrack.get())
					{
						output(TXT("[MusicPlayer%S] switch track to %S"), this == s_player ? TXT("-main") : TXT("-add"), shouldBePlayingTrack.get().to_char());
					}
#endif
					currentMusicDefinition = currentlyRequestedMusicDefinition; // for reference

					Framework::Sample const* requestedSample = nullptr;
					Framework::Sample const* requestedSampleOverlay = nullptr;
					if (currentTrack)
					{
						if (currentlyRequestedTrackType == MusicTrack::Combat &&
							currentlyRequestedHighIntensityCombat)
						{
							requestedSampleOverlay = currentTrack->sampleHighIntensityOverlay.get();
							requestedSample = currentTrack->sampleHighIntensity.get();
						}
						if (!requestedSample)
						{
							requestedSample = currentTrack->sample.get();
						}
					}

					handle_requested_sample(currentMusicDefinition, requestedSample);

					// as we may want to sync overlay to main
					handle_requested_sample_overlay(currentMusicDefinition, requestedSampleOverlay);

					currentTrackId = shouldBePlayingTrack.get();
					currentTrackType = currentlyRequestedTrackType;
					currentHighIntensity = currentlyRequestedHighIntensityCombat;
					currentIncidentalMusicTrackIdx = requestedIncidentalMusicTrackIdx;
				}
			}
		}
		else
		{
			if (currentTrackPlayback.is_playing())
			{
				MusicDefinition const* md = nullptr;
				if (auto* pi = get_pilgrimage_instance(imoInRoom))
				{
					if (auto* p = pi->get_pilgrimage())
					{
						md = &p->get_music_definition();
					}
				}

				float fadeTime = 0.5f;
				Framework::Sample const* playBumperSample = nullptr;

				if (md)
				{
					if (auto* fade = md->get_fade(currentSamplePlayed, nullptr))
					{
						if (fade->fadeTime.is_set())
						{
							fadeTime = fade->fadeTime.get();
						}
						playBumperSample = fade->bumper.get();
					
						if (playBumperSample)
						{
							if (!bumperCooldowns.is_available(playBumperSample->get_name()))
							{
								bumperCooldowns.set_if_longer(playBumperSample->get_name(), Random::get(fade->bumperCooldown.get()));

								playBumperSample = nullptr;
								fadeTime = fade->coolBumperFadeTime.get(fadeTime);
							}
						}

					}
				}

				if (! _imo)
				{
					fadeTime = 0.05f;
					playBumperSample = nullptr;
				}

#ifdef LOG_MUSIC_PLAYER
				output(TXT("[MusicPlayer%S] fade out current (none to play) %.3f"), this == s_player ? TXT("-main") : TXT("-add"), fadeTime);
#endif

				currentTrackPlayback.fade_out(fadeTime); // quick fade out on no music
				previousPlaybacks.push_back(currentTrackPlayback);
				currentTrackPlayback.decouple();
				currentTrackPlaybackAt.clear(); // will get updated if required
				queuedTrackPlayback = ChangeTrackPlayback();
				currentSamplePlayed = nullptr;

				if (playBumperSample && playBumperSample->get_sample())
				{
					bumperPlaybacks.push_front(playBumperSample->play(NP, ::Sound::PlaybackSetup().with_own_fading(), NP, true));
					{
						DullPlayback dullPlayback(dull);
						dullPlayback.handle(bumperPlaybacks.get_first());
					}
					bumperPlaybacks.get_first().resume();
#ifdef AN_DEVELOPMENT
					currentBumperSamplePlayed = playBumperSample;
#endif
				}

				currentTrackId = Name::invalid();
				currentTrack = nullptr;
				currentMusicDefinition = nullptr;
			}
		}
	}

	update(_deltaTime);
}

Optional<float> MusicPlayer::calculate_sync_pt_for_beat_sync(Framework::Sample const* requestedSample, float beatSync, Optional<int> const& _syncDir) const
{
	if (beatSync != 0.0f &&
		currentTrackPlayback.is_playing() &&
		requestedSample && currentSamplePlayed &&
		requestedSample->get_sample() && currentSamplePlayed->get_sample() &&
		requestedSample->get_sample()->get_length() != 0.0f && currentSamplePlayed->get_sample()->get_length() != 0.0f)
	{
		int currentBarsCount = currentSamplePlayed->get_custom_parameters().get_value<int>(NAME(musicTrackBarCount), 0);
		int requestedBarCount = requestedSample->get_custom_parameters().get_value<int>(NAME(musicTrackBarCount), 0);

		if (currentBarsCount == 0 && requestedBarCount == 0)
		{
			warn(TXT("both samples have no \"musicTrackBarCount\" provided (\"%S\", \"%S\")"), currentSamplePlayed->get_name().to_string().to_char(), requestedSample->get_name().to_string().to_char());
		}
		else
		{
			float cLen = currentSamplePlayed->get_sample()->get_length();
			float rLen = requestedSample->get_sample()->get_length();

			if (currentBarsCount == 0)
			{
				currentBarsCount = TypeConversions::Normal::f_i_closest(cLen / (rLen / (float)requestedBarCount));
			}
			if (requestedBarCount == 0)
			{
				requestedBarCount = TypeConversions::Normal::f_i_closest(rLen / (cLen / (float)currentBarsCount));
			}
			currentBarsCount = max(1, currentBarsCount);
			requestedBarCount = max(1, requestedBarCount);

			float atBarsPt = (currentTrackPlayback.get_at() / cLen) * (float)currentBarsCount;
			float roundedAtBarsPt = round_to(atBarsPt, beatSync);
			if (_syncDir.is_set() && _syncDir.get() != 0)
			{
				if (_syncDir.get() > 0)
				{
					// sync forward, be earlier
					while (atBarsPt > roundedAtBarsPt)
					{
						atBarsPt -= beatSync;
					}
				}
				else
				{
					// sync back, be late
					while (atBarsPt < roundedAtBarsPt)
					{
						atBarsPt += beatSync;
					}
				}
			}
			float requestedBarsPt = mod(atBarsPt - roundedAtBarsPt, (float)requestedBarCount);

			return requestedBarsPt / (float)requestedBarCount;
		}
	}
	return NP;
}

void MusicPlayer::fill_change_playback_track(REF_ ChangeTrackPlayback& cpt, MusicDefinition const* md, Framework::Sample const* requestedSample)
{
	cpt = ChangeTrackPlayback();
	cpt.requestedSample = requestedSample;

	if (requestedSample)
	{
		cpt.fadeTime = requestedSample->get_custom_parameters().get_value<float>(NAME(musicTrackFadeTime), cpt.fadeTime);
		if (auto* fadeInTime = requestedSample->get_custom_parameters().get_existing<float>(NAME(musicTrackFadeInTime)))
		{
			cpt.fadeInTime = *fadeInTime;
		}
		if (requestedSample->get_sample() &&
			!requestedSample->get_sample()->is_looped())
		{
			cpt.fadeOutTimeForNonLooped = requestedSample->get_fade_out_time();
		}
		if (auto* fadeOutTime = requestedSample->get_custom_parameters().get_existing<float>(NAME(musicTrackFadeOutTime)))
		{
			cpt.fadeOutTimeForNonLooped = *fadeOutTime;
		}
	}

	if (md)
	{
		if (auto* fade = md->get_fade(currentSamplePlayed, requestedSample))
		{
			if (fade->queue)
			{
				cpt.queue = true;
				cpt.fadeTime = 0.2f; // short sync fade
			}
			if (fade->fadeTime.is_set())
			{
				cpt.fadeTime = fade->fadeTime.get();
			}
			if (fade->fadeInTime.is_set())
			{
				cpt.fadeInTime = fade->fadeInTime.get();
			}
			if (fade->fadeOutTime.is_set())
			{
				cpt.fadeOutTimeForNonLooped = fade->fadeOutTime.get();
			}
			if (fade->startAt.is_set())
			{
				cpt.startAt = fade->startAt.get();
			}
			if (fade->startPt.is_set())
			{
				cpt.startPt = fade->startPt.get();
			}
			if (fade->syncPt)
			{
				cpt.syncPt = true;
			}
			if (fade->beatSync != 0.0f)
			{
				cpt.beatSync = fade->beatSync;
			}
			if (fade->syncDir.is_set())
			{
				cpt.syncDir = fade->syncDir;
			}
			cpt.playBumperSample = fade->bumper.get();

			if (cpt.playBumperSample)
			{
				if (!bumperCooldowns.is_available(cpt.playBumperSample->get_name()))
				{
					bumperCooldowns.set_if_longer(cpt.playBumperSample->get_name(), Random::get(fade->bumperCooldown.get()));

					cpt.playBumperSample = nullptr;
					cpt.fadeTime = fade->coolBumperFadeTime.get(cpt.fadeTime);
				}
			}
		}
	}

	if (currentTrackPlayback.get_sample() && !currentTrackPlayback.get_sample()->is_looped() && cpt.fadeOutTimeForNonLooped.is_set())
	{
		cpt.fadeTime = min(cpt.fadeOutTimeForNonLooped.get(), cpt.fadeTime);
	}
}

void MusicPlayer::handle_requested_sample(MusicDefinition const* md, Framework::Sample const* requestedSample)
{
	if (requestedSample && queuedTrackPlayback.requestedSample == requestedSample)
	{
		// we will play it as soon as possible, it is already queued
		return;
	}
	queuedTrackPlayback = ChangeTrackPlayback();
	if (currentSamplePlayed != requestedSample)
	{
		ChangeTrackPlayback cpt;
		fill_change_playback_track(REF_ cpt, md, requestedSample);

		trackLockedFor = currentTrack ? currentTrack->playAtLeast : 0.0f;

		if (cpt.queue)
		{
			queuedTrackPlayback = cpt;
		}
		else
		{
			change_current_track(cpt, TXT(""), REF_ currentSamplePlayed, REF_ currentTrackPlayback);
		}
	}
}

void MusicPlayer::change_current_track(ChangeTrackPlayback const & cpt, tchar const * _info, REF_ Framework::Sample const* & _currentSamplePlayed, REF_ Sound::Playback & _currentTrackPlayback)
{
	Optional<float> syncAtPt;
	if (cpt.beatSync != 0.0f)
	{
		Optional<float> resSyncPt = calculate_sync_pt_for_beat_sync(cpt.requestedSample, cpt.beatSync, cpt.syncDir);
		if (resSyncPt.is_set())
		{
			syncAtPt = resSyncPt;
		}
	}
	if (_currentTrackPlayback.is_playing())
	{
		if (cpt.syncPt && !syncAtPt.is_set())
		{
			if (auto* s = _currentTrackPlayback.get_sample())
			{
				if (float l = s->get_length())
				{
					syncAtPt = _currentTrackPlayback.get_at() / l;
				}
			}
		}
#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer%S] fade out current (change current)%S %.3f"), this == s_player ? TXT("-main") : TXT("-add"), _info, cpt.fadeTime);
#endif
		_currentTrackPlayback.fade_out(cpt.fadeTime);
		previousPlaybacks.push_front(_currentTrackPlayback);
		_currentTrackPlayback.decouple();
	}

	fadeOutTimeForNonLooped = cpt.fadeOutTimeForNonLooped;

	if (cpt.requestedSample && cpt.requestedSample->get_sample())
	{
#ifdef LOG_MUSIC_PLAYER
		output(TXT("[MusicPlayer%S] fade in%S %S %.3f"), this == s_player ? TXT("-main") : TXT("-add"), _info, cpt.requestedSample->get_name().to_string().to_char(), cpt.fadeInTime.get(cpt.fadeTime));
		if (syncAtPt.is_set())
		{
			output(TXT("[MusicPlayer%S] syncing at %.1f%%"), this == s_player ? TXT("-main") : TXT("-add"), syncAtPt.get() * 100.0f);
		}
		if (! cpt.requestedSample->get_sample()->is_looped())
		{
			output(TXT("[MusicPlayer%S] non looped, ordered to fade out at %.3f"), this == s_player ? TXT("-main") : TXT("-add"), fadeOutTimeForNonLooped.get(-1.0f));
		}
#endif
		_currentSamplePlayed = cpt.requestedSample;
		_currentTrackPlayback = cpt.requestedSample->play(cpt.fadeInTime.get(cpt.fadeTime), ::Sound::PlaybackSetup().with_own_fading(), NP, true);
		if (auto* s = _currentTrackPlayback.get_sample())
		{
			if (syncAtPt.is_set())
			{
				_currentTrackPlayback.set_at(s->get_length() * syncAtPt.get());
			}
			else if (cpt.startAt.is_set())
			{
				_currentTrackPlayback.set_at(mod(Random::get(cpt.startAt.get()), s->get_length()));
			}
			else if (cpt.startPt.is_set())
			{
				_currentTrackPlayback.set_at(mod(Random::get(cpt.startPt.get()), 1.0f) * s->get_length());
			}
		}
		{
			DullPlayback dullPlayback(dull);
			dullPlayback.handle(_currentTrackPlayback);
		}
		_currentTrackPlayback.resume();
	}
	if (cpt.playBumperSample && cpt.playBumperSample->get_sample())
	{
		bumperPlaybacks.push_front(cpt.playBumperSample->play(NP, ::Sound::PlaybackSetup().with_own_fading(), NP, true));
		{
			DullPlayback dullPlayback(dull);
			dullPlayback.handle(bumperPlaybacks.get_first());
		}
		bumperPlaybacks.get_first().resume();
#ifdef AN_DEVELOPMENT
		currentBumperSamplePlayed = cpt.playBumperSample;
#endif
	}
}

void MusicPlayer::handle_requested_sample_overlay(MusicDefinition const* md, Framework::Sample const* requestedSampleOverlay)
{
	if (currentSamplePlayedOverlay != requestedSampleOverlay)
	{
		ChangeTrackPlayback cpt;
		fill_change_playback_track(REF_ cpt, md, requestedSampleOverlay);

		if (cpt.queue)
		{
			warn_dev_investigate(TXT("can't queue overlay samples"));
		}
		change_current_track(cpt, TXT(" (overlay)"), REF_ currentSamplePlayedOverlay, REF_ currentTrackPlaybackOverlay);
		currentTrackPlaybackAt.clear(); // will get updated if required
	}
}

void MusicPlayer::handle_requested_sample_incidental_overlay(MusicDefinition const* md, Framework::Sample const* requestedSampleIncidentalOverlay)
{
	// incidental overlays use fading as defined in them, they should not fade out mid through as game script should not spam with calls
	if (currentSamplePlayedIncidentalOverlay != requestedSampleIncidentalOverlay)
	{
		float fadeTime = 0.0f;
		if (requestedSampleIncidentalOverlay)
		{
			fadeTime = requestedSampleIncidentalOverlay->get_fade_in_time();
		}

		if (currentTrackPlaybackIncidentalOverlay.is_playing() ||
			currentTrackPlaybackIncidentalOverlay.is_active())
		{
#ifdef LOG_MUSIC_PLAYER
			output(TXT("[MusicPlayer%S] fade out current (incidental overlay) %.3f"), this == s_player ? TXT("-main") : TXT("-add"), fadeTime);
#endif
			currentTrackPlaybackIncidentalOverlay.fade_out(currentSamplePlayedIncidentalOverlay ? currentSamplePlayedIncidentalOverlay->get_fade_out_time() : fadeTime);
			previousPlaybacks.push_front(currentTrackPlaybackIncidentalOverlay);
			currentTrackPlaybackIncidentalOverlay.decouple();
		}

		if (requestedSampleIncidentalOverlay && requestedSampleIncidentalOverlay->get_sample())
		{
#ifdef LOG_MUSIC_PLAYER
			output(TXT("[MusicPlayer%S] fade in %S (overlay) %.3f"), this == s_player ? TXT("-main") : TXT("-add"), requestedSampleIncidentalOverlay->get_name().to_string().to_char(), fadeTime);
#endif
			currentSamplePlayedIncidentalOverlay = requestedSampleIncidentalOverlay;
			currentTrackPlaybackIncidentalOverlay = requestedSampleIncidentalOverlay->play(fadeTime, ::Sound::PlaybackSetup().with_own_fading(), NP, true);
			{
				DullPlayback dullPlayback(dull);
				dullPlayback.handle(currentTrackPlaybackIncidentalOverlay);
			}
			currentTrackPlaybackIncidentalOverlay.resume();
		}
	}
}

void MusicPlayer::update(float _deltaTime)
{
	if (isPaused)
	{
		return;
	}

	MEASURE_PERFORMANCE_COLOURED(musicPlayerUpdate, Colour::zxBlue);

	Concurrency::ScopedSpinLock lock(accessLock, true);

	bumperCooldowns.advance(_deltaTime);
	accidentalCooldowns.advance(_deltaTime);

	if (queuedTrackPlayback.requestedSample)
	{
		bool playQueued = false;

		if (currentTrackPlaybackAt.is_set() && currentTrackPlayback.is_active())
		{
			float ctpAt = currentTrackPlayback.get_at();
			if (ctpAt < currentTrackPlaybackAt.get())
			{
				playQueued = true;
				queuedTrackPlayback.startAt = Range(ctpAt); // force sync
			}
		}
		if (playQueued)
		{
			change_current_track(queuedTrackPlayback, TXT(" (queued)"), REF_ currentSamplePlayed, REF_ currentTrackPlayback);
			queuedTrackPlayback = ChangeTrackPlayback();
			currentTrackPlaybackAt.clear(); // will get updated if required
		}
	}

	// update playbacks
	{
		MEASURE_PERFORMANCE_COLOURED(updatePlaybacks, Colour::zxCyan);

		{
			float actualDullTarget = dullTarget * 0.3f; // much less dull
			dull = blend_to_using_time(dull, actualDullTarget, dullBlendTime, _deltaTime);
		}

		DullPlayback dullPlayback(dull);

		if (currentTrackPlayback.is_active())
		{
			currentTrackPlaybackAt = currentTrackPlayback.get_at();
		}
		else
		{
			currentTrackPlaybackAt.clear();
		}
		currentTrackPlayback.update_pitch();
		currentTrackPlayback.update_volume();
		dullPlayback.handle(currentTrackPlayback);
		currentTrackPlayback.advance_fading(_deltaTime);

		currentTrackPlaybackOverlay.update_pitch();
		currentTrackPlaybackOverlay.update_volume();
		dullPlayback.handle(currentTrackPlaybackOverlay);
		currentTrackPlaybackOverlay.advance_fading(_deltaTime);

		currentTrackPlaybackIncidentalOverlay.update_pitch();
		currentTrackPlaybackIncidentalOverlay.update_volume();
		dullPlayback.handle(currentTrackPlaybackIncidentalOverlay);
		currentTrackPlaybackIncidentalOverlay.advance_fading(_deltaTime);

		for_every(p, previousPlaybacks)
		{
			p->update_pitch();
			p->update_volume();
			dullPlayback.handle(*p);
			p->advance_fading(_deltaTime);
		}
		for_every(p, bumperPlaybacks)
		{
			p->update_pitch();
			p->update_volume();
			dullPlayback.handle(*p);
			p->advance_fading(_deltaTime);
		}
		for_every(p, accidentalPlaybacks)
		{
			p->update_pitch();
			p->update_volume();
			dullPlayback.handle(*p);
			p->advance_fading(_deltaTime);
		}
		for_every(p, decoupledPlaybacks)
		{
			p->update_pitch();
			p->update_volume();
			dullPlayback.handle(*p);
			p->advance_fading(_deltaTime);
		}
	}

	// remove last one only if stopped playing
	{
		MEASURE_PERFORMANCE_COLOURED(cleanUpLast, Colour::zxCyan);
		if (! previousPlaybacks.is_empty())
		{
			auto& p = previousPlaybacks.get_last();
			if (!p.is_playing())
			{
				previousPlaybacks.pop_back();
			}
		}
		if (!bumperPlaybacks.is_empty())
		{
			auto& p = bumperPlaybacks.get_last();
			if (!p.is_playing())
			{
				bumperPlaybacks.pop_back();
			}
		}
		if (!accidentalPlaybacks.is_empty())
		{
			auto& p = accidentalPlaybacks.get_last();
			if (!p.is_playing())
			{
				accidentalPlaybacks.pop_back();
			}
		}
		if (!decoupledPlaybacks.is_empty())
		{
			auto& p = decoupledPlaybacks.get_last();
			if (!p.is_playing())
			{
				decoupledPlaybacks.pop_back();
			}
		}
	}
}

void MusicPlayer::log(LogInfoContext& _log, Optional<Colour> const& _defaultColour) const
{
	Concurrency::ScopedSpinLock lock(accessLock);

	{
		_log.set_colour(_defaultColour);
		_log.log(TXT("music"));
		LOG_INDENT(_log);
		_log.log(TXT("track   : %S%S"), currentTrackId.to_char(), queuedTrackPlayback.requestedSample? TXT(" (queued)") : TXT(""));
		_log.log(TXT("musdef  : %p"), currentMusicDefinition); // only pointer, no name, id etc
		_log.log(TXT("type    : %S"),
			currentTrackType == MusicTrack::Ambient ? TXT("ambient") :
			currentTrackType == MusicTrack::Incidental ? TXT("incidental") :
			currentTrackType == MusicTrack::Combat ? (currentHighIntensity ? TXT("combat HI") : TXT("combat")) :
				TXT("--"));
		_log.log(TXT("sample  : %S"),
			currentSamplePlayed ? currentSamplePlayed->get_name().to_string().to_char() : TXT("--"));
		_log.log(TXT("at: %5.1fs  (%03.0f%%)"),
			currentTrackPlayback.get_at(),
			currentTrackPlayback.get_sample() && currentTrackPlayback.get_sample()->get_length() > 0.0f? 100.0f * (currentTrackPlayback.get_at() / currentTrackPlayback.get_sample()->get_length()) : 0.0f
			);
#ifdef AN_SOUND_LOUDNESS_METER
		{
			Optional<float> loudness;
			Optional<float> maxRecentLoudness;
			loudness = currentTrackPlayback.get_loudness();
			maxRecentLoudness = currentTrackPlayback.get_recent_max_loudness();
			_log.set_colour(::Sound::LoudnessMeter::loudness_to_colour(maxRecentLoudness.get(loudness.get(-100.0f))));
			_log.log(TXT("loudness: %3.0fdB^%3.0fdB"), loudness.get(0.0f), maxRecentLoudness.get(0.0f));
			_log.set_colour();
		}
#endif
		_log.log(TXT("dull    : %03.0f%%"), dull);
		_log.log(TXT("fade: %03.0f%%    vol: %03.0f%%    pitch: %03.0f%%"),
			currentTrackPlayback.get_current_fade() * 100.0f,
			currentTrackPlayback.get_current_volume() * 100.0f,
			currentTrackPlayback.get_current_pitch() * 100.0f);
		_log.log(TXT("overlay : %S"),
			currentSamplePlayedOverlay ? currentSamplePlayedOverlay->get_name().to_string().to_char() : TXT("--"));
		_log.log(TXT("at: %5.1fs  (%03.0f%%)"),
			currentTrackPlaybackOverlay.get_at(),
			currentTrackPlaybackOverlay.get_sample() && currentTrackPlaybackOverlay.get_sample()->get_length() > 0.0f? 100.0f * (currentTrackPlaybackOverlay.get_at() / currentTrackPlaybackOverlay.get_sample()->get_length()) : 0.0f
			);
		_log.log(TXT("incidental overlay : %S"),
			currentSamplePlayedIncidentalOverlay ? currentSamplePlayedIncidentalOverlay->get_name().to_string().to_char() : TXT("--"));
		_log.log(TXT("at: %5.1fs  (%03.0f%%)"),
			currentTrackPlaybackIncidentalOverlay.get_at(),
			currentTrackPlaybackIncidentalOverlay.get_sample() && currentTrackPlaybackIncidentalOverlay.get_sample()->get_length() > 0.0f? 100.0f * (currentTrackPlaybackIncidentalOverlay.get_at() / currentTrackPlaybackIncidentalOverlay.get_sample()->get_length()) : 0.0f
			);
		if (!previousPlaybacks.is_empty())
		{
			_log.log(TXT("fading out: %i"), previousPlaybacks.get_size());
		}
		else
		{
			_log.log(TXT(""));
		}
#ifdef AN_DEVELOPMENT
		if (currentBumperSamplePlayed && ! bumperPlaybacks.is_empty())
		{
			_log.log(TXT("bumper: %S"), currentBumperSamplePlayed->get_name().to_string().to_char());
		}
		else
		{
			_log.log(TXT(""));
		}
#endif
	}
	_log.log(TXT(""));
	{
		_log.set_colour(_defaultColour);
		_log.log(TXT("request combat auto"));
		LOG_INDENT(_log);
		_log.log(TXT("high intensity : %5.1fs"), autoCombat.highIntensityTimeLeft);
		_log.log(TXT("low intensity  : %5.1fs"), autoCombat.lowIntensityTimeLeft);
		_log.log(TXT("delay          : %5.1fs"), autoCombat.delay);
	}
}
