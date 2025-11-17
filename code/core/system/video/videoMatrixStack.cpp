#include "videoMatrixStack.h"

#include "video3d.h"

using namespace System;

VideoMatrixStack::VideoMatrixStack(
#ifdef AN_GL
	VideoMatrixMode::Type _id
#endif
)
:	mode( VideoMatrixStackMode::xRight_yForward_zUp )
,	stackSize( 0 )
{
#ifdef AN_GL
	id = _id;
#endif
	stackSize = 1;
	stack[0] = Matrix44::identity;
}

void VideoMatrixStack::clear()
{
	stackSize = 1;
	stack[0] = Matrix44::identity;

	set_vr_for_rendering();
	ready_current();
}

void VideoMatrixStack::set_vr_for_rendering(Optional<Matrix44> const & _mat)
{
	vr = _mat;
	vrForRendering = vr;
	if (mode == VideoMatrixStackMode::xRight_yForward_zUp && vrForRendering.is_set())
	{
		Matrix44 temp = vrForRendering.get();
		Video3D::ready_matrix_for_video_x_right_y_forward_z_up(REF_ temp);
		vrForRendering = temp;
	}
}

void VideoMatrixStack::push_to_world(Matrix44 const & _mat)
{
	an_assert(stackSize < c_stackSize, TXT("matrix stack size exceeded, increase max stack size"));
	stack[stackSize] = stack[stackSize - 1].to_world(_mat);
	++stackSize;

	ready_current();
}

void VideoMatrixStack::push_set(Matrix44 const & _mat)
{
	an_assert(stackSize < c_stackSize, TXT("matrix stack size exceeded, increase max stack size"));
	stack[stackSize] = _mat;
	++ stackSize;

	ready_current();
}

void VideoMatrixStack::pop()
{
	an_assert(stackSize > 1, TXT("matrix stack can't be smaller than 1"));
	-- stackSize;

	ready_current();
}

void VideoMatrixStack::ready_current()
{
	current = stack[stackSize - 1];
	currentForRendering = current;
	if (mode == VideoMatrixStackMode::xRight_yForward_zUp)
	{
		Video3D::ready_matrix_for_video_x_right_y_forward_z_up(REF_ currentForRendering);
	}
}

void VideoMatrixStack::ready_for_rendering(REF_ Matrix44 & _mat) const
{
	if (mode == VideoMatrixStackMode::xRight_yForward_zUp)
	{
		Video3D::ready_matrix_for_video_x_right_y_forward_z_up(REF_ _mat);
	}
}

void VideoMatrixStack::requires_send_all()
{
	send_current_internal();
}

void VideoMatrixStack::force_requires_send_all()
{
	send_current_internal(true);
}

void VideoMatrixStack::send_current_internal(bool _force)
{
#ifdef AN_GL
#ifdef AN_DEVELOPMENT
	if (_force || currentSentForRendering != currentForRendering)
	{
		assert_rendering_thread();
		// this is only required for debug rendering!
		Video3D::get()->load_matrix_for_rendering(id, currentForRendering);
		currentSentForRendering = currentForRendering;
		currentSent = current;
	}
#endif
#endif
}

void VideoMatrixStack::set_mode(VideoMatrixStackMode::Type _mode)
{
	mode = _mode;
}

