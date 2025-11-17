#pragma once

#include "energy.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace TeaForGodEmperor
{
	struct PhysicalViolence
	{
		Optional<float> minSpeed; // mul
		Optional<float> minSpeedStrong; // mul
		Optional<Energy> damage; // set
		Optional<Energy> damageStrong; // set

		bool load_from_xml(IO::XML::Node const* _node);

		void override_with(PhysicalViolence const& _pv);
	};
};
