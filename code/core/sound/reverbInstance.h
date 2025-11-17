#pragma once

#ifdef AN_FMOD
#include "fmod.hpp"
#include "fmod_studio.hpp"
#endif

namespace Sound
{
	class Reverb;

	class ReverbInstance
	{
	public:
		void start(Reverb const * _reverb);
		void stop();

		bool is_active() const { return active > 0.0f; }

		void set_active(float _active, bool _force = false);
		float get_active() const { return active; }

		float get_actual_active() const { return active * power; }

		void update_active();

	private:
#ifdef AN_FMOD
		// this is fake reverb3D that bases on 0,0,0, as everything we relate to the listener that is in 0,0,0
		FMOD::Reverb3D* reverb3D = nullptr;
#endif
		float active = 0.0f;
		float power = 1.0f; // read from reverb
	};
};
