#include "number.h"
#include "..\core\other\parserUtils.h"

Number::Number()
{
	for_count(int, i, COUNT)
	{
		value[i] = 0;
	}
}

bool Number::is_zero() const
{
	char const * a = value;
	for_count(int, i, COUNT)
	{
		if (*a != 0)
		{
			return false;
		}
		++a;
	}
	return true;
}

Number Number::integer(int _a)
{
	Number n;
	n.parse(String::printf(TXT("%i"), _a));
	return n;
}

Number Number::negated() const
{
	Number result = *this;;
	result.sign = -result.sign;
	return result;
}

Number Number::absolute() const
{
	Number result = *this;;
	result.sign = 1;
	return result;
}

void Number::simplify()
{
	while (pointAt > 0 && value[0] == 0)
	{
		shift_down();
		--pointAt;
	}
}

void Number::shift_down()
{
	char* a = &value[0];
	char* b = &value[1];
	for_count(int, i, COUNT - 1)
	{
		*a = *b;
		++a;
		++b;
	}
	value[COUNT - 1] = 0;
}

void Number::shift_up()
{
	char* a = &value[COUNT - 2];
	char* b = &value[COUNT - 1];
	for_count(int, i, COUNT - 1)
	{
		*b = *a;
		--a;
		--b;
	}
	value[0] = 0;
}

bool Number::operator == (Number const & _other) const
{
	Number a = *this;
	Number b = _other;
	if (a.sign != b.sign)
	{
		return a.is_zero() && b.is_zero();
	}
	find_common_point(a, b);

	char* ac = &a.value[COUNT - 1];
	char* bc = &b.value[COUNT - 1];

	char carry = 0;
	for_count(int, i, COUNT)
	{
		if (*ac != *bc)
		{
			return false;
		}
		--ac;
		--bc;
	}
	return true;
}

bool Number::operator < (Number const & _other) const
{
	Number a = *this;
	Number b = _other;
	if (a.sign != b.sign)
	{
		if (a.is_zero() && b.is_zero())
		{
			return false;
		}
		return a.sign < b.sign;
	}
	if (a.sign < 0)
	{
		return b < a;
	}
	find_common_point(a, b);

	char* ac = &a.value[COUNT - 1];
	char* bc = &b.value[COUNT - 1];

	char carry = 0;
	for_count(int, i, COUNT)
	{
		if (*ac < *bc)
		{
			return true;
		}
		if (*ac > * bc)
		{
			return false;
		}
		--ac;
		--bc;
	}
	return false;
}

bool Number::operator <= (Number const & _other) const
{
	Number a = *this;
	Number b = _other;
	if (a.sign != b.sign)
	{
		if (a.is_zero() && b.is_zero())
		{
			return true;
		}
		return a.sign < b.sign;
	}
	if (a.sign < 0)
	{
		// reverse signs
		b.sign = -b.sign;
		a.sign = -a.sign;
		return b <= a;
	}
	find_common_point(a, b);

	char* ac = &a.value[COUNT - 1];
	char* bc = &b.value[COUNT - 1];

	char carry = 0;
	for_count(int, i, COUNT)
	{
		if (*ac < *bc)
		{
			return true;
		}
		if (*ac > * bc)
		{
			return false;
		}
		--ac;
		--bc;
	}
	return true;
}

Number Number::operator + (Number const & _other) const
{
	Number a = *this;
	Number b = _other;
	if (a.sign < 0)
	{
		if (b.sign < 0)
		{
			a.sign = -a.sign;
			b.sign = -b.sign;
			a = a + b;
			a.sign = -a.sign;
			return a;
		}
		else
		{
			a.sign = -a.sign;
			a = a - b;
			a.sign = -a.sign;
			return a;
		}
	}
	if (b.sign < 0)
	{
		b.sign = -b.sign;
		a = a - b;
		return a;
	}
	find_common_point(a, b);

	char* ac = a.value;
	char* bc = b.value;

	char carry = 0;
	for_count(int, i, COUNT)
	{
		*ac = *ac + *bc + carry;
		carry = 0;
		if (*ac > 9)
		{
			*ac -= 10;
			carry += 1;
		}
		++ac;
		++bc;
	}
	an_assert(carry == 0);

	a.simplify();
	return a;
}

Number Number::operator - (Number const & _other) const
{
	Number a = *this;
	Number b = _other;
	if (a.sign < 0)
	{
		if (b.sign < 0)
		{
			a.sign = -a.sign;
			b.sign = -b.sign;
			a = a - b;
			a.sign = -a.sign;
			return a;
		}
		else
		{
			a.sign = -a.sign;
			a = a + b;
			a.sign = -a.sign;
			return a;
		}
	}
	if (b.sign < 0)
	{
		a = a + b;
		return a;
	}
	if (a < b)
	{
		a = b - a;
		a.sign = -a.sign;
		return a;
	}
	find_common_point(a, b);

	char* ac = a.value;
	char* bc = b.value;

	char carry = 0;
	for_count(int, i, COUNT)
	{
		*ac -= carry;
		carry = 0;
		if (*ac < *bc)
		{
			carry = 1;
			*ac = *ac + 10 - *bc;
		}
		else
		{
			*ac = *ac - *bc;
		}
		++ac;
		++bc;
	}
	an_assert(carry == 0);

	a.simplify();
	return a;
}

