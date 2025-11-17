#include "buildStatsInfo.h"

#include "..\game\energy.h"

#include "..\..\framework\text\localisedString.h"

//

using namespace TeaForGodEmperor;

#define VALUE_COLUMN_WIDTH 0
#define use_as_string as_string

void BuildStatsInfo::add_info(REF_ String& _string, Name const& _locStr, Context const& _context)
{
	String prefix;
	while (prefix.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH))
	{
		prefix += String::space();
	}
	_string += prefix + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_info(REF_ String& _string, String const& _value, Context const& _context)
{
	String prefix;
	while (prefix.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH))
	{
		prefix += String::space();
	}
	_string += prefix + _value + TXT("~");
}

void BuildStatsInfo::add_info(REF_ String& _string, Name const& _locStr, String const& _value, Context const& _context)
{
	String prefix;
	while (prefix.get_length() + _value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH))
	{
		prefix += String::space();
	}
	_string += prefix + _value + (_value.is_empty() ? TXT("") : TXT(" ")) + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_info(REF_ String& _string, Name const& _locStr, String const& _value, String const& _penalty, Context const& _context)
{
	String value = _value + _penalty;
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH))
	{
		prefix += String::space();
	}
	_string += prefix + value + (value.is_empty() ? TXT("") : TXT(" ")) + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_info(REF_ String& _string, Name const& _locStr, Optional<bool> const& _value, Context const& _context)
{
	if (_value.get(false))
	{
		add_info(_string, _locStr, _context);
	}
}

void BuildStatsInfo::add_energy_info_as_is(REF_ String& _string, Name const& _locStr, Energy const& _energy, Context const& _context)
{
	if (_energy.is_zero())
	{
		return;
	}
	return add_energy_info_as_is(_string, _locStr, Optional<Energy>(_energy), _context);
}

void BuildStatsInfo::add_energy_info_as_is(REF_ String& _string, Name const& _locStr, Optional<Energy> const& _energy, Context const& _context)
{
	if (! _energy.is_set())
	{
		return;
	}
	String value = _energy.get().use_as_string();
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH))
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_energy_info_as_is(REF_ String& _string, Name const& _locStr, Energy const& _energy, Energy const& _penalty, Context const& _context)
{
	if (_energy.is_zero() && _penalty.is_zero())
	{
		return;
	}
	return add_energy_info_as_is(_string, _locStr, Optional<Energy>(_energy), Optional<Energy>(_penalty), _context);
}

void BuildStatsInfo::add_energy_info_as_is(REF_ String& _string, Name const& _locStr, Optional<Energy> const& _energy, Optional<Energy> const& _penalty, Context const& _context)
{
	if (! _energy.is_set() && ! _penalty.is_set())
	{
		return;
	}
	String value;
	if (_energy.is_set())
	{
		value += _energy.get().use_as_string();
		if (_penalty.is_set())
		{
			value += String::printf(TXT("%S%S"), _penalty.get().is_positive() ? TXT("+") : TXT(""), _penalty.get().use_as_string().to_char());
		}
	}
	else if (_penalty.is_set())
	{
		value = _penalty.get().use_as_string();
	}
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH))
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_energy_info(REF_ String& _string, Name const& _locStr, Energy const& _energy, Context const& _context)
{
	if (_energy.is_zero())
	{
		return;
	}
	return add_energy_info(_string, _locStr, Optional<Energy>(_energy), _context);
}

void BuildStatsInfo::add_energy_info(REF_ String& _string, Name const& _locStr, Optional<Energy> const& _energy, Context const& _context)
{
	if (! _energy.is_set())
	{
		return;
	}
	String value = String::printf(TXT("%S%S"), _energy.get().is_positive() ? TXT("+") : TXT(""), _energy.get().use_as_string().to_char());
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH))
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_energy_info(REF_ String& _string, Name const& _locStr, Energy const& _energy, Energy const & _penalty, Context const& _context)
{
	if (_energy.is_zero() && _penalty.is_zero())
	{
		return;
	}
	return add_energy_info(_string, _locStr, Optional<Energy>(_energy), Optional<Energy>(_penalty), _context);
}

