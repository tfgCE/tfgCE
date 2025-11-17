#include "gameRetro.h"

#include "..\library\library.h"

#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\concurrency\asynchronousJobList.h"
#include "..\..\core\system\sound\soundSystem.h"
#include "..\..\core\system\video\renderTarget.h"

#include "..\..\framework\display\display.h"
#include "..\..\framework\display\drawCommands\displayDrawCommand_border.h"
#include "..\..\framework\display\drawCommands\displayDrawCommand_cls.h"
#include "..\..\framework\display\drawCommands\displayDrawCommand_textAtMultiline.h"
#include "..\..\framework\jobSystem\jobSystem.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// global references
DEFINE_STATIC_NAME_STR(grMainDisplay, TXT("main display"));

// render target outputs
DEFINE_STATIC_NAME(colour);

//

REGISTER_FOR_FAST_CAST(GameRetro);

GameRetro::GameRetro()
{
	mouseGrabbed = false;
}

GameRetro::~GameRetro()
{
}

void GameRetro::create_render_buffers()
{
	base::create_render_buffers();

	// create game output (if doesn't exist)
	if (! gameOutputRT.get())
	{
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(gameOutputResolution); // will be stretched

		// we use ordinary rgb 8bit textures
		rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
			::System::VideoFormat::rgba8,
			::System::BaseVideoFormat::rgba,
			false, // no mip maps
			System::TextureFiltering::nearest, // mag, big pixels
			System::TextureFiltering::linear // allow blending
		));
		
		gameOutputRT = new ::System::RenderTarget(rtSetup);
	}
}

void GameRetro::init_display()
{
	assert_rendering_thread();

	displayType = Library::get_current()->get_global_references().get<Framework::DisplayType>(NAME(grMainDisplay));

	display = new Framework::Display();
	display->setup_with(displayType.get());

	display->ignore_game_frames();

	{
		VectorInt2 originalScreenResolution = display->get_setup().screenResolution;
		Vector2 scale = gameOutputResolution.to_vector2() / originalScreenResolution.to_vector2();
		scale = Vector2::one * max(scale.x, scale.y);
		auto setup = display->get_setup();
		setup.screenResolution = gameOutputResolution; // force to be exact
		setup.wholeDisplayResolution = TypeConversions::Normal::f_i_closest(setup.wholeDisplayResolution.to_vector2() * scale);
		// let's properly calculate border size, it has to be the same on both sides and an integer value, we divide by 2, then multiply by 2
		VectorInt2 borderSize = (setup.wholeDisplayResolution - setup.screenResolution) / 2; // because this would be doubled border on each side
		setup.wholeDisplayResolution = setup.screenResolution + borderSize * 2;
		setup.pixelScale *= scale;
#ifdef USE_DISPLAY_SHADOW_MASK
		setup.shadowMaskPerPixel /= scale;
#endif
		display->use_setup(setup);
		display->setup_output_size(setup.wholeDisplayResolution * 2);
	}

	display->setup_output_size(display->get_setup().wholeDisplayResolution * 2);

	//display->setup_output_size_max(::System::Video3D::get()->get_screen_size(), 1.0f);
	//display->setup_output_size_limit(VectorInt2(100, 100));

	display->init(::Framework::Library::get_current());

	display->use_background(gameOutputRT.get(), 0);
	//display->set_background_fill_mode(Framework::DisplayBackgroundFillMode::Whole);
	display->set_background_fill_mode(Framework::DisplayBackgroundFillMode::Screen);

	clear_display();
}

void GameRetro::load_library_and_prepare_worker(Concurrency::JobExecutionContext const* _executionContext, Framework::GameLoadingLibraryContext* gameLoadingLibraryContext)
{
	base::load_library_and_prepare_worker(_executionContext, gameLoadingLibraryContext);

	if (gameLoadingLibraryContext->libraryLoadLevel == Framework::LibraryLoadLevel::Main)
	{
		// setup display
		{
			struct InitStuff
			: public Concurrency::AsynchronousJobData
			{
				GameRetro* game;
				InitStuff(GameRetro* _game) : game(_game) {}

				static void perform(Concurrency::JobExecutionContext const* _executionContext, void* _data)
				{
					auto* game = ((InitStuff*)_data)->game;
					game->init_display();
				}
			};

			Game::get()->async_system_job(Game::get(), InitStuff::perform, new InitStuff(this));
		}
	}
}

