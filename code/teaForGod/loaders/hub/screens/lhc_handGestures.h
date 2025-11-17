#pragma once

#include "..\loaderHubScreen.h"
#include "..\loaderHubWidget.h"
#include "..\..\..\..\framework\game\gameInput.h"

namespace Loader
{
	namespace HubScreens
	{
		class HandGestures
		: public HubScreen
		{
			FAST_CAST_DECLARE(HandGestures);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			static HandGestures* show(Hub* _hub, std::function<void()> _ok = nullptr, Optional<bool> const & _allowOptionsButton = NP);
			static HandGestures* create(Hub* _hub, std::function<void()> _ok = nullptr, Optional<bool> const& _allowOptionsButton = NP); // don't show!

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);

		private:
			HandGestures(Hub* _hub, std::function<void()> _ok, Optional<bool> const& _allowOptionsButton, Vector2 const & _size, Rotator3 const & _at, float _radius, Vector2 const & _ppa);

		private:
			std::function<void()> onOk; // if nullptr, no button

			::Framework::GameInput gameInput;
			::Framework::GameInput hubInput;
			::Framework::GameInput leftGameInput;
			::Framework::GameInput leftHubInput;
			::Framework::GameInput rightGameInput;
			::Framework::GameInput rightHubInput;

			Hand::Type tryHand = Hand::Right;
			RefCountObjectPtr<HubWidget> handWidget;

			struct Element
			{
				Optional<int> hand;
				Name locStr;
				Framework::TexturePart * tp;
				RefCountObjectPtr<HubWidget> imgWidget;
				RefCountObjectPtr<HubWidget> textWidget;
				Name inputButton;
				bool gameInput = true;

				Element() {}
				Element(Optional<int> const & _hand, Name const& _locStr, Name const& _inputButton, bool _gameInput, Framework::TexturePart * _tp) : hand(_hand), locStr(_locStr), tp(_tp), inputButton(_inputButton), gameInput(_gameInput) {}
			};

			ArrayStatic<ArrayStatic<Element, 4>, 2> elements;

			Framework::TexturePart* shootTP = nullptr;
			Framework::TexturePart* shootThumbTP = nullptr;
		};
	};
};
