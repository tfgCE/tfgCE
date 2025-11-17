#pragma once

#include "iPostProcessGraphProcessingInputRenderTargetsProvider.h"

#include "..\postProcessRenderTargetPointer.h"

#include "..\..\..\core\containers\map.h"

namespace Framework
{
	/**
	 *	Implementation that allows to associate render target with name
	 */
	class PostProcessGraphProcessingInputRenderTargetsProvider
	: public IPostProcessGraphProcessingInputRenderTargetsProvider
	{
	public:
		void clear() { renderTargets.clear(); }
		void set(Name const & _name, ::System::RenderTarget* _renderTarget, int _dataTextureIndex) { renderTargets[_name] = PostProcessRenderTargetPointer(_renderTarget, _dataTextureIndex); }

	public: // IPostProcessGraphProcessingInputRenderTargetsProvider
		implement_ PostProcessRenderTargetPointer const & get_input_render_target(Name const & _inputName) const { if (auto * rt = renderTargets.get_existing(_inputName)) { return *rt; } else { return PostProcessRenderTargetPointer::empty(); } }

	private:
		Map<Name, PostProcessRenderTargetPointer> renderTargets;
	};
};
