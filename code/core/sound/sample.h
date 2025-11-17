#pragma once

#include "sampleParams.h"

#ifdef AN_FMOD
#include "fmod.hpp"
#include "fmod_studio.hpp"
#endif

#include "..\serialisation\serialisableResource.h"
#include "..\serialisation\importer.h"
#include "..\types\name.h"

class Serialiser;

struct String;

namespace Sound
{
	struct Channel;
	struct Playback;

	/**
	 *	It stores low level info about sound. How it attenuates, is it 3d, basic volume etc.
	 *	It's just info for sound system (fmod) how to handle sound.
	 */
	class Sample
	: public SerialisableResource
	{
	public:
		static void initialise_static();
		static void close_static();
		static Importer<Sample, SampleParams> & importer() { return s_importer; }

		Sample();
		virtual ~Sample();

		Playback play(float _fadeIn = 0.0f, Optional<bool> const & _forceOwnFading = NP, Channel const * _channelOverride = nullptr, bool _keepPaused = false) const;
		Playback set_to_play_remain_paused(Optional<bool> const& _forceOwnFading = NP, Channel const * _channelOverride = nullptr) const; // as play but without fading in and is kept paused

		float get_length() const { return length; }
		bool is_looped() const { return params.looped; }
		Range const & get_volume() const { return params.volume; }
		Range const & get_pitch() const { return params.pitch; }

		SampleParams const & get_params() const { return params; }

	public:
		bool import_from_file(String const & _filename, SampleParams const & _params);

	public: // SerialisableResource
		virtual bool serialise(Serialiser & _serialiser);

	private:
		static Importer<Sample, SampleParams> s_importer;

		SampleParams params;
#ifdef AN_FMOD
		FMOD::Sound* sound = nullptr;
#endif
		Channel const * channel = nullptr;

		float length = 0.0f;

		Array<int8> plainData; // just to make it easier to save/load

		void process_params();

		void on_loaded();
	};
};
