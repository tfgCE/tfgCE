#pragma once

#include "..\..\core\math\math.h"

#include "postProcessRenderTarget.h"

#include "..\..\core\concurrency\spinLock.h"

namespace Framework
{
	class PostProcessRenderTargetManager
	{
	public:
		PostProcessRenderTargetManager();
		~PostProcessRenderTargetManager();

		void set_render_target_size(VectorInt2 const & _size);

		PostProcessRenderTarget* get_available_render_target_for(VectorInt2 const & _size, float _scale, bool _withMipMaps, ::System::FragmentShaderOutputSetup const & _setup); // it adds usage to found render target

		void delete_unused();

		void log(LogInfoContext & _log);

	public:
		void add_usage_for(PostProcessRenderTarget* _renderTarget);
		void release_usage_for(PostProcessRenderTarget* _renderTarget);

	private:
		::Concurrency::SpinLock lock = Concurrency::SpinLock(TXT("Framework.PostProcessRenderTargetManager.lock"));

		VectorInt2 renderTargetSize;
		Array<PostProcessRenderTarget*> renderTargets;

		// we release render targets to be available only in next frame
		// this way we won't get situation when we reuse same render target twice in a frame
		// this should be working just fine, but for some reason it does not :/ <- I couldn't even debug it properly using RenderDoc
		// but this way, we should be safe
		int currentFrame = NONE; // taken from system
		static const int FRAME_CHAINS = 3; // let's try more chains, if it helps
		PostProcessRenderTarget* firstUnusedRenderTarget[FRAME_CHAINS];

		void log_internal(LogInfoContext & _log);

		friend struct PostProcessRenderTarget;
	};

};
