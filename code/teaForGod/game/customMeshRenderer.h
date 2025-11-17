#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\types\colour.h"

#include "game.h"

namespace Framework
{
	class Mesh;
};

namespace TeaForGodEmperor
{
	struct CustomMeshRenderer
	{
	public:
		CustomMeshRenderer();
		~CustomMeshRenderer();

		void setup_for(VectorInt2 const & _resolution);

		void set_rotation_speed(Rotator3 const & _rotationSpeed) { rotationSpeed = _rotationSpeed; }

		void set_camera_fov(float _fov) { cameraFOV = _fov; update_offsets_and_distances(); }
		void set_background_colour(Colour const & _backgroundColour) { backgroundColour = _backgroundColour; }
		void set_take_view_space_pt(float _pt) { takesViewSpacePt = _pt; update_offsets_and_distances(); }

		// if using by display, remember to unplug it!
		::System::RenderTarget* get_scene_render_target() const { return renderContext.sceneRenderTarget.get(); }
		::System::RenderTarget* get_final_render_target() const { return renderContext.finalRenderTarget.get(); }

		void update(float _deltaTime);
		void render();

	private:
		VectorInt2 resolution;
		Colour backgroundColour = Colour::black;
		Framework::CustomRenderContext renderContext;

		Transform placement = Transform::identity;
		Rotator3 rotationSpeed = Rotator3::zero;

		float takesViewSpacePt = 0.95f; // this is used with itemRadius (presenceLinkRadius) to calculate distance of object (verticaly or horizontaly)
		float cameraFOV = 40.0f; //

		Framework::Mesh const * renderMesh = nullptr;
		Vector3 meshCentreOffset = Vector3::zero;
		float meshDistance = 0.0f;

		Meshes::Mesh3DInstance meshInstance;

		void set_mesh(Framework::Mesh const * _mesh);

		void update_offsets_and_distances();
	};

};