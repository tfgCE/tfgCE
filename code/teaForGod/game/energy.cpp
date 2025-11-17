#include "energy.h"

#include "..\..\core\other\parserUtils.h"
#include "..\..\core\random\random.h"
#include "..\..\framework\module\module.h"
#include "..\..\framework\module\moduleData.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(energy);

//

Energy Energy::s_zero = Energy(0);
Energy Energy::s_one = Energy(1);

Energy::Energy(float _f)
{
	energy = (int)(_f * (float)factor);
}

Energy Energy::get_random(Random::Generator & _generator, Energy const & _min, Energy const & _max)
{
	return pure(_generator.get_int_from_range(_min.energy, _max.energy));
}

bool Energy::load_from_xml(IO::XML::Attribute const* _attr)
{
	if (_attr && !_attr->get_as_string().is_empty())
	{
		*this = parse(_attr->get_as_string());
	}
	return true;
}

Energy Energy::load_from_attribute_or_from_child(IO::XML::Node const* _node, tchar const* _name, Energy const& _default)
{
	String val = _node->get_string_attribute_or_from_child(_name);
	if (!val.is_empty())
	{
		return parse(val);
	}
	else
	{
		return _default;
	}
}

bool Energy::load_from_attribute_or_from_child(IO::XML::Node const* _node, tchar const* _name)
{
	String val = _node->get_string_attribute_or_from_child(_name);
	if (!val.is_empty())
	{
		*this = parse(val);
		return true;
	}
	return true;
}

bool Energy::load_from_xml(IO::XML::Node const * _node, tchar const * _attr)
{
	if (_node)
	{
		if (_attr)
		{
			if (IO::XML::Attribute const * attr = _node->get_attribute(_attr))
			{
				if (!attr->get_as_string().is_empty())
				{
					*this = parse(attr->get_as_string());
				}
				return true;
			}
			else
			{
				return true;
			}
		}
		else
		{
			*this = parse(_node->get_text());
			return true;
		}
	}
	return true;
}

Energy Energy::parse(String const & txt)
{
	bool isNegative = txt.find_first_of('-') != NONE;
	int dotAt = txt.find_first_of('.');
	if (dotAt >= 0)
	{
		String decimalString = txt.get_sub(dotAt + 1, decimals);
		while (decimalString.get_length() < decimals)
		{
			decimalString += TXT("0");
		}
		return isNegative? negative(ParserUtils::parse_int(txt.get_left(dotAt)), ParserUtils::parse_int(decimalString))
						 : positive(ParserUtils::parse_int(txt.get_left(dotAt)), ParserUtils::parse_int(decimalString));
	}
	else
	{
		return Energy(ParserUtils::parse_int(txt));
	}
}

Energy Energy::get_from_module_data(Framework::Module * _module, Framework::ModuleData const * _moduleData, Name const & _name, Energy const & _default)
{
	if (_module)
	{
		if (Energy const * value = _module->get_owner()->get_variables().get_existing<Energy>(_name))
		{
			return *value;
		}
		if (float const * value = _module->get_owner()->get_variables().get_existing<float>(_name))
		{
			return Energy(*value);
		}
		if (int const * value = _module->get_owner()->get_variables().get_existing<int>(_name))
		{
			return Energy(*value);
		}
	}
	float defaultAsFloat = _default.as_float();
	float asFloat = _moduleData->get_parameter<float>(_module, _name, defaultAsFloat);
	if (asFloat != defaultAsFloat)
	{
		return Energy(asFloat);
	}
	else
	{
		return _default;
	}
}

bool Energy::can_get_from_module_data(Framework::Module * _module, Framework::ModuleData const * _moduleData, Name const & _name)
{
	if (_module)
	{
		if (_module->get_owner()->get_variables().get_existing<Energy>(_name))
		{
			return true;
		}
		if (_module->get_owner()->get_variables().get_existing<float>(_name))
		{
			return true;
		}
		if (_module->get_owner()->get_variables().get_existing<int>(_name))
		{
			return true;
		}
	}
	if (_moduleData->has_parameter<float>(_module, _name))
	{
		return true;
	}
	else
	{
		return false;
	}
}

