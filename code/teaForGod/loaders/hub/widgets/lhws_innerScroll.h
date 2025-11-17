#pragma once

#include "..\loaderHubWidget.h"

namespace Loader
{
	struct HubScreen;
	class Hub;

	namespace HubWidgets
	{
		struct InnerScroll
		{
			enum ScrollType
			{
				NoScroll,
				HorizontalScroll,
				VerticalScroll,
			};
			bool autoHideScroll = true;
			ScrollType scrollType = NoScroll;
			float scrollWidth = 0.0f; // set in constructor anyway
			float scrollSpacing = 0.0f;
			Vector2 scroll = Vector2::zero; // we start at left top

			// has to be provided by external
			AUTO_ Range2 viewWindow; // inside on screen
			AUTO_ Vector2 spaceAvailable;
			AUTO_ Range2 scrollWindow; // where scroll is on screen
			AUTO_ float scrollLength; // length on screen
			AUTO_ Range scrollRange;
			AUTO_ bool scrollRequired;
			AUTO_ bool scrollVisible;

			// running
			struct ScrollHeld
			{
				bool held = false;
				Vector2 heldStartAt;
				Vector2 heldStartScroll;
			};
			ScrollHeld scrollHeld[2];
			ScrollHeld scrollHeldGrip[2];

			InnerScroll();

			void make_sane();

			void calculate_auto_space_available(Range2 const & _at);
			void calculate_other_auto_variables(Range2 const & _at, Vector2 const & _wholeThingToShowSize);
			float calculate_scroll_at(Range2 const & _at) const;

			void render_to_display(Framework::Display* _display, Range2 const & _at, Colour const & _border, Colour const & _inside);

			void push_clip_plane_stack();
			void pop_clip_plane_stack();

			bool handle_internal_on_hold(int _handIdx, bool _gripped, bool _beingHeld, Vector2 const & _cursorAt, Range2 const & _at, Vector2 const & _singleAdvancement, OUT_ bool & _result);
			void handle_internal_on_release(int _handIdx, bool _gripped);

			enum InputFlags
			{
				NoScrollWithStick = 1
			};
			void handle_process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime, Vector2 const & _singleAdvancement, Vector2 const & _pixelsPerAngle, int _flags = 0);
		};
	};
};
