#pragma once

#include "..\general\cooldowns.h"

namespace Framework
{
	namespace AI
	{
		struct MessageTemplate
		{
			Name name;
			float delay = 0.0f;
			Optional<float> cooldown;
			bool playerOnly = false;

			bool load_from_xml(IO::XML::Node const * _node);
		};

		struct MessageTemplateCooldowns
		: public Cooldowns<32>
		{
			typedef Cooldowns<32> base;
			void add(MessageTemplate const & _message);
		};
	};
};
