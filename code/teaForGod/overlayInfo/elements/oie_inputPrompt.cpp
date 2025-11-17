#include "oie_inputPrompt.h"

#include "..\overlayInfoSystem.h"

#include "..\..\..\core\containers\arrayStack.h"
#include "..\..\..\core\system\core.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\game\gameInputDefinition.h"
#include "..\..\..\framework\video\font.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef BUILD_NON_RELEASE
//#define SHOW_MISSING_PROMPTS
#endif

//

using namespace TeaForGodEmperor;
using namespace OverlayInfo;
using namespace Elements;

//

DEFINE_STATIC_NAME(inputPromptPt);

// localised strings
DEFINE_STATIC_NAME_STR(lsCommonOr, TXT("hub; common; or"));

//

REGISTER_FOR_FAST_CAST(InputPrompt);

InputPrompt::~InputPrompt()
{
}

void InputPrompt::advance(OverlayInfo::System const* _system, float _deltaTime)
{
	base::advance(_system, _deltaTime);

	int actualVRControllersIteration = 0;
	if (auto* vr = VR::IVR::get())
	{
		actualVRControllersIteration = vr->get_controllers_iteration();
	}
	if (!vrControllersIteration.is_set() ||
		vrControllersIteration.get() != actualVRControllersIteration)
	{
		prompts.clear();
		vrControllersIteration = actualVRControllersIteration;
		if (auto* gd = Framework::GameInputDefinitions::get_definition(gameInputDefinition))
		{
			ARRAY_STACK(Framework::ProvidedPrompt, providedPrompts, 32);
			ARRAY_STACK(Name, providedPromptTextIds, 32);
			for_every(gi, gameInput)
			{
				gd->provide_prompts(*gi, OUT_ providedPrompts, providedPromptTextIds);
			}
			for_every(p, providedPrompts)
			{
#ifdef SHOW_MISSING_PROMPTS
				output(TXT("use provided prompt \"%S\""), p->prompt.to_char());
#endif
				Prompt prompt;
				prompt.prompt = p->prompt;
				prompt.texturePart = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(p->prompt, true);
				prompt.meshLibInstance = Framework::Library::get_current()->get_global_references().get<Framework::MeshLibInstance>(p->prompt, true);
#ifdef SHOW_MISSING_PROMPTS
				if (auto* ls = prompt.texturePart.get())
				{
					output(TXT("    texture part \"%S\""), ls->get_name().to_string().to_char());
				}
				if (auto* ls = prompt.meshLibInstance.get())
				{
					output(TXT("    mesh lib instance \"%S\""), ls->get_name().to_string().to_char());
				}
#endif
				prompt.forHand = p->forHand;
				prompts.push_back(prompt);
			}
			bool allowTexts = true;
#ifdef AN_CHINA
			// for chinese builds for some odd reason it is not allowed to mix image and text
			if (!providedPrompts.is_empty())
			{
				allowTexts = false;
			}
#endif
			if (allowTexts)
			{
				if (!providedPrompts.is_empty() &&
					!providedPromptTextIds.is_empty())
				{
					Prompt prompt;
					prompt.prompt = NAME(lsCommonOr);
					prompt.text = LOC_STR(NAME(lsCommonOr));
					prompts.push_back(prompt);
				}
				for_every(p, providedPromptTextIds)
				{
					Prompt prompt;
					prompt.prompt = *p;
					prompt.text = LOC_STR(*p);
					prompts.push_back(prompt);
				}
			}
		}
		requiresAdditionalInfoLinesUpdate = true;
	}

	if (requiresAdditionalInfoLinesUpdate)
	{
		requiresAdditionalInfoLinesUpdate = false;

		additionalInfoLines.clear();
		for_every(p, prompts)
		{
			if (!p->text.is_empty())
			{
				additionalInfoLines.push_back(p->text);
			}
		}

		if (!additionalInfo.is_empty())
		{
			if (!additionalInfoLines.is_empty())
			{
				additionalInfoLines.push_back(String::empty());
			}
			additionalInfo.split(String(TXT("~")), additionalInfoLines);
		}
	}
}

void InputPrompt::request_render_layers(OUT_ ArrayStatic<int, 8>& _renderLayers) const
{
	_renderLayers.push_back_unique(renderLayer);
	for_every(prompt, prompts)
	{
		if (prompt->meshLibInstance.is_set())
		{
			_renderLayers.push_back_unique(RenderLayer::Meshes);
			break;
		}
	}
}

