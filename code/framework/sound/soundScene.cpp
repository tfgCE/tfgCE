#include "soundScene.h"
#include "soundCamera.h"
#include "soundContext.h"
#include "soundSources.h"

#include "..\game\game.h"
#include "..\game\gameConfig.h"
#include "..\module\modulePresence.h"
#include "..\module\moduleSound.h"
#include "..\object\object.h"
#include "..\object\temporaryObject.h"
#include "..\presence\pathStringPulling.h"
#include "..\sound\reverb.h"
#include "..\sound\soundSample.h"
#include "..\world\doorInRoom.h"
#include "..\world\room.h"

#include "..\..\core\globalSettings.h"
#include "..\..\core\mainConfig.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\debug\extendedDebug.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\sound\soundSystem.h"
#include "..\..\core\system\input.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define SHORT_LOG

#ifdef INVESTIGATE_SOUND_SYSTEM
	#define OUTPUT_SOUNDS_EXTENDED
	//#define OUTPUT_SOUNDS_EXTENDED_TRIGGER_REMOVALS
#else
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		#define OUTPUT_SOUNDS_EXTENDED
		#define OUTPUT_SOUNDS_EXTENDED_TRIGGER_REMOVALS
	#endif
#endif

#ifdef AN_ALLOW_EXTENDED_DEBUG
	#ifndef OUTPUT_SOUNDS_EXTENDED
		#define OUTPUT_SOUNDS_EXTENDED
	#endif
#endif

//

using namespace Framework;
using namespace Framework::Sound;

//

ScannedRoom::ScannedRoom(Room* _room, ScannedRoom* _enteredFrom, Transform const& _relativeRoomPlacement, DoorInRoom* _enteredThroughDoor, Vector3 const& _propagationDir, float _distance, float _distanceBeyondFirstRoom, int _depth)
: room(_room)
, relativeRoomPlacement(_relativeRoomPlacement)
, enteredFrom(_enteredFrom)
, enteredThroughDoor(_enteredThroughDoor)
, entrancePoint(_enteredThroughDoor->get_placement().get_translation())
, propagationDir(_propagationDir)
, distance(_distance)
, distanceBeyondFirstRoom(_distanceBeyondFirstRoom)
, depth(_depth)
, scanned(false)
{
	soundLevel = _enteredFrom->soundLevel;
	if (Door const * door = _enteredThroughDoor->get_door())
	{
		soundLevel = door->adjust_sound_level(soundLevel);
	}
}

ScannedRoom::ScannedRoom(Room* _room, Vector3 const & _location)
: room(_room)
, enteredFrom(nullptr)
, enteredThroughDoor(nullptr)
, entrancePoint(_location)
, propagationDir(Vector3::zero)
, soundLevel(1.0f)
, distance(0.0f)
, distanceBeyondFirstRoom(0.0f)
, depth(0)
, scanned(false)
{
}

void ScannedRoom::find(REF_ ArrayStack<ScannedRoom> & _rooms, Room* _startRoom, Vector3 const & _startLoc, ScanRoomInfo & _context)
{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
	MEASURE_PERFORMANCE_COLOURED(scannedRoom_find, Colour::zxCyan);
#endif
	_rooms.push_back(ScannedRoom(_startRoom, _startLoc));
	int firstRoomToScan = 0;
	while (true)
	{
		// find room to scan closest to us, remember last first room scanned as more added rooms will be after this one
		ScannedRoom* scanRoom = nullptr;
		for (ScannedRoom* room = &_rooms[firstRoomToScan]; room != _rooms.end(); ++room)
		{
			if (room->scanned)
			{
				if (!scanRoom)
				{
					++firstRoomToScan;
				}
			}
			else
			{
				if (!scanRoom || scanRoom->distance > room->distance)
				{
					scanRoom = room;
				}
			}
		}

		if (scanRoom)
		{
			scan_room(*scanRoom, REF_ _rooms, _context);
			continue;
		}
		else
		{
			// everything done
			break;
		}
	}
}

void ScannedRoom::scan_room(ScannedRoom & _room, REF_ ArrayStack<ScannedRoom> & _roomsToScan, ScanRoomInfo & _context)
{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
	MEASURE_PERFORMANCE(soundScene_scanRoom);
#endif

	_room.scanned = true;

	if (_context.depthLimit.is_set() &&
		_room.depth >= _context.depthLimit.get())
	{
		// no need to look for further
		return;
	}

	if (_context.concurrentRoomsLimit.is_set())
	{
		int alreadyExisting = 0;
		for_every(room, _roomsToScan)
		{
			if (room->room == _room.room)
			{
				++alreadyExisting;
				if (alreadyExisting > _context.concurrentRoomsLimit.get())
				{
					return;
				}
			}
		}
	}

	// get more rooms!
	if (_roomsToScan.has_place_left())
	{
		for_every_ptr(door, _room.room->get_doors())
		{
			if (!door->is_visible())
			{
				continue;
			}
			if (door != _room.enteredThroughDoor)
			{
				float increaseDistanceBy = (door->get_placement().get_translation() - _room.entrancePoint).length();
				Room* nextRoom = door->get_world_active_room_on_other_side();
				if (!nextRoom || !nextRoom->is_world_active())
				{
					continue;
				}
				int alreadyExisting = 0;
				for_every(room, _roomsToScan)
				{
					if (room->room == nextRoom)
					{
						++alreadyExisting;
					}
				}
				float actualIncreaseDistanceBy = increaseDistanceBy * (1.0f + (float)alreadyExisting * 1.5f);
				float distance = _room.distance + actualIncreaseDistanceBy;
				float actualIncreaseDistanceByBeyondFirstRoom = actualIncreaseDistanceBy;
				int increaseDepth = 1;
				if (auto* d = door->get_door())
				{
					if (auto* dt = d->get_door_type())
					{
						if (!dt->does_ignore_sound_distance_beyond_first_room())
						{
							actualIncreaseDistanceByBeyondFirstRoom = 0.0f;
							increaseDepth = 0;
						}
					}
				}
				float distanceBeyondFirstRoom = _room.distanceBeyondFirstRoom + actualIncreaseDistanceByBeyondFirstRoom;
				if (_context.maxDistance.is_set() &&
					_room.distance >= _context.maxDistance.get())
				{
					continue;
				}
				if (_context.maxDistanceBeyondFirstRoom.is_set() &&
					_room.distanceBeyondFirstRoom >= _context.maxDistanceBeyondFirstRoom.get())
				{
					continue;
				}
				int insertAt = (int)(&_room + 1 - _roomsToScan.begin());
				for (ScannedRoom const * room = &_room + 1; room != _roomsToScan.end(); ++room)
				{
					if (room->distance > distance)
					{
						break;
					}
					++insertAt;
				}
				Vector3 propagationDirORS = Vector3::zero;
				if (_context.withPropagationDir)
				{
					Vector3 propagationDirWS = (0.7f * (door->get_hole_centre_WS() - _room.entrancePoint).normal() + 0.3f * _room.propagationDir).normal();
					propagationDirORS = door->get_other_room_transform().vector_to_local(propagationDirWS);
				}
				Transform relativeRoomPlacement = _room.relativeRoomPlacement.to_world(door->get_other_room_transform());
				// we store pointer to ScannedRoom but as we do it in order, we should never move those rooms that we already point at
				_roomsToScan.insert_at(insertAt, ScannedRoom(nextRoom, &_room, relativeRoomPlacement, door->get_door_on_other_side(), propagationDirORS, distance, distanceBeyondFirstRoom, _room.depth + increaseDepth));
				if (!_roomsToScan.has_place_left())
				{
					// no more space left!
					break;
				}
			}
		}
	}
}

//

