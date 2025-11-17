#pragma once

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		namespace Usage
		{
			typedef int Flags;

			enum Type
			{
				World			= 1,
				LoaderHub		= 2,
				CustomLoader	= 4,

				All = World | LoaderHub | CustomLoader,
			};
		};
	};
};
