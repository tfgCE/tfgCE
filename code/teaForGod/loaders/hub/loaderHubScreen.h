#pragma once

#include "loaderHubEnums.h"

#include "..\..\..\core\fastCast.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

struct Vector2;
struct VectorInt2;
struct Colour;

namespace System
{
	class Video3D;
};

namespace Framework
{
	class Display;
	class EnvironmentType;
	class GameInput;
	class TexturePart;
};

namespace TeaForGodEmperor
{
	class Game;
};

namespace Loader
{
	class Hub;
	struct HubWidget;

	namespace HubWidgets
	{
		struct Button;
	};

	struct HubScreenOption
	{
		struct Option
		{
			int value;
			String text;
		};
		Name id;
		Name optionLocStrId;
		String optionStr;
		String text;
		String textOnButton;
		Array<Option> options;
		RangeInt sliderRange = RangeInt(0, 100);
		std::function<void(RangeInt &)> sliderRangeDynamic;
		int minLines = 1;
		int sliderStep = 1;
		float sliderWidthPct = 0.5f;
		int* value = nullptr;
		Optional<Colour> colour;
		Optional<float> smallerDecimals;
		Optional<float> scalePixelsPerAngle;
		std::function<void()> onClick = nullptr;
		std::function<void()> onHold = nullptr;
		std::function<void()> onRelease = nullptr;
		std::function<void()> doPostUpdate = nullptr;
		std::function<String(int)> getAsTextForSlider = nullptr;

		enum Type
		{
			Switch,	// switches between options
			List,	// opens a modal screen with list of options to choose from
			Slider,	// a slider
			Static, // just text, without value
			Header, // as static, but in the centre (may use optionLocStrId or text)
			Button,	// button on the right side
			Text,	// just show text (may have an id, to allow setting value, or use with "text")
			Centred	// as text but both at the centre
		} type = Text;

		HubScreenOption() {}
		HubScreenOption(Name const & _id, Name const & _optionLocStrId, int* _value = nullptr, Type _type = Switch)
		: id(_id), optionLocStrId(_optionLocStrId), value(_value), type(_type)
		{
			//an_assert(!(_type == Text && ! _id.is_valid()), TXT("if text, should have an id"));
			options.make_space_for(20);
		}
		HubScreenOption(Name const & _id, String const & _optionStr, int* _value = nullptr, Type _type = Switch)
		: id(_id), optionStr(_optionStr), value(_value), type(_type)
		{
			//an_assert(!(_type == Text && ! _id.is_valid()), TXT("if text, should have an id"));
			options.make_space_for(20);
		}
		HubScreenOption(String const & _simpleText)
		: id(Name::invalid()), optionLocStrId(Name::invalid()), text(_simpleText), value(nullptr), type(Header)
		{
		}
		HubScreenOption & add_if(bool _condition, int _value, Name const& _locStrId)
		{
			return _condition ? add(_value, LOC_STR(_locStrId)) : *this;
		}
		HubScreenOption & add(int _value, Name const & _locStrId)
		{
			return add(_value, LOC_STR(_locStrId));
		}
		HubScreenOption & add(int _value, String const & _text)
		{
			options.grow_size(1);
			options.get_last().value = _value;
			options.get_last().text = _text;
			return *this;
		}
		HubScreenOption & do_post_update(std::function<void()> _doPostUpdate)
		{
			doPostUpdate = _doPostUpdate;
			return *this;
		}
		HubScreenOption & slider_range(int _min, int _max)
		{
			sliderRange = RangeInt(_min, _max);
			return *this;
		}
		HubScreenOption & slider_range(RangeInt const & _range)
		{
			sliderRange = _range;
			return *this;
		}
		HubScreenOption& slider_range_dynamic(std::function<void(RangeInt & _range)> _sliderRangeDynamic)
		{
			sliderRangeDynamic = _sliderRangeDynamic;
			return *this;
		}
		HubScreenOption & slider_step(int _step)
		{
			sliderStep = _step;
			return *this;
		}
		HubScreenOption & with_slider_width_pct(float _pct)
		{
			sliderWidthPct = _pct;
			return *this;
		}
		HubScreenOption & to_get_text_for_slider(std::function<String(int)> _getAsTextForSlider)
		{
			getAsTextForSlider = _getAsTextForSlider;
			return *this;
		}
		HubScreenOption & with_colour(Colour const & _colour)
		{
			colour = _colour;
			return *this;
		}
		HubScreenOption & with_text(String const & _text)
		{
			text = _text;
			return *this;
		}
		HubScreenOption & with_text_on_button(String const & _textOnButton)
		{
			textOnButton = _textOnButton;
			return *this;
		}
		HubScreenOption& with_on_click(std::function<void()> _onClick)
		{
			onClick = _onClick;
			return *this;
		}
		HubScreenOption& with_on_hold(std::function<void()> _onHold)
		{
			onHold = _onHold;
			return *this;
		}
		HubScreenOption& with_on_release(std::function<void()> _onRelease)
		{
			onRelease = _onRelease;
			return *this;
		}
		HubScreenOption& with_min_lines(int _minLines)
		{
			minLines = _minLines;
			return *this;
		}
		HubScreenOption& with_smaller_decimals(Optional<float> const& _smallerDecimals = 0.75f)
		{
			smallerDecimals = _smallerDecimals;
			return *this;
		}
		HubScreenOption& scale_pixels_per_angle(Optional<float> const& _scalePixelsPerAngle)
		{
			scalePixelsPerAngle = _scalePixelsPerAngle;
			return *this;
		}

