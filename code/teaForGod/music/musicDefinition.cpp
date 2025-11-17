#include "musicDefinition.h"

#include "..\teaForGod.h"
#include "..\..\framework\library\libraryPrepareContext.h"
#include "..\..\framework\sound\soundSample.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

#define LOAD_ON_DEMAND_IF_REQUIRED(sample) if (auto* s = sample.get()) { s->load_on_demand_if_required(); }
#define UNLOAD_FOR_LOAD_ON_DEMAND(sample) if (auto* s = sample.get()) { s->unload_for_load_on_demand(); }

//

bool MusicFade::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= from.load_from_xml(_node, TXT("from"), _lc);
	result &= to.load_from_xml(_node, TXT("to"), _lc);

	fadeTime.load_from_xml(_node, TXT("fadeTime"));
	fadeInTime.load_from_xml(_node, TXT("fadeInTime"));
	fadeOutTime.load_from_xml(_node, TXT("fadeOutTime"));
	startAt.load_from_xml(_node, TXT("startAt"));
	startPt.load_from_xml(_node, TXT("startPt"));
	queue = _node->get_bool_attribute_or_from_child_presence(TXT("queue"), queue);
	syncPt = _node->get_bool_attribute_or_from_child_presence(TXT("syncPt"), syncPt);
	beatSync = _node->get_float_attribute(TXT("beatSync"), beatSync);
	syncDir.load_from_xml(_node, TXT("syncDir"));
	result &= bumper.load_from_xml(_node, TXT("bumper"), _lc);
	bumperCooldown.load_from_xml(_node, TXT("bumperCooldown"));
	coolBumperFadeTime.load_from_xml(_node, TXT("coolBumperFadeTime"));

	return result;
}

bool MusicFade::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= from.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= to.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	result &= bumper.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

void MusicFade::load_on_demand_if_required() const
{
	LOAD_ON_DEMAND_IF_REQUIRED(from);
	LOAD_ON_DEMAND_IF_REQUIRED(to);
}

void MusicFade::unload_for_load_on_demand() const
{
	UNLOAD_FOR_LOAD_ON_DEMAND(from);
	UNLOAD_FOR_LOAD_ON_DEMAND(to);
}

//

MusicAutoCombatDefinition::MusicAutoCombatDefinition()
{
	combat = Element(6.0f, 12.0f, 0.2f);
	indicatePresence = Element(NP, 8.0f, 0.5f);
	bumpHigh = Element(3.0f, NP, 0.5f);
	bumpLow = Element(NP, 5.0f, 0.5f);
	calmDown = Element(1.0f, 4.0f, NP);
}

bool MusicAutoCombatDefinition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	combat.load_from_xml(_node, TXT("combat"), _lc);
	indicatePresence.load_from_xml(_node, TXT("indicatePresence"), _lc);
	bumpHigh.load_from_xml(_node, TXT("bumpHigh"), _lc);
	bumpLow.load_from_xml(_node, TXT("bumpLow"), _lc);
	calmDown.load_from_xml(_node, TXT("bumpLow"), _lc);

	return result;
}

//

bool MusicAutoCombatDefinition::Element::load_from_xml(IO::XML::Node const* _node, tchar const* _childName, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	bool clearRequired = true;
	for_every(node, _node->children_named(_childName))
	{
		if (clearRequired)
		{
			clearRequired = false;
			highIntensityTime = 0.0f;
			lowIntensityTime = 0.0f;
			delay = 0.0f;
		}

		highIntensityTime = node->get_float_attribute_or_from_child(TXT("highIntensityTime"), highIntensityTime);
		lowIntensityTime = node->get_float_attribute_or_from_child(TXT("lowIntensityTime"), lowIntensityTime);
		delay = node->get_float_attribute_or_from_child(TXT("delay"), delay);
	}

	return result;
}

//

bool MusicDefinition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

