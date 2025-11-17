#pragma once

#include "..\..\core\concurrency\mrswLock.h"
#include "..\..\core\concurrency\threadSystemUtils.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObjectPtr.h"
#include "..\..\core\vr\iVR.h"

#include "..\..\framework\library\usedLibraryStored.h"

#include "overlayInfoEnums.h"

namespace Framework
{
	class Font;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		struct Element;

		/**
		 *	This is a system to render text, images around the player.
		 */
		class System
		{

		public:
			static System* get() { return s_system; }

			System();
			~System();

			void update_assets();

			Framework::Font* get_default_font() const;

			bool is_empty() const { return elements.is_empty() && pendingElements.is_empty(); }
			void add(Element* _element);
			void deactivate(Name const& _id, bool _force = false); // force if cannot be deactivated by the system
			void deactivate_but_keep_placement(Name const& _id, bool _force = false); // force if cannot be deactivated by the system
			void deactivate_all(bool _keepAutoDeactivate = false); // will keep all that can't be deactivated by system
			bool deactivate_but_keep(Array<Name> const & _keep, bool _keepAutoDeactivate = false); // returns true if nothing else is there
			void force_deactivate_all(); // deactivates all (ALL! including these that tell they shouldn't be deactivated by system)
			void force_clear(); // removes (so they won't fade out)

			void supress() { ++ supressed; }
			void resume() { an_assert(supressed > 0); --supressed; }
			// because of the need to use camera, most likely it is going to be updated on a rendering thread, shortly before rendering (or when rendering scene is being built)
			void advance(Usage::Type _usage, Optional<Transform> const& _vrCamera, Framework::IModulesOwner* _owner);
			
			void render(Transform const & _vrAnchor, Usage::Type _usage, Optional<VR::Eye::Type> const & _vrEye = NP, ::System::RenderTarget* _nonVRRenderTarget = nullptr);
			// or
			void render_to_current_render_target(Transform const& _vrAnchor, Usage::Type _usage);

			Transform const& get_camera_placement() const { return cameraPlacement; }

			Transform get_anchor_placement() const { return anchor.placement; }
			Transform get_anchor_py_placement() const { return anchor.pyPlacement; }
			void mark_reset_anchor_placement() { anchor.resetPlacement = true; }

			void force_anchor_rotation(Optional<Rotator3> const& _offset = NP) { anchor.forcedRotation = _offset; }

			Transform get_anchor_hud_placement() const { return anchorHUD.placement; }
			Transform get_anchor_hud_py_placement() const { return anchorHUD.pyPlacement; }

			Transform get_vr_anchor_placement() const { return vrAnchorPlacement; }
			
			void clear_depth_buffer_once_per_render() const;

			float get_fade_out_due_to_tips(Element const* _e) const;
			void apply_fade_out_due_to_tips(Element const* _e, REF_ Colour & _colour) const;

		public:
			void on_transform_vr_anchor(Transform const& _localToVRAnchorTransform); // useful for sliding locomotion, if vr anchor has moved we want to move some things related to that (like disableWhenMovesTooFarFrom etc)

		public:
			// void add(Element* _element);
			// void clear_all();

		private:
			static System* s_system;

			Framework::UsedLibraryStored<Framework::Font> defaultFont;

			Transform cameraPlacement = Transform::identity;
			mutable bool depthBufferCleared = false;

			Concurrency::MRSWLock elementsLock; // write if modifying
			Array<RefCountObjectPtr<Element>> elements;

			// pending
			Concurrency::SpinLock pendingElementsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.OverlayInfo.System.pendingElementsLock"));
			bool pendingElementsProcessingRequired = false;
			Array<RefCountObjectPtr<Element>> pendingElements;
			bool pendingForceClear = false;

			int supressed = 0;

			float fadeOutDueToTips = 0.0f; // sort of hack to fade out other elements

			struct Anchor
			{
				bool resetPlacement = false;
				Optional<Rotator3> forcedRotation;
				Vector3 location = Vector3::zero;
				float yaw = 0.0f;
				float pitch = 0.0f;
				CACHED_ Transform placement = Transform::identity;
				CACHED_ Transform pyPlacement = Transform::identity;

				float followCameraYaw = 8.0f;
				float followCameraPitch = 8.0f;
				float followCameraLocationXY = 0.03f;
				float followCameraLocationZ = 0.03f;
			};
			Anchor anchor;
			Anchor anchorHUD;

			Optional<Vector3> followAnchorHUDLocation; // will be blended with actual location, alters yaw

			CACHED_ Transform vrAnchorPlacement = Transform::identity;

			ArrayStatic<int, 8> renderLayers;

			void prepare_to_render(Transform const& _vrAnchor, Usage::Type _usage);
			void render_content(Transform const& _vrAnchor, Usage::Type _usage);
		};
	};
};