Energy Energy::get_from_storage(SimpleVariableStorage const & _storage, Name const & _name, Energy const & _default)
{
	if (Energy const * value = _storage.get_existing<Energy>(_name))
	{
		return *value;
	}
	if (float const * value = _storage.get_existing<float>(_name))
	{
		return Energy(*value);
	}
	if (int const * value = _storage.get_existing<int>(_name))
	{
		return Energy(*value);
	}
	if (String const * value = _storage.get_existing<String>(_name))
	{
		return Energy::parse(*value);
	}
	{
		return _default;
	}
}

Energy Energy::timed(float _deltaTime, REF_ float & _missingBit) const
{
	float timedEnergyF = (float)energy * _deltaTime + _missingBit;
	int sign = timedEnergyF >= 0 ? 1 : -1;
	float signF = timedEnergyF >= 0 ? 1.0f : -1.0f;
	timedEnergyF = abs(timedEnergyF);
	int timedEnergy = (int)timedEnergyF;
	_missingBit = (timedEnergyF - (float)timedEnergy) * signF;
	return pure(timedEnergy * sign);
}

Energy Energy::adjusted(EnergyCoef const & _coef) const
{
	if (_coef.coef != 0 && energy > INT_MAX / _coef.coef) // overflow check
	{
		return pure(1000000); // should be enough
	}
	else
	{
		return pure((energy * _coef.coef) / _coef.factor);
	}
}

Energy Energy::adjusted_rev_one(EnergyCoef const & _coef) const
{
	if (_coef == EnergyCoef::one())
	{
		return pure(0);
	}
	an_assert(_coef.factor - _coef.coef != 0.0f);
	if (_coef.coef != 0 && energy > INT_MAX / (_coef.factor - _coef.coef)) // overflow check
	{
		warn(TXT("should not use such high coef, energy"));
		return pure(0);
	}
	else
	{
		return pure((energy * (_coef.factor - _coef.coef)) / _coef.factor);
	}
}

Energy Energy::adjusted_clamped(EnergyCoef const & _coef) const
{
	return adjusted(clamp(_coef, EnergyCoef::zero(), EnergyCoef::one()));
}

Energy Energy::adjusted_inv(EnergyCoef const & _coef) const
{
	if (_coef.coef != 0 && energy > INT_MAX / _coef.coef) // overflow check
	{
		return pure(0); // should be enough
	}
	else
	{
		return pure((energy * _coef.factor) / _coef.coef);
	}
}

Energy Energy::adjusted_plus_one(EnergyCoef const& _coef) const
{
	EnergyCoef actual = _coef;
	actual.coef += actual.factor;
	return adjusted(actual);
}

Energy Energy::adjusted_plus_one_inv(EnergyCoef const& _coef) const
{
	EnergyCoef actual = _coef;
	actual.coef += actual.factor;
	return adjusted_inv(actual);
}

Energy Energy::mul(float _coef) const
{
	if (_coef == 1.0f)
	{
		return *this;
	}
	if (_coef == -1.0f)
	{
		return pure(-energy);
	}
	return pure((int)((float)energy * _coef));
}

Energy Energy::mul_with_min(float _coef, Energy const& _minValue) const
{
	Energy result = mul(_coef);
	if (*this >= _minValue)
	{
		return max(result, _minValue);
	}
	else
	{
		return result;
	}
}

Energy Energy::div(float _coef) const
{
	if (_coef == 1.0f)
	{
		return *this;
	}
	if (_coef == -1.0f)
	{
		return pure(-energy);
	}
	return pure((int)((float)energy / _coef));
}

Energy Energy::div_safe(float _coef) const
{
	if (_coef == 0.0f)
	{
		return Energy::zero();
	}
	else
	{
		return div(_coef);
	}
}

