#pragma once

#include "..\..\..\core\globalDefinitions.h"

struct Name;
struct Vector2;

namespace Framework
{
	interface_class IPostProcessGraphProcessingResolutionProvider
	{
	public:
		virtual VectorInt2 const & get_resolution_for_processing(Name const & _resolutionLookup) const = 0;
	};
};
