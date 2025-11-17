#pragma once

#include "..\loaderHubScene.h"

#include "..\..\..\..\core\types\optional.h"
#include "..\..\..\..\core\types\string.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class TexturePart;
};

namespace Loader
{
	namespace HubScenes
	{
		class Text
		: public HubScene
		{
			FAST_CAST_DECLARE(Text);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			Text(String const & _text, Optional<float> const & _delay = NP, Name const & _screenIDToReuse = Name::invalid());
			Text(tchar const * _text, Optional<float> const & _delay = NP, Name const & _screenIDToReuse = Name::invalid());
			Text(Name const & _locStrId, Optional<float> const & _delay = NP, Name const & _screenIDToReuse = Name::invalid());
			
			static Text* new_reusing_screen(Name const & _screenIDToReuse) { return new Text(nullptr, NP, _screenIDToReuse); }

			Text* close_on_condition(std::function<bool()> _conditionToClose) { conditionToClose = _conditionToClose; return this; }
			Text* with_ppa(Vector2 const & _ppa) { ppa = _ppa; return this; }
			Text* with_vertical_offset(Rotator3 const & _verticalOffset) { verticalOffset = _verticalOffset; return this; }
			Text* with_margin(VectorInt2 const & _margin) { margin = _margin; return this; }
			Text* with_image(Framework::TexturePart* _tp, Optional<Vector2> const& _scaleSpaceRequired = NP, Optional<Vector2> const & _scaleImage = NP, Optional<bool> const& _background = NP);
			Text* with_text_align(Vector2 const& _align) { textAlign = _align; return this; }
			Text* with_text_colour(Optional<Colour> const & _colour) { textColour = _colour; return this; }
			Text* with_extra_background(Colour const & _extraBackground) { extraBackground = _extraBackground; return this; }
			Text* with_follow_head(Optional<bool> const & _followHead) { followHead = _followHead; return this; }

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ bool does_want_to_end();

		private:
			String text;
			Name screenId;
			std::function<bool()> conditionToClose = nullptr; // if set, won't wait for loader to deactivate to proceed
			Vector2 textAlign = Vector2::half;
			Optional<Colour> textColour;
			Optional<Vector2> ppa;
			Optional<Rotator3> verticalOffset;
			Framework::UsedLibraryStored<Framework::TexturePart> image;
			Vector2 imageScaleSpaceRequired = Vector2::one;
			Vector2 imageScale = Vector2::one;
			bool imageBackground = false; // image is background
			Optional<Colour> extraBackground;
			bool loaderDeactivated = false;
			VectorInt2 margin = VectorInt2(1, 1); // in characters
			float delay = 0.0f;
			float timeToShow = 0.0f;
			Optional<bool> followHead;

			void show();
		};
	};
};