		String const & get_text_value() const;
	};

	struct HubScreenButtonInfo
	{
		Name id;
		String caption;
		Name locStrId;
		HubCustomDrawFunc custom_draw = nullptr;
		std::function<void(HubInputFlags::Flags _flags)> on_click;
		bool highlighted = false;
		bool enabled = true;
		RefCountObjectPtr<HubWidget>* widgetPtr = nullptr;
		bool activateOnTriggerHold = false;
		bool activateOnHold = false;
		bool appearAsText = false;
		float scale = 1.0f;
		Optional<Colour> overrideColour;
		Optional<Vector2> alignPt;

		HubScreenButtonInfo() {}
		HubScreenButtonInfo(Name const & _id, String const & _caption, std::function<void(HubInputFlags::Flags _flags)> _on_click, bool _highlighted = false)
		: id(_id), caption(_caption), on_click(_on_click), highlighted(_highlighted)
		{}
		HubScreenButtonInfo(Name const& _id, String const& _caption, std::function<void()> _on_click = nullptr, bool _highlighted = false)
		: id(_id), caption(_caption), on_click([_on_click](HubInputFlags::Flags) {if (_on_click) _on_click(); }), highlighted(_highlighted)
		{}
		HubScreenButtonInfo(Name const & _id, Name const & _locStrId, std::function<void(HubInputFlags::Flags _flags)> _on_click, bool _highlighted = false)
		: id(_id), locStrId(_locStrId), on_click(_on_click), highlighted(_highlighted)
		{}
		HubScreenButtonInfo(Name const & _id, Name const & _locStrId, std::function<void()> _on_click, bool _highlighted = false)
		: id(_id), locStrId(_locStrId), on_click([_on_click](HubInputFlags::Flags) {if (_on_click) _on_click(); }), highlighted(_highlighted)
		{}
		HubScreenButtonInfo(Name const& _id, HubCustomDrawFunc _custom_draw, std::function<void()> _on_click = nullptr, bool _highlighted = false)
		: id(_id), custom_draw(_custom_draw), on_click([_on_click](HubInputFlags::Flags) {if (_on_click) _on_click(); }), highlighted(_highlighted)
		{}

		HubScreenButtonInfo & store_widget_in(RefCountObjectPtr<HubWidget>* _widgetPtr) { widgetPtr = _widgetPtr; return *this; }
		HubScreenButtonInfo & activate_on_trigger_hold(bool _activateOnTriggerHold = true) { activateOnTriggerHold = _activateOnTriggerHold; return *this; }
		HubScreenButtonInfo & activate_on_hold(bool _activateOnHold = true) { activateOnHold = _activateOnHold; return *this; }
		HubScreenButtonInfo & appear_as_text(bool _appearAsText = true) { appearAsText = _appearAsText; return *this; }
		HubScreenButtonInfo & with_scale(float _scale) { scale = _scale; return *this; }
		HubScreenButtonInfo & be_highlighted(bool _highlighted) { highlighted = _highlighted; return *this; }
		HubScreenButtonInfo & be_enabled(bool _enabled = true) { enabled = _enabled; return *this; }
		HubScreenButtonInfo & be_disabled(bool _disabled = true) { enabled = !_disabled; return *this; }
		HubScreenButtonInfo & with_colour(Optional<Colour> const & _overrideColour) { overrideColour = _overrideColour; return *this; }
		HubScreenButtonInfo & with_align_pt(Optional<Vector2> const & _alignPt) { alignPt = _alignPt; return *this; }
		HubScreenButtonInfo & with_custom_draw(HubCustomDrawFunc _custom_draw) { custom_draw = _custom_draw; return *this; }
	};

