#include "gse_pilgrimOverlayInfo.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\pilgrim\pilgrimOverlayInfo.h"

#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// pilgrim overlay info's tips
DEFINE_STATIC_NAME(open);
DEFINE_STATIC_NAME(lock);
DEFINE_STATIC_NAME_STR(deployWeapon, TXT("deploy weapon"));
DEFINE_STATIC_NAME_STR(holdWeapon, TXT("hold weapon"));
DEFINE_STATIC_NAME_STR(useEnergyCoil, TXT("use energy coil"));
DEFINE_STATIC_NAME_STR(howToUseEnergyCoil, TXT("how to use energy coil"));
DEFINE_STATIC_NAME_STR(guidanceFollowDot, TXT("guidance follow dot"));
DEFINE_STATIC_NAME_STR(upgradeSelection, TXT("upgrade selection"));
DEFINE_STATIC_NAME_STR(custom, TXT("custom"));

//

bool Elements::PilgrimOverlayInfo::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	showMain.load_from_xml(_node, TXT("showMain"));
	
	updateMapIfVisible.load_from_xml(_node, TXT("updateMapIfVisible"));

	showTip.load_from_xml(_node, TXT("showTip"));
	hideTip.load_from_xml(_node, TXT("hideTip"));
	hideAnyTip.load_from_xml(_node, TXT("hideAnyTip"));
	tipPitch.load_from_xml(_node, TXT("tipPitch"));
	tipScale.load_from_xml(_node, TXT("tipScale"));
	tipFollowCamera.load_from_xml(_node, TXT("tipFollowCamera"));
	if (_node->first_child_named(TXT("showCustomTip")))
	{
		tipCustomText.load_from_xml_child(_node, TXT("showCustomTip"), nullptr, true);
		showTip = NAME(custom);
	}

	bootLevel.load_from_xml(_node, TXT("bootLevel"));

	showMainLogAtCamera.load_from_xml(_node, TXT("showMainLogAtCamera"));

	mainLogId.load_from_xml(_node, TXT("mainLogId"));
	mainLogEmptyLine = _node->get_bool_attribute_or_from_child_presence(TXT("mainLogEmptyLine"), mainLogEmptyLine);
	mainLogColour.load_from_xml(_node, TXT("mainLogColour"));
	clearMainLog = _node->get_bool_attribute_or_from_child_presence(TXT("clearMainLog"), clearMainLog);
	clearMainLogLineIndicator = _node->get_bool_attribute_or_from_child_presence(TXT("clearMainLogLineIndicator"), clearMainLogLineIndicator);
	clearMainLogLineIndicatorOnNoVoiceoverActor = _node->get_name_attribute_or_from_child(TXT("clearMainLogLineIndicatorOnNoVoiceoverActor"), clearMainLogLineIndicatorOnNoVoiceoverActor);

	speak.load_from_xml(_node, TXT("speak"), _lc);
	speak.load_from_xml(_node, TXT("speakLine"), _lc);
	speakAutoClear = _node->get_bool_attribute_or_from_child_presence(TXT("speakAutoClear"), speakAutoClear);
	speakAutoClear = ! _node->get_bool_attribute_or_from_child_presence(TXT("noSpeakAutoClear"), ! speakAutoClear);
	speakColour.load_from_xml(_node, TXT("speakColour"));
	speakDelay.load_from_xml(_node, TXT("speakDelay"));
	blockAutoEnergySpeak.load_from_xml(_node, TXT("blockAutoEnergySpeak"));
	speakClearLogOnSpeak.load_from_xml(_node, TXT("speakClearLogOnSpeak"));
	speakAtLine.load_from_xml(_node, TXT("speakAtLine"));

	return result;
}

bool Elements::PilgrimOverlayInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= speak.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

void Elements::PilgrimOverlayInfo::load_on_demand_if_required()
{
	base::load_on_demand_if_required();
	if (auto* s = speak.get())
	{
		s->load_on_demand_if_required();
	}
}

