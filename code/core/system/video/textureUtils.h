#pragma once

#include "..\..\types\colour.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace System
{
	struct TextureSetup;

	namespace TextureUtils
	{
		struct AddBorder
		{
			bool load_from_xml(IO::XML::Node const* _node);

			bool is_required() const { return addBorder > 0.0f; }

			void process(REF_ TextureSetup & _setup);

		private:
			float addBorder = 0.0f;
			float smoothBorder = 0.0f;
			float fullBorderSourceThreshold = 1.0f; // values are scaled from this to 1.0 -> s -> min(s / fbst, 1)
			Colour borderColour = Colour::black;
		};
	};
};
