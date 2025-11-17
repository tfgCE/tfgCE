#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\types\colour.h"

#include "..\library\usedLibraryStored.h"

#include "uiCanvasElement.h"

namespace Framework
{
	class Font;
	class Sprite;

	namespace UI
	{
		struct CanvasButton
		: public SoftPooledObject<CanvasButton>
		, public ICanvasElement
		{
			FAST_CAST_DECLARE(CanvasButton);
			FAST_CAST_BASE(ICanvasElement);
			FAST_CAST_END();

			typedef ICanvasElement base;
		public:
			static CanvasButton* get();

			CanvasButton* set_at(Vector2 const& _at) { at = _at; return this; }
			CanvasButton* set_anchor_rel_placement(Vector2 const& _anchorRelPlacement) { anchorRelPlacement = _anchorRelPlacement; return this; }
			CanvasButton* set_size(Vector2 const& _size) { size = _size; return this; }
			CanvasButton* set_font_scale(Vector2 const & _fontScale = Vector2::one) { fontScale = _fontScale; return this; }
			CanvasButton* set_scale(float _scale = 1.0f) { scale = _scale; return this; }
			CanvasButton* set_width(float _width) { size.x = _width; return this; }
			CanvasButton* set_height(float _height) { size.y = _height; return this; }
			CanvasButton* set_auto_width(Canvas const* _canvas, float _margin = 5.0f);
			CanvasButton* set_default_height(Canvas const* _canvas);
			CanvasButton* set_caption(String const& _caption) { caption = _caption; return this; }
			CanvasButton* set_caption(tchar const * _caption) { caption = _caption; return this; }
			CanvasButton* set_user_data(void const * _userData) { userData = _userData; return this; }
			CanvasButton* set_shortcut(System::Key::Type _key, Optional<ShortcutParams> const & _params = NP, std::function<void(ActContext const&)> _perform = nullptr, String const & _info = String::empty());
			CanvasButton* set_alignment(Vector2 const & _alignment) { alignment = _alignment; return this; }
			CanvasButton* set_highlighted(bool _highlighted) { highlighted = _highlighted; return this; }
			CanvasButton* set_enabled(bool _enabled) { enabled = _enabled; return this; }
			CanvasButton* set_no_frame() { hasFrame = false; return this; }
			CanvasButton* set_frame_width_pt(Optional<float> const& _frameWidthPt) { frameWidthPt = _frameWidthPt; return this; }
			CanvasButton* set_use_frame_width_only_for_action(bool _useFrameWidthOnlyForAction = true) { useFrameWidthOnlyForAction = _useFrameWidthOnlyForAction; return this; }
			CanvasButton* set_inner_frame_with_background(bool _innerFrameWithBackground = true) { innerFrameWithBackground = _innerFrameWithBackground; return this; }
			CanvasButton* set_use_window_paper() { useWindowPaper = true; return this; }
			CanvasButton* set_colour(Optional<Colour> const& _colour) { colour = _colour; return this; }
			CanvasButton* set_sprite(Sprite const * _sprite) { sprite = _sprite; return this; }
			CanvasButton* set_text_only() { textOnly = true; return this; }
			
			CanvasButton* set_on_press(ActFunc _on_press) { do_on_press = _on_press; return this; }
			CanvasButton* set_on_click(ActFunc _on_click) { do_on_click = _on_click; return this; }
			CanvasButton* set_on_double_click(ActFunc _on_double_click) { do_on_double_click = _on_double_click; return this; }

		public:
			bool is_highlighted() const { return highlighted; }
			bool is_enabled() const { return enabled; }

		public:
			Vector2 get_centre() const; // with relative offset
			Vector2 get_centre_local() const; // with relative offset
			Vector2 get_size() const { return size; }
			String const& get_caption() const { return caption; }
			void const* get_user_data() const { return userData; }

		public: // SoftPooledObject<Canvas>
			override_ void on_get();
			override_ void on_release();

		public: // ICanvasElement
			implement_ void release_element() { on_release_element(); release(); }

			implement_ void offset_placement_by(Vector2 const& _offset);
			implement_ Range2 get_placement(Canvas const* _canvas) const;

			implement_ void render(CanvasRenderContext & _context) const;
			implement_ void execute_shortcut(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const;
			implement_ void update_hover_on(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const;

			implement_ void on_press(REF_ PressContext& _context);
			implement_ void on_click(REF_ ClickContext& _context);
			implement_ bool on_double_click(REF_ ClickContext& _context);

		private:
			Vector2 at = Vector2::zero;
			Vector2 size = Vector2::zero;
			Vector2 anchorRelPlacement = Vector2::half;
			Vector2 alignment = Vector2::half;
			String caption;
			void const* userData = nullptr;
			Vector2 fontScale = Vector2::one;
			float scale = 1.0f;
			Optional<Colour> colour;
			UsedLibraryStored<Sprite> sprite; // stretched to fit
			bool hasFrame = true;
			bool useFrameWidthOnlyForAction = false; // if false, will always use it
			bool innerFrameWithBackground = false; // if true, half of the background will be set with button background
			Optional<float> frameWidthPt;
			bool useWindowPaper = false;
			bool textOnly = false;

			bool highlighted = false;
			bool enabled = true;

			ActFunc do_on_press;
			ActFunc do_on_click;
			ActFunc do_on_double_click;

		protected:
			override_ void prepare_context(ActContext& _context, Canvas * _canvas, Optional<Vector2> const& _actAt) const;
		};
	};
};

TYPE_AS_CHAR(Framework::UI::CanvasButton);
