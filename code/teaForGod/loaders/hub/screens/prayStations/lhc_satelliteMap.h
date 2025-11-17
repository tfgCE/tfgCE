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
		class SatelliteMap
		: public HubScreen
		{
			FAST_CAST_DECLARE(SatelliteMap);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			static Name const& name();

			static SatelliteMap* show(Hub* _hub, Optional<Name> const & _name = NP, Optional<Vector2> const& _size = NP, Optional<Rotator3> const& _at = NP, Optional<float> const& _radius = NP, Optional<bool> const& _beVertical = NP, Optional<Rotator3> const& _verticalOffset = NP, Optional<Vector2> const& _pixelsPerAngle = NP);

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);

		private:
			SatelliteMap(Hub* _hub, Name const& _name, Vector2 const & _size, Rotator3 const & _at, float _radius, Optional<bool> const& _beVertical = NP, Optional<Rotator3> const& _verticalOffset = NP, Optional<Vector2> const& _pixelsPerAngle = NP);

		private:
			struct Context
			: public RefCountObject
			{
				Range2 mainResolutionInPixels = Range2::zero;
				Vector2 at = Vector2::zero;
				float deltaTime = 0.0f;
				float waitTime = 0.0f;
				float delay = 0.0f;
				int clear = 5;
				bool next = true;
				int pointsLeft = 200;
				int levelsLeft = 4;
				Colour colour;
			};

			RefCountObjectPtr<Context> contextKeeper;
		}; 
	};
};
