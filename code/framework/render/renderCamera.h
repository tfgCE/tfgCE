#pragma once

#include "..\world\worldCamera.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\vr\iVR.h"

namespace Framework
{
	class Room;
	class IModulesOwner;

	namespace Render
	{
		class Context;

		class Camera
		: public ::Framework::Camera
		{
		public:
			void update_for(::System::RenderTarget const * _rt); // update viewSize but try to keep aspect ratio the same as this could break view frustum!

			void setup(Context & _context);
			void ready_to_render(::System::Video3D* v3d, bool _background = false) const; // sets all rendering
			void switch_background_render(::System::Video3D* v3d, bool _background) const; // switch background

			void adjust_for_vr(VR::Eye::Type _eye, bool _keepWithinRoom = false);
			void adjust_for_vr(VR::Eye::Type _eye, Transform const & _eyeOffsetOverride, bool _keepWithinRoom = false);
	
		public:
			inline void set_background_near_far_plane(Range const & _backgroundPlanes) { backgroundPlanes = _backgroundPlanes; }
			inline void set_background_near_far_plane() { backgroundPlanes = planes; } // same!
			inline Range const & get_background_near_far_plane() const { return backgroundPlanes; }
			inline Matrix44 const & get_background_projection_matrix() const { return backgroundProjectionMatrix; }

		protected:
			Matrix44 backgroundProjectionMatrix;
			Range backgroundPlanes = Range(100.0f, 1000000.0f);

		};

	};
};
