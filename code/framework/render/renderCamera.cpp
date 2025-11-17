#include "renderCamera.h"

#include "renderContext.h"

#include "..\world\doorInRoom.h"
#include "..\world\pointOfInterestInstance.h"
#include "..\world\room.h"

#include "..\module\modulePresence.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

namespace Framework
{
	namespace Render
	{
		void Camera::update_for(::System::RenderTarget const * _rt)
		{
#ifdef AN_DEVELOPMENT
			float prevAspectRatio = viewAspectRatio;
#endif
			viewSize = _rt->get_size();
			float aspectRatioCoef = _rt->get_setup().get_aspect_ratio_coef();
			VectorInt2 viewSizeWithAspectRatioCoef = get_full_for_aspect_ratio_coef(viewSize, aspectRatioCoef);
			viewAspectRatio = aspect_ratio(viewSizeWithAspectRatioCoef);
#ifdef AN_DEVELOPMENT
			an_assert(abs(prevAspectRatio - viewAspectRatio) < 0.02f || cameraType == System::CameraType::Orthogonal, TXT("try to avoid changing aspect ratio"));
#endif
		}

		void Camera::setup(Context & _context)
		{
			an_assert(!viewSize.is_zero(), TXT("setup view size and aspect ratio first"));
#ifdef AN_DEBUG_RENDERER
			debug_renderer_apply_fov_override(fov);
#endif

			float farPlaneFov = fov;

			if (cameraType == ::System::CameraType::Orthogonal)
			{
				Range2 range;
				Vector2 size;
				size.y = 1.0f / orthoScale;
				size.x = viewAspectRatio * size.y;
				range.x.min = size.x * 0.5f * (-1.0f - viewCentreOffset.x);
				range.x.max = size.x * 0.5f * ( 1.0f - viewCentreOffset.x);
				range.y.min = size.y * 0.5f * (-1.0f - viewCentreOffset.y);
				range.y.max = size.y * 0.5f * ( 1.0f - viewCentreOffset.y);
				projectionMatrix = orthogonal_matrix(range, get_near_far_plane().min, get_near_far_plane().max);
				backgroundProjectionMatrix = orthogonal_matrix(range, get_background_near_far_plane().min, get_background_near_far_plane().max);
			}
			else
			{
				// calculate projection matrix (and store it)
				if (fovPort.is_set())
				{
					auto const & fp = fovPort.get();
					projectionMatrix = projection_matrix_assymetric_fov(abs(fp.left), abs(fp.right), abs(fp.up), abs(fp.down), get_near_far_plane().min, get_near_far_plane().max);
					backgroundProjectionMatrix = projection_matrix_assymetric_fov(abs(fp.left), abs(fp.right), abs(fp.up), abs(fp.down), get_background_near_far_plane().min, get_background_near_far_plane().max);
				}
				else
				{
					//	this is a fallback solution that relies on lens centre and unified fov
					Matrix44 viewCentreOffsetMatrix = Matrix44::identity;
					viewCentreOffsetMatrix.m30 = viewCentreOffset.x;
					viewCentreOffsetMatrix.m31 = viewCentreOffset.y;

					projectionMatrix = viewCentreOffsetMatrix.to_world(projection_matrix(fov, viewAspectRatio, get_near_far_plane().min, get_near_far_plane().max));
					backgroundProjectionMatrix = viewCentreOffsetMatrix.to_world(projection_matrix(fov, viewAspectRatio, get_background_near_far_plane().min, get_background_near_far_plane().max));
				}
			}
#ifdef AN_ASSERT
			hasProjectionMatrixCalculated = true;
#endif

			_context.setup_far_plane(farPlaneFov, viewAspectRatio, get_near_far_plane().max);
			_context.setup_background_far_plane(farPlaneFov, viewAspectRatio, get_background_near_far_plane().max);
		}

		void Camera::ready_to_render(System::Video3D* v3d, bool _background) const
		{
			v3d->set_fov(fov); // used by debug but also by custom scale objects
			v3d->set_viewport(viewOrigin, viewSize);

			System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();

			modelViewStack.set_mode(System::VideoMatrixStackMode::xRight_yForward_zUp);
			modelViewStack.clear();

#ifdef AN_DEBUG_RENDERER
			modelViewStack.push_set(placement.to_world(debug_renderer_rendering_offset()).to_matrix().inverted());
#else
			modelViewStack.push_to_world(placement.to_matrix().inverted());
#endif

			if (VR::IVR::get())
			{
				if (VR::IVR::get()->is_render_view_valid())
				{
					// setup vr view
					Transform currentVRView = VR::IVR::get()->get_render_view();
					modelViewStack.set_vr_for_rendering(currentVRView.to_matrix().inverted());
				}
			}
			else
			{
				modelViewStack.set_vr_for_rendering(modelViewStack.get_first()); // first acts as view
			}

			System::VideoClipPlaneStack& clipPlaneStack = v3d->access_clip_plane_stack();
			clipPlaneStack.clear();

			switch_background_render(v3d, _background);
		}

		void Camera::switch_background_render(System::Video3D* v3d, bool _background) const
		{
			Range const & nearFarPlane = _background ? get_background_near_far_plane() : get_near_far_plane();
			Matrix44 const & useProjectionMatrix = _background ? backgroundProjectionMatrix : projectionMatrix;

			v3d->set_near_far_plane(nearFarPlane.min, nearFarPlane.max);
			v3d->set_3d_projection_matrix(useProjectionMatrix);
		}

		void Camera::adjust_for_vr(VR::Eye::Type _eye, bool _keepWithinRoom)
		{
			an_assert(VR::IVR::get(), TXT("there should be vr object created"));
			VR::RenderInfo const & renderInfo = VR::IVR::get()->get_render_info();
			Transform eyeOffsetToUse = renderInfo.get_eye_offset_to_use(_eye);
			offset_location(eyeOffsetToUse.get_translation(), _keepWithinRoom);
			offset_orientation(eyeOffsetToUse.get_orientation());
			set_view_centre_offset(renderInfo.lensCentreOffset[_eye] + viewCentreOffset);
			set_fov(renderInfo.fov[_eye], renderInfo.fovPort[_eye]);
		}

		void Camera::adjust_for_vr(VR::Eye::Type _eye, Transform const & _eyeOffsetOverride, bool _keepWithinRoom)
		{
			an_assert(VR::IVR::get(), TXT("there should be vr object created"));
			VR::RenderInfo const & renderInfo = VR::IVR::get()->get_render_info();
			offset_location(_eyeOffsetOverride.get_translation(), _keepWithinRoom);
			offset_orientation(_eyeOffsetOverride.get_orientation());
			set_view_centre_offset(renderInfo.lensCentreOffset[_eye] + viewCentreOffset);
			set_fov(renderInfo.fov[_eye], renderInfo.fovPort[_eye]);
		}

	};
};
