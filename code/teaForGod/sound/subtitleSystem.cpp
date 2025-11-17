#include "subtitleSystem.h"

#include "..\game\game.h"
#include "..\game\gameConfig.h"
#include "..\modules\gameplay\modulePilgrim.h"

#include "..\overlayInfo\overlayInfoSystem.h"
#include "..\overlayInfo\elements\oie_text.h"

#include "..\pilgrimage\pilgrimage.h"
#include "..\pilgrimage\pilgrimageInstance.h"

#include "..\..\core\debug\debug.h"

#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\object\actor.h"
#include "..\..\framework\sound\soundSample.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

bool SubtitleSystem::withSubtitles = true;
float SubtitleSystem::subtitlesScale = 1.0f;

SubtitleSystem* SubtitleSystem::s_system = nullptr;

SubtitleSystem::SubtitleSystem()
{
	an_assert(!s_system);
	s_system = this;
}

SubtitleSystem::~SubtitleSystem()
{
	remove_all();
	if (s_system == this)
	{
		s_system = nullptr;
	}
}

void SubtitleSystem::set_distance_for_subtitles(Optional<float> const& _distanceForSubtitles)
{
	if (s_system)
	{
		s_system->distanceForSubtitles = _distanceForSubtitles;
	}
}

void SubtitleSystem::set_background_for_subtitles(Optional<Colour> const& _backgroundForSubtitles)
{
	if (s_system)
	{
		s_system->backgroundForSubtitles = _backgroundForSubtitles;
	}
}

void SubtitleSystem::set_offset_for_subtitiles(Optional<Rotator3> const& _offsetForSubtitles)
{
	if (s_system)
	{
		s_system->offsetForSubtitles = _offsetForSubtitles;
	}
}

void SubtitleSystem::remove_if_removed(SubtitleId const& _id, SubtitleId const& _ifRemovedId)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);
	for (int i = 0; i < subtitles.get_size(); ++i)
	{
		auto& s = subtitles[i];
		if (s.id == _id)
		{
			s.removeIfRemovedId = _ifRemovedId;
		}
	}
}

void SubtitleSystem::remove(SubtitleId const& _id, float _delay)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);
	for(int i = 0; i < subtitles.get_size(); ++ i)
	{
		auto& s = subtitles[i];
		if (s.id == _id)
		{
			if (_delay > 0.0f)
			{
				s.timeToRemove = _delay;
			}
			else
			{
				if (auto* e = s.overlayElement.get())
				{
					e->deactivate();
				}
				s.overlayElement.clear();
				subtitles.remove_at(i);
				--i;
			}
		}
	}
}

void SubtitleSystem::remove_all()
{
	Concurrency::ScopedSpinLock lock(accessLock, true);
	for_every(s, subtitles)
	{
		if (auto* e = s->overlayElement.get())
		{
			e->deactivate();
		}
	}
	subtitles.clear();
}

void SubtitleSystem::add_user_log(String const& _line, bool _continuation)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	userLog.push_back(_line);
	++userLogVer;

	timeSinceUserLogActive = 0.0f;
}

void SubtitleSystem::clear_user_log()
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	userLog.clear();
	++userLogVer;
}

void SubtitleSystem::trim_user_log(int _length)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	int userLogLength = userLog.get_size();
	if (userLogLength > _length)
	{
		while (userLogLength > _length)
		{
			userLog.pop_front();
			--userLogLength;
		}
		++userLogVer;
	}
}

void SubtitleSystem::for_user_log(int _lines, std::function<void(String const&)> _do)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	int userLogLength = userLog.get_size();
	int idx = _lines - userLogLength;
	for_every(line, userLog)
	{
		if (idx)
		{
			_do(*line);
		}
		++idx;
	}
}

SubtitleId SubtitleSystem::get(String const& _subtitle) const
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_every(s, subtitles)
	{
		if (s->subtitle == _subtitle)
		{
			return s->id;
		}
	}

	return NONE;
}

SubtitleId SubtitleSystem::get(Name const& _subtitleLS) const
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_every(s, subtitles)
	{
		if (s->subtitleLS == _subtitleLS)
		{
			return s->id;
		}
	}

	return NONE;
}