String Energy::format(String const & _format) const
{
	an_assert(_format.get_left(1) == TXT("%"));
	an_assert(_format.get_right(1) == TXT("f"));
	int dotAt = _format.find_first_of('.');
	String bigFormat;
	String smallFormat;
	int decims;
	int decim = mod(abs(energy), factor);
	int whole = (abs(energy) - decim) / factor;
	if (dotAt != NONE)
	{
		bigFormat = _format.get_sub(1, dotAt - 1);
		smallFormat = _format.get_sub(dotAt + 1, _format.get_length() - 2 - dotAt);
		decims = ParserUtils::parse_int(_format.get_sub(dotAt + 1, _format.get_length() - dotAt - 1));
	}
	else
	{
		bigFormat = _format.get_sub(1, _format.get_length() - 2);
		decims = decimals;
	}
	if (decims > 0)
	{
		String result;
		String wholeFormat;
		wholeFormat += TXT("%");
		wholeFormat += bigFormat;
		wholeFormat += TXT("i");
		String decimFormat = String::printf(TXT("%%0%ii"), decims);
		int useDecims = clamp(decims, 0, decimals);
		if (useDecims > 0)
		{
			int divideBy = 1;
			for (int inc = useDecims; inc < decimals; ++inc)
			{
				divideBy *= 10;
			}
			decim /= divideBy;
			String format = wholeFormat + TXT(".") + decimFormat;
			return String::printf(format.to_char(),
				sign(energy) * whole,
				decim);
		}
	}
	String format;
	format += TXT("%");
	format += bigFormat;
	format += TXT("i");
	return String::printf(format.to_char(), sign(energy) * whole); // whole! we want to show less, floor values
}

String Energy::as_string(int _decimals) const
{
	tchar format[26];
	tsprintf(format, 25, TXT("%%.%if"), _decimals >= 0? _decimals : decimals);
	return String::printf(format, as_float());
}

String Energy::as_string_relative(int _decimals) const
{
	tchar format[26];
	tsprintf(format, 25, TXT("%c%%.%if"), is_negative()? '-' : '+',  _decimals >= 0? _decimals : decimals);
	return String::printf(format, abs(as_float()));
}

String Energy::as_string_auto_decimals() const
{
	if ((energy % factor) == 0)
	{
		return as_string(0);
	}
	else
	{
		return as_string(2);
	}
}

EnergyCoef Energy::as_part_of(Energy const& _other, bool _clampTo01) const
{
	int ecPure = EnergyCoef::factor * energy / _other.energy;
	EnergyCoef part = EnergyCoef::pure(ecPure);
	if (_clampTo01)
	{
		part = clamp(part, EnergyCoef::zero(), EnergyCoef::one());
	}
	return part;
}

EnergyCoef Energy::div_to_coef(Energy const& _other) const
{
	return _other.energy == 0 ? EnergyCoef::zero() : EnergyCoef::pure(energy * EnergyCoef::factor / _other.energy);
}

//

EnergyCoef EnergyCoef::s_zero(0, 0);
EnergyCoef EnergyCoef::s_half(0, factor * 5 / 10);
EnergyCoef EnergyCoef::s_one(1, 0);
EnergyCoef EnergyCoef::s_onePercent(0, factor * 1 / 100);

EnergyCoef::EnergyCoef(float _f)
{
	coef = (int)(_f * (float)factor);
}

EnergyCoef EnergyCoef::get_random(Random::Generator& _generator, EnergyCoef const& _min, EnergyCoef const& _max)
{
	return pure(_generator.get_int_from_range(_min.coef, _max.coef));
}

bool EnergyCoef::load_from_attribute_or_from_child(IO::XML::Node const* _node, tchar const* _name, bool _requiresPercentageSign)
{
	String val = _node->get_string_attribute_or_from_child(_name);
	if (!val.is_empty())
	{
		*this = parse(val, _requiresPercentageSign);
		return true;
	}
	return true;
}

bool EnergyCoef::load_from_xml(IO::XML::Attribute const* _attr)
{
	if (_attr && !_attr->get_as_string().is_empty())
	{
		*this = parse(_attr->get_as_string());
	}
	return true;
}

bool EnergyCoef::load_from_xml(IO::XML::Node const * _node, tchar const * _attr, bool _requiresPercentageSign)
{
	if (_attr)
	{
		if (IO::XML::Attribute const * attr = _node->get_attribute(_attr))
		{
			if (!attr->get_as_string().is_empty())
			{
				*this = parse(attr->get_as_string(), _requiresPercentageSign);
			}
			return true;
		}
		else
		{
			return true;
		}
	}
	else
	{
		*this = parse(_node->get_text(), _requiresPercentageSign);
		return true;
	}
}

