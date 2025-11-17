#include "overlayInfoSystem.h"

#include "overlayInfoElement.h"

#include "elements\oie_inputPrompt.h"

#include "..\game\gameSettings.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"
#include "..\..\core\vr\iVR.h"

#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace OverlayInfo;

//

// global references
DEFINE_STATIC_NAME_STR(defaultFont, TXT("overlay info font"));

// element id
DEFINE_STATIC_NAME(tip);
DEFINE_STATIC_NAME(waitForLook);

//

OverlayInfo::System* OverlayInfo::System::s_system = nullptr;

OverlayInfo::System::System()
{
	an_assert(!s_system);
	s_system = this;

	SET_EXTRA_DEBUG_INFO(renderLayers, TXT("OverlayInfo::System.renderLayers"));
}

OverlayInfo::System::~System()
{
	an_assert(s_system == this);
	s_system = nullptr;
}

void OverlayInfo::System::force_clear()
{
	Concurrency::ScopedSpinLock lock(pendingElementsLock);

	pendingForceClear = true;
	pendingElementsProcessingRequired = true;
}

void OverlayInfo::System::add(Element* _element)
{
	Concurrency::ScopedSpinLock lock(pendingElementsLock);

	pendingElements.push_back(RefCountObjectPtr<Element>(_element));
	pendingElementsProcessingRequired = true;
}

void OverlayInfo::System::on_transform_vr_anchor(Transform const& _localToVRAnchorTransform)
{
	// we can't move vr anchor placement for these, we should only move elements
	for_every_ref(element, elements)
	{
		element->apply_vr_displacement(_localToVRAnchorTransform, true);
	}
}

