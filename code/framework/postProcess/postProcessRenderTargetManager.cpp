#include "postProcessRenderTargetManager.h"

#include "..\..\core\system\core.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\fragmentShaderOutputSetup.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

PostProcessRenderTargetManager::PostProcessRenderTargetManager()
{
	for (int c = 0; c < FRAME_CHAINS; ++c)
	{
		firstUnusedRenderTarget[c] = nullptr;
	}
}

PostProcessRenderTargetManager::~PostProcessRenderTargetManager()
{
	lock.acquire();

	while (!renderTargets.is_empty())
	{
		delete renderTargets.get_first();
	}

	lock.release();
}

void PostProcessRenderTargetManager::add_usage_for(PostProcessRenderTarget* _renderTarget)
{
	lock.acquire();

	_renderTarget->internal_add_usage();

	lock.release();
}

void PostProcessRenderTargetManager::release_usage_for(PostProcessRenderTarget* _renderTarget)
{
	lock.acquire();

	_renderTarget->internal_release_usage();

	lock.release();
}

void PostProcessRenderTargetManager::set_render_target_size(VectorInt2 const & _size)
{
	todo_important(TXT("who would use that and why?"));
	if (renderTargetSize == _size)
	{
		return;
	}

	lock.acquire();
	renderTargetSize = _size;
	for_every_ptr(renderTarget, renderTargets)
	{
		renderTarget->internal_resize(renderTargetSize);
	}
	lock.release();
}

PostProcessRenderTarget* PostProcessRenderTargetManager::get_available_render_target_for(VectorInt2 const & _size, float _scale, bool _withMipMaps, ::System::FragmentShaderOutputSetup const & _setup)
{
	lock.acquire();
	PostProcessRenderTarget* found = nullptr;

	int systemFrame = ::System::Core::get_frame();
	if (currentFrame != systemFrame)
	{
		for (int c = 0; c < FRAME_CHAINS - 1; ++c)
		{
			auto & thisC = firstUnusedRenderTarget[c];
			auto & nextC = firstUnusedRenderTarget[c + 1];
			// put next-frame-unused into unused (at the end, tail)
			if (!thisC)
			{
				thisC = nextC;
				nextC = nullptr;
			}
			else
			{
				PostProcessRenderTarget* last = thisC;
				while (last->nextUnused)
				{
					last = last->nextUnused;
				}
				last->nextUnused = nextC;
				if (last->nextUnused)
				{
					last->nextUnused->prevUnused = last;
				}
				nextC = nullptr;
			}
		}
		currentFrame = systemFrame;
	}

	{	// find available render target that matches requested render target setup
		PostProcessRenderTarget* available = firstUnusedRenderTarget[0];
		while (available)
		{
			if (available->does_match(_size, _withMipMaps, _setup))
			{
				found = available;
				break;
			}
			available = available->nextUnused;
		}
	}

	if (!found)
	{
		// create render target - will be added to both lists
		// copy setup from fragment shader output setup
		::System::RenderTargetSetup setup(_size);
		setup.copy_output_textures_from(_setup);
		setup.set_size(_size);
		setup.force_mip_maps(_withMipMaps);
		found = new PostProcessRenderTarget(this, setup);
	}

	found->internal_add_usage();

	found->get_render_target()->use_scaling(_scale);

	lock.release();

	return found;
}

void PostProcessRenderTargetManager::delete_unused()
{
	lock.acquire();

	for (int c = 0; c < FRAME_CHAINS; ++c)
	{
		while (firstUnusedRenderTarget[c])
		{
			delete_and_clear(firstUnusedRenderTarget[c]);
		}
	}

	lock.release();
}

void PostProcessRenderTargetManager::log(LogInfoContext & _log)
{
	lock.acquire();

	log_internal(_log);

	lock.release();
}

void PostProcessRenderTargetManager::log_internal(LogInfoContext & _log)
{
	_log.log(TXT("PostProcessRenderTargetManager (%i):"), renderTargets.get_size());
	LOG_INDENT(_log);
	int idx = 0;
	for_every_ptr(rt, renderTargets)
	{
		String text;
		auto * art = rt->get_render_target();
		VectorInt2 size = art->get_size();
		text += String::printf(TXT(" %02i %ix%i outputs:%i"), idx, size.x, size.y, art->get_setup().get_output_texture_count());
		bool inAnyChain = false;
		for (int c = 0; c < FRAME_CHAINS; ++c)
		{
			if (firstUnusedRenderTarget[c] == rt)
			{
				inAnyChain = true;
			}
		}
		if (inAnyChain ||
			rt->prevUnused)
		{
			int inChain = NONE;
			for (int c = 0; c < FRAME_CHAINS; ++c)
			{
				PostProcessRenderTarget* crt = firstUnusedRenderTarget[c];
				while (crt)
				{
					if (crt == rt)
					{
						inChain = c;
						break;
					}
					crt = crt->nextUnused;
				}
				if (inChain == NONE)
				{
					break;
				}
			}
			text += String::printf(TXT("(unused: %i)"), inChain);
		}
		_log.log(text.to_char());
		++idx;
	}
}
