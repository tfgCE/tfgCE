#include "math.h"

//

DEFINE_STATIC_NAME(cubic);
DEFINE_STATIC_NAME_STR(cubicIn, TXT("cubic in"));
DEFINE_STATIC_NAME_STR(cubicOut, TXT("cubic out"));
DEFINE_STATIC_NAME(quadratic);
DEFINE_STATIC_NAME(sinus);

//

float BlendCurve::apply(Name const & _type, float _a)
{
	if (_type == NAME(cubic)) return cubic(_a);
	if (_type == NAME(cubicIn)) return cubic_in(_a);
	if (_type == NAME(cubicOut)) return cubic_out(_a);
	if (_type == NAME(quadratic)) return quadratic(_a);
	if (_type == NAME(sinus)) return sinus(_a);

	return linear(_a);
}