void OverlayInfo::System::advance(Usage::Type _usage, Optional<Transform> const & _vrCamera, Framework::IModulesOwner* _owner)
{
	if (pendingElementsProcessingRequired)
	{
		Concurrency::ScopedSpinLock lockPE(pendingElementsLock);
		Concurrency::ScopedMRSWLockWrite lockE(elementsLock);

		pendingElementsProcessingRequired = false;
		if (pendingForceClear)
		{
			elements.clear();
			pendingForceClear = false;
		}
		elements.add_from(pendingElements);
		pendingElements.clear();
	}

	{
		float targetFadeOutDueToTips = 0.0f;
		for_every_ref(e, elements)
		{
			if (e->get_id() == NAME(tip) ||
				fast_cast<Elements::InputPrompt>(e))
			{
				targetFadeOutDueToTips = 1.0f;
				break;
			}
		}
		fadeOutDueToTips = blend_to_using_speed_based_on_time(fadeOutDueToTips, targetFadeOutDueToTips, 0.5f, ::System::Core::get_raw_delta_time());
	}


	if (supressed)
	{
		return;
	}

	float deltaTime = ::System::Core::get_delta_time();

	vrAnchorPlacement = Transform::identity;
	if (_owner &&
		_owner->get_presence())
	{
		vrAnchorPlacement = _owner->get_presence()->get_vr_anchor();
		Transform vrAnchorLastDisplacement = _owner->get_presence()->get_vr_anchor_last_displacement();
		for_every_ref(element, elements)
		{
			element->apply_vr_displacement(vrAnchorLastDisplacement);
		}
	}

	if (_vrCamera.is_set())
	{
		cameraPlacement = _vrCamera.get();
	}
	else
	{
		if (auto* vr = VR::IVR::get())
		{
			if (vr->is_controls_view_valid())
			{
				cameraPlacement = vr->get_controls_view();
			}
		}
	}

	if (elements.is_empty())
	{
		// keep it reset
		anchor.resetPlacement = true;
		anchorHUD.resetPlacement = true;
	}

	{
		if (anchorHUD.resetPlacement)
		{
			followAnchorHUDLocation.clear();
		}

		if (!anchorHUD.resetPlacement &&
			!followAnchorHUDLocation.is_set())
		{
			followAnchorHUDLocation = anchorHUD.location;
		}
		if (followAnchorHUDLocation.is_set())
		{
			Vector3 l = anchorHUD.location;
			Vector3& fl = followAnchorHUDLocation.access();
			fl.z = l.z; // always follow flat
			Vector3 dir = (l - fl).normal();

			float const followSpeed = 0.2f;
			float const distStationary = 0.025f;
			float const distFollowing = 0.1f;
			float const distFollowingMax = 0.2f;

			// move towards the current location
			fl = fl + dir * min(deltaTime * followSpeed, (l - fl).length());

			// but keep close so we won't be lagging to long
			fl = l + (fl - l).normal() * min(distFollowingMax, (fl - l).length());

			float dist = (fl - l).length();

			float useFollowing = clamp((dist - distStationary) / (distFollowing - distStationary), 0.0f, 1.0f);
			anchorHUD.followCameraYaw = 60.0f * (1.0f - useFollowing) + useFollowing * 5.0f;
			anchorHUD.followCameraPitch = 60.0f * (1.0f - useFollowing) + useFollowing * 5.0f;
		}
	}

	for_count(int, iAnchor, 2)
	{
		auto& useAnchor = iAnchor == 0 ? anchor : anchorHUD;
		{
			Rotator3 cameraOrientationAsRotator = cameraPlacement.get_orientation().to_rotator();
			float cameraYaw = cameraOrientationAsRotator.yaw;
			float cameraPitch = cameraOrientationAsRotator.pitch;
			if (useAnchor.forcedRotation.is_set())
			{
				useAnchor.yaw = useAnchor.forcedRotation.get().yaw;
				useAnchor.pitch = useAnchor.forcedRotation.get().pitch;
			}
			else if (useAnchor.resetPlacement)
			{
				useAnchor.yaw = cameraYaw;
				useAnchor.pitch = cameraPitch;
			}
			else
			{
				for (int i = 0; i < 2; ++i)
				{
					float shouldBeAt = i == 0? cameraYaw : cameraPitch;
					float const anchorFollowCamera = i == 0 ? useAnchor.followCameraYaw : useAnchor.followCameraPitch;
					float & what = i == 0 ? useAnchor.yaw : useAnchor.pitch;

					//

					float diff = Rotator3::normalise_axis(shouldBeAt - what);

					float changeRequired = 0.0f;
					if (diff > anchorFollowCamera)
					{
						changeRequired += diff - anchorFollowCamera;
					}
					else if (diff < -anchorFollowCamera)
					{
						changeRequired += diff - (-anchorFollowCamera);
					}

					if (changeRequired != 0.0f)
					{
						float const speed = 720.0f;
						float coef = clamp(changeRequired / 20.0f, -1.0f, 1.0f);
						coef = sign(coef) * sqr(coef);
						float changeDone = deltaTime * speed * coef;

						if (changeRequired > 0.0f && changeDone > changeRequired)
						{
							changeDone = changeRequired;
						}
						if (changeRequired < 0.0f && changeDone < changeRequired)
						{
							changeDone = changeRequired;
						}

						what += changeDone;
						what = Rotator3::normalise_axis(what);
					}
				}
			}
		}

		{
			Vector3 cameraLocation = cameraPlacement.get_translation();
			Vector3 cameraForward = cameraPlacement.get_axis(Axis::Forward);

			if (useAnchor.resetPlacement)
			{
				useAnchor.location = cameraLocation;
			}
			else
			{
				Vector3 diff = (cameraLocation - useAnchor.location);

				{	// Z
					float dist = diff.z;
					if (dist > useAnchor.followCameraLocationZ)
					{
						useAnchor.location.z += (dist - useAnchor.followCameraLocationZ);
					}
					else if (dist < -useAnchor.followCameraLocationZ)
					{
						useAnchor.location.z += (dist - (-useAnchor.followCameraLocationZ));
					}
				}
				{	// XY but with distance only perpendicular to camera, along camera move immediately
					Vector3 diffxy = diff;
					Vector3 cameraForwardXY = cameraForward;
					cameraForwardXY.z = 0.0f;
					cameraForwardXY.normalise();
					diffxy.drop_using(cameraForwardXY);
					float dist = diffxy.length_2d();
					if (dist > useAnchor.followCameraLocationXY)
					{
						useAnchor.location += diffxy.normal_2d() * (dist - useAnchor.followCameraLocationXY) * Vector3(1.0f, 1.0f, 0.0f);
					}
					useAnchor.location += cameraForwardXY * Vector3::dot(diffxy, cameraForwardXY);
				}
			}
		}

		useAnchor.placement = Transform(useAnchor.location, Rotator3(0.0f, useAnchor.yaw, 0.0f).to_quat());
		useAnchor.pyPlacement = Transform(useAnchor.location, Rotator3(useAnchor.pitch, useAnchor.yaw, 0.0f).to_quat());
		useAnchor.resetPlacement = false;
	}

	{
		Concurrency::ScopedMRSWLockWrite lock(elementsLock);
		for (int i = 0; i < elements.get_size(); ++i)
		{
			auto* e = elements[i].get();

			if (e->get_usage() & _usage)
			{
				e->advance(this, deltaTime);
			}
			if (e->has_been_deactivated())
			{
				elements.remove_at(i);
				--i;
			}
		}
	}
}

