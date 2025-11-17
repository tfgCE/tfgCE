#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\math\mathUtils.h"

struct Vector2;

namespace Framework
{
	class TexturePart;

	struct TexturePartUse
	{
		TexturePart* texturePart = nullptr;
		bool mirroredX = false; // before rotation
		bool mirroredY = false; // before rotation
		int rotated90 = 0; // clockwise

		TexturePartUse() {}
		explicit TexturePartUse(TexturePart* _tp) : texturePart(_tp) {}
		static TexturePartUse as_is(TexturePart* _tp) { return TexturePartUse(_tp); }

		void get_uv_info(OUT_ Vector2& startCorner00, OUT_ Vector2& upSide, OUT_ Vector2& rightSide) const;

		bool operator==(TexturePartUse const& _other) const
		{
			return texturePart == _other.texturePart
				&& mirroredX == _other.mirroredX
				&& mirroredY == _other.mirroredY
				&& mod(rotated90, 4) == mod(_other.rotated90, 4)
				;
		}
	};
};
