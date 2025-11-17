#pragma once

#include "displayDrawCommand_textAtBase.h"

namespace Framework
{
	struct DisplayRegion;

	namespace DisplayDrawCommands
	{
		// for vertical alignment use TextAtMultiline
		class TextAt
		: public PooledObject<TextAt>
		, public TextAtBase
		{
			typedef TextAtBase base;
		public:
		};
	};

};
