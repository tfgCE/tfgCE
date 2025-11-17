#pragma once

#include "editorBase.h"

//

namespace Framework
{
	class Sprite;

	namespace UI
	{
		struct CanvasButton;
	};

	namespace Editor
	{
		class BasicInfo
		: public Base
		{
			FAST_CAST_DECLARE(BasicInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;

		public:
			BasicInfo();
			virtual ~BasicInfo();

		public: // Base
			override_ void show_asset_props();
		};
	};
};