void PlayingSceneSound::update_apparent_sound_location()
{
	apparentSoundLocation = stringPulledLocation.normal() * stringDistance;
	if (apparentVelocityDeltaTime > 0.0f)
	{
		bool firstTime = ! apparentVelocity.is_set();
		apparentVelocityPrevStringDistance = firstTime ? stringDistance : apparentVelocityStringDistance;
		apparentVelocityStringDistance = stringDistance;
		apparentVelocity = stringPulledLocation.normal() * blend_to_using_time(
			apparentVelocity.get(Vector3::zero).length(),
			(apparentVelocityStringDistance - apparentVelocityPrevStringDistance) / apparentVelocityDeltaTime,
			firstTime? 0.0f : 0.1f, apparentVelocityDeltaTime);
	}
}

//

Scene::Scene()
{
	todo_note(TXT("hardcoded value, maybe should be taken from main sound settings, channel count?"));
	soundsLimit = MainConfig::global().get_sound().maxConcurrentSounds;
	sounds.make_space_for(soundsLimit);
}

Scene::~Scene()
{
	clear();
}

void Scene::update(float _deltaTime, SoundSources & _soundSources, Optional<SceneUpdateContext> const& _context)
{
	MEASURE_PERFORMANCE_COLOURED(updateSoundScene, Colour::zxGreen);

	Concurrency::ScopedSpinLock lockSounds(soundsOpLock);

	if (owner)
	{
		clear_internal();
		owner = nullptr;
	}

	anyOwnerIMO.clear();

	// set that sounds should not be played, when we find sounds to play, we will play them
	// while we're doing it, update "at" for sound sources but only using sounds that are already playing!
	for_every(sound, sounds)
	{
		/*
		if (sound->state != PlayingSceneSoundState::NotUsed &&
			sound->shouldBeHeard &&
			sound->soundSource.is_set() &&
			sound->playback.is_playing())
		{
			// store sound source to be up to date
			sound->soundSource->set_at(sound->playback.get_at() + sound->playback.distance_to_time(sound->stringDistance));
		}
		*/
		sound->shouldBeHeard = false;
	}

	if (!isPaused)
	{
		add_sounds_from(_deltaTime, _soundSources);

		update_playback(_deltaTime, _context);

		update_extra_playbacks(_context);
	}
}

void Scene::update(float _deltaTime, ::Framework::Sound::Camera const & _camera, ModulePresence* _owner, Optional<SceneUpdateContext> const& _context)
{
	MEASURE_PERFORMANCE_COLOURED(updateSoundScene, Colour::zxGreen);

	Concurrency::ScopedSpinLock lockSounds(soundsOpLock);

	camera = _camera;

	if (owner != _owner)
	{
		clear_internal();
		owner = _owner;
	}

	if (owner && (!anyOwnerIMO.get() || anyOwnerIMO.get() != owner->get_owner()))
	{
		anyOwnerIMO = owner->get_owner();
	}
	if (!anyOwnerIMO.get() && _camera.get_in_room())
	{
		// get any actor from the room that camera is in
		//FOR_EVERY_OBJECT_IN_ROOM(object, _camera.get_in_room())
		an_assert(_camera.get_in_room()->is_world_active());
		for_every_ptr(object, _camera.get_in_room()->get_objects()) // this should be safe as we're doing it only for rooms that are world active and they follow strict rules
		{
			if (auto* p = object->get_presence())
			{
				anyOwnerIMO = object;
				break;
			}
		}
	}

	camera.ownerForSoundSourcePresencePath = owner;
	if (!camera.ownerForSoundSourcePresencePath && anyOwnerIMO.get())
	{
		camera.ownerForSoundSourcePresencePath = anyOwnerIMO->get_presence();
	}
	camera.update_from_owner_to_camera();

	// set that sounds should not be played, when we find sounds to play, we will play them
	// while we're doing it, update "at" for sound sources but only using sounds that are already playing!
	for_every(sound, sounds)
	{
		/*
		if (sound->state != PlayingSceneSoundState::NotUsed &&
			sound->shouldBeHeard &&
			sound->soundSource.is_set() &&
			sound->playback.is_playing())
		{
			// store sound source to be up to date
			sound->soundSource->set_at(sound->playback.get_at() + sound->playback.distance_to_time(sound->stringDistance));
		}
		*/
		sound->shouldBeHeard = false;
	}

	for_every(reverb, reverbs)
	{
		reverb->target = 0.0f;
	}

	if (!isPaused)
	{
		find_sounds_to_play(_deltaTime, camera);

		update_playback(_deltaTime, _context);

		update_extra_playbacks(_context);
		for_every(ep, extraPlaybacks)
		{
			if (!ep->playback.is_playing())
			{
				extraPlaybacks.remove_fast_at(for_everys_index(ep));
				break; // we will remove more during following frame, we shouldn't get too many anyway
			}
		}
	}
}

void Scene::clear()
{
	Concurrency::ScopedSpinLock lockSounds(soundsOpLock);

	clear_internal();
}

void Scene::clear_internal()
{
	for_every(sound, sounds)
	{
		if (sound->state != PlayingSceneSoundState::NotUsed)
		{
			// stop immediately
			sound->playback.stop();
		}
	}
	sounds.clear();

	for_every(reverb, reverbs)
	{
		reverb->reverbInstance.stop();
	}
	reverbs.clear();

	for_every(ep, extraPlaybacks)
	{
		ep->playback.stop();
	}
	extraPlaybacks.clear();
}

void Scene::stop_extra_playback(Name const & _id, Optional<float> const& _fadeOutTime)
{
	Concurrency::ScopedSpinLock lockSounds(soundsOpLock);

	for_every(ep, extraPlaybacks)
	{
		if (ep->id == _id &&
			ep->playback.is_playing())
		{
			float fadeOutTime = _fadeOutTime.get(ep->fadeOutTime);
			ep->playback.fade_out(fadeOutTime);
		}
	}
}

void Scene::add_extra_playback(::Sound::Playback const& _playback, Name const& _id, Optional<float> const& _fadeOutTime)
{
	Concurrency::ScopedSpinLock lockSounds(soundsOpLock);

	ExtraPlayback ep;
	ep.id = _id;
	ep.playback = _playback;
	ep.fadeOutTime = _fadeOutTime.get(0.2f);
	extraPlaybacks.push_back(ep);
	if (is_paused())
	{
		extraPlaybacks.get_last().playback.pause();
	}
}

void Scene::pause()
{
	if (is_paused())
	{
		return;
	}

	Concurrency::ScopedSpinLock lockSounds(soundsOpLock);
	TagCondition const * channelTagCondition = nullptr;
	if (auto* game = Game::get())
	{
		channelTagCondition = &game->get_sound_fade_channel_tag_condition();
	}
	
	isPaused = true;
	for_every(sound, sounds)
	{
		if (sound->state != PlayingSceneSoundState::NotUsed &&
			sound->shouldBeHeard &&
			sound->soundSource.is_set())
		{
			if (channelTagCondition)
			{
				if (auto* channel = sound->playback.get_channel())
				{
					if (! channelTagCondition->check(channel->tags))
					{
						continue;
					}
				}
			}
			sound->playback.pause();
		}
	}

	for_every(ep, extraPlaybacks)
	{
		ep->playback.pause();
	}
}

void Scene::resume()
{
	if (! is_paused())
	{
		return;
	}

	Concurrency::ScopedSpinLock lockSounds(soundsOpLock);
	TagCondition const * channelTagCondition = nullptr;
	if (auto* game = Game::get())
	{
		channelTagCondition = &game->get_sound_fade_channel_tag_condition();
	}

	isPaused = false;
	for_every(sound, sounds)
	{
		if (sound->state == PlayingSceneSoundState::InUse &&
			sound->shouldBeHeard &&
			sound->soundSource.is_set())
		{
			if (channelTagCondition)
			{
				if (auto* channel = sound->playback.get_channel())
				{
					if (!channelTagCondition->check(channel->tags))
					{
						continue;
					}
				}
			}
			sound->playback.resume_with_fade_in();
		}
	}

	for_every(ep, extraPlaybacks)
	{
		ep->playback.resume();
	}
}