EnergyCoef EnergyCoef::parse(String const & _txt, bool _requiresPercentageSign)
{
	String txt = _txt;
	if (_requiresPercentageSign)
	{
		if (! String::does_contain(txt.to_char(), TXT("%")))
		{
			warn(TXT("expecting \"%%\" character for energy coef"));
		}
	}
	bool isNegative = txt.find_first_of('-') != NONE;
	int dotAt = txt.find_first_of('.');
	EnergyCoef result;
	if (dotAt >= 0)
	{
		String decimalString = txt.get_sub(dotAt + 1, decimals);
		while (decimalString.get_length() < decimals)
		{
			decimalString += TXT("0");
		}
		result = isNegative? negative(ParserUtils::parse_int(txt.get_left(dotAt)), ParserUtils::parse_int(decimalString))
						   : positive(ParserUtils::parse_int(txt.get_left(dotAt)), ParserUtils::parse_int(decimalString));
	}
	else
	{
		result = EnergyCoef(ParserUtils::parse_int(txt));
	}
	if (String::does_contain(txt.to_char(), TXT("%")))
	{
		result = result / 100;
	}
	return result;
}

EnergyCoef EnergyCoef::load_from_attribute_or_from_child(IO::XML::Node const * _node, tchar const * _name, EnergyCoef const & _default, bool _requiresPercentageSign)
{
	String val = _node->get_string_attribute_or_from_child(_name);
	if (!val.is_empty())
	{
		return parse(val, _requiresPercentageSign);
	}
	else
	{
		return _default;
	}
}

EnergyCoef EnergyCoef::get_from_module_data(Framework::Module * _module, Framework::ModuleData const * _moduleData, Name const & _name, EnergyCoef const & _default, bool _requiresPercentageSign)
{
	if (_module)
	{
		if (EnergyCoef const* value = _module->get_owner()->get_variables().get_existing<EnergyCoef>(_name))
		{
			return *value;
		}
		if (float const* value = _module->get_owner()->get_variables().get_existing<float>(_name))
		{
			return EnergyCoef(*value);
		}
		if (int const* value = _module->get_owner()->get_variables().get_existing<int>(_name))
		{
			return EnergyCoef(*value);
		}
	}
	String toParse = _moduleData->get_parameter<String>(_module, _name, String::empty());
	if (!toParse.is_empty())
	{
		return parse(toParse, _requiresPercentageSign);
	}
	return _default;
}

EnergyCoef EnergyCoef::get_from_storage(SimpleVariableStorage const & _storage, Name const& _name, EnergyCoef const& _default)
{
	if (EnergyCoef const* value = _storage.get_existing<EnergyCoef>(_name))
	{
		return *value;
	}
	if (float const* value = _storage.get_existing<float>(_name))
	{
		return EnergyCoef(*value);
	}
	if (int const* value = _storage.get_existing<int>(_name))
	{
		return EnergyCoef(*value);
	}
	if (String const* value = _storage.get_existing<String>(_name))
	{
		return EnergyCoef::parse(*value);
	}
	{
		return _default;
	}
}

String EnergyCoef::as_string(int _decimals) const
{
	tchar format[26];
	tsprintf(format, 25, TXT("%%.%if"), _decimals >= 0 ? _decimals : decimals);
	return String::printf(format, as_float());
}

String EnergyCoef::as_string_auto_decimals() const
{
	int mul = 1;
	int dec = 0;
	while (dec < decimals)
	{
		if (((coef * mul) % (factor)) == 0)
		{
			return as_string(dec);
		}
		mul *= 10;
		++dec;
	}
	return as_string();
}

String EnergyCoef::as_string_percentage_auto_decimals() const
{
	int mul = 100;
	int dec = 0;
	while (dec < decimals - 2)
	{
		if (((coef * mul) % (factor)) == 0)
		{
			return as_string_percentage(dec);
		}
		mul *= 10;
		++dec;
	}
	return as_string_percentage();
}

String EnergyCoef::as_string_percentage(int _decimals) const
{
	if (_decimals > 0)
	{
		tchar format[26];
		tsprintf(format, 25, TXT("%%.%if%%%%"), _decimals);
		return String::printf(format, (as_float() * 100.0f));
	}
	else
	{
		return String::printf(TXT("%i%%"), as_int_percentage());

	}
}