void OverlayInfo::System::deactivate_but_keep_placement(Name const& _id, bool _force)
{
	{
		Concurrency::ScopedMRSWLockRead lock(elementsLock);
		for_every_ref(e, elements)
		{
			if (e->get_id() == _id &&
				(_force || ! e->get_cant_be_deactivated_by_system()))
			{
				e->keep_placement();
				e->deactivate();
			}
		}
	}
	{
		Concurrency::ScopedSpinLock lock(pendingElementsLock);
		for_every_ref(e, pendingElements)
		{
			if (e->get_id() == _id &&
				(_force || !e->get_cant_be_deactivated_by_system()))
			{
				e->keep_placement(); 
				e->deactivate();
			}
		}
	}
}

void OverlayInfo::System::deactivate(Name const& _id, bool _force)
{
	{
		Concurrency::ScopedMRSWLockRead lock(elementsLock);
		for_every_ref(e, elements)
		{
			if (e->get_id() == _id &&
				(_force || ! e->get_cant_be_deactivated_by_system()))
			{
				e->deactivate();
			}
		}
	}
	{
		Concurrency::ScopedSpinLock lock(pendingElementsLock);
		for_every_ref(e, pendingElements)
		{
			if (e->get_id() == _id &&
				(_force || !e->get_cant_be_deactivated_by_system()))
			{
				e->deactivate();
			}
		}
	}
}

void OverlayInfo::System::deactivate_all(bool _keepAutoDeactivate)
{
	{
		Concurrency::ScopedMRSWLockRead lock(elementsLock);
		for_every_ref(e, elements)
		{
			if (!e->get_cant_be_deactivated_by_system() &&
				(!_keepAutoDeactivate || e->get_auto_deactivate_time_left() == 0.0f))
			{
				e->deactivate();
			}
		}
	}
	{
		Concurrency::ScopedSpinLock lock(pendingElementsLock);
		for_every_ref(e, pendingElements)
		{
			if (!e->get_cant_be_deactivated_by_system() &&
				(!_keepAutoDeactivate || e->get_auto_deactivate_time_left() == 0.0f))
			{
				e->deactivate();
			}
		}
	}
}

void OverlayInfo::System::force_deactivate_all()
{
	{
		Concurrency::ScopedMRSWLockRead lock(elementsLock);
		for_every_ref(e, elements)
		{
			e->deactivate();
		}
	}
	{
		Concurrency::ScopedSpinLock lock(pendingElementsLock);
		for_every_ref(e, pendingElements)
		{
			e->deactivate();
		}
	}
}

bool OverlayInfo::System::deactivate_but_keep(Array<Name> const& _keep, bool _keepAutoDeactivate)
{
	bool allGone = true;
	{
		Concurrency::ScopedMRSWLockRead lock(elementsLock);
		for_every_ref(e, elements)
		{
			if (!e->get_cant_be_deactivated_by_system() &&
				!_keep.does_contain(e->get_id()) &&
				(!_keepAutoDeactivate || e->get_auto_deactivate_time_left() == 0.0f))
			{
				allGone = false;
				e->deactivate();
			}
		}
	}
	{
		Concurrency::ScopedSpinLock lock(pendingElementsLock);
		for_every_ref(e, pendingElements)
		{
			if (!e->get_cant_be_deactivated_by_system() &&
				!_keep.does_contain(e->get_id()) &&
				(!_keepAutoDeactivate || e->get_auto_deactivate_time_left() == 0.0f))
			{
				allGone = false;
				e->deactivate();
			}
		}
	}
	return allGone;
}

void OverlayInfo::System::clear_depth_buffer_once_per_render() const
{
	if (depthBufferCleared)
	{
		return;
	}

	depthBufferCleared = true;

	auto* v3d = ::System::Video3D::get();
	v3d->clear_depth_stencil();
}

void OverlayInfo::System::render(Transform const& _vrAnchor, Usage::Type _usage, Optional<VR::Eye::Type> const& _vrEye, ::System::RenderTarget* _nonVRRenderTarget)
{
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active())
	{
		return;
	}
