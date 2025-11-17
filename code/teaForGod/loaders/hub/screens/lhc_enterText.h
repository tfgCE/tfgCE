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
		class EnterText
		: public HubScreen
		{
			FAST_CAST_DECLARE(EnterText);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			enum Keyboard
			{
				LettersAndNumbers,
				NetworkAddress,
				NetworkPort,
			};
		public:
			static void show(Hub* _hub, String const & _info, String const & _text, std::function<void(String const & _text)> _onOK, std::function<void()> _onCancel, bool _continueButtonOnly = false, bool _asSingleScreen = false, Keyboard _keyboard = Keyboard::LettersAndNumbers);

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);

		private:
			EnterText(Hub* _hub, String const & _info, int _infoLineCount, String const & _text, std::function<void(String const& _text)> _onOK, std::function<void()> _onCancel, bool _continueButtonOnly, Vector2 const & _size, Rotator3 const & _at, float _radius, Keyboard _keyboard);

		private:
			String info;
			String text;
			std::function<void(String const& _text)> onOK;
			std::function<void()> onCancel;

			RefCountObjectPtr<HubWidget> editWidget;

			float timeToBlink = 0.0f;
			bool blinkNow = false;

			void update_text();
		};
	};
};
