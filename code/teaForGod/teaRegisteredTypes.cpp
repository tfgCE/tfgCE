#include "teaRegisteredTypes.h"

#define AN_REGISTER_TYPES_OFFSET 3000

#include "..\core\other\registeredTypeRegistering.h"

#include "ai\aiPerceptionLock.h"
#include "ai\aiPerceptionPause.h"
#include "game\damage.h"
#include "game\energy.h"
#include "game\weaponSetup.h"

#include "..\framework\library\library.h"
#include "..\framework\library\usedLibraryStored.inl"

using namespace TeaForGodEmperor;

// defaults
Random::Number<Energy> randomNumberEnergy;
Random::Number<EnergyCoef> randomNumberEnergyCoef;
WeaponSetup emptyWeaponSetup(nullptr);

//

// should be mirrored in headers with DECLARE_REGISTERED_TYPE
DEFINE_TYPE_LOOK_UP_OBJECT(AI::PerceptionLock, &AI::PerceptionLock::empty());
DEFINE_TYPE_LOOK_UP_OBJECT(AI::PerceptionPause, &AI::PerceptionPause::empty());
DEFINE_TYPE_LOOK_UP_OBJECT(DamageInfo, nullptr);
DEFINE_TYPE_LOOK_UP_OBJECT(Random::Number<Energy>, &randomNumberEnergy);
DEFINE_TYPE_LOOK_UP_OBJECT(Random::Number<EnergyCoef>, &randomNumberEnergyCoef);
DEFINE_TYPE_LOOK_UP_OBJECT(WeaponSetup, &emptyWeaponSetup);
DEFINE_TYPE_LOOK_UP_PTR(AI::PerceptionLock);
DEFINE_TYPE_LOOK_UP_PTR(AI::PerceptionPause);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Damage, &Damage::zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Energy, &Energy::s_zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(EnergyCoef, &EnergyCoef::s_zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(EnergyRange, &EnergyRange::empty);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(EnergyCoefRange, &EnergyCoefRange::empty);

//

void ::TeaForGodEmperor::RegisteredTypes::initialise_static()
{
}

void ::TeaForGodEmperor::RegisteredTypes::close_static()
{
}

//
//
// parsing

void parse_value__energy(String const & _from, void * _into)
{
	CAST_TO_VALUE(Energy, _into) = Energy::parse_from(_from, Energy::zero());
}
ADD_SPECIALISED_PARSE_VALUE(Energy, parse_value__energy);

void parse_value__energy_coef(String const & _from, void * _into)
{
	CAST_TO_VALUE(EnergyCoef, _into) = EnergyCoef::parse_from(_from, EnergyCoef::zero());
}
ADD_SPECIALISED_PARSE_VALUE(EnergyCoef, parse_value__energy_coef);

void parse_value__energy_range(String const & _from, void * _into)
{
	CAST_TO_VALUE(EnergyRange, _into) = EnergyRange::parse_from(_from, EnergyRange::empty);
}
ADD_SPECIALISED_PARSE_VALUE(EnergyRange, parse_value__energy_range);

void parse_value__energy_coef_range(String const & _from, void * _into)
{
	CAST_TO_VALUE(EnergyCoefRange, _into) = EnergyCoefRange::parse_from(_from, EnergyCoefRange::empty);
}
ADD_SPECIALISED_PARSE_VALUE(EnergyCoefRange, parse_value__energy_coef_range);

//
//
// saving to xml

bool save_to_xml__energy(IO::XML::Node * _node, void const * value)
{
	_node->add_text(CAST_TO_VALUE(Energy, value).as_string_auto_decimals());
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(Energy, save_to_xml__energy);

bool save_to_xml__energy_coef(IO::XML::Node * _node, void const * value)
{
	_node->add_text(CAST_TO_VALUE(EnergyCoef, value).as_string_percentage());
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(EnergyCoef, save_to_xml__energy_coef);

bool save_to_xml__energy_range(IO::XML::Node * _node, void const * value)
{
	_node->add_text(CAST_TO_VALUE(EnergyRange, value).as_string());
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(EnergyRange, save_to_xml__energy_range);

bool save_to_xml__energy_coef_range(IO::XML::Node * _node, void const * value)
{
	_node->add_text(CAST_TO_VALUE(EnergyCoefRange, value).as_string());
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(EnergyCoefRange, save_to_xml__energy_coef_range);

ADD_SPECIALISED_SAVE_TO_XML_USING(WeaponSetup, save_to_xml);

//
//
// loading from xml

bool load_from_xml__energy(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(Energy, _into) = Energy::parse_from(_node->get_internal_text(), Energy::zero());
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(Energy, load_from_xml__energy);

bool load_from_xml__energy_coef(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(EnergyCoef, _into) = EnergyCoef::parse_from(_node->get_internal_text(), EnergyCoef::zero());
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(EnergyCoef, load_from_xml__energy_coef);

bool load_from_xml__energy_range(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(EnergyRange, _into) = EnergyRange::parse_from(_node->get_internal_text(), EnergyRange::empty);
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(EnergyRange, load_from_xml__energy_range);

bool load_from_xml__energy_coef_range(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(EnergyCoefRange, _into) = EnergyCoefRange::parse_from(_node->get_internal_text(), EnergyCoefRange::empty);
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(EnergyCoefRange, load_from_xml__energy_coef_range);

ADD_SPECIALISED_LOAD_FROM_XML_USING(WeaponSetup, load_from_xml);

//
//
// logging

void log_value__energy(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [Energy] : %S"), _name, CAST_TO_VALUE(Energy, _value).format(String(TXT("%.3f"))).to_char());
	}
	else
	{
		_log.log(TXT("[Energy] : %S"), CAST_TO_VALUE(Energy, _value).format(String(TXT("%.3f"))).to_char());
	}
}
ADD_SPECIALISED_LOG_VALUE(Energy, log_value__energy);

void log_value__energy_coef(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [EnergyCoef] : %S"), _name, CAST_TO_VALUE(EnergyCoef, _value).as_string_percentage(2).to_char());
	}
	else
	{
		_log.log(TXT("[EnergyCoef] : %S"), CAST_TO_VALUE(EnergyCoef, _value).as_string_percentage(2).to_char());
	}
}
ADD_SPECIALISED_LOG_VALUE(EnergyCoef, log_value__energy_coef);

void log_value__energy_range(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [EnergyRange] : %S"), _name, CAST_TO_VALUE(EnergyRange, _value).as_string().to_char());
	}
	else
	{
		_log.log(TXT("[EnergyRange] : %S"), CAST_TO_VALUE(EnergyRange, _value).as_string().to_char());
	}
}
ADD_SPECIALISED_LOG_VALUE(EnergyRange, log_value__energy_range);

void log_value__energy_coef_range(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [EnergyCoefRange] : %S"), _name, CAST_TO_VALUE(EnergyCoefRange, _value).as_string().to_char());
	}
	else
	{
		_log.log(TXT("[EnergyCoefRange] : %S"), CAST_TO_VALUE(EnergyCoefRange, _value).as_string().to_char());
	}
}
ADD_SPECIALISED_LOG_VALUE(EnergyCoefRange, log_value__energy_coef_range);

ADD_SPECIALISED_LOG_VALUE_USING_LOG(TeaForGodEmperor::AI::PerceptionLock);
ADD_SPECIALISED_LOG_VALUE_USING_LOG(TeaForGodEmperor::AI::PerceptionPause);
