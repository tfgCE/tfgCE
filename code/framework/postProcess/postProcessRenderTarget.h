#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\system\video\renderTargetPtr.h"

namespace System
{
	struct FragmentShaderOutputSetup;
	struct RenderTargetSetup;
	class RenderTarget;
};

namespace Framework
{
	class PostProcessRenderTargetManager;

	struct PostProcessRenderTarget
	{
	public:
		PostProcessRenderTarget(PostProcessRenderTargetManager * _manager, ::System::RenderTargetSetup& _setup);
		~PostProcessRenderTarget();

		void add_usage();
		void release_usage();

		bool does_match(VectorInt2 const & _size, bool _withMipMaps, ::System::FragmentShaderOutputSetup const & _setup) const;

		::System::RenderTarget* get_render_target() { return renderTarget.get(); }

		void mark_next_frame_releasable() { nextFrameReleasable = true; }

	private: friend class PostProcessRenderTargetManager;
		void internal_add_usage();
		void internal_release_usage();

		void internal_resize(VectorInt2 const & _newSize);

	private:
		void internal_add_to_unused();
		void internal_remove_from_unused();

	private:
		bool nextFrameReleasable = false;

		PostProcessRenderTargetManager* manager;
		int usageCounter;
		::System::RenderTargetPtr renderTarget;

		// unused chain
		PostProcessRenderTarget* nextUnused;
		PostProcessRenderTarget* prevUnused;
	};

};
