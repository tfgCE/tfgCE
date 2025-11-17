#include "oie_texturePart.h"

#include "..\overlayInfoSystem.h"

#include "..\..\..\core\system\core.h"
#include "..\..\..\framework\video\texturePart.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace OverlayInfo;
using namespace Elements;

//

REGISTER_FOR_FAST_CAST(TexturePart);

void TexturePart::render(OverlayInfo::System const* _system, int _renderLayer) const
{
	if (_renderLayer != renderLayer)
	{
		return;
	}
	if (texturePart)
	{
		float scale = 1.0f;
		Vector2 tpSize = texturePart->get_size();
		if (verticalSize.is_set())
		{
			scale = verticalSize.get() / tpSize.y;
		}
		if (horizontalSize.is_set())
		{
			scale = horizontalSize.get() / tpSize.x;
		}
		if (size.is_set())
		{
			scale = min(size.get() / tpSize.x, size.get() / tpSize.y);
		}
		{
			auto* v3d = ::System::Video3D::get();
			auto& mvs = v3d->access_model_view_matrix_stack();
			auto placement = get_placement();
			mvs.push_to_world(placement.to_matrix());

			float activeDueToCamera = 1.0f;
			{
				Transform cameraPlacement = _system->get_camera_placement();
				float alongDir = abs(Vector3::dot(placement.get_axis(Axis::Y), placement.get_translation() - cameraPlacement.get_translation()));
				activeDueToCamera *= clamp(alongDir / 0.3f, 0.0f, 1.0f);
			}
			Colour useColour = colour.with_alpha(active * activeDueToCamera).mul_rgb(pulse);

			_system->apply_fade_out_due_to_tips(this, REF_ useColour);

			Framework::TexturePartDrawingContext tpdContext;
			tpdContext.scale = Vector2(scale);
			tpdContext.colour = useColour;
			tpdContext.blend = useColour.a < 0.99f;
			tpdContext.showPt = showPt;
			texturePart->draw_at(v3d, Vector3::zero, tpdContext);
			mvs.pop();
		}
	}
}
