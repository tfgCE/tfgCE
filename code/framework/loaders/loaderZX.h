#pragma once

#include "iLoader.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\random\random.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\system\timeStamp.h"

struct Vector2;
struct VectorInt2;
struct Colour;

namespace System
{
	class Video3D;
};

namespace Loader
{
	class ZX
	: public ILoader
	{
	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);

		//override_ void deactivate() { }

	private:
		float y = 0.0f;
		Optional<float> startingY;
		System::TimeStamp loadingTS;
		Array<bool> loadingBits;
		Random::Generator randomGenerator;

		void draw_blocks_at(System::Video3D * _v3d, Vector2 const & _screenOffset, Colour const & _colour, VectorInt2 const & _at, tchar const * const * _bytes);
	};
};
