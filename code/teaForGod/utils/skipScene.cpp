#include "skipScene.h"

#include "..\game\game.h"
#include "..\overlayInfo\overlayInfoSystem.h"
#include "..\overlayInfo\elements\oie_customDraw.h"
#include "..\overlayInfo\elements\oie_inputPrompt.h"

#include "..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// game input definitions
DEFINE_STATIC_NAME_STR(gidAllTimeControls, TXT("allTimeControls"));

// game input
DEFINE_STATIC_NAME_STR(giSkipScene, TXT("skipScene"));
DEFINE_STATIC_NAME_STR(giSkipSceneIndicate, TXT("skipSceneIndicate"));
DEFINE_STATIC_NAME_STR(giSkipScenePrompt, TXT("skipScenePrompt"));

// localised strings
DEFINE_STATIC_NAME_STR(lsSkipScenePrompt, TXT("common; skip scene prompt"));

//

using namespace TeaForGodEmperor;

//

void SkipScene::prompt_skip_scene()
{
	promptSkipScene = max(3.0f, promptSkipScene);
}

void SkipScene::reset()
{
	if (auto* oie = maySkipPromptOIE.get())
	{
		oie->deactivate();
	}
	if (auto* oie = maySkipProgressOIE.get())
	{
		oie->deactivate();
	}

	{
		allowed = false;
		skipScene = false;
		maySkipPromptOIE.clear();
		maySkipProgressOIE.clear();
		promptSkipScene = 0.0f;
		skipSceneProgress = 0.0f;
	}
}

void SkipScene::update(float _deltaTime)
{
	if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
	{
		// prompt skip scene
		{
			if (allowed)
			{
				if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
				{
					auto& input = g->access_all_time_controls_input();
					if (input.is_button_pressed(NAME(giSkipScenePrompt)))
					{
						promptSkipScene = 2.0f;
					}
				}
			}

			promptSkipScene -= _deltaTime;
			if (skipSceneProgress > 0.0f)
			{
				promptSkipScene = max(0.01f, promptSkipScene);
			}
		}

		// skip scene
		{
			bool maySkip = allowed && !skipScene && promptSkipScene > 0.0f;
			if (maySkip)
			{
				if (!maySkipPromptOIE.get())
				{
					auto* ip = new TeaForGodEmperor::OverlayInfo::Elements::InputPrompt(NAME(gidAllTimeControls), NAME(giSkipScene));

					ip->with_id(NAME(giSkipScene));
					ip->with_pulse();
					ip->with_scale(overlayScale);

					ip->with_additional_info(LOC_STR(NAME(lsSkipScenePrompt)));

					ip->with_location(TeaForGodEmperor::OverlayInfo::Element::Relativity::RelativeToAnchor);
					ip->with_rotation(TeaForGodEmperor::OverlayInfo::Element::Relativity::RelativeToAnchor, Rotator3(overlayPitch, 0.0f, 0.0f));
					ip->with_distance(overlayDistance);

					ois->add(ip);
					maySkipPromptOIE = ip;
				}
				if (!maySkipProgressOIE.get())
				{
					auto* c = new TeaForGodEmperor::OverlayInfo::Elements::CustomDraw();

					c->with_id(NAME(giSkipScene));

					c->with_location(TeaForGodEmperor::OverlayInfo::Element::Relativity::RelativeToAnchor);
					c->with_rotation(TeaForGodEmperor::OverlayInfo::Element::Relativity::RelativeToAnchor, Rotator3(overlayPitch, 0.0f, 0.0f));
					c->with_distance(max(overlayDistance * 0.9f, overlayDistance - 0.5f));

					//c->with_pulse();
					c->with_draw(
						[this](float _active, float _pulse, Colour const& _colour)
						{
							if (skipSceneProgress > 0.0f && allowed && !skipScene)
							{
								Colour colour = _colour.mul_rgb(_pulse).with_alpha(_active);

								float radius = (overlayDistance * 0.5f) * 0.8f * 1.05f * overlayScale;
								radius *= 0.3f;
								float apparentSkipSceneProgress = skipSceneProgress < 0.001f ? 0.0f : skipSceneProgress;
								::System::Video3DPrimitives::ring_xz(colour, Vector3::zero, radius * 0.9f, radius * 1.0f, _active < 1.0f, 0.0f, apparentSkipSceneProgress, radius * 0.05f);
								//::System::Video3DPrimitives::fill_circle_xz(_colour.mul_rgb(_pulse).with_alpha(_active), Vector3::zero, 1.0f * lookAt, _active < 1.0f);
							}
						});

					ois->add(c);
					maySkipProgressOIE = c;
				}
				{
					float targetSkipSceneProgress = 0.0f;
					if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						auto& input = g->access_all_time_controls_input();
						if (input.is_button_pressed(NAME(giSkipScene)))
						{
							targetSkipSceneProgress = 1.0f;
						}
						if (input.is_button_pressed(NAME(giSkipSceneIndicate)))
						{
							targetSkipSceneProgress = max(0.0001f, targetSkipSceneProgress);
						}
					}
					skipSceneProgress = blend_to_using_speed_based_on_time(skipSceneProgress, targetSkipSceneProgress, 1.0f, _deltaTime);
					if (skipSceneProgress >= 1.0f)
					{
						// skip
						skipScene = true;
					}
				}
			}
			else
			{
				if (auto* mspoie = maySkipPromptOIE.get())
				{
					mspoie->deactivate();
					maySkipPromptOIE.clear();
				}
				if (auto* mspoie = maySkipProgressOIE.get())
				{
					mspoie->deactivate();
					maySkipProgressOIE.clear();
				}
			}
		}
	}
}