Framework::GameScript::ScriptExecutionResult::Type Elements::PilgrimOverlayInfo::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* g = Game::get_as<Game>())
	{
		todo_multiplayer_issue(TXT("we assume sp"));
		if (auto* pa = g->access_player().get_actor())
		{
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				auto& pio = mp->access_overlay_info();

				if (showMain.is_set())
				{
					pio.show_main(showMain.get());
				}
				if (updateMapIfVisible.get(false))
				{
					pio.update_map_if_visible();
				}
				if (showTip.is_set() || tipCustomText.is_valid())
				{
					TeaForGodEmperor::PilgrimOverlayInfo::ShowTipParams params;
					params.be_forced().with_pitch_offset(tipPitch).with_scale(tipScale);
					if (tipFollowCamera.is_set())
					{
						params.follow_camera_pitch(tipFollowCamera.get());
					}
					if (tipCustomText.is_valid())
					{
						params.with_localised_string_id(tipCustomText.get_id());
						pio.show_tip(PilgrimOverlayInfoTip::Custom, params);
					}
					else if (showTip == NAME(open))
					{
						pio.show_tip(PilgrimOverlayInfoTip::InputOpen, params);
					}
					else if (showTip == NAME(lock))
					{
						pio.show_tip(PilgrimOverlayInfoTip::InputLock, params);
					}
					else if (showTip == NAME(deployWeapon))
					{
						pio.show_tip(PilgrimOverlayInfoTip::InputDeployWeapon, params);
					}
					else if (showTip == NAME(holdWeapon))
					{
						pio.show_tip(PilgrimOverlayInfoTip::InputHoldWeapon, params);
					}
					else if (showTip == NAME(useEnergyCoil))
					{
						pio.show_tip(PilgrimOverlayInfoTip::InputUseEnergyCoil, params);
					}
					else if (showTip == NAME(howToUseEnergyCoil))
					{
						pio.show_tip(PilgrimOverlayInfoTip::InputHowToUseEnergyCoil, params);
					}
					else if (showTip == NAME(guidanceFollowDot))
					{
						pio.show_tip(PilgrimOverlayInfoTip::GuidanceFollowDot, params);
					}
					else if (showTip == NAME(upgradeSelection))
					{
						pio.show_tip(PilgrimOverlayInfoTip::UpgradeSelection, params);
					}
					else
					{
						todo_implement(TXT("tip not implemented"));
					}
				}
				if (hideAnyTip.is_set() && hideAnyTip.get())
				{
					pio.hide_tips();
				}
				if (hideTip.is_set())
				{
					if (hideTip == NAME(open))
					{
						pio.hide_tip(PilgrimOverlayInfoTip::InputOpen);
					}
					else if (hideTip == NAME(lock))
					{
						pio.hide_tip(PilgrimOverlayInfoTip::InputLock);
					}
					else if (hideTip == NAME(deployWeapon))
					{
						pio.hide_tip(PilgrimOverlayInfoTip::InputDeployWeapon);
					}
					else if (hideTip == NAME(holdWeapon))
					{
						pio.hide_tip(PilgrimOverlayInfoTip::InputHoldWeapon);
					}
					else if (hideTip == NAME(useEnergyCoil))
					{
						pio.hide_tip(PilgrimOverlayInfoTip::InputUseEnergyCoil);
					}
					else if (hideTip == NAME(howToUseEnergyCoil))
					{
						pio.hide_tip(PilgrimOverlayInfoTip::InputHowToUseEnergyCoil);
					}
					else if (hideTip == NAME(guidanceFollowDot))
					{
						pio.hide_tip(PilgrimOverlayInfoTip::GuidanceFollowDot);
					}
					else if (hideTip == NAME(upgradeSelection))
					{
						pio.hide_tip(PilgrimOverlayInfoTip::UpgradeSelection);
					}
					else if (hideTip == NAME(custom))
					{
						pio.hide_tip(PilgrimOverlayInfoTip::Custom);
					}
					else
					{
						todo_implement(TXT("tip not implemented"));
					}
				}
				if (showMainLogAtCamera.is_set())
				{
					pio.show_main_log_at_camera(showMainLogAtCamera.get());
				}
				if (clearMainLog)
				{
					pio.clear_main_log();
				}
				if (clearMainLogLineIndicator)
				{
					pio.clear_main_log_line_indicator();
				}
				if (clearMainLogLineIndicatorOnNoVoiceoverActor.is_valid())
				{
					pio.clear_main_log_line_indicator_on_no_voiceover_actor(clearMainLogLineIndicatorOnNoVoiceoverActor);
				}
				if (mainLogEmptyLine)
				{
					pio.add_main_log(String::empty());
				}
				if (mainLogId.is_set())
				{
					pio.add_main_log(LOC_STR(mainLogId.get()), mainLogColour);
				}
				if (bootLevel.is_set())
				{
					pio.set_boot_level(bootLevel.get());
				}
				if (auto* s = speak.get())
				{
					pio.speak(s, TeaForGodEmperor::PilgrimOverlayInfo::SpeakParams()
						.with_colour(speakColour)
						.allow_auto_clear(speakAutoClear)
						.speak_delay(speakDelay)
						.clear_log_on_speak(speakClearLogOnSpeak)
						.at_line(speakAtLine));
				}
				if (blockAutoEnergySpeak.is_set())
				{
					pio.set_auto_energy_speak_blocked(blockAutoEnergySpeak.get());
				}
			}
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
