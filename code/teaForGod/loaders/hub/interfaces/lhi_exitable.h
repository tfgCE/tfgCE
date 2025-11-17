#pragma once

#include "..\loaderHubScreen.h"

namespace Loader
{
	namespace HubInterfaces
	{
		interface_class IExitable
		{
		public:
			virtual ~IExitable() {}

			virtual void exit_exitable() = 0;
			virtual void get_placement_and_size_for(Hub* _hub, Name const & _id, OUT_ Vector2 & _size, OUT_ Rotator3 & _at, OUT_ float & _radius, OUT_ bool & _beVertical, OUT_ Rotator3 & _verticalOffset, OUT_ Vector2& _ppa) = 0;
		};
	};
};
