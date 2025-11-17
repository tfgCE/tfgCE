#include "vrFovPort.h"

#include "..\math\math.h"

#ifdef AN_WINDOWS
#include "OVR_CAPI_GL.h"
#endif

#ifdef AN_WINDOWS
VRFovPort::VRFovPort(ovrFovPort_ const & _ovrFovPort)
: left(-_ovrFovPort.LeftTan)
, right(_ovrFovPort.RightTan)
, up(_ovrFovPort.UpTan)
, down(-_ovrFovPort.DownTan)
{
	tan_to_deg();
}
#endif

void VRFovPort::tan_to_deg()
{
	left = atan_deg(left);
	right = atan_deg(right);
	up = atan_deg(up);
	down = atan_deg(down);
}

float VRFovPort::get_fov() const
{
	an_assert(is_sane());
	return abs(up) + abs(down);
}

Vector2 VRFovPort::get_lens_centre_offset() const
{
	an_assert(is_sane());
	/**		.
	 *	. a	| b	.	<----
	 *	:\	|  /:		|
	 *	: \	| / :		| h
	 *	:  \|/  :		|
	 *	*ax	o bx*	<----
	 *
	 *	we know tangens of a and b and because for same distance (h) we actually have proportions
	 *	easy, right?
	 */
	float const tLeft = tan_deg(left);
	float const tRight = tan_deg(right);
	float const tUp = tan_deg(up);
	float const tDown = tan_deg(down);
	Vector2 tanSum(abs(tLeft) + abs(tRight), abs(tDown) + abs(tUp));
	if (tanSum.x != 0.0f && tanSum.y != 0.0f)
	{
		Vector2 offset(-1.0f + 2.0f * (abs(tLeft) / tanSum.x),
					   -1.0f + 2.0f * (abs(tDown) / tanSum.y));
		return offset;
	}
	else
	{
		return Vector2::zero;
	}
}