Number Number::operator * (Number const & _other) const
{
	Number a = *this;
	Number b = _other;


	Number result;
	result.sign = a.sign * b.sign;
	result.pointAt = a.pointAt + b.pointAt;
	char* rStart = result.value;
	char* ac = a.value;
	for_count(int, i, COUNT)
	{
		char* rc = rStart;
		char* bc = b.value;
		char carry = 0;
		for_count(int, j, COUNT - 1 - (int)(ac - a.value))
		{
			*rc = *rc + *ac * *bc + carry;
			carry = 0;
			while (*rc > 9)
			{
				*rc -= 10;
				carry += 1;
			}
			++rc;
			++bc;
		}
		an_assert(carry == 0);

		++ac;
		++rStart;
	}

	result.simplify();
	return result;
}

Number Number::operator / (Number const & _other) const
{
	if (_other.is_zero())
	{
		warn(TXT("dividing by zero"));
		return Number();
	}
	Number a = *this;
	Number b = _other;

	int pAt = a.pointAt;
	for_count(int, i, _other.pointAt)
	{
		a.shift_up();
	}

	Number div = b;
	div.pointAt = 0;

	char * ac = &a.value[COUNT - 1];
	while (ac > a.value && *ac == 0)
	{
		--ac;
	}

	String resultStr;

	Number now;
	while (ac >= a.value)
	{
		now.shift_up();
		now.value[0] = *ac;
		char r = 0;
		while (div <= now)
		{
			now = now - div;
			r++;
		}
		resultStr += '0' + r;
		if (ac == a.value &&
			pAt < 16 && !now.is_zero())
		{
			a.shift_up();
			++ac;
			++pAt;
		}
		--ac;
	}

	Number result;
	result.sign = a.sign * b.sign;
	int resultStrLen = resultStr.get_length();
	for_count(int, i, resultStrLen)
	{
		result.value[i] = (char)resultStr.get_data()[resultStrLen - 1 - i] - '0';
	}
	result.pointAt = pAt;
	result.simplify();
	return result;
}

Number Number::operator +=(Number const & _other)
{
	*this = *this + _other;
	return *this;
}

Number Number::operator -=(Number const & _other)
{
	*this = *this - _other;
	return *this;
}

void Number::find_common_point(Number & a, Number & b)
{
	while (a.pointAt < b.pointAt)
	{
		a.shift_up();
		++a.pointAt;
	}
	while (b.pointAt < a.pointAt)
	{
		b.shift_up();
		++b.pointAt;
	}
}

String Number::to_string(Optional<int> _pointAt) const
{
	Number v = *this;
	if (_pointAt.is_set())
	{
		v.set_point_at(_pointAt.get());
	}

	String result;
	if (v.sign < 0)
	{
		result += TXT("-");
	}

	char const * ac = &v.value[COUNT - 1];
	while (ac > v.value && *ac == 0 && ac - v.value > v.pointAt)
	{
		--ac;
	}

	while (ac >= v.value)
	{
		result += '0' + *ac;
		if (ac - v.value == v.pointAt && v.pointAt > 0)
		{
			result += TXT(".");
		}
		--ac;
	}

	return result;
}

bool Number::parse(String const & _value)
{
	*this = Number();
	bool result = true;
	String v = _value.trim();
	v = v.replace(String::space(), String::empty());
	bool dotPresent = false;
	for_count(int, at, v.get_length())
	{
		if (v.get_data()[at] == '-')
		{
			if (at == 0)
			{
				sign = -1;
			}
			else
			{
				result = false;
			}
		}
		else if (v.get_data()[at] == '.' ||
				 v.get_data()[at] == ',')
		{
			if (dotPresent)
			{
				result = false;
			}
			dotPresent = true;
		}
		else
		{
			int n = (v.get_data()[at] - '0');
			if (n >= 0 && n <= 9)
			{
				shift_up();
				value[0] = n;
				if (dotPresent)
				{
					++pointAt;
				}
			}
		}
	}

	return result;
}

void Number::set_point_at(int _pointAt)
{
	while (pointAt < _pointAt)
	{
		shift_up();
		++pointAt;
	}
	if (pointAt > _pointAt)
	{
		while (pointAt > _pointAt + 1)
		{
			shift_down();
			--pointAt;
		}

		int rest = value[0];
		shift_down();
		--pointAt;
		if (rest >= 5)
		{
			char *ac = value;
			for_count(int, i, COUNT)
			{
				*ac += 1;
				if (*ac > 9)
				{
					*ac -= 10;
				}
				else
				{
					break;
				}
				++ac;
			}
		}
	}
}
