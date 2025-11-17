#include "parserUtils.h"
#include "../types/string.h"

#include "typeConversions.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

bool does_char_string_contains(tchar const * _string, tchar const * _contains)
{
	while (*_string != 0)
	{
		if (*_string == *_contains)
		{
			tchar const * ts = _string;
			tchar const * cs = _contains;
			while (*ts != 0 && *cs != 0)
			{
				if (*ts != *cs)
				{
					break;
				}
				++ts;
				++cs;
			}
			if (*cs == 0)
			{
				return true;
			}
		}
		++_string;
	}
	return false;
}

bool ParserUtils::parse_bool(tchar const * _value, bool _defValue)
{
	bool value = _defValue;
	ParserUtils::parse_bool_ref(_value, REF_ value);
	return value;
}

bool ParserUtils::parse_bool_ref(tchar const * _value, REF_ bool & _refValue)
{
	if (String::compare_icase(_value, TXT("true")) || String::compare_icase(_value, TXT("yes")))
	{
		_refValue = true;
		return true;
	}
	if (String::compare_icase(_value, TXT("false")) || String::compare_icase(_value, TXT("no")))
	{
		_refValue = false;
		return true;
	}
	return false;
}

bool ParserUtils::parse_bool_out(tchar const * _value, OUT_ bool & _outValue)
{
	if (String::compare_icase(_value, TXT("true")) || String::compare_icase(_value, TXT("yes")))
	{
		_outValue = true;
		return true;
	}
	if (String::compare_icase(_value, TXT("false")) || String::compare_icase(_value, TXT("no")))
	{
		_outValue = false;
		return true;
	}
	_outValue = false;
	return false;
}

float ParserUtils::parse_float(tchar const * _value, float _defValue)
{
	float value = _defValue;
	ParserUtils::parse_float_ref(_value, REF_ value);
	return value;
}

bool ParserUtils::parse_float_ref(tchar const * _value, REF_ float & _refValue)
{
	int length = (int)tstrlen(_value);
	if (length == 0)
	{
		return false;
	}
	if (does_char_string_contains(_value, TXT("-inf")) ||
		does_char_string_contains(_value, TXT("-infinity")))
	{
		_refValue = -INF_FLOAT;
		return true;
	}
	if (does_char_string_contains(_value, TXT("inf")) ||
		does_char_string_contains(_value, TXT("infinity")))
	{
		_refValue = INF_FLOAT;
		return true;
	}
	// check for *t* (eg. 0t32 2t8 to handle texturing, it will translate into 0.5 / 32.0, 2.5 / 8.0)
	// and for *p*t* (eg 1p8t32 -> 1+8t32 -> 9t32
	// and */* (divide)
	tchar const * ch = _value;
	allocate_stack_var(tchar, first, length + 1);
	for (int i = 0; i < length; ++i, ++ch)
	{
		if (*ch == '/' ||
			*ch == 't' ||
			*ch == 'E')
		{
			tsprintf(first, length + 1, _value);
			first[i] = 0;
			float uValue = (float)ttof(first);
			if (*ch == 't')
			{
				tchar *ch = first;
				while (*ch != 0)
				{
					if (*ch == 'p')
					{
						*ch = 0;
						uValue = (float)(ttof(first) + ttof(ch + 1));
						break;
					}
					++ch;
				}
			}
			float ofValue = (float)ttof(&_value[i + 1]);
			if (*ch == 't')
			{
				_refValue = (uValue + 0.5f) / ofValue;
			}
			else if (*ch == '/')
			{
				_refValue = uValue  / ofValue;
			}
			else if (*ch == 'E')
			{
				int e = TypeConversions::Normal::f_i_closest(ofValue);
				while (e > 0)
				{
					uValue *= 10.0f;
					--e;
				}
				while (e < 0)
				{
					uValue *= 0.1f;
					++e;
				}
			}
			else
			{
				error(TXT("what? why"));
				return false;
			}
			return true;
		}
	}
	_refValue = (float)ttof(_value);
	while (*_value != 0)
	{
		if (*_value == '%')
		{
			_refValue /= 100.0f;
			break;
		}
		++_value;
	}
	return true;
}