void GameRetro::before_game_starts()
{
	base::before_game_starts();
}

void GameRetro::after_game_ended()
{
	display.clear();

	for_every_ptr(layer, layers)
	{
		delete layer;
	}
	layers.clear();

	base::after_game_ended();
}

String GameRetro::get_game_name() const
{
	return String(::System::Core::get_app_name());
}

void GameRetro::clear_display()
{
	display->draw_all_commands_immediately();
	display->add((new Framework::DisplayDrawCommands::Border(borderColour)));
	borderColourUsed = borderColour;

	display->add((new Framework::DisplayDrawCommands::CLS(Colour::none)));
}

void GameRetro::advance_frame()
{
	scoped_call_stack_info(TXT("advance frame"));

	input.update_start();

	bool keepGoing = true;
	while (keepGoing)
	{
		// advance system
		::System::Core::set_time_multiplier();
		::System::Core::advance();

#ifdef WITH_IN_GAME_EDITOR
		// it has to be done here, as otherwise we won't catch tilde being pressed as Core::advance might be called more than one time during "advance_frame"
		if (show_editor_if_requested())
		{
			input.update_start();
		}
#endif

		float const deltaTime = ::System::Core::get_delta_time();

		input.read_input(deltaTime);

		if (jobSystem)
		{
			scoped_call_stack_info(TXT("-> clean up system jobs"));
			jobSystem->clean_up_finished_off_system_jobs();
			jobSystem->access_advance_context().set_delta_time(deltaTime);
			jobSystem->execute_main_executor(1); // execute one job to update screen
		}

		if (auto* ss = ::System::Sound::get())
		{
			scoped_call_stack_info(TXT("-> sound update"));
			ss->update(::System::Core::get_raw_delta_time());
		}

		advance_controls();

		advance_fading();

		advance_later_loading();

		// display things
		{
			WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);

			::System::Video3D* v3d = ::System::Video3D::get();

			v3d->update();

			update_game_output();

			// update border
			{
				if (borderColourUsed != borderColour)
				{
					display->add((new Framework::DisplayDrawCommands::Border(borderColour)));
					borderColourUsed = borderColour;
				}
			}

			display->advance(deltaTime);

			show_output();
		
			apply_fading_to(nullptr);

			/*
			{
				an_assert(!::System::Video3D::get()->is_frame_buffer_bound());
				System::RecentCapture::store_pixel_data(); // do it just before ending render as we will get stuck on CPU
				an_assert(!::System::Video3D::get()->is_frame_buffer_bound());
			}
			*/

			{
				MEASURE_PERFORMANCE(v3d_displayBuffer);
				v3d->display_buffer();
			}
		}
			
		timeTillGameFrame -= deltaTime;
		if (timeTillGameFrame <= 0.0f)
		{
			// next frame!
			keepGoing = false; 
			timeTillGameFrame = max(0.0f, timeTillGameFrame + gameFrameTime);
		}
	}

	input.update_end();
}

void GameRetro::update_game_output()
{
	assert_rendering_thread();

	update_layers();

	combine_layers();
}

void GameRetro::make_sure_layer_exists(int _idx)
{
	while (layers.get_size() <= _idx)
	{
		layers.push_back();
		layers.get_last() = new Layer();
	}
}

void GameRetro::set_layer_intermediate_layer(int _idx, bool _intermediateLayer, Optional<System::TextureFiltering::Type> const& _intermediateLayerMagFilter, Optional<System::TextureFiltering::Type> const& _intermediateLayerMinFilter)
{
	make_sure_layer_exists(_idx);

	System::TextureFiltering::Type intermediateLayerMagFilter = _intermediateLayerMagFilter.get(System::TextureFiltering::nearest);
	System::TextureFiltering::Type intermediateLayerMinFilter = _intermediateLayerMinFilter.get(System::TextureFiltering::linear);

	auto& l = *layers[_idx];
	if (l.intermediateLayer != _intermediateLayer ||
		l.intermediateLayerMagFilter != intermediateLayerMagFilter ||
		l.intermediateLayerMinFilter != intermediateLayerMinFilter)
	{
		l.renderingBuffer.close();
		l.intermediateLayerRT.clear();
		l.intermediateLayer = _intermediateLayer;
		l.intermediateLayerMagFilter = intermediateLayerMagFilter;
		l.intermediateLayerMinFilter = intermediateLayerMinFilter;

		// reinit sprite rendering buffer / rendering target
		set_layer(_idx, l.at, l.size);
	}
}

