#pragma once

#include "postProcessRenderTarget.h"

#include "..\..\core\system\video\video.h"

namespace Framework
{
	/*
	 *	Points at data texture in render target or in post process render target.
	 *	Manages post process render target's usage.
	 */
	struct PostProcessRenderTargetPointer
	{
	public:
		static PostProcessRenderTargetPointer const & empty() { return s_empty; }

		PostProcessRenderTargetPointer();
		PostProcessRenderTargetPointer(PostProcessRenderTargetPointer const & _other);
		PostProcessRenderTargetPointer(PostProcessRenderTarget* _postProcessRenderTarget, int _dataTextureIndex);
		PostProcessRenderTargetPointer(::System::RenderTarget* _renderTarget, int _dataTextureIndex);
		~PostProcessRenderTargetPointer();

		PostProcessRenderTargetPointer& operator=(PostProcessRenderTargetPointer const & _other);

		void clear();

		void mark_next_frame_releasable();

		void set(PostProcessRenderTargetPointer const & _postProcessRenderTargetPointer);
		void set(PostProcessRenderTarget* _postProcessRenderTarget, int _dataTextureIndex);
		void set(::System::RenderTarget* _renderTarget, int _dataTextureIndex);

		::System::RenderTarget* get_render_target() const;
		int get_data_texture_index() const;

		::System::TextureID get_texture_id() const;

	private:
		static PostProcessRenderTargetPointer s_empty;

		PostProcessRenderTarget* postProcessRenderTarget;
		::System::RenderTargetPtr renderTarget;
		int dataTextureIndex;

		void add_usage();
		void release_usage();
	};

};