int ParserUtils::parse_int(tchar const * _value, int _defValue)
{
	int value = _defValue;
	ParserUtils::parse_int_ref(_value, REF_ value);
	return value;
}

bool ParserUtils::parse_int_ref(tchar const * _value, REF_ int & _refValue)
{
	int len = (int)tstrlen(_value);
	if (len == 0)
	{
		return false;
	}
	if (does_char_string_contains(_value, TXT("-*")) ||
		does_char_string_contains(_value, TXT("-inf")) ||
		does_char_string_contains(_value, TXT("-infinite")) ||
		does_char_string_contains(_value, TXT("-infinity")))
	{
		_refValue = -INF_INT;
		return true;
	}
	if (does_char_string_contains(_value, TXT("*")) ||
		does_char_string_contains(_value, TXT("inf")) ||
		does_char_string_contains(_value, TXT("infinite")) ||
		does_char_string_contains(_value, TXT("infinity")))
	{
		_refValue = INF_INT;
		return true;
	}
	int found = 0;
	tchar const * ch = _value;
	while (found < len)
	{
		if (*ch == '*')
		{
			String value = String(_value);
			value.replace(String::space(), String(TXT("")));
			if (value == TXT("*"))
			{
				_refValue = INF_INT;
				return true;
			}
			if (value == TXT("-*"))
			{
				_refValue = -INF_INT;
				return true;
			}
		}
		++found;
		++ch;
	}
	_refValue = ttoi(_value);
	return true;
}

int ParserUtils::parse_int(String const & _value, int _defValue)
{
	int value = _defValue;
	ParserUtils::parse_int_ref(_value, REF_ value);
	return value;
}

int64 ParserUtils::parse_int64(tchar const * _value, int64 _defValue)
{
	int64 value = _defValue;
	ParserUtils::parse_int64_ref(_value, REF_ value);
	return value;
}

bool ParserUtils::parse_int64_ref(tchar const * _value, REF_ int64 & _refValue)
{
	int64 len = tstrlen(_value);
	if (len == 0)
	{
		return false;
	}
	if (does_char_string_contains(_value, TXT("-*")) ||
		does_char_string_contains(_value, TXT("-inf")) ||
		does_char_string_contains(_value, TXT("-infinite")) ||
		does_char_string_contains(_value, TXT("-infinity")))
	{
		_refValue = -INF_INT;
		return true;
	}
	if (does_char_string_contains(_value, TXT("*")) ||
		does_char_string_contains(_value, TXT("inf")) ||
		does_char_string_contains(_value, TXT("infinite")) ||
		does_char_string_contains(_value, TXT("infinity")))
	{
		_refValue = INF_INT;
		return true;
	}
	{
		int64 at = 0;
		tchar const * ch = _value;
		while (at < len)
		{
			if (*ch == '*')
			{
				String value = String(_value);
				value.replace(String::space(), String(TXT("")));
				if (value == TXT("*"))
				{
					_refValue = INF_INT;
					return true;
				}
				if (value == TXT("-*"))
				{
					_refValue = -INF_INT;
					return true;
				}
			}
			++at;
			++ch;
		}
	}
	int64 sign = 1;
	int64 result = 0;
	{
		int at = 0;
		tchar const * ch = _value;
		if (_value[at] == '-')
		{
			++at;
			++ch;
			sign = -1;
		}
		while (at < len)
		{
			if (*ch >= '0' && *ch <= '9')
			{
				result = result * 10 + *ch - '0';
			}
			else
			{
				return false;
			}
			++at;
			++ch;
		}
	}
	_refValue = result * sign;
	return true;
}

int64 ParserUtils::parse_int64(String const & _value, int64 _defValue)
{
	int64 value = _defValue;
	ParserUtils::parse_int64_ref(_value.to_char(), REF_ value);
	return value;
}

uint ParserUtils::parse_hex(tchar const * _value, uint _defValue)
{
	uint value = _defValue;
	ParserUtils::parse_hex_ref(_value, REF_ value);
	return value;
}

