#include "sceneBuildContext.h"
#include "renderCamera.h"

#include "renderScene.h"

#include "..\modulesOwner\modulesOwner.h"
#include "..\object\object.h"
#include "..\world\doorInRoom.h"

#include "..\..\core\debug\debugRenderer.h"

//

using namespace Framework;
using namespace Framework::Render;

//

int SceneBuildContext::depthLimit = 20;

void SceneBuildContext::setup(REF_ Scene & _scene, Render::Camera const & _camera)
{
	scene = &_scene;
	sceneView = _scene.view;

	camera = _camera;
	depth = 0;

	if (viewFrustum)
	{
		viewFrustum->clear();
	}
	else
	{
		viewFrustum = new ::System::ViewFrustum();
	}
	viewPlanes.clear();

	// reset "through door" matrix
	throughDoorMatrix = Matrix44::identity;

	// planes
	planes = camera.get_near_far_plane();

	float fov = camera.get_fov();

#ifdef AN_DEBUG_RENDERER
	debug_renderer_apply_fov_override(fov);
#endif

	// create view frustum, but first we need fov size
	{
		fovSize.y = tan_deg(fov * 0.5f);
		fovSize.x = fovSize.y * camera.get_view_aspect_ratio();
	}

	Vector2 viewCentreOffset = camera.get_view_centre_offset() * Vector2(fovSize.x, fovSize.y);

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	if (sceneView == SceneView::External)
	{
		// empty - all around
		viewFrustum->build();
	}
	else
#endif
	{
		float left;
		float right;
		float down;
		float up;
		if (_camera.get_fov_port().is_set())
		{
			auto const fovPort = _camera.get_fov_port().get();
			left = tan_deg(fovPort.left);
			right = tan_deg(fovPort.right);
			down = tan_deg(fovPort.down);
			up = tan_deg(fovPort.up);
		}
		else
		{
			left = -fovSize.x - viewCentreOffset.x;
			right = fovSize.x - viewCentreOffset.x;
			down = -fovSize.y - viewCentreOffset.y;
			up = fovSize.y - viewCentreOffset.y;
		}
		Vector3 lt = Vector3( left, 1.0f, up);
		Vector3 rt = Vector3(right, 1.0f, up);
		Vector3 rb = Vector3(right, 1.0f, down);
		Vector3 lb = Vector3( left, 1.0f, down);
		viewFrustum->add_point(lt);
		viewFrustum->add_point(rt);
		viewFrustum->add_point(rb);
		viewFrustum->add_point(lb);
		viewFrustum->build();
	}

	// store view matrix (as inverted placement of camera)
	viewMatrix = camera.get_placement().to_matrix().inverted();

	// aspect ratio from size
	aspectRatio = camera.get_view_aspect_ratio();

	nearPlane = get_near_far_plane().min;
	float const contextAspectRatio = get_aspect_ratio();
	float const contextFovSize = get_fov_size().length(); // we get length to catch corners
	   /*				door plane										   door plane
		*				|												  /
		*				|												 /
		*			----x-------*-----near-plane----				----x-------*-----near-plane----
		*				|		|									   /		|
		*				|		|									  /	<-		|
		*				|		|									 /	  -fd-	|
		*				|<-fd-->Camera								/		  ->Camera
		*				|										   /
		*		front distance might be here but we actually want to catch point marked "x"!
		*		we have fovsize which is to cover corner of fov
		*		but then we have to remember to catch point x
		*		this is crucial if we're not exactly parallel (to camera dir)
		*		extend then maxdist by utilizing near plane dist too!
		*		and extend a little bit more, just to catch front plane - if it will fall out of view when rendering, nothing bad will happen
		*/
	maxDoorDist = nearPlane
		* max(1.0f, contextAspectRatio) // so we cover side but won't go miss front plane if x/y is < 1.0
		* max(1.0f, contextFovSize); // to handle wider fovs (for narrow ones we're fine with 1.0)
	maxDoorDist = sqrt(sqr(maxDoorDist) + sqr(nearPlane)); // to catch non-parallel (to camera dir) doors
	maxDoorDist *= 1.05f; // extend a bit

#ifdef AN_DEVELOPMENT
	roomProxyCount = 0;
	presenceLinkProxyCount = 0;
	doorInRoomProxyCount = 0;
#endif
}

SceneBuildContext::~SceneBuildContext()
{
	delete_and_clear(viewFrustum);
}

void SceneBuildContext::reset_custom()
{
	additionalRendersOf.clear();
	overrideMaterialRendersOf.clear();
}

void SceneBuildContext::set_additional_render_of(AdditionalRenderOfParams const& _params)
{
	AdditionalRenderOf aro;
	aro.imo = _params.imo;
	aro.forDoorInRooms = _params.forDoorInRooms;
	aro.tagged = _params.tagged;
	aro.notInRoom = _params.notInRoom;
	aro.useMaterial = _params.useMaterial;

	additionalRendersOf.push_back(aro);
}

void SceneBuildContext::for_additional_render_of(Room const * _inRoom, IModulesOwner const * _owner, std::function<void(::System::MaterialInstance const * _material)> _do) const
{
	Tags const* ownerTags = nullptr;
	if (auto* o = _owner->get_as_object())
	{
		ownerTags = &o->get_tags();
	}
	for_every(aro, additionalRendersOf)
	{
		if ((aro->imo == nullptr || aro->imo == _owner) &&
			(! aro->forDoorInRooms) &&
			(aro->notInRoom == nullptr || aro->notInRoom != _inRoom) &&
			(! aro->tagged.is_valid() || (ownerTags && ownerTags->get_tag(aro->tagged))))
		{
			_do(aro->useMaterial);
		}
	}
}

void SceneBuildContext::for_additional_render_of(Room const* _inRoom, DoorInRoom const* _doorInRoom, std::function<void(::System::MaterialInstance const* _material)> _do) const
{
	Tags const* ownerTags = &_doorInRoom->get_tags();
	for_every(aro, additionalRendersOf)
	{
		if ((aro->imo == nullptr) &&
			(aro->forDoorInRooms) &&
			(aro->notInRoom == nullptr || aro->notInRoom != _inRoom) &&
			(!aro->tagged.is_valid() || (ownerTags && ownerTags->get_tag(aro->tagged))))
		{
			_do(aro->useMaterial);
		}
	}
}

void SceneBuildContext::set_override_material_render_of(IModulesOwner const * _owner, ::System::MaterialInstance const * _useMaterial, int _partsAsBits)
{
	OverrideMaterialRenderOf omro;
	omro.imo = _owner;
	omro.useMaterial = _useMaterial;
	omro.partsAsBits = _partsAsBits;
	overrideMaterialRendersOf.push_back(omro);
}

void SceneBuildContext::for_override_material_render_of(Room const* _inRoom, IModulesOwner const * _owner, std::function<void(::System::MaterialInstance const* _material, int _partsAsBits)> _do) const
{
	for_every(omro, overrideMaterialRendersOf)
	{
		if (omro->imo == _owner)
		{
			_do(omro->useMaterial, omro->partsAsBits);
		}
	}
}


