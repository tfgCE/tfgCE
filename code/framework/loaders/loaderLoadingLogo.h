#pragma once

#include "loaderBase.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\random\random.h"
#include "..\..\core\types\colour.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\system\video\renderTarget.h"
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
	class LoadingLogo
	: public Base
	{
		typedef Base base;
	public:
		LoadingLogo(bool _onlySplash = false, Optional<float> const& _showLimit = NP, Optional<bool> const& _waitForVR = NP);
		~LoadingLogo();

		void display_at(::System::Video3D * _v3d, bool _vr, Vector2 const & _at = Vector2::zero, float _scale = 1.0f);

		void setup_static_display();

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);

		override_ void deactivate() { targetActivation = 0.0f; }

	private:
		bool onlySplash = false;

		Optional<float> startingYaw; // nots used for "onlySplash"
		Optional<Rotator3> rotation; // used only for "onlySplash"

		bool waitForVR = false;

		float activation = 0.0f;
		float targetActivation = 1.0f;
		float blockDeactivationFor = 0.0f; // in some cases we want to display a little bit longer

		float logoHeight = 5.0f;
		float logoDistance = 12.0f;

		::Meshes::Mesh3D* logoMesh = nullptr;
		::Meshes::Mesh3D* logoDemoMesh = nullptr;
		::Meshes::Mesh3DInstance logoMeshInstance;
		::Meshes::Mesh3DInstance logoDemoMeshInstance;
		::System::Material* logoMaterial = nullptr;
		::System::Material* logoDemoMaterial = nullptr;

		RefCountObjectPtr<::System::Shader> logoVertexShader;
		RefCountObjectPtr<::System::Shader> logoFragmentShader;
		RefCountObjectPtr<::System::ShaderProgram> logoShaderProgram;

		::System::Texture* logoTexture = nullptr;
		::System::Texture* logoDemoTexture = nullptr;

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
		Optional<float> showTimeLeft;

		Array<::System::Texture*> splashTextures;
		Array<::System::Texture*> otherSplashTextures;

		void construct_logos();
		void construct_rt();
		void construct_shaders();

		void add_point(REF_ Array<int8> & _vertexData, REF_ int & _pointIdx, ::System::VertexFormat const & _vertexFormat, Vector2 const & _point, Vector2 const & _uv = Vector2::zero);
		void add_triangle(REF_ Array<int32> & _elements, REF_ int & _elementIdx, int32 _a, int32 _b, int32 _c);

		void update_starting_yaw_if_required();
	};
};