String EnergyCoef::as_string_percentage_relative(int _decimals) const
{
	tchar sign = is_negative() ? '-' : '+';
	if (_decimals > 0)
	{
		tchar format[26];
		tsprintf(format, 25, TXT("%%c%%.%if%%%%"), _decimals);
		return String::printf(format, sign, abs(as_float() * 100.0f));
	}
	else
	{
		return String::printf(TXT("%c%i%%"), sign, abs(as_int_percentage()));

	}
}

//

EnergyRange EnergyRange::empty(Energy(9999), Energy(-9999));

Energy EnergyRange::get_random() const
{
	return Energy::pure(Random::get_int_from_range(min.get_pure(), max.get_pure()));
}

Energy EnergyRange::get_random(Random::Generator & rg) const
{
	return Energy::pure(rg.get_int_from_range(min.get_pure(), max.get_pure()));
}

EnergyRange EnergyRange::get_from_module_data(Framework::Module* _module, Framework::ModuleData const* _moduleData, Name const& _name, EnergyRange const& _default)
{
	if (_module)
	{
		if (EnergyRange const* value = _module->get_owner()->get_variables().get_existing<EnergyRange>(_name))
		{
			return *value;
		}
		if (Energy const* value = _module->get_owner()->get_variables().get_existing<Energy>(_name))
		{
			return EnergyRange(*value);
		}
		if (float const* value = _module->get_owner()->get_variables().get_existing<float>(_name))
		{
			return EnergyRange(Energy(*value));
		}
		if (int const* value = _module->get_owner()->get_variables().get_existing<int>(_name))
		{
			return EnergyRange(Energy(*value));
		}
	}
	String asString = _moduleData->get_parameter<String>(_module, _name, String::empty());
	if (! asString.is_empty())
	{

		return EnergyRange::parse(asString);
	}
	else
	{
		return _default;
	}
}

bool EnergyRange::can_get_from_module_data(Framework::Module* _module, Framework::ModuleData const* _moduleData, Name const& _name)
{
	if (_module)
	{
		if (_module->get_owner()->get_variables().get_existing<EnergyRange>(_name))
		{
			return true;
		}
		if (_module->get_owner()->get_variables().get_existing<Energy>(_name))
		{
			return true;
		}
		if (_module->get_owner()->get_variables().get_existing<float>(_name))
		{
			return true;
		}
		if (_module->get_owner()->get_variables().get_existing<int>(_name))
		{
			return true;
		}
	}
	if (_moduleData->has_parameter<float>(_module, _name))
	{
		return true;
	}
	else
	{
		return false;
	}
}

EnergyRange EnergyRange::get_from_storage(SimpleVariableStorage const& _storage, Name const& _name, EnergyRange const& _default)
{
	if (EnergyRange const* value = _storage.get_existing<EnergyRange>(_name))
	{
		return *value;
	}
	if (float const* value = _storage.get_existing<float>(_name))
	{
		return EnergyRange(Energy(*value));
	}
	if (int const* value = _storage.get_existing<int>(_name))
	{
		return EnergyRange(Energy(*value));
	}
	if (String const* value = _storage.get_existing<String>(_name))
	{
		return EnergyRange::parse(*value);
	}
	{
		return _default;
	}
}

EnergyRange EnergyRange::parse(String const& _txt)
{
	EnergyRange result = EnergyRange::empty;
	result.load_from_string(_txt);
	return result;
}

bool EnergyRange::load_from_xml(IO::XML::Node const * _node, tchar const * _attr)
{
	if (IO::XML::Attribute const * attr = _node->get_attribute(_attr))
	{
		return load_from_string(attr->get_as_string());
	}
	else
	{
		return true;
	}
}

