#pragma once

#include "..\loaderHubScreen.h"
#include "..\interfaces\lhi_exitable.h"

#include "..\..\..\..\core\types\hand.h"

namespace TeaForGodEmperor
{
	class PilgrimSetup;
};

namespace Loader
{
	namespace HubScreens
	{
		class ExitIExitable
		: public HubScreen
		{
			FAST_CAST_DECLARE(ExitIExitable);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;

		public:
			static Name const & name();
			static void show(Hub* _hub, HubInterfaces::IExitable* _exitable, Vector2 const & _extraSizePt = Vector2::zero);

		private:
			HubInterfaces::IExitable* exitable = nullptr;
			ExitIExitable(Hub* _hub, HubInterfaces::IExitable* _exitable, Vector2 const & _size, Rotator3 const & _at, float _radius, bool _beVertical, Rotator3 const & _verticalOffset, Vector2 const & _pixelsPerAngle);
		};
	};
};