void Scene::find_sounds_to_play(float _deltaTime, ::Framework::Sound::Camera const & _camera)
{
	MEASURE_PERFORMANCE_COLOURED(soundSceneFindSoundsToPlay, Colour::zxCyanBright);
	Room* inRoom = _camera.get_in_room();

	if (!inRoom)
	{
		return;
	}

	// add reverb from starting room only
#ifdef SOUND_NO_REVERBS
	if (false)
#endif
	if (auto * roomReverb = inRoom->get_reverb())
	{
		SceneSoundReverb* existing = nullptr;
		SceneSoundReverb* firstAvailable = nullptr;
		for_every(reverb, reverbs)
		{
			if (reverb->reverb == roomReverb)
			{
				existing = reverb;
				break;
			}
			if (!reverb->reverbInstance.is_active())
			{
				firstAvailable = reverb;
			}
		}
		if (!existing && firstAvailable)
		{
			existing = firstAvailable;
		}
		if (!existing)
		{
			reverbs.grow_size(1);
			existing = &reverbs.get_last();
		}
		existing->reverb = roomReverb;
		existing->target = 1.0f;
		if (!existing->reverbInstance.is_active())
		{
			existing->reverbInstance.start(roomReverb->get_reverb());
		}
	}

	// add for owner
	if (IModulesOwner const * ownerIMO = _camera.get_owner())
	{
		if (ModuleSound * sound = ownerIMO->get_sound())
		{
			add_sounds_from(_deltaTime, sound->access_sounds(), nullptr, &_camera, true);
		}
	}

#ifdef INVESTIGATE_SOUND_SYSTEM
	output(TXT("[sound scene] find sounds to play, room r%p \"%S\""), inRoom, inRoom->get_name().to_char())
#endif

	// go through all rooms starting with this one, scan rooms by distance (closest are scanned first)
	ARRAY_STACK(ScannedRoom, rooms, MAX_ROOM_TO_SCAN);

	todo_hack(TXT("additional offset hack applied, until presence path breaking is solved"));
	ScannedRoom::find(rooms, inRoom, _camera.get_total_placement().get_translation(),
		ScanRoomInfo().with_concurrent_rooms_limit(GameConfig::get()->get_max_concurrent_apparent_room_sound_sources())
		);
	
	{
		MEASURE_PERFORMANCE_COLOURED(soundScene_addSoundsFromScannedRooms, Colour::zxBlue);
		// add sounds from scanned list
		for_every(room, rooms)
		{
			add_sounds_from(_deltaTime, *room, _camera);
		}
	}
}

void Scene::add_sounds_from(float _deltaTime, ScannedRoom & _room, ::Framework::Sound::Camera const & _camera)
{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
	MEASURE_PERFORMANCE_CONTEXT(_room.room->get_performance_context_info());
#endif

	{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
		MEASURE_PERFORMANCE(soundScene_addSounds_objects);
#endif
		for_every_ptr(object, _room.room->get_objects())
		{
			if (ModuleSound* moduleSound = object->get_sound())
			{
				if (!moduleSound->access_sounds().is_empty())
				{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
					MEASURE_PERFORMANCE_CONTEXT(object->get_performance_context_info());
#endif
					add_sounds_from(_deltaTime, moduleSound->access_sounds(), &_room, &_camera, false);
				}
			}
		}
	}
	{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
		MEASURE_PERFORMANCE(soundScene_addSounds_temporaryObjects);
#endif
		for_every_ptr(object, _room.room->get_temporary_objects())
		{
			if (ModuleSound* moduleSound = object->get_sound())
			{
				if (!moduleSound->access_sounds().is_empty())
				{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
					MEASURE_PERFORMANCE_CONTEXT(object->get_performance_context_info());
#endif
					add_sounds_from(_deltaTime, moduleSound->access_sounds(), &_room, &_camera, false);
				}
			}
		}
	}
	{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
		MEASURE_PERFORMANCE(soundScene_addSounds_doors);
#endif
		for_every_ptr(door, _room.room->get_doors())
		{
			if (door != _room.enteredThroughDoor)
			{
				an_assert(door->get_door());
				auto* d = door->get_door();
				if (door->is_visible() && !d->access_sounds().is_empty())
				{
					add_sounds_from(_deltaTime, d->access_sounds(), &_room, &_camera, false);
				}
			}
		}
	}
	if (! _room.room->access_sounds().is_empty())
	{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
		MEASURE_PERFORMANCE(soundScene_addSounds_room);
#endif
		add_sounds_from(_deltaTime, _room.room->access_sounds(), &_room, &_camera, false);
	}
}

void Scene::add_sounds_from(float _deltaTime, SoundSources & _soundSources)
{
	add_sounds_from(_deltaTime, _soundSources, nullptr, nullptr, false);
}

