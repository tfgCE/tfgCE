#include "mainConfig.h"
#include "io/xml.h"

#ifdef AN_SDL
#include "SDL.h"
#endif

#include "copyToMainConfig.h"
#include "concurrency\scopedSpinLock.h"
#include "concurrency\threadSystemUtils.h"
#include "other\parserUtils.h"
#include "physicalSensations\psOWO.h"
#include "system\core.h"
#include "tags\tagCondition.h"
#include "vr\iVR.h"
#include "vr\vrOffsets.h"

//

#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define VERBOSE_UPDATE_VR
#endif

//

MainConfig MainConfig::s_global =
	MainConfig::clean()
	;

MainConfig MainConfig::s_defaults =
	MainConfig::clean()
	;

CopyToMainConfig mainConfig_copyToMainConfig;

void MainConfig::initialise_static()
{
}

void MainConfig::reset_static()
{
	s_global = MainConfig::clean();
	s_defaults = MainConfig::clean();
}

void MainConfig::close_static()
{
	// clear before we realise we have a memory leak
	s_global.access_sound().channels.clear();
}

MainConfig::MainConfig()
: allowVR(false)
, owoPort(PhysicalSensations::OWO::default_owo_port())
{
#ifdef AN_VR
	allowVR = true;
#endif
#ifdef AN_ASSUME_AUTO_REPORT_BUGS
	reportBugs = Option::True;
#endif
	update_auto();
}

void MainConfig::update_auto()
{
	timeReservedForSubmittingRenderFrame = (float)timeReservedForSubmittingRenderFrame_us / 1000000.0f; // 1000 * 1000
}

int MainConfig::get_thread_number_forced() const
{
#ifdef AN_DEVELOPMENT
	//return 12;
#endif
	return 0;
}

template <typename EnumStruct, typename EnumStructType>
static EnumStructType parse_using_enum_MAX(String const& _value, EnumStructType _default)
{
	for_count(int, i, EnumStructType::MAX)
	{
		if (_value == EnumStruct::to_char((EnumStructType)i))
		{
			return (EnumStructType)i;
		}
	}
	return _default;
}

#define LOAD_INT_SETTING(variable) variable = _node->get_int_from_child(TXT(#variable), variable);
#define LOAD_FLOAT_SETTING(variable) variable = _node->get_float_from_child(TXT(#variable), variable);
#define LOAD_BOOL_SETTING(variable) variable = _node->get_bool_from_child(TXT(#variable), variable);
#define LOAD_NAME_SETTING(variable) variable = _node->get_name_from_child(TXT(#variable), variable);
#define LOAD_STRING_SETTING(variable) variable = _node->get_string_from_child(TXT(#variable), variable);
#define LOAD_ENUM_SETTING(type, name) name = parse_using_enum_MAX<type, type::Type>(_node->get_string_from_child(TXT(#name)), name);

bool MainConfig::check_system_tag_required(IO::XML::Node const* _node) const
{
	if (_node->has_attribute(TXT("systemTagRequired")))
	{
		TagCondition systemTagRequired;
		if (systemTagRequired.load_from_xml_attribute(_node, TXT("systemTagRequired")))
		{
			if (!systemTagRequired.check(System::Core::get_system_tags()))
			{
				return false;
			}
		}
	}
	return true;
}

bool MainConfig::load_to_be_copied_to_main_config_xml(IO::XML::Node const* _node)
{
	return mainConfig_copyToMainConfig.load_to_be_copied_to_main_config_xml(_node);
}

void MainConfig::save_to_be_copied_to_main_config_xml(IO::XML::Node* _containerNode)
{
	mainConfig_copyToMainConfig.save_to_be_copied_to_main_config_xml(_containerNode);
}

bool MainConfig::load_user_vr_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
#ifdef AN_VR
	LOAD_FLOAT_SETTING(vrHapticFeedback);
	LOAD_FLOAT_SETTING(vrHorizontalScaling);
	LOAD_FLOAT_SETTING(vrScaling);
	if (vrHorizontalScaling < 1.0f)
	{
		vrHorizontalScaling = 1.0f;
	}
	if (vrHorizontalScalingAtLeast < 1.0f)
	{
		vrHorizontalScalingAtLeast = 1.0f;
	}
	if (vrScaling == 0.0f)
	{
		vrScaling = 1.0f;
	}
#endif
	return result;
}

bool MainConfig::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
#ifndef AN_ASSUME_AUTO_REPORT_BUGS
	// load only if non auto
	for_every(node, _node->children_named(TXT("reportBugs")))
	{
		reportBugs = Option::parse(node->get_text().to_char(), reportBugs, Option::Ask | Option::True | Option::False | Option::Unknown);
	}
