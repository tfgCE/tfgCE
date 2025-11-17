#include "oie_altTexturePart.h"

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

REGISTER_FOR_FAST_CAST(AltTexturePart);

void AltTexturePart::advance(System const* _system, float _deltaTime)
{
	base::advance(_system, _deltaTime);

	time = mod(time + _deltaTime, periodTime);
}

void AltTexturePart::render(OverlayInfo::System const* _system, int _renderLayer) const
{
	if (_renderLayer != renderLayer)
	{
		return;
	}
	bool useAlt = false;
	if (altTexturePartUse.texturePart)
	{
		useAlt = altFirst? time < periodTime * altPt : time > periodTime * (1.0f - altPt);
	}
	bool useNormal = !useAlt || altOnTop;
	for_count(int, altNow, 2)
	{
		if ((altNow && useAlt) ||
			(!altNow && useNormal))
		{
			auto const & useTP = altNow ? altTexturePartUse : texturePartUse;
			if (useTP.texturePart)
			{
				float scale = 1.0f;
				Vector2 tpSize = useTP.texturePart->get_size();
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
					if (altNow && altOffsetPt.is_set())
					{
						Vector2 off = tpSize * scale * altOffsetPt.get();
						placement = placement.to_world(Transform(off.to_vector3_xz(), Quat::identity));
					}
					mvs.push_to_world(placement.to_matrix());

					float activeDueToCamera = 1.0f;
					{
						Transform cameraPlacement = _system->get_camera_placement();
						float alongDir = abs(Vector3::dot(placement.get_axis(Axis::Y), placement.get_translation() - cameraPlacement.get_translation()));
						activeDueToCamera *= clamp(alongDir / 0.3f, 0.0f, 1.0f);
					}
					Colour useColour = (altNow ? altColour.get(colour) : colour).with_alpha(active * activeDueToCamera).mul_rgb(pulse);

					_system->apply_fade_out_due_to_tips(this, REF_ useColour);

					Framework::TexturePartDrawingContext tpdContext;
					tpdContext.scale = Vector2(scale);
					tpdContext.colour = useColour;
					tpdContext.blend = useColour.a < 0.99f;
					tpdContext.use = useTP;
					useTP.texturePart->draw_at(v3d, Vector3::zero, tpdContext);
					mvs.pop();
				}
			}
		}
	}
}
