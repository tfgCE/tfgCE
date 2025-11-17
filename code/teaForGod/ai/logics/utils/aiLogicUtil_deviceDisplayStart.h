#pragma once

namespace System
{
	class Video3D;
};

namespace Framework
{
	class Display;
};

namespace TeaForGodEmperor
{
	class EXMType;
	class LineModel;

	namespace AI
	{
		namespace Logics
		{
			namespace Utils
			{
				void draw_device_display_start(Framework::Display* _display, ::System::Video3D* _v3d, int _frame);
			};
		};
	};
};