bool ParserUtils::parse_hex_ref(tchar const * _value, REF_ uint & _refValue)
{
	int len = (int)tstrlen(_value);
	if (len == 0)
	{
		return false;
	}
	uint read = 0;
	tchar const * ch = _value;
	while (*ch != 0)
	{
		int chInt = 0;
		if (*ch >= '0' && *ch <= '9')
		{
			chInt += *ch - '0';
		}
		else if (*ch >= 'a' && *ch <= 'f')
		{
			chInt += *ch + 10 - 'a';
		}
		else if (*ch >= 'A' && *ch <= 'F')
		{
			chInt += *ch + 10 - 'A';
		}
		else
		{
			return false;
		}
		read = read * 16 + chInt;
		++ch;
	}
	_refValue = read;
	return true;
}

bool ParserUtils::char_is_valid_word_letter(tchar _ch)
{
	return (_ch >= 'A' && _ch <= 'Z') || (_ch >= 'a' && _ch <= 'z') || _ch == '_';
}

bool ParserUtils::char_is_valid_word_letter(char8 _ch)
{
	return (_ch >= 'A' && _ch <= 'Z') || (_ch >= 'a' && _ch <= 'z') || _ch == '_';
}

bool ParserUtils::char_is_number(tchar _ch)
{
	return (_ch >= '0' && _ch <= '9');
}

bool ParserUtils::char_is_number(char8 _ch)
{
	return (_ch >= '0' && _ch <= '9');
}

bool ParserUtils::is_empty_char(tchar const _ch)
{
	return _ch <= 32 || _ch == String::no_break_space_char();
}

bool ParserUtils::is_empty_char(char8 const _ch)
{
	return _ch <= 32;
}

//

bool ParserUtils::parse_bool(String const & _value, bool _defValue)
{
	return ParserUtils::parse_bool(_value.to_char(), _defValue);
}

bool ParserUtils::parse_bool_ref(String const & _value, REF_ bool & _refValue)
{
	return ParserUtils::parse_bool_ref(_value.to_char(), _refValue);
}

bool ParserUtils::parse_bool_out(String const & _value, OUT_ bool & _outValue)
{
	return ParserUtils::parse_bool_out(_value.to_char(), _outValue);
}

float ParserUtils::parse_float(String const & _value, float _defValue)
{
	return ParserUtils::parse_float(_value.to_char(), _defValue);
}

bool ParserUtils::parse_float_ref(String const & _value, REF_ float & _refValue)
{
	return ParserUtils::parse_float_ref(_value.to_char(), _refValue);
}

bool ParserUtils::parse_int_ref(String const & _value, REF_ int & _refValue)
{
	return ParserUtils::parse_int_ref(_value.to_char(), _refValue);
}

String ParserUtils::single_to_hex(int _value)
{
	an_assert(_value >= 0 && _value < 16);
	int cur = _value;
	if (cur >= 0 && cur <= 9)
	{
		return String::single('0' + cur);
	}
	else if (cur >= 10 && cur <= 15)
	{
		return String::single('a' + cur - 10);
	}
	return String(TXT("?"));
}

String ParserUtils::void_to_hex(void* _value)
{
#ifdef AN_64
	return uint64_to_hex((uint64)_value);
#else
	return int_to_hex((int)_value);
#endif
}

String ParserUtils::int_to_hex(int _value)
{
	String result;
	if (_value < 0)
	{
		return uint_to_hex((uint)_value);
	}
	for_count(int, i, 2 * sizeof(_value))
	{
		int cur = _value & 0x0f;
		an_assert(cur >= 0 && cur <= 15);
		tchar ch = '0';
		if (cur >= 0 && cur <= 9)
		{
			ch = '0' + cur;
		}
		else if (cur >= 10 && cur <= 15)
		{
			ch = 'a' + cur - 10;
		}
		result = String::single(ch) + result;
		_value = _value >> 4;
	}
	return result;
}

