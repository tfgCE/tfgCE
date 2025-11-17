#pragma once

#include "iPostProcessGraphProcessingResolutionProvider.h"

#include "..\..\..\core\containers\map.h"
#include "..\..\..\core\math\math.h"

namespace Framework
{
	/**
	 *	Implementation that allows to associate render target with name
	 */
	class PostProcessGraphProcessingResolutionProvider
	: public IPostProcessGraphProcessingResolutionProvider
	{
	public:
		void clear() { resolutions.clear(); }
		void set(Name const & _name, VectorInt2 const & _resolution) { resolutions[_name] = _resolution; }

	public: // IPostProcessGraphProcessingInputRenderTargetsProvider
		implement_ VectorInt2 const & get_resolution_for_processing(Name const & _resolutionLookup) const { if (auto * fr = resolutions.get_existing(_resolutionLookup)) { return *fr; } else { return VectorInt2::zero; } }

	private:
		Map<Name, VectorInt2> resolutions;
	};
};
