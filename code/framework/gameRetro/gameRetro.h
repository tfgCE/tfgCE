#pragma once

#include "gameRetroInput.h"

#include "..\game\game.h"
#include "..\video\basicDrawRenderingBuffer.h"

#include "..\..\core\concurrency\gate.h"
#include "..\..\core\latent\latentFrameInspection.h"

//

namespace Framework
{
	class AssetPackage;
	class Display;
	class DisplayType;

	class GameRetro
	: public Framework::Game
	{
		FAST_CAST_DECLARE(Game);
		FAST_CAST_BASE(Framework::Game);
		FAST_CAST_END();

		typedef Framework::Game base;

	public:
		GameRetro();

	public:
		override_ void before_game_starts();
		override_ void after_game_ended();

		override_ void create_render_buffers();

	protected:
		virtual ~GameRetro();

		override_ String get_game_name() const;

	protected:
		override_ void load_library_and_prepare_worker(Concurrency::JobExecutionContext const* _executionContext, Framework::GameLoadingLibraryContext* _gameLoadingLibraryContext);

		// ---

	public:
		// for simpler games we use old school, straightforward and procedural approach
		// when we are ready to render frame, we render frame
		virtual void go() = 0; // implement this!

	protected:
		void init_display(); // rendering thread only
		void clear_display();

		void advance_frame(); // call this to display stuff on output, on screen etc
		
		void update_game_output();
		void show_output(); // game output -> display, display -> output, output -> actual output/screen

		::Framework::Display* get_display() const { return display.get(); }

	public:
		static Framework::UsedLibraryStored<Framework::Sprite> get_sprite(tchar const* _libraryName);
		static Framework::UsedLibraryStored<Framework::Sprite> get_sprite_tagged(tchar const* _tagged);
		static void get_asset_packages_tagged(OUT_ Array< Framework::UsedLibraryStored<Framework::AssetPackage>>& _into, tchar const* _tagged);
		static Framework::AssetPackage* get_asset_package_tagged(tchar const* _tagged);

	protected:
		struct DoInBackgroundContext
		{
			String showText;

			DoInBackgroundContext& show_text(tchar const* _text) { showText = _text; return *this; }
		};
		void do_in_background(std::function<void()> _do, DoInBackgroundContext const& _context = DoInBackgroundContext());

	//
	// general game info
	protected:
		// constructor
		VectorInt2 gameOutputResolution = VectorInt2(160, 192);

		// init
		Framework::UsedLibraryStored<Framework::DisplayType> displayType;

		// runtime
		Colour outputBackgroundColour = Colour::black; // this is main output's background colour
		Colour gameBackgroundColour = Colour::black; // this is the game's output's background colour
		Colour borderColour = Colour::black; // this is the game's output's background colour
		float gameFrameTime = 0.1f; // this is how often we update game and this is also when we call advance frame, when do we quit updating

	private:
		Colour borderColourUsed = Colour::none;

	//
	// game running
	private:
		RefCountObjectPtr<::Framework::Display> display;

		::System::RenderTargetPtr gameOutputRT; // this is the game's output render target, as where do we render our data, this is then used by the display to render stuff to the actual screen

		float timeTillGameFrame = 0.0f;

	//
	// layers and layer management
	private:
		struct Layer
		{
			// intermediate render targets are used to get all the layers together and scale them only afterwards
			bool intermediateLayer = false;
			::System::RenderTargetPtr intermediateLayerRT;
			System::TextureFiltering::Type intermediateLayerMagFilter = System::TextureFiltering::nearest; // do not blend when magnifying
			System::TextureFiltering::Type intermediateLayerMinFilter = System::TextureFiltering::linear;
			Framework::BasicDrawRenderingBuffer renderingBuffer;
			VectorInt2 at = VectorInt2::zero;
			VectorInt2 size = VectorInt2::zero;
			Optional<int> renderToIntermediateLayerIdx; // 

			bool fitToOutput = false;
			bool stretchToOutput = false;
			Optional<RangeInt2> clipTo;
			Optional<Vector2> clipScale; // scale does not affect at, at is always lower bottom, used only for clipTo 
			Optional<RangeInt2> fitTo;
			Optional<Vector2> fitPt; // used for fit to output and fit to
			Optional<RangeInt2> stretchTo;
		};
		Array<Layer*> layers;
		
		void update_layers();
		void combine_layers();

	protected:
		void set_layer_intermediate_layer(int _idx, bool _intermediateRenderTarget = true, Optional<System::TextureFiltering::Type> const & _intermediateLayerMagFilter = NP, Optional<System::TextureFiltering::Type> const& _intermediateLayerMinFilter = NP);
		void set_layer_render_to_intermediate_layer(int _idx, Optional<int> const& _toIdx);

		// can be called every frame to scroll
		void set_layer(int _idx, Optional<VectorInt2> const& _at = NP, Optional<VectorInt2> const& _size = NP);
		
		// setup
		void set_layer_as_is(int _idx); // clears clip to, stretch to, etc
		void set_layer_clip_to(int _idx, RangeInt2 const & _to, Optional<Vector2> const& _clipScale = NP);
		void set_layer_fit_to(int _idx, RangeInt2 const & _to, Optional<Vector2> const & _pt = NP);
		void set_layer_fit_to_output(int _idx, Optional<Vector2> const& _pt = NP);
		void set_layer_stretch_to(int _idx, RangeInt2 const & _to);
		void set_layer_stretch_to_output(int _idx);

		Framework::BasicDrawRenderingBuffer& access_layer(int _idx) { return layers[_idx]->renderingBuffer; }

	private:
		void make_sure_layer_exists(int _idx);

	//
	// input
	protected:
		GameRetroInput::State input;
	};
};
