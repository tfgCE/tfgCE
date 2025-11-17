#pragma once

#include "..\..\core\types\optional.h"
#include "..\..\core\types\string.h"

struct Range;

namespace Framework
{
	struct SampleTextCache
	{
	public:
		String const& get(String const & _text, Optional<float> const & _at = NP, OPTIONAL_ OUT_ bool* _clear = nullptr) const;
		Range get_range(String const& _text) const;

	private:
		struct CachedText
		{
			float at = 0.0f;
			bool clear = false;
			String text;
		};
		CACHED_ mutable String cachedLocalisedTextsFor;
		CACHED_ mutable String wholeText;
		CACHED_ mutable Array<CachedText> cachedLocalisedTexts;
	};
};
