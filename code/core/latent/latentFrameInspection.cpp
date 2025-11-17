#include "latentFrameInspection.h"

using namespace Latent;

void FramesInspection::ready()
{
	frames.clear();
}

bool FramesInspection::add(Frame const * _frame)
{
	frames.push_back(_frame);
	return frames.get_size() - 1 == frameIdx;
}

void FramesInspection::change_frame(int _by)
{
	frameIdx += _by;
	if (frameIdx < NONE) frameIdx = frames.get_size() - 1;
	if (frameIdx >= frames.get_size()) frameIdx = NONE;
}

Frame const * FramesInspection::get_frame_to_inspect() const
{
	return frames.is_index_valid(frameIdx) ? frames[frameIdx] : nullptr;
}