SubtitleId SubtitleSystem::add(Name const& _subtitleLS, Optional<SubtitleId> const& _continuationOfId, Optional<bool> const& _forced, Optional<Colour> const& _colour, Optional<Colour> const& _backgroundColour)
{
	add_user_log(LOC_STR(_subtitleLS), _continuationOfId.is_set());

	Concurrency::ScopedSpinLock lock(accessLock, true);

	{
		bool ok = false;
		while (!ok)
		{
			ok = freeId != NONE;
			for_every(s, subtitles)
			{
				if (s->id == freeId)
				{
					++freeId;
					ok = false;
				}
			}
		}
	}

	Subtitle s;
	s.id = freeId;
	s.forced = _forced.get(false);
	s.colour = _colour;
	s.backgroundColour = _backgroundColour;
	s.continuationOfId = _continuationOfId.get(NONE);
	++freeId;
	s.subtitleLS = _subtitleLS;
	subtitles.push_front(s);

	return s.id;
}

SubtitleId SubtitleSystem::add(String const & _subtitle, Optional<SubtitleId> const& _continuationOfId, Optional<bool> const& _forced, Optional<Colour> const& _colour, Optional<Colour> const& _backgroundColour)
{
	add_user_log(_subtitle, _continuationOfId.is_set());

	Concurrency::ScopedSpinLock lock(accessLock, true);

	{
		bool ok = false;
		while (!ok)
		{
			ok = freeId != NONE;
			for_every(s, subtitles)
			{
				if (s->id == freeId)
				{
					++freeId;
					ok = false;
				}
			}
		}
	}

	Subtitle s;
	s.id = freeId;
	s.forced = _forced.get(false);
	s.colour = _colour;
	s.backgroundColour = _backgroundColour;
	s.continuationOfId = _continuationOfId.get(NONE);
	++freeId;
	s.subtitle = _subtitle;
	subtitles.push_front(s);

	return s.id;
}