void GameRetro::set_layer_render_to_intermediate_layer(int _idx, Optional<int> const& _toIdx)
{
	an_assert(!_toIdx.is_set() || _toIdx.get() > _idx);

	make_sure_layer_exists(_idx);

	auto& l = *layers[_idx];
	l.renderToIntermediateLayerIdx = _toIdx;
}

void GameRetro::set_layer(int _idx, Optional<VectorInt2> const& _at, Optional<VectorInt2> const& _size)
{
	make_sure_layer_exists(_idx);

	auto& l = *layers[_idx];

	if (_size != l.size)
	{
		if (l.renderingBuffer.is_ok())
		{
			l.renderingBuffer.close();
		}
		if (l.intermediateLayerRT.get())
		{
			l.intermediateLayerRT.clear();
		}
	}
	if (_size.is_set())
	{
		l.size = _size.get();
	}
	else if (l.size.is_zero()) // only once (if not provided and not set)
	{
		l.size = gameOutputRT->get_size();
	}

	l.at = _at.get(l.at);

	if (l.intermediateLayer)
	{
		if (!l.intermediateLayerRT.get())
		{
			::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(l.size); // will be stretched

			// we use ordinary rgb 8bit textures
			// linear filtering as we upscale
			rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
				::System::VideoFormat::rgba8,
				::System::BaseVideoFormat::rgba,
				false, // no mip maps
				l.intermediateLayerMagFilter,
				l.intermediateLayerMinFilter
			));

			l.intermediateLayerRT = new ::System::RenderTarget(rtSetup);
		}
	}
	else
	{
		if (!l.renderingBuffer.is_ok())
		{
			l.renderingBuffer.init(l.size);
		}
	}
}

void GameRetro::set_layer_as_is(int _idx)
{
	if (layers.is_index_valid(_idx))
	{
		auto& l = *layers[_idx];
		l.fitToOutput = false;
		l.stretchToOutput = false;
		l.clipTo.clear();
		l.clipScale.clear();
		l.fitTo.clear();
		l.stretchTo.clear();
	}
}

void GameRetro::set_layer_fit_to_output(int _idx, Optional<Vector2> const& _pt)
{
	if (layers.is_index_valid(_idx))
	{
		auto& l = *layers[_idx];
		l.fitToOutput = true;
		l.stretchToOutput = false;
		l.clipTo.clear();
		l.clipScale.clear();
		l.fitTo.clear();
		l.fitPt = _pt;
		l.stretchTo.clear();
	}
}

void GameRetro::set_layer_stretch_to_output(int _idx)
{
	if (layers.is_index_valid(_idx))
	{
		auto& l = *layers[_idx];
		l.fitToOutput = false;
		l.stretchToOutput = true;
		l.clipTo.clear();
		l.clipScale.clear();
		l.fitTo.clear();
		l.stretchTo.clear();
	}
}

void GameRetro::set_layer_clip_to(int _idx, RangeInt2 const& _to, Optional<Vector2> const & _clipScale)
{
	if (layers.is_index_valid(_idx))
	{
		auto& l = *layers[_idx];
		l.fitToOutput = false;
		l.stretchToOutput = false;
		l.clipTo = _to;
		l.clipScale = _clipScale.get(Vector2::one);
		l.fitTo.clear();
		l.stretchTo.clear();
	}
}

void GameRetro::set_layer_fit_to(int _idx, RangeInt2 const& _to, Optional<Vector2> const& _pt)
{
	if (layers.is_index_valid(_idx))
	{
		auto& l = *layers[_idx];
		l.fitToOutput = false;
		l.stretchToOutput = false;
		l.clipTo.clear();
		l.clipScale.clear();
		l.fitTo = _to;
		l.fitPt = _pt;
		l.stretchTo.clear();
	}
}

void GameRetro::set_layer_stretch_to(int _idx, RangeInt2 const& _to)
{
	if (layers.is_index_valid(_idx))
	{
		auto& l = *layers[_idx];
		l.fitToOutput = false;
		l.stretchToOutput = false;
		l.clipTo.clear();
		l.clipScale.clear();
		l.fitTo.clear();
		l.stretchTo = _to;
	}
}