bool EnergyRange::load_from_string(String const & _string)
{
	List<String> tokens;
	_string.split(String::comma(), tokens);
	if (tokens.get_size() == 1)
	{
		tokens.clear();
		_string.split(String(TXT("to")), tokens);
		if (tokens.get_size() == 1)
		{
			tokens.clear();
			_string.split(String(TXT("+-")), tokens);
			if (tokens.get_size() == 2)
			{
				List<String>::Iterator iToken0 = tokens.begin();
				List<String>::Iterator iToken1 = iToken0; ++iToken1;
				Energy base = Energy::parse(*iToken0);
				Energy off = Energy::parse(*iToken1);
				min = base - off;
				max = base + off;
				return true;
			}
		}
	}
	if (tokens.get_size() == 2)
	{
		List<String>::Iterator iToken0 = tokens.begin();
		List<String>::Iterator iToken1 = iToken0; ++iToken1;
		min = Energy::parse(*iToken0);
		max = Energy::parse(*iToken1);
		return true;
	}
	else
	{
		if (!_string.is_empty() && tokens.get_size() == 1)
		{
			// single value
			min = Energy::parse(_string);
			max = min;
			return true;
		}
	}
	return _string.is_empty();
}

String EnergyRange::as_string(int _decimals) const
{
	if (is_empty())
	{
		return String::empty();
	}
	else if (min == max)
	{
		return String::printf(TXT("%S"), min.as_string(_decimals).to_char());
	}
	else
	{
		return String::printf(TXT("%S to %S"), min.as_string(_decimals).to_char(), max.as_string(_decimals).to_char());
	}
}

//

EnergyCoefRange EnergyCoefRange::empty(EnergyCoef(9999), EnergyCoef(-9999));

EnergyCoef EnergyCoefRange::get_random() const
{
	return EnergyCoef::pure(Random::get_int_from_range(min.get_pure(), max.get_pure()));
}

EnergyCoef EnergyCoefRange::get_random(Random::Generator& rg) const
{
	return EnergyCoef::pure(rg.get_int_from_range(min.get_pure(), max.get_pure()));
}

bool EnergyCoefRange::load_from_xml(IO::XML::Node const * _node, tchar const * _attr)
{
	if (IO::XML::Attribute const * attr = _node->get_attribute(_attr))
	{
		return load_from_string(attr->get_as_string());
	}
	else
	{
		return true;
	}
}

bool EnergyCoefRange::load_from_string(String const & _string)
{
	List<String> tokens;
	_string.split(String::comma(), tokens);
	if (tokens.get_size() == 1)
	{
		tokens.clear();
		_string.split(String(TXT("to")), tokens);
		if (tokens.get_size() == 1)
		{
			tokens.clear();
			_string.split(String(TXT("+-")), tokens);
			if (tokens.get_size() == 2)
			{
				List<String>::Iterator iToken0 = tokens.begin();
				List<String>::Iterator iToken1 = iToken0; ++iToken1;
				EnergyCoef base = EnergyCoef::parse(*iToken0);
				EnergyCoef off = EnergyCoef::parse(*iToken1);
				min = base - off;
				max = base + off;
				return true;
			}
		}
	}
	if (tokens.get_size() == 2)
	{
		List<String>::Iterator iToken0 = tokens.begin();
		List<String>::Iterator iToken1 = iToken0; ++iToken1;
		min = EnergyCoef::parse(*iToken0);
		max = EnergyCoef::parse(*iToken1);
		return true;
	}
	else
	{
		if (!_string.is_empty() && tokens.get_size() == 1)
		{
			// single value
			min = EnergyCoef::parse(_string);
			max = min;
			return true;
		}
	}
	return _string.is_empty();
}

EnergyCoefRange EnergyCoefRange::parse(String const& _txt)
{
	EnergyCoefRange result = EnergyCoefRange::empty;
	result.load_from_string(_txt);
	return result;
}

String EnergyCoefRange::as_string(int _decimals) const
{
	if (is_empty())
	{
		return String::empty();
	}
	else if (min == max)
	{
		return String::printf(TXT("%S"), min.as_string(_decimals).to_char());
	}
	else
	{
		return String::printf(TXT("%S to %S"), min.as_string(_decimals).to_char(), max.as_string(_decimals).to_char());
	}
}

String EnergyCoefRange::as_string_percentage(int _decimals) const
{
	if (is_empty())
	{
		return String::empty();
	}
	else if (min == max)
	{
		return String::printf(TXT("%S"), min.as_string_percentage(_decimals).to_char());
	}
	else
	{
		return String::printf(TXT("%S to %S"), min.as_string_percentage(_decimals).to_char(), max.as_string_percentage(_decimals).to_char());
	}
}