	struct HubScreen
	: public RefCountObject
	{
		FAST_CAST_DECLARE(HubScreen);
		FAST_CAST_END();
		
	public:
		static int compare_ref(void const* _a, void const* _b)
		{
			RefCountObjectPtr<HubScreen> const& a = *(plain_cast<RefCountObjectPtr<HubScreen>>(_a));
			RefCountObjectPtr<HubScreen> const& b = *(plain_cast<RefCountObjectPtr<HubScreen>>(_b));
			return a->radius > b->radius? B_BEFORE_A :
				   a->radius < b->radius ? A_BEFORE_B : 
				   A_AS_B;
		};

		static int compare(void const* _a, void const* _b)
		{
			HubScreen* a = *(plain_cast<HubScreen*>(_a));
			HubScreen* b = *(plain_cast<HubScreen*>(_b));
			return a->radius > b->radius? B_BEFORE_A :
				   a->radius < b->radius ? A_BEFORE_B : 
				   A_AS_B;
		};

	public:
		static const float DEFAULT_YAW_DEAD_ZONE_BY_BORDER;

		static Vector2 s_fontSizeInPixels;
		static Vector2 s_pixelsPerAngle;
		Vector2 pixelsPerAngle;

		Hub* hub = nullptr;
		
		bool alwaysRender = false; // otherwise, does lazy render

		bool dontClearOnRender = false;
		int forceRedraw = 3;
		bool neverGoesToBackground = false;

		std::function<void()> do_on_deactivate = nullptr;

		bool canBeUsed = false; // only render and go through it if canBeUsed

		Name id;
		float active = 0.0f;
		float activeTarget = 1.0f;
		bool moveAway = false;
		Vector3 moveAwayDir = Vector3::zero;
		RefCountObjectPtr<Framework::Display> display;
		Rotator3 at = Rotator3::zero; // do not modify directly, use set_placement
		float radius = 5.0f;
		
		bool modal = false;
		bool clickingOutsideDeactivates = false; // only if modal

		bool blockTriggerHold = false; // if cursor is held over this screen, trigger hold for buttons is ignored

		bool alwaysProcessInput = false;

		// if modal kicks in
		float inBackground = 0.0f;
		float inBackgroundTarget = 0.0f;
		bool forceAppearForeground = false;

		bool notAvailableToCursor = false;

		Optional<bool> followHead; // allows following yaw always, appears to be following head through rendering/input
		Optional<float> followYawDeadZone; // off the centre
		Optional<float> followYawDeadZoneByBorder; // if off the border
		Optional<Name> followScreen;
		Optional<float> beAtYaw; // instead of follow, be at explicit yaw

		// in angles
		Vector2 size = Vector2(90.0f, 90.0f); 
		Vector2 border = Vector2(10.0f, 10.0f);
		Range2 wholeScreen;
		Range2 mainScreen;
		//

		Range2 mainResolutionInPixels = Range2::zero;
		Range2 wholeResolutionInPixels = Range2::zero;

		Framework::UsedLibraryStored<Framework::Mesh> mesh;
		Meshes::Mesh3DInstance meshInstance;
		Transform placement = Transform::identity;
		Transform finalPlacement = Transform::identity;
		Rotator3 verticalOffset = Rotator3::zero; // radius may enlarge, as radius is kept at "at"
		bool beVertical = false; // will be vertical, not leaning, also vertical size is calculated a bit differently (using sin_deg(1.0) as vertical "pixel" - this is also used for vertical offset!

		Optional<Transform> lastHubHeadPlacement;

		Array<RefCountObjectPtr<HubWidget>> widgets;

		::System::TimeStamp timeSinceLastActive;

		HubScreen();
		HubScreen(Hub* _hub, Name const & _id, Vector2 const & _size, Rotator3 const & _at, float _radius, Optional<bool> const & _beVertical = NP, Optional<Rotator3> const & _verticalOffset = NP, Optional<Vector2> const & _pixelsPerAngle = NP);

		Transform calculate_screen_centre() const;

		void force_redraw() { forceRedraw = max(forceRedraw, 1); }

		void activate() { activeTarget = 1.0f; }
		void deactivate() { moveAway = true;  bool callOnDeactivate = activeTarget != 0.0f; activeTarget = 0.0f; clean_up_widgets(); if (callOnDeactivate) on_deactivate(); }
		virtual void on_deactivate() { if (do_on_deactivate) do_on_deactivate(); }
		bool is_active() const { return activeTarget == 1.0f; }
		bool can_be_used() const { return canBeUsed; }

		void force_appear_foreground(bool _force = true) { forceAppearForeground = _force; }
		void into_background() { inBackgroundTarget = 1.0f; }
		void into_foreground() { inBackgroundTarget = 0.0f; }

		void be_modal(bool _clickingOutsideDeactivates = false) { modal = true; clickingOutsideDeactivates = _clickingOutsideDeactivates; }
		bool is_modal() const { return modal; }
		bool does_clicking_outside_deactivate() const { return modal && clickingOutsideDeactivates; }

		void be_vertical(bool _beVertical = true) { beVertical = _beVertical; set_placement(at); }

		void clear();
		void clean_up_widgets();

		HubWidgets::Button* add_help_button();

		virtual void advance(float _deltaTime, bool _beyondModal);
		virtual void process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime) {}

