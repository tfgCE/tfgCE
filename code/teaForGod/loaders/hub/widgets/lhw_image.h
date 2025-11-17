#pragma once

#include "..\loaderHubWidget.h"

namespace Framework
{
	class TexturePart;
};

namespace Loader
{
	struct HubScreen;
	class Hub;

	namespace HubWidgets
	{
		struct Image
		: public HubWidget
		{
			FAST_CAST_DECLARE(Image);
			FAST_CAST_BASE(HubWidget);
			FAST_CAST_END();

		public:
			typedef HubWidget base;

			Framework::TexturePart* texturePart;

			Vector2 alignPt = Vector2::half;

			Vector2 scale;

			Colour colour = Colour::white;

			Image() {}
			Image(Name const & _id, Range2 const & _at, Framework::TexturePart* _texturePart, Optional<Vector2> const & _scale = NP, Optional<Colour> const & _colour = NP) : base(_id, _at), texturePart(_texturePart), scale(_scale.get(Vector2::one)), colour(_colour.get(Colour::white)) {}
			Image(Name const & _id, Vector2 const & _at, Framework::TexturePart* _texturePart, Optional<Vector2> const & _scale = NP, Optional<Colour> const & _colour = NP);

		public: // HubWidget
			implement_ void render_to_display(Framework::Display* _display);
		};
	};
};