void InputPrompt::render(OverlayInfo::System const* _system, int _renderLayer) const
{
	if (_renderLayer != renderLayer && _renderLayer != RenderLayer::Meshes)
	{
		return;
	}
	if (auto* font = _system->get_default_font())
	{
		if (auto* f = font->get_font())
		{
			auto* v3d = ::System::Video3D::get();
			auto& mvs = v3d->access_model_view_matrix_stack();
			mvs.push_to_world(get_placement().to_matrix());
			Vector3 relAt = mvs.get_current().get_translation();
			float additionalScale = calculate_additional_scale(relAt.length(), useAdditionalScale);
			float useScaleBase = scale * additionalScale;
			float useScaleForMesh = useScaleBase * magic_number 2.0f;
			float useScale = useScaleBase * (0.05f / 16.0f); // 16 pixels -> 0.05?
			float active = get_faded_active();
			float spacing = 0.05f;
			float width = 0.0f;
			float noTextureWidth = 128.0f * useScale;
			for_every(prompt, prompts)
			{
#ifndef SHOW_MISSING_PROMPTS
				if (prompt->meshLibInstance.is_set() || prompt->texturePart.is_set())
#endif
				{
					if (prompt->texturePart.is_set())
					{
						width += useScale * prompt->texturePart->get_size().x;
					}
					else
					{
						width += noTextureWidth;
					}
					width += spacing;
				}
			}
			width -= spacing; // last
			//
			Vector3 at(-width * 0.5f, 0.0f, 0.0f);
			::System::FontDrawingContext fdc;
			::Framework::TexturePartDrawingContext tpdc;
			::Meshes::Mesh3DRenderingContext mrc;
			mrc.with_existing_blend().with_existing_depth_mask().with_existing_face_display();
			Colour useColour = colour.with_alpha(active).mul_rgb(pulse);
			_system->apply_fade_out_due_to_tips(this, REF_ useColour);
			if (active < 1.0f)
			{
				// force to blend
				tpdc.blend = true;
			}
			fdc.blend = true; // always blend fonts
			tpdc.scale = Vector2::one * useScale;
			tpdc.colour = useColour;
			float inputPromptPeriod = 1.0f;
			float inputPromptPt = ::System::Core::get_timer_mod(inputPromptPeriod) / inputPromptPeriod;
			Vector2 aiAt = Vector2::zero;

			for_every(prompt, prompts)
			{
				float horizontalScale = 1.0f;
				{
					Hand::Type dominantHand = Hand::Right;
					if (auto* vr = VR::IVR::get())
					{
						dominantHand = vr->get_dominant_hand();
					}
					if (forHand.get(dominantHand) == Hand::Left)
					{
						horizontalScale = -1.0f;
					}
					if (prompt->forHand.is_set())
					{
						horizontalScale = prompt->forHand.get() == Hand::Left? -1.0f : 1.0f;
					}
				}
				tpdc.scale.x = horizontalScale * abs(tpdc.scale.x);
				Vector3 renderAt = at;
#ifndef SHOW_MISSING_PROMPTS
				if (prompt->meshLibInstance.is_set() || prompt->texturePart.is_set())
#endif
				{
					if (prompt->texturePart.is_set())
					{
						at.x += useScale * prompt->texturePart->get_size().x;
					}
					else
					{
						at.x += noTextureWidth;
					}
				}
				if (prompt->texturePart.is_set())
				{
					aiAt.y = min(aiAt.y, -useScale * prompt->texturePart->get_left_bottom_offset().y);
					if (_renderLayer == renderLayer)
					{
						Vector3 rAt = renderAt;
						rAt.x += -prompt->texturePart->get_left_bottom_offset().x * useScale;
						prompt->texturePart->draw_at(v3d, rAt, tpdc);
					}
				}
				{
					Vector3 rAt = renderAt;
					rAt.x += noTextureWidth * 0.5f;
					aiAt.y = min(aiAt.y, -noTextureWidth * 0.5f);
					if (prompt->meshLibInstance.is_set() && _renderLayer == RenderLayer::Meshes)
					{
						_system->clear_depth_buffer_once_per_render();
						mvs.push_to_world(translation_matrix(rAt));
						{
							Vector3 rel = mvs.get_current().get_translation();
							mvs.push_set(look_matrix(rel, rel.normal(), mvs.get_current().vector_to_world(Vector3::zAxis).normal())); // orientate to be always facing same dir - kind of a billboard
						}
						mvs.push_to_world(scale_matrix(Vector3(horizontalScale, 1.0f, 1.0f) * (useScaleForMesh * (BlendCurve::cubic(clamp(active * 2.0f, 0.0f, 1.0f))))));
						mvs.push_to_world(prompt->meshLibInstance->get_placement().to_matrix());
						auto& mi = prompt->meshLibInstance->access_mesh_instance();
						for_count(int, i, mi.get_material_instance_count())
						{
							if (auto* m = mi.access_material_instance(i))
							{
								m->set_uniform(NAME(inputPromptPt), inputPromptPt);
							}
						}
						v3d->push_state();
						v3d->depth_test(::System::Video3DCompareFunc::Less);
						v3d->stencil_test();
						v3d->depth_mask();
						v3d->front_face_order(horizontalScale >= 0.0f? ::System::FaceOrder::CW : ::System::FaceOrder::CCW);
						prompt->meshLibInstance->get_mesh_instance().render(v3d, mrc);
						v3d->pop_state();
						mvs.pop();
						mvs.pop();
						mvs.pop();
						mvs.pop();
					}
#ifdef SHOW_MISSING_PROMPTS
					else if (!prompt->meshLibInstance.is_set() && !prompt->texturePart.is_set() && _renderLayer == renderLayer)
					{
						f->draw_text_at(v3d, prompt->prompt.to_char(), Colour::red, rAt, Vector2::one * useScale, Vector2::half, false, fdc);
					}
#endif
				}
				at.x += spacing;
			}
			if (!additionalInfoLines.is_empty())
			{
				int lineIdx = 0;
				float horizontalAlign = 0.5f;
				float verticalAlign = 1.0f;
				float lineSpacing = 0.25f;
				for_every(line, additionalInfoLines)
				{
					Vector2 pt = Vector2::half;
					pt.x = horizontalAlign;
					pt.y -= ((float)(additionalInfoLines.get_size() - 1) * (1.0f - verticalAlign) + 0.5f - (float)lineIdx) * (1.0f + lineSpacing);
					f->draw_text_at(v3d, line->to_char(), useColour, aiAt, Vector2::one * useScale, pt, false, fdc);
					++lineIdx;
				}
			}
			mvs.pop();
		}
	}
}