		void set_placement(Rotator3 const & _at);
		void update_placement() { set_placement(at); }

		virtual void render_display_and_ready_to_render(bool _lazyRender);

		void add_widget(HubWidget* _w);
		void remove_widget(HubWidget* _w);
		void widget_to_front(HubWidget* _w);
		HubWidget* get_widget(Name const& _id) const;
		void move_widget_to_foreground(Name const & _id);

		Vector2 const & get_size() const { return size; }
		void set_size(Vector2 const & _size, bool _createDisplayAndMesh = true);
		float compress_vertically(Optional<float> const & _maxGap = NP, Optional<bool> const & _compressEdges = NP); // moves widgets closer to avoid gaps larger than _maxGap, if has to reduce bottom gap, set second parameter to true, returns difference in angles (negative)
		void centre_vertically(Range2 const& _within); // rearranges widgets placed within "_within" to be centred vertically

		Range2 add_option_widgets(Array<HubScreenOption> const & _options, Range2 const & range, Optional<float> const& _lineSize = NP, Optional<float> const & _lineSpacing = NP, Optional<float> const & _optionInsideSpacing = NP, Optional<float> const & _centreAxis = NP, Optional<float> const& _fontScale = NP); // returns occupied space

		HubWidgets::Button* add_button_widget(HubScreenButtonInfo const & _button, Vector2 const & _at, Optional<Vector2> const & _pt = NP, Optional<Vector2> const & _insideSpacing = NP);
		void add_button_widgets(REF_ Array<HubScreenButtonInfo> & _buttons, Vector2 const & _leftBottom, Vector2 const & _rightTop, Optional<float> const & spacing = NP);
		void add_button_widgets(REF_ Array<HubScreenButtonInfo> & _buttons, Range2 const & _range, Optional<float> const & spacing = NP);
		void add_button_widgets_grid(REF_ Array<HubScreenButtonInfo> & _buttons, VectorInt2 const & _grid, Vector2 const & _leftBottom, Vector2 const & _rightTop, Optional<Vector2> const & _spacing = NP, bool _autoHighlightSelected = false);
		void add_button_widgets_grid(REF_ Array<HubScreenButtonInfo> & _buttons, VectorInt2 const & _grid, Range2 const & _range, Optional<Vector2> const & _spacing = NP, bool _autoHighlightSelected = false);

		void clear_special_highlights();
		void special_highlight(Name const& _widgetId, bool _highlight = true);

		void dont_follow();

	private:
		bool redrawOnceAgain = false; // to have both frames look the same
		bool drawOnDisplayNow = false;

		void create_display_and_mesh();
		void create_mesh();
	};

};