void BuildStatsInfo::add_energy_info(REF_ String& _string, Name const& _locStr, Optional<Energy> const& _energy, Optional<Energy> const& _penalty, Context const& _context)
{
	if (! _energy.is_set() && ! _penalty.is_set())
	{
		return;
	}
	String value;
	if (_energy.is_set())
	{
		value += String::printf(TXT("%S%S"), _energy.get().is_positive() ? TXT("+") : TXT(""), _energy.get().use_as_string().to_char());
	}
	if (_penalty.is_set())
	{
		value += String::printf(TXT("%S%S"), _penalty.get().is_positive() ? TXT("+") : TXT(""), _penalty.get().use_as_string().to_char());
	}
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH))
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_energy_coef_info(REF_ String& _string, Name const& _locStr, EnergyCoef const& _energyCoef, Context const& _context)
{
	if (_energyCoef.is_zero())
	{
		return;
	}
	return add_energy_coef_info(_string, _locStr, Optional<EnergyCoef>(_energyCoef), _context);
}

void BuildStatsInfo::add_energy_coef_info(REF_ String& _string, Name const& _locStr, Optional<EnergyCoef> const& _energyCoef, Context const& _context)
{
	if (! _energyCoef.is_set() || _energyCoef.get().is_zero())
	{
		return;
	}
	String value = String::printf(TXT("%S%i"), _energyCoef.get().is_positive() ? TXT("+") : TXT(""), _energyCoef.get().as_int_percentage());
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT("% ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_energy_coef_info(REF_ String& _string, Name const& _locStr, EnergyCoef const& _energyCoef, EnergyCoef const & _penalty, Context const& _context)
{
	if (_energyCoef.is_zero() && _penalty.is_zero())
	{
		return;
	}
	return add_energy_coef_info(_string, _locStr, Optional<EnergyCoef>(_energyCoef), Optional<EnergyCoef>(_penalty), _context);
}

void BuildStatsInfo::add_energy_coef_info(REF_ String& _string, Name const& _locStr, Optional<EnergyCoef> const& _energyCoef, Optional<EnergyCoef> const& _penalty, Context const& _context)
{
	if ((! _energyCoef.is_set() || _energyCoef.get().is_zero()) &&
		(!_penalty.is_set() || _penalty.get().is_zero()))
	{
		return;
	}
	String value;
	if (_energyCoef.is_set() && ! _energyCoef.get().is_zero())
	{
		value += String::printf(TXT("%S%i"), _energyCoef.get().is_positive() ? TXT("+") : TXT(""), _energyCoef.get().as_int_percentage());
		value += TXT("%");
	}
	if (_penalty.is_set() && !_penalty.get().is_zero())
	{
		value += String::printf(TXT("%S%i"), _penalty.get().is_positive() ? TXT("+") : TXT(""), _penalty.get().as_int_percentage());
		value += TXT("%");
	}
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_energy_coef_info_as_is(REF_ String& _string, Name const& _locStr, Optional<EnergyCoef> const& _energyCoef, Context const& _context)
{
	if (!_energyCoef.is_set() || _energyCoef.get().is_zero())
	{
		return;
	}
	String value = String::printf(TXT("%i"), _energyCoef.get().as_int_percentage());
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT("% ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_float_info_as_is(REF_ String& _string, Name const& _locStr, float const& _value, Context const& _context)
{
	if (_value == 0.0f)
	{
		return;
	}
	return add_float_info_as_is(_string, _locStr, Optional<float>(_value), _context);
}

void BuildStatsInfo::add_float_info_as_is(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Context const& _context)
{
	if (! _value.is_set())
	{
		return;
	}
	String value = String::printf(TXT("%.2f"), _value.get());
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_float_info_as_is(REF_ String& _string, Name const& _locStr, float const& _value, float const& _penalty, Context const& _context)
{
	if (_value == 0.0f && _penalty == 0.0f)
	{
		return;
	}
	return add_float_info_as_is(_string, _locStr, Optional<float>(_value), Optional<float>(_penalty), _context);
}

void BuildStatsInfo::add_float_info_as_is(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Optional<float> const& _penalty, Context const& _context)
{
	if (!_value.is_set() && ! _penalty.is_set())
	{
		return;
	}
	String value;
	if (_value.is_set())
	{
		value += String::printf(TXT("%.2f"), _value.get());
		if (_penalty.is_set())
		{
			value += String::printf(TXT("%S%.2f"), _penalty.get() >= 0.0f? TXT("+") : TXT(""), _penalty.get());
		}
	}
	else if (_penalty.is_set())
	{
		value += String::printf(TXT("%.2f"), _penalty.get());
	}
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_float_info(REF_ String& _string, Name const& _locStr, float const& _value, Context const& _context)
{
	if (_value == 0.0f)
	{
		return;
	}
	return add_float_info(_string, _locStr, Optional<float>(_value), _context);
}

void BuildStatsInfo::add_float_info(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Context const& _context)
{
	if (! _value.is_set())
	{
		return;
	}
	String value = String::printf(TXT("%S%.2f"), _value.is_set() && _value.get()  >= 0.0f? TXT("+") : TXT(""), _value.get());
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_float_info(REF_ String& _string, Name const& _locStr, float const& _value, float const & _penalty, Context const& _context)
{
	if (_value == 0.0f && _penalty == 0.0f)
	{
		return;
	}
	return add_float_info(_string, _locStr, Optional<float>(_value), Optional<float>(_penalty), _context);
}

void BuildStatsInfo::add_float_info(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Optional<float> const & _penalty, Context const& _context)
{
	if (! _value.is_set() && !_penalty.is_set())
	{
		return;
	}
	String value;
	if (_value.is_set())
	{
		value += String::printf(TXT("%S%.0f"), _value.is_set() && _value.get() >= 0.0f ? TXT("+") : TXT(""), _value.get() * 100.0f);
	}
	if (_penalty.is_set())
	{
		value += String::printf(TXT("%S%.0f"), _penalty.is_set() && _penalty.get() >= 0.0f ? TXT("+") : TXT(""), _penalty.get() * 100.0f);
	}
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_float_coef_info(REF_ String& _string, Name const& _locStr, float const& _coef, Context const& _context)
{
	if (_coef == 0.0f)
	{
		return;
	}
	return add_float_coef_info(_string, _locStr, Optional<float>(_coef), _context);
}

void BuildStatsInfo::add_float_coef_info(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Context const& _context)
{
	if (! _value.is_set() || _value.get() == 0.0f)
	{
		return;
	}
	String value = String::printf(TXT("%S%.0f"), _value.is_set() && _value.get() >= 0.0f ? TXT("+") : TXT(""), _value.get() * 100.0f);
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT("% ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_float_coef_info(REF_ String& _string, Name const& _locStr, float const& _coef, float const & _penalty, Context const& _context)
{
	if (_coef == 0.0f && _penalty == 0.0f)
	{
		return;
	}
	return add_float_coef_info(_string, _locStr, Optional<float>(_coef), Optional<float>(_penalty), _context);
}

void BuildStatsInfo::add_float_coef_info(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Optional<float> const & _penalty, Context const& _context)
{
	if ((!_value.is_set() || _value.get() == 0.0f) &&
		(!_penalty.is_set() || _penalty.get() == 0.0f))
	{
		return;
	}
	String value;
	if (!_value.is_set() || _value.get() == 0.0f)
	{
		value += String::printf(TXT("%S%.0f"), _value.is_set() && _value.get() >= 0.0f ? TXT("+") : TXT(""), _value.get() * 100.0f);
	}
	if (!_penalty.is_set() || _penalty.get() == 0.0f)
	{
		value += String::printf(TXT("%S%.0f"), _penalty.is_set() && _penalty.get() >= 0.0f ? TXT("+") : TXT(""), _penalty.get() * 100.0f);
	}
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT("% ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_float_coef_info_as_is(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Context const& _context)
{
	if (!_value.is_set() || _value.get() == 0.0f)
	{
		return;
	}
	String value = String::printf(TXT("%.0f"), _value.get() * 100.0f);
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT("% ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::add_int_as_is(REF_ String& _string, Name const& _locStr, int const& _value, Context const& _context)
{
	if (_value == 0)
	{
		return;
	}

	add_int_as_is(_string, _locStr, Optional<int>(_value), _context);
}

void BuildStatsInfo::add_int_as_is(REF_ String& _string, Name const& _locStr, Optional<int> const& _value, Context const& _context)
{
	if (!_value.is_set())
	{
		return;
	}
	String value = String::printf(TXT("%i"), _value.get());
	String prefix;
	while (prefix.get_length() + value.get_length() < _context.valueColumnWidth.get(VALUE_COLUMN_WIDTH) - 1)
	{
		prefix += String::space();
	}
	_string += prefix + value + TXT(" ") + LOC_STR(_locStr) + TXT("~");
}

void BuildStatsInfo::remove_trailing_end_lines(REF_ String& _string)
{
	while (!_string.is_empty() && _string.get_data()[_string.get_length() - 1] == '~')
	{
		_string.cut_right_inline(1);
	}
}
