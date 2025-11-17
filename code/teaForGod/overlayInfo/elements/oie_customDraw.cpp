#include "oie_customDraw.h"

#include "..\overlayInfoSystem.h"

#include "..\..\..\core\system\core.h"
#include "..\..\..\framework\video\font.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace OverlayInfo;
using namespace Elements;

//

REGISTER_FOR_FAST_CAST(CustomDraw);

void CustomDraw::render(OverlayInfo::System const* _system, int _renderLayer) const
{
	if (_renderLayer != renderLayer)
	{
		return;
	}
	if (auto* font = _system->get_default_font())
	{
		if (auto* f = font->get_font())
		{
			auto* v3d = ::System::Video3D::get();
			auto& mvs = v3d->access_model_view_matrix_stack();
			Transform usePlacement = get_placement();
			mvs.push_to_world(usePlacement.to_matrix());
			Vector3 relAt = mvs.get_current().get_translation();
			float useScale = size;
			if (autoScale)
			{
				useScale *= calculate_additional_scale(relAt.length(), useAdditionalScale);
			}
			mvs.push_to_world(scale_matrix(Vector3::one * useScale));
			float active = get_faded_active();
			float activeDueToCamera = active;
			if (oneSided)
			{
				Matrix44 checkPlacement = mvs.get_current();
				if (Vector3::dot(checkPlacement.vector_to_world(Vector3::zAxis), -checkPlacement.get_translation()) < 0.0f)
				{
					activeDueToCamera = 0.0f;
				}
			}
			//
			if (custom_draw != nullptr && activeDueToCamera > 0.0f)
			{
				Colour useColour = colour;
				_system->apply_fade_out_due_to_tips(this, REF_ useColour);
				custom_draw(activeDueToCamera, pulse, useColour);
			}
			//
			mvs.pop();
			mvs.pop();
		}
	}
}

void CustomDraw::start_blend(float _active)
{
	if (_active < 1.0f)
	{
		auto* v3d = ::System::Video3D::get();
		v3d->mark_enable_blend();
	}
}

void CustomDraw::end_blend(float _active)
{
	if (_active < 1.0f)
	{
		auto* v3d = ::System::Video3D::get();
		v3d->mark_disable_blend();
	}
}
