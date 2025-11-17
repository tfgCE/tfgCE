#pragma once

#include "..\loaderHubScreen.h"
#include "..\..\..\..\core\types\hand.h"

namespace Loader
{
	namespace HubWidgets
	{
		struct Text;
	};

	namespace HubScreens
	{
		class DrawingBoard
		: public HubScreen
		{
			FAST_CAST_DECLARE(DrawingBoard);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			static DrawingBoard* show(Hub* _hub, Vector2 const & _size, Rotator3 const& _at, float _radius);

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);
			override_ void process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime);
			override_ void render_display_and_ready_to_render(bool _lazyRender);

		private:
			DrawingBoard(Hub* _hub, Vector2 const& _size, Rotator3 const & _at, float _radius, Vector2 const& _ppa);

		private:
			::System::RenderTargetPtr drawingBoard;

			Optional<Vector2> penAt[Hand::MAX];
			Optional<Vector2> penWasAt[Hand::MAX];
			bool penShifted[Hand::MAX];

			bool clearDrawingBoard = false;
		};
	};
};
