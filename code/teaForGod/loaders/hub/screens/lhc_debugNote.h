#pragma once

#include "..\loaderHubScreen.h"

namespace Loader
{
	namespace HubWidgets
	{
		struct Text;
	};

	namespace HubScreens
	{
		class DebugNote
		: public HubScreen
		{
			FAST_CAST_DECLARE(DebugNote);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			static void show(Hub* _hub, String const & _info, String const & _text, bool _quickSend = false, bool _sendLogAlways = false);

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);

		private:
			DebugNote(Hub* _hub, String const & _info, int _infoLineCount, String const & _text, Vector2 const & _size, Rotator3 const & _at, float _radius, bool _sendLogAlways);

		private:
			String info;
			String text;

			RefCountObjectPtr<HubWidget> editWidget;

			void update_text();

			void send_note(bool _withLog = false);
			void write_note();
		};
	};
};