#ifndef NO_MUSIC
	struct ReadTrackInfo
	{
		MusicTrack::Type type;
		tchar const * name;
		ReadTrackInfo() {}
		ReadTrackInfo(MusicTrack::Type const& _type, tchar const * _name) : type(_type), name(_name) {}
	};

	Array< ReadTrackInfo> readTrackInfos;
	readTrackInfos.push_back(ReadTrackInfo(MusicTrack::Ambient, TXT("ambient")));
	readTrackInfos.push_back(ReadTrackInfo(MusicTrack::AmbientIncidental, TXT("ambientIncidental")));
	readTrackInfos.push_back(ReadTrackInfo(MusicTrack::Combat, TXT("combat")));
	readTrackInfos.push_back(ReadTrackInfo(MusicTrack::Incidental, TXT("incidental")));

	for_every(rti, readTrackInfos)
	{
		for_every(node, _node->children_named(rti->name))
		{
			for_every(trackNode, node->children_named(TXT("track")))
			{
				MusicTrack track;
				if (track.load_from_xml(trackNode, _lc))
				{
					track.type = rti->type; // override
					tracks.push_back(track);
				}
			}
		}
	}

	for_every(nodeById, _node->children_named(TXT("tracks")))
	{
		Name id = nodeById->get_name_attribute(TXT("id"));
		for_every(rti, readTrackInfos)
		{
			for_every(trackNode, nodeById->children_named(rti->name))
			{
				MusicTrack track;
				if (track.load_from_xml(trackNode, _lc))
				{
					track.id = id; // override
					track.type = rti->type; // override
					tracks.push_back(track);
				}
			}
		}
	}

	for_every(node, _node->children_named(TXT("fades")))
	{
		for_every(fadeNode, node->children_named(TXT("fade")))
		{
			MusicFade fade;
			if (fade.load_from_xml(fadeNode, _lc))
			{
				fades.push_back(fade);
			}
		}
	}

	for_every(node, _node->children_named(TXT("autoCombat")))
	{
		result &= autoCombatDefinition.load_from_xml(node, _lc);
	}
#endif

	return result;
}

bool MusicDefinition::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(track, tracks)
	{
		result &= track->prepare_for_game(_library, _pfgContext);
	}

	for_every(fade, fades)
	{
		result &= fade->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

void MusicDefinition::load_on_demand_if_required() const
{
	for_every(fade, fades)
	{
		fade->load_on_demand_if_required();
	}
	for_every(track, tracks)
	{
		track->load_on_demand_if_required();
	}
}

void MusicDefinition::unload_for_load_on_demand() const
{
	for_every(fade, fades)
	{
		fade->unload_for_load_on_demand();
	}
	for_every(track, tracks)
	{
		track->unload_for_load_on_demand();
	}
}

MusicTrack const* MusicDefinition::get_track(Name const& _id, MusicTrack::Type _type) const
{
	for_every(track, tracks)
	{
		if (track->id == _id &&
			track->type == _type)
		{
			return track;
		}
	}

	return nullptr;
}

MusicFade const* MusicDefinition::get_fade(Framework::Sample const* _from, Framework::Sample const* _to) const
{
	MusicFade const* bestFade = nullptr;
	int bestFadeMatch = 0;
	for_every(fade, fades)
	{
		if (bestFadeMatch < 4 && fade->from.get() == _from && fade->to.get() == _to)
		{
			// both match
			bestFadeMatch = 4;
			bestFade = fade;
		}
		if (bestFadeMatch < 3 &&
			(fade->to.get() == _to && !fade->from.get()))
		{
			// matches to (no from)
			bestFadeMatch = 3;
			bestFade = fade;
		}
		if (bestFadeMatch < 2 &&
			(fade->from.get() == _from && ! fade->to.get()))
		{
			// matches from (no to)
			bestFadeMatch = 2;
			bestFade = fade;
		}
		if (bestFadeMatch < 1 && !fade->from.is_set() && !fade->to.is_set())
		{
			// none defined
			bestFadeMatch = 1;
			bestFade = fade;
		}
	}

	return bestFade;
}
