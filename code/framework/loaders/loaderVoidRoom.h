#pragma once

#include "loaderBase.h"

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

namespace Loader
{
	namespace VoidRoomLoaderType
	{
		enum Type
		{
			None,
			PreviewGame,
			SubspaceScavengers,
			TeaForGod,
			TeaForGodEmperor,
			NullPoint,
			BHG,
			LoadingFailed,
			EndingGame,
		};
	};

	class VoidRoom
	: public Base
	{
		typedef Base base;
	public:
		VoidRoom(VoidRoomLoaderType::Type _type, Optional<Colour> const & _backgroundColour = NP, Optional<Colour> const & _letterColour = NP, Optional<Colour> const& _fadeFrom = NP, Optional<Colour> const& _fadeTo = NP);
		~VoidRoom();

		VoidRoom & for_init() { functionType = ForInit; construct_function_mesh(); construct_logos(); return *this; }
		VoidRoom & for_shutdown() { functionType = ForShutdown; construct_function_mesh(); construct_logos(); return *this; }
		VoidRoom & for_video() { functionType = ForVideo; construct_function_mesh(); construct_logos(); return *this; }

		void display_at(::System::Video3D * _v3d, bool _vr, Vector2 const & _at = Vector2::zero, float _scale = 1.0f);

		void setup_static_display();

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);

		override_ void deactivate() { targetActivation = 0.0f; }

	private:
		enum FunctionType
		{
			NoFunction,
			ForInit,
			ForShutdown,
			ForVideo
		};

		FunctionType functionType = NoFunction;

		float startingYaw = 0;

		struct Letter
		{
			float visible = 0.0f;
			float visibilityBlend = 0.0f;
			float delay = 0.0f;
			float angle = 0.0f;
			float rotationStartedAt = 0.0f;
			float rotationPT = 1.0f;
			float rotateBy = 0.0f;
			float rotationLength = 0.5f;
			float timeToRotation = 0.0f;
			int rotationsLeft = 0;
			bool rotates = false;
			Vector2 offsetScale = Vector2::zero;
			::Meshes::Mesh3D * letter = nullptr;
			::Meshes::Mesh3DInstance letterInstance;
		};
		Letter letters[8]; // "void room"

		VoidRoomLoaderType::Type type = VoidRoomLoaderType::PreviewGame;
		Colour backgroundColour;
		Colour letterColour;
		Colour fadeFromColour;
		Colour fadeToColour;

		float letterSize = 1.0f; // size of a single letter
		float letterSpacing = 0.2f;
		CACHED_ float voidRoomSquareSize;
		float thickness = 0.2f;
		int pointsOnArc = 32;

		float lineThickness = 0.05f;
		float deepPT = 0.5f;
		float centrePT = 0.5f;

		Vector2 functionMeshPixelSize = Vector2(0.1f, 0.1f);
		float functionMeshHeight = 0.0f;

		float activation = 0.0f;
		float targetActivation = 1.0f;
		float blockDeactivationFor = 0.0f; // in some cases we want to display a little bit longer

		// they have size of 1x1 and point 0,0 in centre
		::Meshes::Mesh3D * letterV = nullptr;
		::Meshes::Mesh3D * letterO = nullptr;
		::Meshes::Mesh3D * letterI = nullptr;
		::Meshes::Mesh3D * letterD = nullptr;
		::Meshes::Mesh3D * letterR = nullptr;
		::Meshes::Mesh3D * letterM = nullptr;

		::Meshes::Mesh3D * voidRoom = nullptr;
		::Meshes::Mesh3DInstance voidRoomInstance; // bottom to up

		::Meshes::Mesh3D * functionMesh = nullptr;
		::Meshes::Mesh3DInstance functionMeshInstance; // top to down

		struct LogoMesh
		{
			::Meshes::Mesh3D* mesh = nullptr;
			::Meshes::Mesh3DInstance meshInstance;
			::System::Material* material = nullptr;
			float visible = 0.0f;
		};
		Array<LogoMesh> logoMeshes; // start from the bottom and go up
		float logosHeight = 0.0f;
		float logoShowDelay = 0.0f;
		float logoVisibilityBlend = 0.0f;
		Array<LogoMesh> otherLogoMeshes; // credits etc
		float otherLogosHeight = 0.0f;

		RefCountObjectPtr<::System::Shader> logoVertexShader;
		RefCountObjectPtr<::System::Shader> logoFragmentShader;
		RefCountObjectPtr<::System::ShaderProgram> logoShaderProgram;

		Array<::System::Texture*> splashTextures;
		Array<::System::Texture*> otherSplashTextures;

		void construct_logos();
		void construct_letters();
		void construct_function_mesh();
		void construct_shaders();

		void add_point(REF_ Array<int8> & _vertexData, REF_ int & _pointIdx, ::System::VertexFormat const & _vertexFormat, Vector2 const & _point, Vector2 const & _uv = Vector2::zero);
		void add_triangle(REF_ Array<int32> & _elements, REF_ int & _elementIdx, int32 _a, int32 _b, int32 _c);
	};
};
