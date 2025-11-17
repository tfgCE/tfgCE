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
	class Text
	: public Base
	{
		typedef Base base;
	public:
		Text(String const & _text, Framework::Font* _font = nullptr, Optional<float> const & _scale = NP, Optional<Colour> const& _backgroundColour = NP, Optional<Colour> const& _textColour = NP, Optional<Colour> const& _fadeFrom = NP, Optional<Colour> const& _fadeTo = NP);
		~Text();

		void display_at(::System::Video3D * _v3d, bool _vr, Vector2 const & _at = Vector2::zero, float _scale = 1.0f);

		void setup_static_display();

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);

		override_ void deactivate() { targetActivation = 0.0f; }

	private:
		String text;
		Framework::Font* font;
		float scale = 0.0f;

		float startingYaw = 0;

		Colour backgroundColour;
		Colour textColour;
		Colour fadeFromColour;
		Colour fadeToColour;

		float activation = 0.0f;
		float targetActivation = 1.0f;
		float blockDeactivationFor = 0.0f; // in some cases we want to display a little bit longer

		Framework::UsedLibraryStored<Framework::Mesh> text2DMesh;
		Framework::UsedLibraryStored<Framework::Mesh> text3DMesh;
		Meshes::Mesh3DInstance text2DMeshInstance;
		Meshes::Mesh3DInstance text3DMeshInstance;

		void construct_text_mesh();

		void add_point(REF_ Array<int8> & _vertexData, REF_ int & _pointIdx, ::System::VertexFormat const & _vertexFormat, Vector2 const & _point, Vector2 const & _uv = Vector2::zero);
		void add_triangle(REF_ Array<int32> & _elements, REF_ int & _elementIdx, int32 _a, int32 _b, int32 _c);
	};
};