void Scene::add_sounds_from(float _deltaTime, SoundSources & _soundSources, ScannedRoom * _room, ::Framework::Sound::Camera const * _camera, bool _ownerOnly)
{
	// nothing to add as we don't have owner
	if (_camera && _ownerOnly && !_camera->get_owner())
	{
		return;
	}
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
	MEASURE_PERFORMANCE(soundScene_addSounds);
#endif

	for_every(sound, _soundSources.access_sounds())
	{
		if (!sound->is_set()) // might be not compressed!
		{
			continue;
		}

#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
		MEASURE_PERFORMANCE_CONTEXT(sound->get()->access_sample_instance().get_sample() ? sound->get()->access_sample_instance().get_sample()->get_performance_context_info() : String::empty());
#endif

		SoundSource* soundSource = sound->get();
		Sample const * soundSample = soundSource->get_sample_instance().get_sample();

		if (!soundSample)
		{
			continue;
		}
		
		if (_camera)
		{
			if (_ownerOnly)
			{
				if (!soundSource->is_hearable_only_by_owner() || soundSource->get_owner() != _camera->get_owner())
				{
					// not owner
					continue;
				}
			}
			else
			{
				if (soundSource->is_hearable_only_by_owner())
				{
					// skip all hearable only by owner, we will add them in separate step
					continue;
				}
			}
		}

		float const soundSampleHearingDistance = soundSource->get_hearing_distance();
		float const soundSampleHearingDistanceBeyondFirstRoom = soundSource->get_hearing_distance_beyond_first_room().is_set() ? soundSource->get_hearing_distance_beyond_first_room().get() : -1.0f;

		{
			if (_camera && _room)
			{
				if (soundSampleHearingDistance > 0.0f &&
					_room->distance > soundSampleHearingDistance)
				{
					// won't be heard in this room
					continue;
				}
				if (soundSampleHearingDistanceBeyondFirstRoom >= 0.0f &&
					_room->distanceBeyondFirstRoom > soundSampleHearingDistanceBeyondFirstRoom)
				{
					// won't be heard in this room
					continue;
				}
			}
		}

		{
			int const maxConcurrentmaxConcurrentApparentSoundSources = soundSample->get_max_concurrent_apparent_sound_sources();
			if (maxConcurrentmaxConcurrentApparentSoundSources > 0)
			{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
				MEASURE_PERFORMANCE(soundScene_addSounds_checkConcurrent);
#endif

				int existing = 0;
				// check if during this scanning procedure there were added sound sources
				for_every(existingSound, sounds)
				{
					if (existingSound->state == PlayingSceneSoundState::InUse && existingSound->shouldBeHeard)
					{
						if (existingSound->soundSource.get() == soundSource)
						{
							++existing;
							if (existing >= maxConcurrentmaxConcurrentApparentSoundSources)
							{
								break;
							}
						}
					}
				}
				if (existing >= maxConcurrentmaxConcurrentApparentSoundSources)
				{
					// already enough sound sources
					continue;
				}
			}
		}
		
		PresencePath soundSourcePresencePath; // path from owner to target, through which door we should go to end at target
		soundSourcePresencePath.be_temporary_snapshot();
		Vector3 stringPulledSoundSourceLocation = Vector3::zero;
		float stringPulledSoundSourceDistance = 0.0f;
		float stringPulledSoundSourceDistanceBeyondFirstRoom = 0.0f;
		float soundSourcePureSoundLevel = 1.0f;

		if (_camera && _room)
		{
			Transform inRoomPlacement = soundSource->get_placement_in_room(_room->room);

#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (_deltaTime > 0.0f)
			{
				debug_filter(soundSceneFindSounds);
				if (debug_is_filter_allowed())
				{
					debug_context(sound->get()->get_in_room());
					debug_draw_sphere(true, true, Colour::red, 0.5f, Sphere(inRoomPlacement.get_translation(), 0.1f));
					debug_no_context();
				}
				debug_no_filter();
			}
#endif

			// calculate sound source location and distance (relative to camera)
#ifdef AN_DEVELOPMENT_OR_PROFILER
			Vector3 debugStringPulledSoundLocation;
#endif

			// build presence path to sound source
			an_assert(_camera);
			soundSourcePresencePath.set_owner_presence(_camera->ownerForSoundSourcePresencePath);
			soundSourcePresencePath.follow_if_target_lost();
			soundSourcePresencePath.set_target_presence(soundSource->get_presence());
			if (!soundSource->get_presence())
			{
				// might be possible for sounds played in rooms that have no proper owner
				soundSourcePresencePath.allow_no_target_presence();
			}

#ifdef AN_DEVELOPMENT_OR_PROFILER
			/*
			static bool emptyLineRequired = true;
			if (::System::Input::get()->has_key_been_pressed(::System::Key::C))
			{
				if (emptyLineRequired)
				{
					output(TXT("_____"));
				}
				output(TXT("sound source %i"), sound->get());
				emptyLineRequired = false;
			}
			else
			{
				emptyLineRequired = true;
			}
			*/
#endif

			// very simple version of string pulling
			// we just need to know approximate location of first node on the path (or straight path if available)
			// we start from sound location and pull towards camera
			// on the way we build path (reversed, then we will set in in path object in reverse order)
			{
				// from check:
				//			camera => a => b => c
				//	etdr:	c->b, b->a, a->camera
				// from camera:
				//			owner => d => e => camera
				//	fotc:	owner->d, d->e, e->camera
				ScannedRoom* check = _room;
				ARRAY_STACK(DoorInRoom*, enteredThroughDoorReverse, MAX_ROOM_TO_SCAN);
				ARRAY_STACK(DoorInRoom*, enteredThroughDoorReverseAll, MAX_ROOM_TO_SCAN);
				ARRAY_STACK(DoorInRoom*, fromOwnerToCamera, MAX_ROOM_TO_SCAN);
				while (check)
				{
					if (check->enteredThroughDoor)
					{
						enteredThroughDoorReverse.push_back(check->enteredThroughDoor);
						enteredThroughDoorReverseAll.push_back(check->enteredThroughDoor);
					}
					check = check->enteredFrom;
				}
				for_every_ref(door, _camera->fromOwnerToCamera.get_through_doors())
				{
					fromOwnerToCamera.push_back(door);
				}

				{
					// for cases below we need a special approach
					//			camera => a => owner => b => c
					//	etdr:	c->b, b->owner, owner->a, a->camera
					//	fotc:	owner->a, a->camera
					// if we would join them fotc + etdr(reversed+on other side), we would have:
					//	sspp:	owner->a, a->camera : camera->a, a->owner, owner->b, b->c
					// but we want
					//	sspp:	owner->b, b->c
					// we just have to remove doubled from the end for etdr and fotc

					while (!fromOwnerToCamera.is_empty() && !enteredThroughDoorReverse.is_empty() &&
						   fromOwnerToCamera.get_last() == enteredThroughDoorReverse.get_last())
					{
						fromOwnerToCamera.pop_back();
						enteredThroughDoorReverse.pop_back();
					}
				}

				// string pulling should use from camera to sound! that's why we keep enteredThroughDoorReverseAll and we do not add fromOwnerToCamera
				PathStringPulling stringPulling;
				{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
					MEASURE_PERFORMANCE(soundScene_addSounds_stringPulling);
#endif

					an_assert(_camera);
					stringPulling.set_start(_camera->get_in_room(), _camera->get_total_placement().get_translation());
					stringPulling.set_end(_room->room, inRoomPlacement.get_translation());

					// add from camera, from owner to where camera is (but cut, check above)
					{
						an_assert(_camera);
						for_every_ptr(throughDoor, fromOwnerToCamera)
						{
							soundSourcePresencePath.push_through_door(throughDoor);
						}
					}

					// add reversed with door on the other side (but cut, check above)
					for_every_reverse_ptr(enteredThroughDoor, enteredThroughDoorReverse)
					{
						soundSourcePresencePath.push_through_door(enteredThroughDoor->get_door_on_other_side());
					}
					for_every_reverse_ptr(enteredThroughDoor, enteredThroughDoorReverseAll)
					{
						stringPulling.push_through_door(enteredThroughDoor->get_door_on_other_side());
					}

					stringPulling.string_pull();
					an_assert(soundSourcePresencePath.debug_check_if_path_valid());
				}

				float lastNodeDist = 0.0f;
				for_every(node, stringPulling.get_path())
				{
					float nodeDist = (node->nextNode - node->node).length();
					stringPulledSoundSourceDistance += nodeDist;
					lastNodeDist = nodeDist;

					todo_note(TXT("hardcoded but maybe should be set per sample?"));
					if (node->enteredThroughDoor)
					{
						float fullSoundHear = clamp(Vector3::dot((node->nextNode - node->node).normal(), (node->node - node->prevNode).normal()) * 0.2f + 0.8f, 0.60f, 1.0f);
						if (node->enteredThroughDoor)
						{
							if (auto* d = node->enteredThroughDoor->get_door())
							{
								if (auto* dt = d->get_door_type())
								{
									if (dt->does_ignore_sound_distance_beyond_first_room())
									{
										// less obstruction
										fullSoundHear = 1.0f - (1.0f - fullSoundHear) * 0.25f;
									}
								}
							}
						}
						soundSourcePureSoundLevel = max(0.0f, (soundSourcePureSoundLevel - (1.0f - fullSoundHear) * 0.05f)* fullSoundHear);
#ifdef AN_DEVELOPMENT_OR_PROFILER
						if (_deltaTime > 0.0f)
						{
							debug_filter(soundSceneFindSounds);
							if (debug_is_filter_allowed())
							{
								debug_context(node->enteredThroughDoor->get_in_room());
								debug_draw_line(true, Colour::blue, node->node, node->nextNode);
								debug_no_context();
							}
							debug_no_filter();
						}
#endif
					}
					else
					{
#ifdef AN_DEVELOPMENT_OR_PROFILER
						if (_deltaTime > 0.0f)
						{
							debug_filter(soundSceneFindSounds);
							if (debug_is_filter_allowed())
							{
								debug_context(stringPulling.get_starting_room());
								debug_draw_line(true, Colour::green, node->node, node->nextNode);
								debug_no_context();
							}
							debug_no_filter();
						}
#endif
					}
				}
				stringPulledSoundSourceDistanceBeyondFirstRoom = stringPulledSoundSourceDistance - lastNodeDist;

				Vector3 stringPulledSoundLocation = stringPulling.get_path().get_first().nextNode;
				stringPulledSoundSourceLocation = _camera->get_total_placement().location_to_local(stringPulledSoundLocation);

#ifdef AN_DEVELOPMENT_OR_PROFILER
				debugStringPulledSoundLocation = stringPulledSoundLocation;
#endif
			}

#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (_deltaTime > 0.0f)
			{
				debug_filter(soundSceneFindSounds);
				if (debug_is_filter_allowed())
				{
					debug_context(_camera->get_in_room());
					debug_push_transform(_camera->get_placement());
					// where is everyone?
					debug_pop_transform();
					debug_no_context();
				}
				debug_no_filter();
			}
#endif

			if (soundSampleHearingDistance > 0.0f &&
				stringPulledSoundSourceDistance > soundSampleHearingDistance)
			{
				// beyond range
				continue;
			}
			if (soundSampleHearingDistanceBeyondFirstRoom >= 0.0f &&
				stringPulledSoundSourceDistanceBeyondFirstRoom > soundSampleHearingDistanceBeyondFirstRoom)
			{
				// beyond range
				continue;
			}

#ifdef AN_DEBUG_RENDERER
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (_deltaTime > 0.0f)
			{
				Vector3 cameraLoc = _camera->get_total_placement().get_translation();
				Vector3 debugApparentSoundLocation = cameraLoc + (debugStringPulledSoundLocation - cameraLoc).normal() * stringPulledSoundSourceDistance;
				debug_filter(soundSceneFindSounds);
				if (debug_is_filter_allowed())
				{
					debug_context(_camera->get_in_room());
					debug_draw_line(true, Colour::yellow, cameraLoc, debugApparentSoundLocation);
					debug_draw_line(true, Colour::green, cameraLoc, debugStringPulledSoundLocation);
					debug_draw_sphere(true, true, Colour::yellow, 0.1f, Sphere(debugApparentSoundLocation, 0.1f));
					debug_no_context();
				}
				debug_no_filter();
			}
#endif
#endif
		}

		{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
			MEASURE_PERFORMANCE(soundScene_addSounds_add);
#endif

			// find if there is such sound already existing
			PlayingSceneSound* existingSound = nullptr;
			if (_room)
			{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
				MEASURE_PERFORMANCE(soundScene_addSounds_checkExisting);
#endif
				for_every(checkSound, sounds)
				{
					if (checkSound->state == PlayingSceneSoundState::InUse &&
						checkSound->soundSource.get() == soundSource)
					{
						if (PresencePath::is_same(checkSound->path, soundSourcePresencePath))
						{
							existingSound = checkSound;
							break;
						}
					}
				}
			}
			else
			{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
				MEASURE_PERFORMANCE(soundScene_addSounds_checkExisting);
#endif
				for_every(checkSound, sounds)
				{
					if (checkSound->state == PlayingSceneSoundState::InUse &&
						checkSound->soundSource.get() == soundSource &&
						! checkSound->path.is_active() &&
						! checkSound->roomRef)
					{
						existingSound = checkSound;
						break;
					}
				}
			}

			if (auto* soundSample = soundSource->get_sample_instance().get_sound_sample())
			{
				float stringPulledSoundSourceDistancePt = soundSampleHearingDistance > 0.0f ? stringPulledSoundSourceDistance / soundSampleHearingDistance : 0.5f;
				int soundSamplePriority = soundSample->get_params().priority;
				if (!existingSound)
				{
					if ((existingSound = get_available_sound(soundSamplePriority, stringPulledSoundSourceDistancePt)))
					{
						existingSound->path = soundSourcePresencePath;
						existingSound->roomRef = _room? _room->room : nullptr;
#ifdef AN_DEVELOPMENT
						existingSound->path.set_user_function(TXT("sound path"));
#endif
						existingSound->soundSource = soundSource;
						existingSound->occlusionLevel.clear();
						existingSound->apparentVelocity.clear();
					}
				}

				if (existingSound)
				{
					// update locations
					existingSound->stringPulledLocation = stringPulledSoundSourceLocation;
					existingSound->apparentVelocityDeltaTime = _deltaTime;
					existingSound->soundSamplePriority = soundSamplePriority;
					existingSound->stringDistance = stringPulledSoundSourceDistance;
					existingSound->stringDistancePt = stringPulledSoundSourceDistancePt;
					existingSound->stringDistanceBeyondFirstRoom = stringPulledSoundSourceDistanceBeyondFirstRoom;
					float newOcclusionLevel = 1.0f - soundSourcePureSoundLevel * (_room ? _room->soundLevel : 1.0f);
					if (!existingSound->occlusionLevel.is_set())
					{
						existingSound->occlusionLevel = newOcclusionLevel;
					}
					else
					{
						existingSound->occlusionLevel = blend_to_using_speed_based_on_time(existingSound->occlusionLevel.get(), newOcclusionLevel, 0.5f, _deltaTime);
					}
					existingSound->update_apparent_sound_location();

					// mark to play
					existingSound->shouldBeHeard = true;

					// mark used
					existingSound->state = PlayingSceneSoundState::InUse;
				}
			}
		}
	}
}