void GameRetro::update_layers()
{
	auto* v3d = ::System::Video3D::get();

	for_every_ptr(layer, layers)
	{
		if (layer->renderingBuffer.is_ok())
		{
			layer->renderingBuffer.update(v3d);
		}
	}
}

void GameRetro::combine_layers()
{
	struct RenderingSetup
	{
		bool active = false;
		::System::ForRenderTargetOrNone forRT;
		VectorInt2 rtSize;

		RenderingSetup() {}
		~RenderingSetup() { end(); }

		void render_to(System::RenderTarget* _rt)
		{
			if (active && _rt == forRT.rt)
			{
				// good as it is
				return;
			}

			if (active)
			{
				end();
			}

			forRT = ::System::ForRenderTargetOrNone(_rt);
			rtSize = forRT.get_size();

			forRT.bind();

			auto* v3d = ::System::Video3D::get();

			v3d->set_viewport(VectorInt2::zero, rtSize);
			v3d->set_2d_projection_matrix_ortho(rtSize.to_vector2());
			v3d->access_model_view_matrix_stack().clear();
			v3d->access_clip_plane_stack().clear();
			v3d->set_near_far_plane(0.02f, 100.0f);
			v3d->setup_for_2d_display();
			v3d->ready_for_rendering();

			active = true;
		}
		void end()
		{
			if (active)
			{
				forRT.unbind();
				active = false;
			}
		};
	} renderingSetup;

	auto* v3d = ::System::Video3D::get();
	
	// clear everything to background

	for_every_ptr(layer, layers)
	{
		if (auto* rt = layer->intermediateLayerRT.get())
		{
			renderingSetup.render_to(rt);
			//
			v3d->clear_colour_depth_stencil(gameBackgroundColour);
		}
	}
	
	renderingSetup.render_to(gameOutputRT.get());
	//
	v3d->clear_colour_depth_stencil(gameBackgroundColour);

	//

	v3d->mark_enable_blend();

	for_every_ptr(layer, layers)
	{
		if (layer->renderToIntermediateLayerIdx.is_set())
		{
			auto* rt = layers[layer->renderToIntermediateLayerIdx.get()]->intermediateLayerRT.get();
			an_assert(rt);
			renderingSetup.render_to(rt);
		}
		else
		{
			renderingSetup.render_to(gameOutputRT.get());
		}
		System::RenderTarget* rt = nullptr;
		if (layer->renderingBuffer.is_ok())
		{
			rt = layer->renderingBuffer.get_render_target();
		}
		else
		{
			rt = layer->intermediateLayerRT.get();
		}
		if (rt)
		{
			if (layer->clipTo.is_set())
			{
				VectorInt2 at = layer->at;
				VectorInt2 size = layer->size;
				RangeInt2 availableRange = RangeInt2::empty;
				availableRange.include(at);
				availableRange.include(at + size - VectorInt2::one);

				VectorInt2 clipBL = layer->clipTo.get().bottom_left();
				VectorInt2 clipTR = layer->clipTo.get().top_right();
				Vector2 clipScale = layer->clipScale.get(Vector2::one);
					
				// we only want to show what is available
				clipBL = availableRange.clamp(clipBL);
				clipTR = availableRange.clamp(clipTR);

				Vector2 atScaled = at.to_vector2() * clipScale;
				Vector2 sizeScaled = size.to_vector2() * clipScale;

				Vector2 clipBLOffset = clipBL.to_vector2() - atScaled;
				Vector2 clipTROffset = clipTR.to_vector2() - atScaled;
				Vector2 clipBLOffsetPt = clipBLOffset / sizeScaled;
				Vector2 clipTROffsetPt = (clipTROffset + Vector2::one) / sizeScaled;
				Range2 uvRange = Range2::empty;
				uvRange.include(clipBLOffsetPt);
				uvRange.include(clipTROffsetPt);
				rt->render(0, v3d, clipBL.to_vector2(), (clipTR - clipBL + VectorInt2::one).to_vector2(), uvRange);
			}
			else if (layer->fitTo.is_set())
			{
				rt->render_fit(0, v3d, layer->fitTo.get().bottom_left().to_vector2(), layer->fitTo.get().length().to_vector2(), layer->fitPt);
			}
			else if (layer->stretchTo.is_set())
			{
				rt->render(0, v3d, layer->stretchTo.get().bottom_left().to_vector2(), layer->stretchTo.get().length().to_vector2());
			}
			else if (layer->fitToOutput)
			{
				rt->render_fit(0, v3d, Vector2::zero, renderingSetup.rtSize.to_vector2(), layer->fitPt);
			}
			else if (layer->stretchToOutput)
			{
				rt->render(0, v3d, Vector2::zero, renderingSetup.rtSize.to_vector2());
			}
			else
			{
				rt->render(0, v3d, layer->at.to_vector2(), layer->size.to_vector2());
			}
		}
	}

	v3d->mark_disable_blend();
}

