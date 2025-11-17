#pragma once

#include "..\containers\array.h"

namespace Latent
{
	class Frame;

	struct FramesInspection
	{
		Array<Frame const *> frames;
		int frameIdx = NONE;

		void ready();
		bool add(Frame const * _frame);
		void change_frame(int _by);
		Frame const * get_frame_to_inspect() const;
	};
};
