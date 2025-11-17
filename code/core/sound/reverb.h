#pragma once

#ifdef AN_FMOD
#include "fmod.hpp"
#include "fmod_studio.hpp"
#endif

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Sound
{
	class ReverbInstance;

	/**
	 *	Stores reverb setup
	 */
	class Reverb
	{
	public:
		Reverb();
		~Reverb();

		bool load_from_xml(IO::XML::Node const * _node);

	private:
		float power = 1.0f;
#ifdef AN_FMOD
		FMOD_REVERB_PROPERTIES properties = FMOD_PRESET_OFF;
#endif
		friend class ReverbInstance;
	};
};
