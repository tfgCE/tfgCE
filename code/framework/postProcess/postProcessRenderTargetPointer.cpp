#include "postProcessRenderTargetPointer.h"

#include "..\..\core\system\video\renderTarget.h"

using namespace Framework;

PostProcessRenderTargetPointer PostProcessRenderTargetPointer::s_empty = PostProcessRenderTargetPointer();

PostProcessRenderTargetPointer::PostProcessRenderTargetPointer()
: postProcessRenderTarget(nullptr)
, renderTarget(nullptr)
, dataTextureIndex(NONE)
{
}

PostProcessRenderTargetPointer::PostProcessRenderTargetPointer(PostProcessRenderTarget* _postProcessRenderTarget, int _dataTextureIndex)
: postProcessRenderTarget(_postProcessRenderTarget)
, renderTarget(nullptr)
, dataTextureIndex(_dataTextureIndex)
{
	add_usage();
}

PostProcessRenderTargetPointer::PostProcessRenderTargetPointer(::System::RenderTarget* _renderTarget, int _dataTextureIndex)
: postProcessRenderTarget(nullptr)
, renderTarget(_renderTarget)
, dataTextureIndex(_dataTextureIndex)
{
}

PostProcessRenderTargetPointer::PostProcessRenderTargetPointer(PostProcessRenderTargetPointer const & _other)
: postProcessRenderTarget(_other.postProcessRenderTarget)
, renderTarget(_other.renderTarget)
, dataTextureIndex(_other.dataTextureIndex)
{
	add_usage();
}

PostProcessRenderTargetPointer::~PostProcessRenderTargetPointer()
{
	clear();
}

PostProcessRenderTargetPointer& PostProcessRenderTargetPointer::operator=(PostProcessRenderTargetPointer const & _other)
{
	release_usage();
	postProcessRenderTarget = _other.postProcessRenderTarget;
	renderTarget = _other.renderTarget;
	dataTextureIndex = _other.dataTextureIndex;
	add_usage();
	return *this;
}

void PostProcessRenderTargetPointer::set(PostProcessRenderTargetPointer const & _postProcessRenderTargetPointer)
{
	if (_postProcessRenderTargetPointer.postProcessRenderTarget)
	{
		set(_postProcessRenderTargetPointer.postProcessRenderTarget, _postProcessRenderTargetPointer.dataTextureIndex);
	}
	else if (_postProcessRenderTargetPointer.renderTarget.is_set())
	{
		set(_postProcessRenderTargetPointer.renderTarget.get(), _postProcessRenderTargetPointer.dataTextureIndex);
	}
	else
	{
		set((PostProcessRenderTarget*)nullptr, 0);
	}
}

void PostProcessRenderTargetPointer::set(PostProcessRenderTarget* _postProcessRenderTarget, int _dataTextureIndex)
{
	release_usage();
	postProcessRenderTarget = _postProcessRenderTarget;
	renderTarget = nullptr;
	dataTextureIndex = _dataTextureIndex;
	add_usage();
}

void PostProcessRenderTargetPointer::set(::System::RenderTarget* _renderTarget, int _dataTextureIndex)
{
	release_usage();
	postProcessRenderTarget = nullptr;
	renderTarget = _renderTarget;
	dataTextureIndex = _dataTextureIndex;
	add_usage();
}

void PostProcessRenderTargetPointer::clear()
{
	release_usage();
	postProcessRenderTarget = nullptr;
	renderTarget = nullptr;
	dataTextureIndex = NONE;
}

void PostProcessRenderTargetPointer::add_usage()
{
	if (postProcessRenderTarget)
	{
		postProcessRenderTarget->add_usage();
	}
}

void PostProcessRenderTargetPointer::release_usage()
{
	if (postProcessRenderTarget)
	{
		postProcessRenderTarget->release_usage();
	}
}

void PostProcessRenderTargetPointer::mark_next_frame_releasable()
{
	if (postProcessRenderTarget)
	{
		postProcessRenderTarget->mark_next_frame_releasable();
	}
}

::System::RenderTarget* PostProcessRenderTargetPointer::get_render_target() const
{
	if (postProcessRenderTarget)
	{
		an_assert(dataTextureIndex != NONE);
		an_assert(postProcessRenderTarget->get_render_target());
		return postProcessRenderTarget->get_render_target();
	}
	if (renderTarget.is_set())
	{
		an_assert(dataTextureIndex != NONE);
		return renderTarget.get();
	}
	return nullptr;
}

int PostProcessRenderTargetPointer::get_data_texture_index() const
{
	if (postProcessRenderTarget)
	{
		an_assert(dataTextureIndex != NONE);
		return dataTextureIndex;
	}
	if (renderTarget.is_set())
	{
		an_assert(dataTextureIndex != NONE);
		return dataTextureIndex;
	}
	return NONE;
}

::System::TextureID PostProcessRenderTargetPointer::get_texture_id() const
{
	if (postProcessRenderTarget)
	{
		an_assert(dataTextureIndex != NONE);
		an_assert(postProcessRenderTarget->get_render_target());
		an_assert(postProcessRenderTarget->get_render_target()->is_resolved());
		return postProcessRenderTarget->get_render_target()->get_data_texture_id(dataTextureIndex);
	}
	if (renderTarget.is_set())
	{
		an_assert(dataTextureIndex != NONE);
		an_assert(renderTarget->is_resolved());
		return renderTarget->get_data_texture_id(dataTextureIndex);
	}
	return 0; // no texture
}
