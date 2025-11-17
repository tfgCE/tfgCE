#pragma once

#include "iLoader.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\random\random.h"
#include "..\..\core\types\colour.h"

struct Vector2;
struct VectorInt2;

namespace System
{
	class Video3D;
};

namespace Loader
{
	class Circles
	: public ILoader
	{
	public:
		Circles(Colour const & _backgroundColour = Colour::blackWarm, Colour const & _circleColour = Colour::whiteWarm);

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);

	private:
		float angle; // current rotation
		float speed;
		int circleCount;
		float radius; // radius at which they move
		float circleRadius; // single circle
		Colour backgroundColour;
		Colour circleColour;
	};
};