void Scene::update_extra_playbacks(Optional<SceneUpdateContext> const& _context)
{
#ifndef SOUND_NO_LOW_PASS
	if (MainConfig::global().get_sound().enableOcclusionLowPass)
	{
		SceneUpdateContext context = _context.get(SceneUpdateContext());
		for_every(ep, extraPlaybacks)
		{
			ep->playback.set_low_pass_gain(max(0.1f, (1.0f - context.dull)));
		}
	}
#endif
	for_every(ep, extraPlaybacks)
	{
		if (!ep->playback.is_playing())
		{
			extraPlaybacks.remove_fast_at(for_everys_index(ep));
			break; // we will remove more during following frame, we shouldn't get too many anyway
		}
	}
}

void Scene::update_playback(float _deltaTime, Optional<SceneUpdateContext> const& _context)
{
	MEASURE_PERFORMANCE_COLOURED(soundSceneUpdatePlayback, Colour::zxRedBright);

#ifndef SOUND_NO_LOW_PASS
	SceneUpdateContext context = _context.get(SceneUpdateContext());
#endif

	if (auto * ss = ::System::Sound::get())
	{
		todo_note(TXT("velocity!"));
		ss->set_listener_3d_params(Vector3::zero);
	}

#ifdef OUTPUT_SOUNDS_EXTENDED
	bool debugOutputSounds = false;
	static int debugFrame = 0;
	++debugFrame;
#endif

#ifndef SOUND_NO_LOW_PASS
	bool useLowPass = MainConfig::global().get_sound().enableOcclusionLowPass;
#endif

	for_every(sound, sounds)
	{
		if (sound->state != PlayingSceneSoundState::NotUsed)
		{
#ifdef MEASURE_PERFORMANCE_SOUND_DETAILED
			MEASURE_PERFORMANCE_CONTEXT(sound->soundSource->access_sample_instance().get_sample() ? sound->soundSource->access_sample_instance().get_sample()->get_performance_context_info() : String::empty());
			MEASURE_PERFORMANCE(soundSceneUpdatePlayback_sound);
#endif
#ifdef AN_DEBUG_RENDERER
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (_deltaTime > 0.0f)
			{
				debug_filter(soundSceneShowSounds);
				debug_context(sound->soundSource->get_in_room());
				debug_draw_sphere(true, true, Colour::blue, 0.01f, Sphere(sound->soundSource->get_placement_in_room(sound->soundSource->get_in_room()).get_translation(), 0.1f));
				debug_no_context();

				Vector3 cameraLoc = camera.get_total_placement().location_to_world(Vector3(0.0f, 0.0f, -0.02f));
				Vector3 debugApparentSoundLocation = camera.get_total_placement().location_to_world(sound->apparentSoundLocation);
				Vector3 debugApparentVelocity = camera.get_total_placement().vector_to_world(sound->apparentVelocity.get(Vector3::zero));
				debug_context(camera.get_in_room());
				Colour colour = Colour::yellow;
				if (sound->state == PlayingSceneSoundState::FadingOut)
				{
					colour = Colour::c64Brown; // fading out from scene
				}
				debug_draw_arrow(true, Colour::green, cameraLoc, camera.get_total_placement().location_to_world(sound->stringPulledLocation));
				debug_draw_arrow(true, colour, cameraLoc, debugApparentSoundLocation);
				debug_draw_arrow(true, Colour::purple, debugApparentSoundLocation, debugApparentSoundLocation+ debugApparentVelocity);
				debug_draw_sphere(true, true, colour, 0.1f, Sphere(debugApparentSoundLocation, 0.08f));
				debug_no_context();
				debug_no_filter();
			}
#endif
#endif
			bool removeSilentSound = false;
			bool resumeSound = false; // if we add sound, we add it paused, we have to set everything and resume it
			// hardcoded value to fade out sound in case we don't know what has happened to it - maybe it is too far away, maybe it was removeed
			if (sound->shouldBeHeard && sound->state == PlayingSceneSoundState::InUse)
			{
				// it should be heard, but maybe it wants to be stopped?
				if (sound->soundSource->should_stop())
				{
					// check whether we're still playing
					if (sound->playback.is_playing())
					{
						::Sound::PlaybackState::Type playbackState = sound->playback.get_state();
						if (playbackState == ::Sound::PlaybackState::Playing ||
							playbackState == ::Sound::PlaybackState::FadingIn ||
							playbackState == ::Sound::PlaybackState::FadingOut)
						{
							// fade out sound that is playing
							float fadeOut = sound->soundSource->get_fade_out();
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
							IF_EXTENDED_DEBUG(SoundScene)
#endif
							{
								if (playbackState != ::Sound::PlaybackState::FadingOut)
								{
									output_colour(0, 1, 1, 1);
									output(TXT("[sndscn] on stop: fade out %S [%.1fm]"), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
									output_colour();
									debugOutputSounds = true;
								}
							}
#endif
							sound->playback.fade_out(fadeOut);
						}
						else if (playbackState == ::Sound::PlaybackState::Silence)
						{
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
							IF_EXTENDED_DEBUG(SoundScene)
#endif
							{
								output_colour(0, 1, 1, 1);
								output(TXT("[sndscn] on stop: remove silent sound %S [%.1fm]"), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
								output_colour();
								debugOutputSounds = true;
							}
#endif
							removeSilentSound = true;
							sound->playback.stop(); // stop it right now, it's silent, right?
						}
					}
					else
					{
						// we're no longer playing, remove that sound!
						removeSilentSound = true;
						sound->playback.stop(); // stop just in case
					}

					if (removeSilentSound)
					{
						// drop that sound as we want to remove it and we wanted actually to stop it!
						sound->soundSource->drop();
					}

					// keep scene fade as it is
				}
				else
				{
					if (Sample const * sample = sound->soundSource->get_sample_instance().get_sample())
					{
						if (!sound->playback.is_playing() ||
							sound->playback.get_sample() != sample->get_sample()) // maybe we still keep old sample? change it!
						{
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
							IF_EXTENDED_DEBUG(SoundScene)
#endif
							{
								if (!sound->playback.is_playing())
								{
									output_colour(0, 1, 1, 1);
									output(TXT("[sndscn] playback not playing %S"), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"));
									output_colour();
								}
								if (sound->playback.get_sample() != sample->get_sample())
								{
									output_colour(0, 1, 1, 1);
									output(TXT("[sndscn] playback differs, not %S"), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"));
									output_colour();
								}
							}
#endif
							// stop any current playback and replace it with new one
							sound->playback.stop();
							sound->playback.decouple();
							sound->playback = sample->get_sample()->set_to_play_remain_paused(); // will be paused until we tell it otherwise
#ifdef INVESTIGATE_SOUND_SYSTEM
							sound->playback.set_dev_info(sample->get_name().to_string());
#endif
							if (sound->soundSource->get_distances().is_set())
							{
								// override_ distances
								sound->playback.set_3d_min_max_distances(sound->soundSource->get_distances().get());
							}
							float at = sound->soundSource->get_at();// not used +sound->playback.distance_to_time(sound->stringDistance);
							// hardcoded value to fade in sound in case we should start in the middle, if we won't it will be changed
							if (sound->soundSource->has_just_started())
							{
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
								IF_EXTENDED_DEBUG(SoundScene)
#endif
								{
									output_colour(0, 1, 1, 1);
									output(TXT("[sndscn] start new sound %S [%.1fm]"), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
									output_colour();
									debugOutputSounds = true;
								}
#endif
								// just started, do not scene fade in, fade in sound normally
								sound->playback.fade_in_on_start(sound->soundSource->get_fade_in());
								sound->currentSceneFade = 1.0f;
								sound->targetSceneFade = 1.0f;
							}
							else
							{
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
								IF_EXTENDED_DEBUG(SoundScene)
#endif
								{
									output_colour(0, 1, 1, 1);
									output(TXT("[sndscn] fade in existing sound %S [%.1fm]"), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
									output_colour();
									debugOutputSounds = true;
								}
#endif
								// sound already existed, scene fade in
								sound->playback.fade_in_on_start(0.1f);
								sound->currentSceneFade = 0.0f;
								sound->targetSceneFade = 1.0f;
							}
							// we want all sounds to be identical to sound source
							// and as sound source has sample volume (and we use final volume), we clear it here
							sound->playback.set_sample_volume();
							sound->playback.set_sample_pitch(); // same for pitch
							// we will be setting volume and pitch in a second
							sound->playback.set_at(at);
							{
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
								IF_EXTENDED_DEBUG(SoundScene)
#endif
								{
									output_colour(0, 1, 1, 1);
									output(TXT("[sndscn] start playback (at:%.3f->%.3f [%p]) %S [%.1fm]"), sound->soundSource->get_at(), sound->playback.get_at(), sound->soundSource.get(), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
									output_colour();
									debugOutputSounds = true;
								}
#endif
							}
							resumeSound = true; // we added it paused!
						}
						else
						{
							::Sound::PlaybackState::Type playbackState = sound->playback.get_state();
							if (playbackState != ::Sound::PlaybackState::Playing &&
								playbackState != ::Sound::PlaybackState::FadingIn)
							{
								sound->playback.fade_in(sound->soundSource->get_fade_in());
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
								IF_EXTENDED_DEBUG(SoundScene)
#endif
								{
									output_colour(0, 1, 1, 1);
									output(TXT("[sndscn] fade in as it was not playing or not fading in %S [%.1fm]"), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
									output_colour();
									debugOutputSounds = true;
								}
#endif
							}
							sound->targetSceneFade = 1.0f;
						}
					}
					else
					{
						// no sample?
						removeSilentSound = true;
						sound->playback.stop(); // stop just in case
					}
				}
			}
			else
			{
				// scene fade out, play normally, just get with the scene volume down
				removeSilentSound = true;
				sound->targetSceneFade = 0.0f;
			}

			// update scene fade
			float const sceneFadeTime = 1.0f;
			sound->currentSceneFade = blend_to_using_speed_based_on_time(sound->currentSceneFade, sound->targetSceneFade, sceneFadeTime, _deltaTime);
			if (sound->soundSource->get_be_at().is_set())
			{
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
				IF_EXTENDED_DEBUG(SoundScene)
#endif
				{
					output_colour(0, 1, 1, 1);
					output(TXT("[sndscn] set playback (at:%.3f->%.3f [%p]) %S [%.1fm]"), sound->soundSource->get_be_at().get(), sound->playback.get_at(), sound->soundSource.get(), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
					output_colour();
					debugOutputSounds = true;
				}
#endif
				sound->playback.set_at(sound->soundSource->get_be_at().get());
			}

			if (removeSilentSound || resumeSound || sound->playback.is_playing())
			{
				// update even if we call to remove sound, we might be fading out
				sound->playback.set_3d_params(sound->apparentSoundLocation, sound->apparentVelocity.get(Vector3::zero));
				todo_note(TXT("occlusion for reverb!"));
				float occlusionAsVolumeModifierFor2D = 1.0f;
#ifndef SOUND_NO_OCCLUSSION
				if (sound->playback.is_3d_sample())
				{
					sound->playback.set_3d_occlusion(sound->occlusionLevel.get(), sound->occlusionLevel.get());
				}
				else
				{
					occlusionAsVolumeModifierFor2D = 1.0f - sound->occlusionLevel.get();
				}
#endif
				sound->playback.advance_fading(_deltaTime);
				if (sound->soundSource.is_set())
				{
					float beyondFirstRoomVolume = 1.0f;
					if (sound->soundSource->get_sample_instance().is_set())
					{
						an_assert(sound->soundSource->get_sample_instance().get_sample());
						Optional<float> const & sampleDistanceBeyondFirstRoom = sound->soundSource->get_hearing_distance_beyond_first_room();
						if (sampleDistanceBeyondFirstRoom.is_set())
						{
							beyondFirstRoomVolume = max(0.0f, 1.0f - sound->stringDistanceBeyondFirstRoom / sampleDistanceBeyondFirstRoom.get());
						}
					}
#ifndef SOUND_NO_LOW_PASS
					if (useLowPass)
					{
						sound->playback.set_low_pass_gain(max(0.1f, (1.0f - 0.7f * sound->occlusionLevel.get()) * (1.0f - context.dull)));
					}
#endif
					sound->playback.set_volume(
						sound->soundSource->get_sample_instance().get_final_volume() *
						sound->soundSource->get_volume_alterations() *
						sound->currentSceneFade * beyondFirstRoomVolume *
						occlusionAsVolumeModifierFor2D /* if it is 2d, see above */);
					Optional<float> const & pitchRelatedToFadeBase = sound->soundSource->get_pitch_related_to_fade_base();
					sound->playback.set_pitch(
						sound->soundSource->get_sample_instance().get_final_pitch() *
						sound->soundSource->get_pitch_alterations() *
						(pitchRelatedToFadeBase.is_set() ? max(0.0f, (pitchRelatedToFadeBase.get() + (1.0f - pitchRelatedToFadeBase.get()) * sound->playback.get_current_fade())) : 1.0f));
				}
				sound->playback.update_volume();
				sound->playback.update_pitch();
				if (resumeSound)
				{
					sound->playback.resume(); // all parameters set!
				}
			}
			if (removeSilentSound)
			{
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef OUTPUT_SOUNDS_EXTENDED_TRIGGER_REMOVALS
#ifdef AN_ALLOW_EXTENDED_DEBUG
				IF_EXTENDED_DEBUG(SoundScene)
#endif
				{
					if (sound->state == PlayingSceneSoundState::InUse)
					{
						output_colour(0, 1, 1, 1);
						output(TXT("[sndscn] \"remove sound\" triggered (at:%3f->%.3f [%p]) %S [%.1fm]"), sound->soundSource->get_at(), sound->playback.get_at(), sound->soundSource.get(), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
						output_colour();
						debugOutputSounds = true;
					}
				}
#endif
#endif
				if (sound->playback.get_current_volume() == 0.0f) // will use all our previous volumes set
				{
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
					IF_EXTENDED_DEBUG(SoundScene)
#endif
					{
						output_colour(0, 1, 1, 1);
						output(TXT("[sndscn] stop sound as its volume is 0.0 (at:%3f->%.3f [%p]) %S [%.1fm]"), sound->soundSource->get_at(), sound->playback.get_at(), sound->soundSource.get(), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
						output_colour();
						debugOutputSounds = true;
					}
#endif
					// will remove it
					sound->playback.stop();
				}
				if (! sound->playback.is_playing())
				{
#ifdef OUTPUT_SOUNDS_EXTENDED
#ifdef AN_ALLOW_EXTENDED_DEBUG
					IF_EXTENDED_DEBUG(SoundScene)
#endif
					{
						output_colour(0, 1, 1, 1);
						output(TXT("[sndscn] actual removal of sound (at:%3f->%.3f [%p]) %S [%.1fm]"), sound->soundSource->get_at(), sound->playback.get_at(), sound->soundSource.get(), sound->soundSource->get_sample_instance().get_sample() ? sound->soundSource->get_sample_instance().get_sample()->get_name().to_string().to_char() : TXT("<no name>"), sound->stringDistance);
						output_colour();
						debugOutputSounds = true;
					}
#endif
					sound->playback.decouple();

					sound->state = PlayingSceneSoundState::NotUsed;
					// this is last one
					if (sound + 1 == sounds.end())
					{
						// remove all sounds that are not in use from end of array
						int keepNumber = sounds.get_size();
						for_every_reverse(goBack, sounds)
						{
							if (goBack->state != PlayingSceneSoundState::NotUsed)
							{
								break;
							}
							--keepNumber;
						}
						an_assert(keepNumber >= 0);
						sounds.set_size(keepNumber);
						break;
					}
				}
			}
		}
	}

	int activeReverbNumToKeep = 0;
	for_every(reverb, reverbs)
	{
		if (reverb->reverbInstance.get_active() != reverb->target)
		{
			reverb->reverbInstance.set_active(blend_to_using_speed_based_on_time(reverb->reverbInstance.get_active(), reverb->target, 1.0f, _deltaTime));
			if (reverb->reverbInstance.get_active() <= 0.0f && _deltaTime > 0.0f)
			{
				reverb->reverbInstance.stop();
				reverb->reverb = nullptr;
			}
		}
		if (reverb->reverbInstance.get_active() > 0.0f)
		{
			activeReverbNumToKeep = for_everys_index(reverb) + 1;
		}
	}
	if (reverbs.get_size() > activeReverbNumToKeep && _deltaTime > 0.0f)
	{
		for_range(int, i, reverbs.get_size(), activeReverbNumToKeep - 1)
		{
			reverbs[i].reverbInstance.stop();
		}
		reverbs.set_size(activeReverbNumToKeep);
	}

#ifdef OUTPUT_SOUNDS_EXTENDED
	bool debugOutputSoundsNow = false;

#ifdef AN_ALLOW_EXTENDED_DEBUG
	IF_EXTENDED_DEBUG(SoundScene)
	{
		if (debugOutputSounds)
		{
			debugOutputSoundsNow = true;
		}
	}

	IF_EXTENDED_DEBUG(SoundSceneLogEveryFrame)
	{
		if (::System::Core::get_delta_time() > 0.0f)
		{
			debugOutputSoundsNow = true;
		}
	}
#endif

	if (debugOutputSoundsNow)
	{
		LogInfoContext tempLog;
		tempLog.log(TXT("sound frame: %i, in %i %S"), debugFrame, camera.get_in_room(), camera.get_in_room() ? camera.get_in_room()->get_name().to_char() : TXT("--"));
		log(tempLog, true, Colour::blue);
		tempLog.output_to_output();
	}
#endif
}

void Scene::log(LogInfoContext & _log, bool _skipNotUsed, Optional<Colour> const & _defaultColour) const
{
	{
		_log.set_colour(_defaultColour);
		_log.log(TXT("reverbs"));
		LOG_INDENT(_log);
		int idx = 0;
		int reverbLines = 0;
		for_every(reverb, reverbs)
		{
			if (reverb->reverb)
			{
				_log.set_colour(Colour::green);
				_log.log(TXT("%2i [%5.3f:%5.3f] %S"), idx,
					reverb->reverbInstance.get_active(),
					reverb->reverbInstance.get_actual_active(),
					reverb->reverb->get_name().to_string().to_char()
				);
				++reverbLines;
			}
			else if (!_skipNotUsed)
			{
				_log.set_colour(Colour::green);
				_log.log(TXT("%2i"), idx);
			}
			++idx;
		}
		while (reverbLines < 4)
		{
			_log.log(TXT("")); // empty line
			++reverbLines;
		}
	}
	{
		_log.log(TXT("sounds"));
		LOG_INDENT(_log);
		int idx = 0;
		for_every(sound, sounds)
		{
			if (sound->state != PlayingSceneSoundState::NotUsed)
			{
				bool isPlaying = sound->playback.is_playing();
				_log.set_colour(_defaultColour);
				if (sound->state == PlayingSceneSoundState::FadingOut)
				{
					_log.set_colour(_defaultColour.is_set()? _defaultColour.get() * 0.5f : _defaultColour);
				}
				if (sound->soundSource->get_sample_instance().get_sample())
				{
				}
				else if (sound->playback.get_sample())
				{
					_log.set_colour(Colour::yellow);
				}
				else
				{
					_log.set_colour(Colour::red);
				}
				auto * sample = sound->soundSource->get_sample_instance().get_sample();
#ifndef SHORT_LOG
				auto * soundSample = sound->soundSource->get_sample_instance().get_sound_sample();
#endif
				Optional<float> loudness;
				Optional<float> maxRecentLoudness;
#ifdef AN_SOUND_LOUDNESS_METER
				loudness = sound->playback.get_loudness();
				maxRecentLoudness = sound->playback.get_recent_max_loudness();
#endif
				if (!isPlaying)
				{
					_log.set_colour(Colour::lerp(0.2f, Colour::black, Colour::white));
				}
				else
				{
					_log.set_colour(::Sound::LoudnessMeter::loudness_to_colour(maxRecentLoudness.get(loudness.get(-100.0f))));
				}
#ifdef SHORT_LOG
				_log.log(TXT("%2i [%c] [d:%8.2fm>%6.2fm] vol:%3.0f [%03.0fdB^%03.0fdB] (at:%7.3f->%7.3f) %S"),
#else
				_log.log(TXT("%2i [%c] [d:%8.2fm>%6.2fm] vol:%3.0f [%03.0fdB^%03.0fdB] <-- sfd:%3.0f fd:%3.0f oc:%3.0f v:%3.0f p:%3.0f  (at:%7.3f->%7.3f) p%c%03i, %S"),
#endif
					idx,
					sample ? (sound->state == PlayingSceneSoundState::FadingOut ? 'f' : 'p')
					: (sound->playback.get_sample() ? '!' : '?'),
					sound->stringDistance,
					sound->stringDistanceBeyondFirstRoom,

					sound->playback.get_current_volume() * 100.0f,
					loudness.get(0.0f),
					maxRecentLoudness.get(0.0f),

					sound->soundSource->get_at(),
					sound->playback.get_at(),

#ifndef SHORT_LOG
					sound->currentSceneFade * 100.0f,
					sound->playback.get_current_fade() * 100.0f,
					sound->occlusionLevel.get() * 100.0f,
					sound->soundSource->get_sample_instance().get_set_volume() * 100.0f * sound->soundSource->get_volume_alterations(),
					sound->playback.get_current_pitch() * 100.0f * sound->soundSource->get_pitch_alterations(),

					sound->soundSource->get_at(),
					sound->playback.get_at(),

					soundSample ? (soundSample->get_params().priority > 0? '+' : (soundSample->get_params().priority < 0 ? '-' : '=')) : ' ',
					soundSample ? abs(soundSample->get_params().priority) : 0,
#endif

					sample ? sample->get_name().to_string().to_char() : TXT("--")
				);
			}
			else
			{
				an_assert(!sound->playback.is_playing());
				if (!_skipNotUsed)
				{
					_log.set_colour(Colour::grey);
					_log.log(TXT("%2i"), idx);
				}
			}
			++idx;
		}
	}
	{
		_log.log(TXT("extra playbacks"));
		LOG_INDENT(_log);
		int idx = 0;
		for_every(ep, extraPlaybacks)
		{
			{
				Optional<float> loudness;
				Optional<float> maxRecentLoudness;
#ifdef AN_SOUND_LOUDNESS_METER
				loudness = ep->playback.get_loudness();
				maxRecentLoudness = ep->playback.get_recent_max_loudness();
#endif
#ifdef AN_SOUND_LOUDNESS_METER
				_log.set_colour(::Sound::LoudnessMeter::loudness_to_colour(maxRecentLoudness.get(loudness.get(-100.0f))));
#else
				_log.set_colour(_defaultColour);
				if (ep->playback.get_state() == ::Sound::PlaybackState::FadingOut)
				{
					_log.set_colour(_defaultColour.is_set()? _defaultColour.get() * 0.5f : _defaultColour);
				}
#endif
				auto * soundSample = ep->playback.get_sample();
				_log.log(TXT("%2i [%c] vol:%3.0f [%03.0fdB^%03.0fdB] <-- fd:%3.0f p:%3.0f  (at:%7.3f) p%c%03i %S"),
					idx,
					soundSample ? (ep->playback.get_state() == ::Sound::PlaybackState::FadingOut ? 'f' : 'p') : '!',

					ep->playback.get_current_volume() * 100.0f,

					loudness.get(0.0f),
					maxRecentLoudness.get(0.0f),

					ep->playback.get_current_fade() * 100.0f,
					ep->playback.get_current_pitch() * 100.0f,

					ep->playback.get_at(),

					soundSample ? (soundSample->get_params().priority > 0 ? '+' : (soundSample->get_params().priority < 0 ? '-' : '=')) : ' ',
					soundSample ? abs(soundSample->get_params().priority) : 0,

					ep->id.to_char()
				);
			}
			++idx;
		}
	}
	_log.set_colour();
}

PlayingSceneSound* Scene::get_available_sound(int _priority, float _distancePt)
{
	for_every(sound, sounds)
	{
		if (sound->state == PlayingSceneSoundState::NotUsed)
		{
			an_assert(!sound->playback.is_playing());
			return sound;
		}
	}

	if (sounds.get_size() < soundsLimit)
	{
		sounds.grow_size(1);
		return &sounds.get_last();
	}
	else
	{
		int lowestPriority = _priority;
		float greatestDistancePt = _distancePt;
		int bestIdx = NONE;
		// remove one played
		for_every(sound, sounds)
		{
			if (sound->state != PlayingSceneSoundState::NotUsed)
			{
				if (sound->soundSamplePriority < lowestPriority ||
					(sound->soundSamplePriority == lowestPriority && sound->stringDistancePt > greatestDistancePt))
				{
					lowestPriority = sound->soundSamplePriority;
					greatestDistancePt = sound->stringDistancePt;
					bestIdx = for_everys_index(sound);
				}
			}
		}
		if (bestIdx != NONE)
		{
			auto* sound = &sounds[bestIdx];
			// stop immediately and replace	
			sound->playback.stop();
			sound->playback.decouple();
			sound->soundSource->drop();
			sound->state = PlayingSceneSoundState::NotUsed;
			return sound;
		}
	}

	return nullptr;
}