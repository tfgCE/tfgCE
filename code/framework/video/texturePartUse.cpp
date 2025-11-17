#include "texturePartUse.h"

#include "texturePart.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//
 
void TexturePartUse::get_uv_info(OUT_ Vector2& startCorner00, OUT_ Vector2& upSide, OUT_ Vector2& rightSide) const
{
	Vector2 uv0 = texturePart->get_uv_0();
	Vector2 uv1 = texturePart->get_uv_1();
	Vector2 sc = uv0;
	Vector2 up = Vector2(0.0f, uv1.y - uv0.y);
	Vector2 rt = Vector2(uv1.x - uv0.x, 0.0f);
	if (mirroredX)
	{
		sc += rt;
		rt = -rt;
	}
	if (mirroredY)
	{
		sc += up;
		up = -up;
	}
	//  0   1   2   3
	// +-1 1-+ +-0 0-+
	// | | | | | | | |
	// 0-+ +-0 1-+ +-1
	int useRot90 = mod(rotated90, 4);
	if (useRot90 <= 1)
	{
		if (useRot90 == 0)
		{
			startCorner00 = sc;
			upSide = up;
			rightSide = rt;
		}
		else
		{
			startCorner00 = sc + rt;
			upSide = -rt;
			rightSide = up;
		}
	}
	else
	{
		if (useRot90 == 2)
		{
			startCorner00 = sc + up + rt;
			upSide = -up;
			rightSide = -rt;
		}
		else
		{
			startCorner00 = sc + up;
			upSide = rt;
			rightSide = -up;
		}
	}
}
