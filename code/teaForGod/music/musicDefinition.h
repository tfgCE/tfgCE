#pragma once

#include "musicTrack.h"

namespace TeaForGodEmperor
{
	struct MusicFade
	{
	public:
		// at least one should be defined
		Framework::UsedLibraryStored<Framework::Sample> from;
		Framework::UsedLibraryStored<Framework::Sample> to;

		Optional<float> fadeTime;
		Optional<float> fadeInTime;
		Optional<float> fadeOutTime;
		Optional<Range> startPt;
		Optional<Range> startAt;
		bool queue = false;
		bool syncPt = false; // overlays are synced to the main one
		float beatSync = 0.0f; // realitive to bar length (4 beats)
		Optional<int> syncDir; // if +1 will sync forward (will play earlier part)
		Framework::UsedLibraryStored<Framework::Sample> bumper;
		Optional<Range> bumperCooldown;
		Optional<float> coolBumperFadeTime; // if bumper is omitted

	public:
		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		void load_on_demand_if_required() const;
		void unload_for_load_on_demand() const;
	};

	struct MusicAutoCombatDefinition
	{
		struct Element
		{
			float highIntensityTime = 6.0f;
			float lowIntensityTime = 12.0f;
			float delay = 0.2f;

			Element() {}
			Element(Optional<float> const& _highIntensityTime, Optional<float> const& _lowIntensityTime, Optional<float> const& _delay) : highIntensityTime(_highIntensityTime.get(0.0f)), lowIntensityTime(_lowIntensityTime.get(0.0f)), delay(_delay.get(0.0f)) {}

			bool load_from_xml(IO::XML::Node const* _node, tchar const * _childName, Framework::LibraryLoadingContext& _lc);
		};

		Element combat;
		Element indicatePresence;
		Element bumpHigh;
		Element bumpLow;
		Element calmDown;

		MusicAutoCombatDefinition();

		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
	};

	struct MusicDefinition
	{
	public:
		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		void load_on_demand_if_required() const;
		void unload_for_load_on_demand() const;

		MusicTrack const* get_track(Name const & _id, MusicTrack::Type _type) const;

		MusicFade const* get_fade(Framework::Sample const* _from, Framework::Sample const* _to) const;

		MusicAutoCombatDefinition const& get_auto_combat_definition() const { return autoCombatDefinition; }
	private:
		Array<MusicFade> fades;
		Array<MusicTrack> tracks;
		MusicAutoCombatDefinition autoCombatDefinition;
	};


};