#endif
	set_report_bugs(reportBugs); // to make sure it is valid for a given platform
	LOAD_INT_SETTING(threadLimit);
	if (threadLimit <= 0) threadLimit = INF_INT;
	LOAD_BOOL_SETTING(threadsUseAffinityMask);
	LOAD_ENUM_SETTING(ThreadPriority, mainThreadPriority);
	LOAD_ENUM_SETTING(ThreadPriority, otherThreadsPriority);
	LOAD_BOOL_SETTING(allowExtraThreads);
	LOAD_STRING_SETTING(systemReservedThreadsString);
	timeReservedForSubmittingRenderFrame_us = _node->get_int_attribute_or_from_child(TXT("timeReservedForSubmittingRenderFrame_us"), timeReservedForSubmittingRenderFrame_us);
	update_auto();
	if (systemReservedThreadsString.is_empty())
	{
		systemReservedThreadsString = String(TXT("auto"));
	}
	if (systemReservedThreadsString == TXT("auto"))
	{
		systemReservedThreads = NONE;
	}
	else
	{
		systemReservedThreads = ParserUtils::parse_int(systemReservedThreadsString, -1);
	}
	LOAD_INT_SETTING(systemReservedThreads);
#ifdef AN_VR
	LOAD_BOOL_SETTING(allowVR);
#ifdef AN_ANDROID
	allowVR = true; // always, doesn't make sense otherwise, we don't support non vr android devices
#endif
	if (!keepForceVRAsIs)
	{
		LOAD_STRING_SETTING(forceVR);
		if (forceVR == TXT("false")) forceVR = String::empty();
	}
	LOAD_NAME_SETTING(pretendVRInputSystem);
	if (allowVR)
	{
		// no pretendVRInputSystem if we're allowing VR
		pretendVRInputSystem = Name::invalid();
	}
	if (auto* node = _node->first_child_named(TXT("vrMode")))
	{
		vrMode = VRMode::parse(node->get_text().trim());
	}
	for_every(node, _node->children_named(TXT("vrMap")))
	{
		vrMap.margin.load_from_xml_child_node(node, TXT("margin"));
		vrMap.yaw = node->get_float_attribute_or_from_child(TXT("rotate"), vrMap.yaw);
		vrMap.offset.load_from_xml_child_node(node, TXT("offset"));
		vrMap.size.load_from_xml(node, TXT("size"));
		if (node->get_float_attribute_or_from_child(TXT("noSize")))
		{
			vrMap.size.clear();
		}
		if (vrMap.size.is_set())
		{
			if (vrMap.size.get().x <= 0.0f || vrMap.size.get().x >= MAX_VR_SIZE ||
				vrMap.size.get().y <= 0.0f || vrMap.size.get().y >= MAX_VR_SIZE)
			{
				warn(TXT("invalid vrMap.size value, reverting to default"));
				vrMap.size.clear();
			}
		}
	}
	load_user_vr_from_xml(_node);
	immobileVR = Option::parse(_node->get_string_from_child(TXT("immobileVR")).to_char(), immobileVR, Option::Auto | Option::True | Option::False);
	immobileVRSize.load_from_xml_child_node(_node, TXT("immobileVRSize"));
	for_every(node, _node->children_named(TXT("vrOffset")))
	{
		String const& handStr = node->get_string_attribute(TXT("hand"));
		if (!handStr.is_empty())
		{
			Hand::Type hand = Hand::parse(handStr);
			vrOffsets[hand].orientation.load_from_xml(node);
			VR::Offsets::apply_custom_offsets();
		}
	}
	validate_immobile_vr();
#else
	allowVR = false;
	forceVR = false;
#endif
	result &= video.load_from_xml(_node->first_child_named(TXT("video")));
	result &= sound.load_from_xml(_node->first_child_named(TXT("sound")));

	physicalSensationsSystem = _node->get_name_attribute_or_from_child(TXT("physicalSensationsSystem"), physicalSensationsSystem);
	owoAddress = _node->get_string_attribute_or_from_child(TXT("owoAddress"), owoAddress);
	owoPort = _node->get_int_attribute_or_from_child(TXT("owoPort"), owoPort);
	bhapticsSetup = _node->get_string_attribute_or_from_child(TXT("bhapticsSetup"), bhapticsSetup);

	LOAD_BOOL_SETTING(blockJoystickInput);
	LOAD_BOOL_SETTING(swapJoystickInput);

	return result;
}

#undef LOAD_INT_SETTING
#undef LOAD_BOOL_SETTING

void MainConfig::save_user_vr_to_xml(IO::XML::Node* _node) const
{
#ifdef AN_VR
	_node->set_float_to_child(TXT("vrHapticFeedback"), vrHapticFeedback);
	_node->set_float_to_child(TXT("vrHorizontalScaling"), vrHorizontalScaling);
	_node->set_float_to_child(TXT("vrScaling"), vrScaling);
#endif
}

