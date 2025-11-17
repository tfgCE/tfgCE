#pragma once

#include "..\..\loaderHubScreen.h"

namespace Loader
{
	namespace HubWidgets
	{
		struct Text;
	};

	namespace HubScreens
	{
		class SpirographMandala
		: public HubScreen
		{
			FAST_CAST_DECLARE(SpirographMandala);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			static Name const& name();

			static void show(Hub* _hub, Optional<Name> const & _name = NP, Optional<Vector2> const& _size = NP, Optional<Rotator3> const& _at = NP, Optional<float> const& _radius = NP, Optional<bool> const& _beVertical = NP, Optional<Rotator3> const& _verticalOffset = NP, Optional<Vector2> const& _pixelsPerAngle = NP);

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);

		private:
			SpirographMandala(Hub* _hub, Name const& _name, Vector2 const & _size, Rotator3 const & _at, float _radius, Optional<bool> const& _beVertical = NP, Optional<Rotator3> const& _verticalOffset = NP, Optional<Vector2> const& _pixelsPerAngle = NP);

		private:
			struct Context
			: public RefCountObject
			{
				float deltaTime = 0.0f;
				bool clear = true;
				bool next = true;
				float delta = 20.0f;
				float a = 0.0f;
				float radius = 1.0f;
				float radiusDelta = 0.0f;
				Colour colour = Colour::red;
				int linesLeft = 200;
				float waitTime = 5.0f;
				float delay = 0.0f;
			};

			RefCountObjectPtr<Context> contextKeeper;
		};
	};
};