#endif

	if (GameSettings::get().difficulty.noOverlayInfo)
	{
		return;
	}

	if (elements.is_empty() || supressed || _vrAnchor.get_scale() <= 0.0f)
	{
		return;
	}

	Concurrency::ScopedMRSWLockRead lock(elementsLock);

	prepare_to_render(_vrAnchor, _usage);

	::System::RenderTarget* rt = _nonVRRenderTarget;
	if (_vrEye.is_set())
	{
		an_assert(VR::IVR::get());
		if (auto* vr = VR::IVR::get())
		{
			rt = vr->get_render_render_target(_vrEye.get());
		}
	}

	an_assert(rt);
	rt->bind();

	render_content(_vrAnchor, _usage);

	rt->unbind();
}

void OverlayInfo::System::render_to_current_render_target(Transform const& _vrAnchor, Usage::Type _usage)
{
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active())
	{
		return;
	}
#endif

	if (elements.is_empty() || supressed || _vrAnchor.get_scale() <= 0.0f)
	{
		return;
	}

	Concurrency::ScopedMRSWLockRead lock(elementsLock);

	prepare_to_render(_vrAnchor, _usage);

	render_content(_vrAnchor, _usage);
}

void OverlayInfo::System::prepare_to_render(Transform const& _vrAnchor, Usage::Type _usage)
{
	// elementsLock should be locked to read

	depthBufferCleared = false;

	renderLayers.clear();

	for (int i = 0; i < elements.get_size(); ++i)
	{
		auto* e = elements[i].get();

		if (e->get_usage() & _usage)
		{
			e->request_render_layers(OUT_ renderLayers);
		}
	}

	sort(renderLayers, compare_int);
}

void OverlayInfo::System::render_content(Transform const& _vrAnchor, Usage::Type _usage)
{
	// elementsLock should be locked to read

	auto* v3d = ::System::Video3D::get();

	v3d->setup_for_2d_display();

	auto& modelViewStack = v3d->access_model_view_matrix_stack();
	modelViewStack.push_to_world(_vrAnchor.to_matrix());
	
	auto& clipPlaneStack = v3d->access_clip_plane_stack();
	clipPlaneStack.clear();

	{
		struct ElementRef
		{
			Element const* element = nullptr;
			float dist = 0.0f;
			ElementRef(Element const* _element = nullptr, float _dist = 0.0f) : element(_element), dist(_dist) {}

			static int compare(void const* _a, void const* _b)
			{
				ElementRef const& a = *plain_cast<ElementRef>(_a);
				ElementRef const& b = *plain_cast<ElementRef>(_b);
				if (a.dist > b.dist) return A_BEFORE_B;
				if (a.dist < b.dist) return B_BEFORE_A;
				return A_AS_B;
			}
		};

		ARRAY_STACK(ElementRef, sortedElements, elements.get_size());
		for (int i = 0; i < elements.get_size(); ++i)
		{
			auto* e = elements[i].get();

			if (e->get_usage() & _usage)
			{
				Transform placement = e->get_placement();
				float dist = (placement.get_translation() - cameraPlacement.get_translation()).length();
				sortedElements.push_back(ElementRef(e, dist));
			}
		}
		sort(sortedElements);

		for_every(renderLayer, renderLayers)
		{
			for_every(eRef, sortedElements)
			{
				eRef->element->render(this, *renderLayer);
			}
		}
	}

	modelViewStack.pop();
}

void OverlayInfo::System::update_assets()
{
	defaultFont = Framework::Library::get_current()->get_global_references().get<Framework::Font>(NAME(defaultFont));
}

Framework::Font* OverlayInfo::System::get_default_font() const
{
	return defaultFont.get();
}

float OverlayInfo::System::get_fade_out_due_to_tips(Element const* _e) const
{
	if (fadeOutDueToTips > 0.0f)
	{
		if (_e->get_id() != NAME(tip) &&
			_e->get_id() != NAME(waitForLook) &&
			! fast_cast<Elements::InputPrompt>(_e))
		{
			return fadeOutDueToTips;
		}
	}
	return 0.0f;
}

void OverlayInfo::System::apply_fade_out_due_to_tips(Element const* _e, REF_ Colour& _colour) const
{
	if (fadeOutDueToTips > 0.0f)
	{
		float fadeOut = get_fade_out_due_to_tips(_e);
		float affect = 1.0f - fadeOut * 0.5f;
		_colour *= affect;
	}
}
