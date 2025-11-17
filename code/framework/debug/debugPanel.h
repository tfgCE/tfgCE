#pragma once

#include "..\..\core\containers\list.h"
#include "..\..\core\math\math.h"
#include "..\..\core\types\colour.h"
#include "..\..\core\types\hand.h"
#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\types\string.h"

#include <functional>

#ifndef AN_CLANG
namespace VR
{
	namespace Eye
	{
		enum Type;
	};
};
#else
#include "..\..\core\vr\iVR.h"
#endif

namespace Framework
{
	class Game;
	struct DebugPanel;

	struct DebugPanelOption
	{
	public:
		typedef std::function<void(int _value)> Action;

	public:
		DebugPanelOption & no_options() { noOptions = true; return *this; }
		DebugPanelOption & add(tchar const * _value, Optional<int> const & _optionValue = NP, Optional<Colour> const & _optionColour = NP);
		DebugPanelOption & add_off_on();
		DebugPanelOption & set_on_press(Action _on_press) { on_press = _on_press; return *this; }
		DebugPanelOption & set_on_hold(Action _on_hold) { on_hold = _on_hold; return *this; }
		DebugPanelOption & do_not_highlight() { useHighlight = false; return *this; }

		void set_text(tchar const * _name) { name = _name; }

	private:
		struct Value
		{
			String text;
			Optional<int> value;
			Optional<Colour> colour;
			Value() {}
			Value(tchar const * _value, Optional<int> const & _optionValue = NP, Optional<Colour> const & _optionColour = NP)
			: text(_value)
			, value(_optionValue)
			, colour(_optionColour)
			{}
		};

		Name id;
		String name;
		Optional<Colour> backgroundColour;
		Optional<Colour> textColour;

		int valueIdx = 0;
		bool useHighlight = true;
		Array<Value> values; // if empty, just off/on
		bool noOptions = false;
		Action on_press = nullptr;
		Action on_hold = nullptr;

		int get_value() const { return values.is_index_valid(valueIdx) ? values[valueIdx].value.get(valueIdx) : valueIdx; }

		friend struct DebugPanel;
	};
	
	struct DebugPanel
	{
	public:
		DebugPanel(Game* _game, Optional<float> const & _baseVRScale = NP);

		void advance(bool _vr);
		void render_2d();
		void render_vr(VR::Eye::Type _eye);

		void add_separator();
		DebugPanelOption & add_option(Name const & _id, tchar const * _name, Optional<Colour> const & _backgroundColour = NP, Optional<Colour> const & _textColour = NP);
		int get_option(Name const & _id);
		void set_option(Name const & _id, int _value);

		int get_option_index(Name const & _id);
		void set_option_index(Name const & _id, int _value);

		void keep_position(bool _keepPosition = true) { keepPosition = _keepPosition; }
		void auto_orientate(bool _autoOrientate = true) { autoOrientate = _autoOrientate; }
		void allow_change_orientation(bool _allowChangeOrientation = true, float _scale = 1.0f) { orientationScale = _allowChangeOrientation ? _scale : 0.0f; }
		void follow_camera(bool _follow, float _deadzone = 20.0f) { followCamera = _follow; followCameraYaw = _follow ? _deadzone : 0.0f;  }
		void set_default_panel_placement(Transform const& _defaultPanelPlacement) { defaultPanelPlacement = _defaultPanelPlacement; }

		void be_relative_to_camera() { relativeToCamera = true; }

		void show_performance(bool _showPerformance = true) { showPerformance = _showPerformance; }
		void show_log_title(tchar const* _title = nullptr);
		void show_log(LogInfoContext const* _log, bool _backwards = false) { showLog = _log; showLogBackwards = _backwards; }
		void setup_show_log(int _showLogLineCount) { showLogLineCount = _showLogLineCount; }
		
		void lock(bool _locked) { locked = _locked; }
		void lock_dragging(bool _lockedDragging) { lockedDragging = _lockedDragging; }

		void option_lock(bool _lock) { optionsLocked = _lock; }

	public:
		Vector2 buttonSize = Vector2(270.0f, 60.0f);
		Vector2 buttonSpacing = Vector2(20.0f, 10.0f);
		float fontScale = 1.25f;

	private:
		Game* game = nullptr;

		bool optionsLocked = false; // when creating, so they can't be accessed

		bool locked = false;
		bool lockedDragging = false;

		bool followCamera = false;
		float followCameraYaw = 20.0f;
		Optional<float> cameraYaw;

		bool keepPosition = false;
		bool relativeToCamera = false;
		float orientationScale = 0.0f;
		bool autoOrientate = false;

		float baseVRScale = 1.0f;

		Optional<Transform> defaultPanelPlacement;

		Transform renderAtOffset = Transform::identity;
		Optional<Hand::Type> draggedByHand;
		Optional<Transform> draggedHandLastPose;

		Vector2 useViewSize = Vector2::zero;
		int lastFrameForVR = 0;
		Optional<Matrix44> renderVRAtBasePlacement; // if follow camera, this is without camera!
		Matrix44 renderVRAtBase; // this is the final in the vr space
		Matrix44 renderVRAtScaling;
		Matrix44 renderVRAt = Matrix44::identity;
		Vector2 renderVRMouseAt[2];
		Vector2 stickVR[2];
		bool inputActivateVR[2];

		List<DebugPanelOption> debugPanelOptions;

		bool showPerformance = false;

		LogInfoContext const* showLog = nullptr;
		String showLogTitle;
		int showLogLine = 0;
		int showLogLineBackwards = 0;
		bool showLogBackwards = false;
		float showLogLineBits = 0;
		int showLogLineCount = 30;

		void update_view_size();

		void render(bool _vr);
	};
};