void MainConfig::save_to_xml(IO::XML::Node * _node, bool _userConfig) const
{
#ifndef AN_ASSUME_AUTO_REPORT_BUGS
	// save only if non auto
	_node->set_string_to_child(TXT("reportBugs"), Option::to_char(reportBugs));
#endif
	if (!_userConfig)
	{
		_node->set_int_to_child(TXT("threadLimit"), threadLimit != INF_INT ? threadLimit : 0);
		_node->set_bool_to_child(TXT("threadsUseAffinityMask"), threadsUseAffinityMask);
		_node->set_string_to_child(TXT("mainThreadPriority"), ThreadPriority::to_char(mainThreadPriority));
		_node->set_string_to_child(TXT("otherThreadsPriority"), ThreadPriority::to_char(otherThreadsPriority));
		_node->set_bool_to_child(TXT("allowExtraThreads"), allowExtraThreads);
		if (systemReservedThreads != -1)
		{
			_node->set_int_to_child(TXT("systemReservedThreads"), systemReservedThreads);
		}
		else
		{
			_node->set_string_to_child(TXT("systemReservedThreads"), TXT("auto"));
		}
		_node->set_int_to_child(TXT("timeReservedForSubmittingRenderFrame_us"), timeReservedForSubmittingRenderFrame_us);
	}
	// don't save vr related?
#ifdef AN_VR
	//if (!allowVR) { _node->set_bool_to_child(TXT("allowVR"), allowVR); }
	//if (!forceVR.is_empty()) { _node->set_string_to_child(TXT("forceVR"), forceVR); }
	//if (!pretendVRInputSystem.is_empty()) { _node->set_string_to_child(TXT("pretendVRInputSystem"), pretendVRInputSystem.to_string()); }
	// don't save sitting vr?
	if (!_userConfig)
	{
		_node->add_node(TXT("vrMode"))->add_text(VRMode::to_char(vrMode));
	}
	if (auto* node = _node->add_node(TXT("vrMap")))
	{
		vrMap.margin.save_to_xml_child_node(node, TXT("margin"));
		node->add_node(TXT("rotate"))->add_text(String::printf(TXT("%.3f"), vrMap.yaw));
		vrMap.offset.save_to_xml_child_node(node, TXT("offset"));
		if (vrMap.size.is_set())
		{
			vrMap.size.get().save_to_xml_child_node(node, TXT("size"));
		}
		else
		{
			node->add_node(TXT("noSize"));
		}
	}
	save_user_vr_to_xml(_node);
	_node->set_string_to_child(TXT("immobileVR"), Option::to_char(immobileVR));
	if (! immobileVRSize.is_zero())
	{
		immobileVRSize.save_to_xml_child_node(_node, TXT("immobileVRSize"));
	}
	for_count(int, hIdx, Hand::MAX)
	{
		if (auto* node = _node->add_node(TXT("vrOffset")))
		{
			node->set_attribute(TXT("hand"), Hand::to_char((Hand::Type)hIdx));
			vrOffsets[hIdx].orientation.save_to_xml(node);
		}
	}
#endif
	video.save_to_xml(_node->add_node(TXT("video")), _userConfig);
	sound.save_to_xml(_node->add_node(TXT("sound")), _userConfig);

	_node->set_string_to_child(TXT("physicalSensationsSystem"), physicalSensationsSystem.to_char());
	if (!owoAddress.is_empty())
	{
		_node->set_string_to_child(TXT("owoAddress"), owoAddress);
	}
	if (owoPort != 0 && owoPort != PhysicalSensations::OWO::default_owo_port())
	{
		_node->set_int_to_child(TXT("owoPort"), owoPort);
	}
	if (!bhapticsSetup.is_empty())
	{
		_node->set_string_to_child(TXT("bhapticsSetup"), bhapticsSetup);
	}

	if (blockJoystickInput)
	{
		_node->set_bool_attribute(TXT("blockJoystickInput"), blockJoystickInput);
	}
	if (swapJoystickInput)
	{
		_node->set_bool_attribute(TXT("swapJoystickInput"), swapJoystickInput);
	}
}

bool MainConfig::is_option_set(Name const & _option) const
{
#ifdef AN_VR
	if (_option == TXT("allowvr")) return allowVR;
	if (_option == TXT("forcevr")) return ! forceVR.is_empty();
	if (_option == TXT("immobileVR")) return immobileVRNow;
#endif

	return video.is_option_set(_option) || sound.is_option_set(_option);
}

