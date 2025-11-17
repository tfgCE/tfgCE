#pragma once

#include "..\utils\skipScene.h"
#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\sound\playback.h"
#include "..\..\core\vr\vrEnums.h"
#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\loaders\loaderBase.h"

//

class SimpleVariableStorage;

namespace Framework
{
	class MeshStatic;
	class Sample;
	namespace Render
	{
		class Camera;
	};
};

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		struct Element;
	};
};

namespace Loader
{
	class StarField
	: public Loader::Base
	{
		typedef Loader::Base base;
	public:
		StarField(Optional<Transform> const & _forward = NP);
		~StarField();

		void learn_from(SimpleVariableStorage const& _params);

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);
		implement_ bool activate();
		implement_ void deactivate();

	private:
		Random::Generator rg;

		Transform forward;

		bool shouldDeactivate = false;

		float activation = 0.0f;
		float targetActivation = 1.0f;
		float apparentActivation = 0.0f;

		float timeActive = 0.0f;
		float remainActiveTime = 0.0f;

		float fadeInTime = 1.2f;
		float fadeOutTime = 0.6f;
		float fadeInDelayLeft = 0.0f;
		bool forcePostFadeInImmediately = false;
		float postFadeOutWait = 0.0f;

		float voiceoverDelayLeft = 0.0f;
		float voiceoverPostDelayLeft = 0.0f; // if negative, ends before voiceover reaches end
		bool silenceVoiceoverAtEnd = true;
		Framework::UsedLibraryStored<Framework::Sample> voiceover;
		Name voiceoverActor;
		bool voiceoverSpoke = false;

		bool playedSample = false;
		float playSampleDelayLeft = 0.0f;
		bool appearFadedOutOnNoSample = false;
		Framework::UsedLibraryStored<Framework::Sample> playSample;
		::Sound::Playback playedSamplePlayback;

		Framework::UsedLibraryStored<Framework::MeshStatic> planetMesh;
		Transform planetMeshAt = Transform::identity;
		Vector3 planetRefAt = Vector3::zero;
		float planetScalePerSecond = 1.0f;
		Meshes::Mesh3DInstance planetMeshInstance;

		Framework::UsedLibraryStored<Framework::MeshStatic> moonMesh;
		Transform moonMeshAt = Transform::identity;
		Vector3 moonRefAt = Vector3::zero;
		float moonScalePerSecond = 1.0f;
		Meshes::Mesh3DInstance moonMeshInstance;

		Framework::UsedLibraryStored<Framework::MeshStatic> sunMesh;
		Transform sunMeshAt = Transform::identity;
		Meshes::Mesh3DInstance sunMeshInstance;

		Framework::UsedLibraryStored<Framework::MeshStatic> shipMesh;
		Meshes::Mesh3DInstance shipMeshInstance;

		bool promptSkipScene = false;
		float blockPromptSkipSceneForLeft = 0.0f;
		TeaForGodEmperor::SkipScene skipScene;

		Transform viewPlacement = Transform::identity;

		float speed = 20.0f;
		float radius = 1000.0f;
		struct Star
		{
			Vector3 loc;
			Colour colour;
		};
		Array<Star> movingStars;
		int movingStarCount = 10000;
		Array<Star> staticStars;
		int staticStarCount = 3000;
		float starsDimmer = 0.0f;

		bool ship = false;
		Transform shipAt = Transform::identity;
		Vector3 shipSpeed = Vector3::zero;
		Colour shipColour = Colour::white;
		float shipScalePerSecond = 1.0f;

		Array<Star> starVertices;

		::System::VertexFormat *starsVertexFormat;

		void render_star_field(::System::RenderTarget* _rt, ::Framework::Render::Camera const & _camera, bool _vr, VR::Eye::Type _eye);

		void setup_overlay(bool _active);
	};
};
