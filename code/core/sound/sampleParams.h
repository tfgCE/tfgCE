#pragma once

#include "..\math\math.h"
#include "..\types\name.h"

struct String;

class Serialiser;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Sound
{
	struct SampleParams
	{
		bool looped = false;
		bool is3D = false; // type="3d"
		bool useAs3D = false; // type="2dUseAs3d"
		bool headRelative = false;
		bool isStream = false;
		float dopplerLevel = 1.0f; // 0 none, 1 normal, 5 exaggerated
		Range volume = Range(1.0f);
		Range pitch = Range(1.0f);
		Range distances = Range(1.0f, 100.0f); // min and max distance
		Name channel;
		Optional<bool> usesReverbs;
		int priority = 0; // -127 (least important) to 127 (most important)

		SampleParams();

		bool load_from_xml(IO::XML::Node const * _node);

		bool serialise(Serialiser & _serialiser);

		void prepare_create_sound_flags(REF_ int & _flags) const;
	};

};
