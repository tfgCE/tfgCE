#pragma once

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

struct SoundCompressorConfig
{
	float threshold = 0.0f;
	float ratio = 2.5f;
	// these are here in seconds while fmod uses ms, there's conversion when setting
	float attack = 0.020f;
	float release = 0.100f;

	bool load_from_xml(IO::XML::Node const* _node);
};

