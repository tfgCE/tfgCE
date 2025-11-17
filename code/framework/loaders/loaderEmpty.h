#pragma once

#include "loaderBase.h"

#include "..\library\usedLibraryStored.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\random\random.h"
#include "..\..\core\types\colour.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\system\video\renderTargetPtr.h"

struct Vector2;
struct VectorInt2;

namespace System
{
	class Texture;
	class Video3D;
};

namespace Framework
{
	class Font;
	class Mesh;
};

namespace Loader
{
	class Empty
	: public Base
	{
		typedef Base base;
	public:
		Empty(Optional<Colour> const& _backgroundColour = NP, Optional<Colour> const& _fadeFrom = NP, Optional<Colour> const& _fadeTo = NP);
		~Empty();

		void display_at(::System::Video3D * _v3d, bool _vr, Vector2 const & _at = Vector2::zero, float _scale = 1.0f);

		void setup_static_display();

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);

		override_ void deactivate() { targetActivation = 0.0f; }

	private:
		Colour backgroundColour;
		Colour fadeFromColour;
		Colour fadeToColour;

		float activation = 0.0f;
		float targetActivation = 1.0f;
		float blockDeactivationFor = 0.0f; // in some cases we want to display a little bit longer
	};
};