void MainConfig::update_vr()
{
	immobileVRNow = immobileVRAuto;
	if (immobileVR == Option::True) immobileVRNow = true;
	if (immobileVR == Option::False) immobileVRNow = false;

	if (auto* vr = VR::IVR::get())
	{
#ifdef VERBOSE_UPDATE_VR
		output(TXT("[main config] update vr"));
#endif
#ifdef AN_INSPECT_VR_INVALID_ORIGIN
		output(TXT("[vr_invalid_origin] main config update"));
#endif
		vr->update_scaling();
		vr->update_play_area_rect();
		vr->update_map_vr_space();
		vr->update_input_tags();
	}
}

void MainConfig::set_vr_horizontal_scaling(float _vrHorizontalScaling)
{
	vrHorizontalScaling = max(1.0f, _vrHorizontalScaling);
	update_vr();
}

void MainConfig::set_vr_horizontal_scaling_min_value(float _vrHorizontalScalingAtLeast)
{
	vrHorizontalScalingAtLeast = max(1.0f, _vrHorizontalScalingAtLeast);
	update_vr();
}

void MainConfig::set_vr_scaling(float _vrScaling)
{
	vrScaling = _vrScaling;
	if (vrScaling == 0.0f)
	{
		vrScaling = 1.0f;
	}
	update_vr();
}

void MainConfig::set_vr_map_margin(Vector2 const & _margin)
{
	vrMap.margin = _margin;
	update_vr();
}

void MainConfig::set_vr_map_space_rotate(float _yaw)
{
	vrMap.yaw = _yaw;
	update_vr();
}

void MainConfig::set_vr_map_space_size(Optional<Vector2> const& _size)
{
	vrMap.size = _size;
	update_vr();
}

void MainConfig::set_vr_map_space_offset(Vector3 const & _offset)
{
	vrMap.offset = _offset;
	update_vr();
}

void MainConfig::set_immobile_vr_size(Optional<Vector2> const& _immobileVRSize)
{
	immobileVRSize = _immobileVRSize.get(Vector2::zero);
	validate_immobile_vr();
	update_vr();
}

void MainConfig::set_immobile_vr(Option::Type _immobileVR)
{
	immobileVR = _immobileVR;
	validate_immobile_vr();
	update_vr();
}

void MainConfig::set_immobile_vr_auto(bool _immobileVRAuto, bool _forceToAuto)
{
	if (_forceToAuto)
	{
		output(TXT("forcing immobile vr auto (and be immobile)"));
		immobileVR = Option::Auto;
	}
	immobileVRAuto = _immobileVRAuto;
	validate_immobile_vr();
	update_vr();
}

void MainConfig::set_report_bugs(Option::Type _reportBugs)
{
	reportBugs = _reportBugs;
	if (reportBugs == Option::Unknown)
	{
		// keep it this way
		return;
	}
	if (reportBugs != Option::Ask &&
		reportBugs != Option::False &&
		reportBugs != Option::True)
	{
		reportBugs = Option::Ask;
	}
#ifdef AN_ANDROID
	if (reportBugs == Option::Ask)
	{
		reportBugs = Option::True;
	}
#endif
}

void MainConfig::set_thread_setup_from(MainConfig const& _config)
{
	threadsUseAffinityMask = _config.threadsUseAffinityMask;
	mainThreadPriority = _config.mainThreadPriority;
	otherThreadsPriority = _config.otherThreadsPriority;
	threadLimit = _config.threadLimit;
	systemReservedThreads = _config.systemReservedThreads;
	systemReservedThreadsString = _config.systemReservedThreadsString;
	timeReservedForSubmittingRenderFrame_us = _config.timeReservedForSubmittingRenderFrame_us;
	update_auto();
}

void MainConfig::set_physical_sensations_system(Name const & _pss)
{
	physicalSensationsSystem = _pss;
}

void MainConfig::set_owo_address(String const & _addres)
{
	owoAddress = _addres;
}

void MainConfig::set_owo_port(int _port)
{
	owoPort = _port;
}

void MainConfig::set_bhaptics_setup(String const & _bhapticsSetup)
{
	bhapticsSetup = _bhapticsSetup;
}

void MainConfig::validate_immobile_vr()
{
	if (immobileVR != Option::True &&
		immobileVR != Option::Auto)
	{
		immobileVR = Option::False;
	}
	if (immobileVRSize.x <= 0.0f || immobileVRSize.y <= 0.0f)
	{
		immobileVRSize = Vector2(10.0f, 10.0f);
	}
}

MainConfig::VROffset const& MainConfig::get_vr_offset(Hand::Type _hand) const
{
	return vrOffsets[_hand];
}

void MainConfig::set_vr_offsets(VROffset const& _leftHandVROffset, VROffset const& _rightHandVROffset)
{
	vrOffsets[Hand::Left] = _leftHandVROffset;
	vrOffsets[Hand::Right] = _rightHandVROffset;
	VR::Offsets::apply_custom_offsets();
}

//

Transform MainConfig::VROffset::to_transform() const
{
	return Transform(Vector3::zero, orientation.to_quat());
}