String ParserUtils::uint_to_hex(uint _value)
{
	String result;
	for_count(int, i, 2 * sizeof(_value))
	{
		uint cur = _value & 0x0f;
		an_assert(cur >= 0 && cur <= 15);
		tchar ch = '0';
		if (/*cur >= 0 &&*/cur <= 9)
		{
			ch = '0' + cur;
		}
		else if (cur >= 10 && cur <= 15)
		{
			ch = 'a' + cur - 10;
		}
		result = String::single(ch) + result;
		_value = _value >> 4;
	}
	return result;
}

String ParserUtils::uint64_to_hex(uint64 _value)
{
	String result;
	for_count(int, i, 2 * sizeof(_value))
	{
		uint cur = _value & 0x0f;
		an_assert(cur >= 0 && cur <= 15);
		tchar ch = '0';
		if (/*cur >= 0 && */cur <= 9)
		{
			ch = '0' + cur;
		}
		else if (cur >= 10 && cur <= 15)
		{
			ch = 'a' + cur - 10;
		}
		result = String::single(ch) + result;
		_value = _value >> 4;
	}
	return result;
}

int ParserUtils::hex_to_int(String const & _hex)
{
	int result = 0;
	for_every(ch, _hex.get_data())
	{
		if (*ch == 0)
		{
			break;
		}
		int cur = *ch - '0';
		if (*ch >= 'a' && *ch <= 'f')
		{
			cur = *ch - 'a' + 10;
		}
		if (*ch >= 'A' && *ch <= 'F')
		{
			cur = *ch - 'A' + 10;
		}
		if (cur < 0 || cur > 15)
		{
			cur = 0;
		}
		result = (result << 4) | cur;
	}
	return result;
}

uint ParserUtils::hex_to_uint(String const & _hex)
{
	uint result = 0;
	for_every(ch, _hex.get_data())
	{
		if (*ch == 0)
		{
			break;
		}
		uint cur = *ch - '0';
		if (*ch >= 'a' && *ch <= 'f')
		{
			cur = *ch - 'a' + 10;
		}
		if (*ch >= 'A' && *ch <= 'F')
		{
			cur = *ch - 'A' + 10;
		}
		if (/*cur < 0 ||*/cur > 15)
		{
			cur = 0;
		}
		result = (result << 4) | cur;
	}
	return result;
}

int ParserUtils::bin_to_int(String const& _bin)
{
	int result = 0;
	for_every(ch, _bin.get_data())
	{
		if (*ch == 0)
		{
			break;
		}
		int cur = *ch == '1'? 1 : 0;
		result = (result << 1) | cur;
	}
	return result;
}

String ParserUtils::int_to_bin(int _value)
{
	String result;
	uint v = (uint)_value;
	for_count(int, i, 2 * sizeof(v))
	{
		int cur = v & 0x01;
		an_assert(cur >= 0 && cur <= 1);
		tchar ch = cur? '1' : '0';
		result = String::single(ch) + result;
		v = v >> 1;
		if (v == 0)
		{
			break;
		}
	}
	return result;
}

String ParserUtils::float_to_string_auto_decimals(float _value, int _limit)
{
	int decimals = 0;
	float roundTo = 1.0f;
	tchar useFormat[10];
	tchar resultValue[60];
	while (decimals < _limit)
	{
#ifdef AN_CLANG
		tsprintf(useFormat, 9, TXT("%%.%if"), decimals);
		tsprintf(resultValue, 59, useFormat, _value);
#else
		tsprintf_s(useFormat, TXT("%%.%if"), decimals);
		tsprintf_s(resultValue, useFormat, _value);
#endif
		float parsedValue;
		parse_float_ref(resultValue, parsedValue);
		if (parsedValue == _value)
		{
			break;
		}
		++decimals;
		roundTo *= 0.1f;
	}
	String result;
	while (true)
	{
#ifdef AN_CLANG
		tsprintf(useFormat, 59, TXT("%%.%if"), decimals);
#else
		tsprintf_s(useFormat, TXT("%%.%if"), decimals);
#endif
		result = String::printf(useFormat, _value);
		// get rid of trailing zeros if we somehow ended up with them
		if (decimals > 0 && result[result.get_length() - 1] == '0')
		{
			--decimals;
			continue;
		}
		else
		{
			break;
		}
	}
	return result;
}