void GameRetro::show_output()
{
	assert_rendering_thread();

	display->update_display();

	// render display to main render output
	if (auto* rt = get_main_render_output_render_buffer()) // if not available, means we don't use it and already rendered stuff
	{
		auto* v3d = ::System::Video3D::get();

		::System::ForRenderTargetOrNone forRT(rt);
		VectorInt2 rtSize = forRT.get_size();

		forRT.bind();

		v3d->set_viewport(VectorInt2::zero, rtSize);
		v3d->set_2d_projection_matrix_ortho(rtSize.to_vector2());
		v3d->access_model_view_matrix_stack().clear();
		v3d->access_clip_plane_stack().clear();
		v3d->set_near_far_plane(0.02f, 100.0f);
		v3d->setup_for_2d_display();
		v3d->ready_for_rendering();

		v3d->clear_colour_depth_stencil(outputBackgroundColour);

		display->render_2d(v3d, rtSize.to_vector2() * 0.5f, rtSize.to_vector2());

		forRT.unbind();
	}

	// render main render output to actual output
	render_main_render_output_to_output();
}

Framework::UsedLibraryStored<Framework::Sprite> GameRetro::get_sprite(tchar const* _libraryName)
{
	Framework::UsedLibraryStored<Framework::Sprite> sprite;
	sprite.set_name(Framework::LibraryName(String(_libraryName)));
	sprite.find_may_fail(Library::get_current());
	return sprite;
}

Framework::UsedLibraryStored<Framework::Sprite> GameRetro::get_sprite_tagged(tchar const* _tagged)
{
	Framework::UsedLibraryStored<Framework::Sprite> sprite;
	TagCondition tagCondition;
	tagCondition.load_from_string(String(_tagged));
	for_every_ptr(sp, Library::get_current()->get_sprites().get_tagged(tagCondition))
	{
		sprite = sp;
		break;
	}
	return sprite;
}

void GameRetro::get_asset_packages_tagged(OUT_ Array< Framework::UsedLibraryStored<Framework::AssetPackage>> & _into, tchar const* _tagged)
{
	TagCondition tagCondition;
	tagCondition.load_from_string(String(_tagged));
	for_every_ptr(ap, Library::get_current()->get_asset_packages().get_tagged(tagCondition))
	{
		_into.push_back();
		_into.get_last() = ap;
	}
	sort(_into);
}

Framework::AssetPackage* GameRetro::get_asset_package_tagged(tchar const* _tagged)
{
	Array< Framework::UsedLibraryStored < Framework::AssetPackage>> aps;

	get_asset_packages_tagged(OUT_ aps, _tagged);

	return !aps.is_empty() ? aps[0].get() : nullptr;
}

void GameRetro::do_in_background(std::function<void()> _do, DoInBackgroundContext const& _context)
{
	clear_display();
	if (!_context.showText.is_empty())
	{
		get_display()->add((new Framework::DisplayDrawCommands::TextAtMultiline())->text(_context.showText)->in_region(get_display()->get_char_screen_rect())->h_align_centre_fine()->v_align_centre());
	}

	async_background_job(this, _do);

	while (true)
	{
		advance_frame();

		if (get_job_system()->get_asynchronous_list(system_jobs())->has_finished() &&
			get_job_system()->get_asynchronous_list(background_jobs())->has_finished() &&
			get_job_system()->get_asynchronous_list(loading_worker_jobs())->has_finished() &&
			get_job_system()->get_asynchronous_list(preparing_worker_jobs())->has_finished() &&
			get_job_system()->get_asynchronous_list(async_worker_jobs())->has_finished())
		{
			break;
		}
	}

	clear_display();
}
