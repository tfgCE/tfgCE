#include "postProcessRenderTarget.h"

#include "postProcess.h"
#include "postProcessRenderTargetManager.h"

#include "..\debugSettings.h"

#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\fragmentShaderOutputSetup.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

PostProcessRenderTarget::PostProcessRenderTarget(PostProcessRenderTargetManager * _manager, ::System::RenderTargetSetup& _setup)
: manager(_manager)
, usageCounter(0)
, nextUnused(nullptr)
, prevUnused(nullptr)
{
	manager->renderTargets.push_back(this);

	internal_add_to_unused();

	an_assert(! _setup.should_use_msaa());
	RENDER_TARGET_CREATE_INFO(TXT("post process render target"));
	renderTarget = new ::System::RenderTarget(_setup);
}

PostProcessRenderTarget::~PostProcessRenderTarget()
{
	manager->renderTargets.remove(this);
	internal_remove_from_unused();
	renderTarget = nullptr;
}

void PostProcessRenderTarget::add_usage()
{
	manager->add_usage_for(this);
}

void PostProcessRenderTarget::release_usage()
{
	manager->release_usage_for(this);
}

void PostProcessRenderTarget::internal_add_usage()
{
	an_assert(manager->lock.is_locked());
	if (usageCounter == 0)
	{
		internal_remove_from_unused();
	}
	++usageCounter;
#ifdef AN_LOG_POST_PROCESS_RT_DETAILED
	if (PostProcessDebugSettings::logRTDetailed)
	{
		output(TXT("pprt: %i (using: %i)"), get_render_target(), usageCounter);
	}
#endif
}

void PostProcessRenderTarget::internal_release_usage()
{
	an_assert(manager->lock.is_locked());
	--usageCounter;
#ifdef AN_LOG_POST_PROCESS_RT_DETAILED
	if (PostProcessDebugSettings::logRTDetailed)
	{
		output(TXT("pprt: %i (not using: %i)"), get_render_target(), usageCounter);
	}
#endif
	if (usageCounter == 0)
	{
		mark_next_frame_releasable(); // because otherwise it goes really wrong
		internal_add_to_unused();
	}
}

void PostProcessRenderTarget::internal_add_to_unused()
{
	an_assert(manager->lock.is_locked());
	an_assert(nextUnused == nullptr);
	an_assert(prevUnused == nullptr);

#ifdef AN_LOG_POST_PROCESS_RT
	if (PostProcessDebugSettings::logRT)
	{
		output(TXT("pprt: %i -> unused"), get_render_target());
	}
#endif

	auto & firstUnusedRenderTarget = nextFrameReleasable ? manager->firstUnusedRenderTarget[manager->FRAME_CHAINS - 1] : manager->firstUnusedRenderTarget[0];
	nextUnused = firstUnusedRenderTarget;
	if (nextUnused)
	{
		an_assert(nextUnused->prevUnused == nullptr, TXT("first, right?"));
		nextUnused->prevUnused = this;
	}
	firstUnusedRenderTarget = this;

	nextFrameReleasable = false;
}

void PostProcessRenderTarget::internal_remove_from_unused()
{
	an_assert(manager->lock.is_locked());

	nextFrameReleasable = false;

#ifdef AN_LOG_POST_PROCESS_RT
	if (PostProcessDebugSettings::logRT)
	{
		output(TXT("pprt: %i -> used"), get_render_target());
	}
#endif

	if (nextUnused)
	{
		nextUnused->prevUnused = prevUnused;
	}
	if (prevUnused)
	{
		prevUnused->nextUnused = nextUnused;
	}
	else
	{
		for (int i = 0; i < manager->FRAME_CHAINS; ++i)
		{
			if (manager->firstUnusedRenderTarget[i] == this)
			{
				manager->firstUnusedRenderTarget[i] = nextUnused;
				break;
			}
		}
	}

	nextUnused = nullptr;
	prevUnused = nullptr;
}

void PostProcessRenderTarget::internal_resize(VectorInt2 const & _newSize)
{
	::System::RenderTargetSetup setup = renderTarget->get_setup();
	setup.set_size(_newSize);
	setup.dont_use_msaa();
	RENDER_TARGET_CREATE_INFO(TXT("post process render target, resize"));
	renderTarget = new ::System::RenderTarget(setup);
}

bool PostProcessRenderTarget::does_match(VectorInt2 const & _size, bool _withMipMaps, ::System::FragmentShaderOutputSetup const & _setup) const
{
	if (renderTarget->get_size(true) == _size &&
		renderTarget->get_setup().get_output_texture_count() == _setup.get_output_texture_count() &&
		renderTarget->should_use_mip_maps() == _withMipMaps)
	{
		bool allSame = true;
		::System::RenderTargetSetup const & renderTargetSetup = renderTarget->get_setup();
		for_count(int, outputTextureIndex, _setup.get_output_texture_count())
		{
			if (! renderTargetSetup.get_output_texture_definition(outputTextureIndex).does_match(_setup.get_output_texture_definition(outputTextureIndex), true))
			{
				allSame = false;
				break;
			}
		}
		if (allSame)
		{
			return true;
		}
	}

	return false;
}