void SubtitleSystem::update(float _deltaTime)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	if (! userLog.is_empty())
	{
		timeSinceUserLogActive += _deltaTime;
		if (timeSinceUserLogActive > 5.0f * 60.0f) // clear after 5 minutes
		{
			clear_user_log();
		}
	}

	float useDistanceForSubtitles = distanceForSubtitles.get(0.5f);
	if (! distanceForSubtitles.is_set())
	{
		todo_note(TXT("does it actually make sense to change the subtitles depth?"));
		todo_multiplayer_issue(TXT("we just get player here"));
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
				{
					auto& poi = mp->access_overlay_info();
					if (poi.is_trying_to_show_map())
					{
						useDistanceForSubtitles = 0.5f;
					}
					else
					{
						useDistanceForSubtitles = 1.0f;
					}
				}
			}
		}
	}

	// create overlay info elements
	{
		for (int i = 0; i < subtitles.get_size(); ++i)
		{
			auto& s = subtitles[i];
			bool removeIt = false;
			if (s.removeIfRemovedId != NONE)
			{
				bool removed = true;
				for_every(os, subtitles)
				{
					if (os->id == s.removeIfRemovedId)
					{
						removed = false;
						break;
					}
				}
				if (removed)
				{
					removeIt = true;
				}
			}
			if (s.timeToRemove > 0.0f)
			{
				s.timeToRemove -= _deltaTime;
				if (s.timeToRemove <= 0.0f)
				{
					removeIt = true;
				}
			}

			if (removeIt || (!withSubtitles && !s.forced))
			{
				if (auto* e = s.overlayElement.get())
				{
					e->deactivate();
				}
				s.overlayElement.clear();
				s.lineOffsetRead = false;
			}

			if (removeIt)
			{
				subtitles.remove_at(i);
				--i;
			}
		}

		{
			// check if we should add a new line and if we have enough space
			// if we don't have, remove all previous subtitles first
			for_every(s, subtitles)
			{
				if (!s->overlayElement.get() && (! withSubtitles || s->forced))
				{
					int maxLines = 0;
					if (auto* gc = GameConfig::get_as<GameConfig>())
					{
						maxLines = gc->get_subtitles_line_limit();
					}
					if (maxLines > 0)
					{
						int linesActive = 0;
						for (int i = 0; i < subtitles.get_size(); ++i)
						{
							auto& s = subtitles[i];
							if (auto* oiet = fast_cast<OverlayInfo::Elements::Text>(s.overlayElement.get()))
							{
								int lc = oiet->get_lines_count();
								if (lc > 0 && (oiet->is_active() || oiet->get_faded_active() > 0.5f)) // only if active, otherwise ignore it and be over it
								{
									linesActive = max(lc, linesActive);
								}
							}
						}
						if (linesActive >= maxLines)
						{
							for (int i = 0; i < subtitles.get_size(); ++i)
							{
								auto& s = subtitles[i];
								if (auto* e = s.overlayElement.get())
								{
									e->deactivate();
									s.overlayElement.clear();
									subtitles.remove_at(i);
									--i;
								}
							}
							break;
						}
					}
				}
			}
		}

		// check if we should reset line offset (after we're done with removal of lines)
		{
			bool anySubtitleHasOE = false;
			for_every(s, subtitles)
			{
				if (s->overlayElement.is_set())
				{
					anySubtitleHasOE = true;
					break;
				}
			}
			if (!anySubtitleHasOE)
			{
				lineOffset = 0;
				lineOffsetForId = NONE;
			}
		}

		for_every_reverse(s, subtitles) // in reverse to readd in proper order
		{
			if (!s->overlayElement.get() && (withSubtitles || s->forced))
			{
				if (overlayInfoSystem)
				{
					if (lineOffsetForId != NONE && s->continuationOfId == lineOffsetForId)
					{
						// we continue existing one
						--lineOffset;
					}
					lineOffset = max(0, lineOffset);

					String text;
					text.access_data().make_space_for(100);
					for_count(int, i, lineOffset)
					{
						text += TXT("~");
					}
					if (s->subtitleLS.is_valid())
					{
						text += LOC_STR(s->subtitleLS);
					}
					else
					{
						text += s->subtitle;
					}

					auto* e = new OverlayInfo::Elements::Text(text);
					Rotator3 offset = offsetForSubtitles.get(Rotator3(-10.0f, 0.0f, 0.0f));

					e->with_location(OverlayInfo::Element::Relativity::RelativeToAnchor/*RelativeToCameraFlat*/);
					e->with_follow_camera_location_z(0.1f);
					e->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchorPY, offset);
					e->with_distance(useDistanceForSubtitles);
					e->with_render_layer(OverlayInfo::Element::RenderLayer::Subtitle);

					if (s->backgroundColour.is_set())
					{
						if (s->backgroundColour.get().a > 0.0f)
						{
							e->with_background(s->backgroundColour.get());
						}
					}
					else if (backgroundForSubtitles.is_set())
					{
						if (backgroundForSubtitles.get().a > 0.0f)
						{
							e->with_background(backgroundForSubtitles.get());
						}
					}
					else
					{
						e->with_background(Colour::black.with_alpha(0.5f));
					}
					if (s->colour.is_set())
					{
						e->with_colour(s->colour.get());
					}

					e->with_scale(subtitlesScale);
					e->with_max_line_width(50);

					e->with_vertical_align(1.0f);

					e->set_cant_be_deactivated_by_system();

					overlayInfoSystem->add(e);

					s->overlayElement = e;
				}
				else
				{
					break;
				}
			}
			if (!s->lineOffsetRead)
			{
				if (auto* oiet = fast_cast<OverlayInfo::Elements::Text>(s->overlayElement.get()))
				{
					int lc = oiet->get_lines_count();
					if (lc > 0 && (oiet->is_active() || oiet->get_faded_active() > 0.5f)) // only if active, otherwise ignore it and be over it
					{
						lineOffset = lc + 1; // to get an extra space
						lineOffsetForId = s->id;
						s->lineOffsetRead = true;
					}
					else
					{
						break;
					}
				}
			}
		}
	}
}

void SubtitleSystem::log(LogInfoContext& _log) const
{
	Concurrency::ScopedSpinLock lock(accessLock, true);
	{
		_log.set_colour();
		for_every(s, subtitles)
		{
			if (s->subtitleLS.is_valid())
			{
				_log.log(TXT("%S"), LOC_STR(s->subtitleLS).to_char());
			}
			else
			{
				_log.log(TXT("%S"), s->subtitle.to_char());
			}
		}
	}
}
