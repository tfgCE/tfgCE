#pragma once

#include "..\core\containers\arrayStatic.h"
#include "..\core\types\optional.h"
#include "..\core\types\string.h"

struct Number
{
	static const int COUNT = 256;
	int sign = 1;
	char value[COUNT]; // 0 is lowest
	int pointAt = 0; // 0 is 1 = 1, 1 is 10 = 1.0, 2 is 100 = 1.00 etc

	Number();

	bool is_zero() const;
	bool is_positive() const { return sign > 0 && !is_zero(); }
	bool is_negative() const { return sign < 0 && !is_zero(); }
	
	Number negated() const;
	Number absolute() const;

	static Number integer(int _a);

	void make_absolute() { sign = 1; }
	void set_point_at(int _pointAt);
	void limit_point_at_to(int _pointAt) { if (pointAt > _pointAt) set_point_at(_pointAt); }

	bool parse(String const & _value);
	String to_string(Optional<int> _pointAt = NP) const;

	Number operator + (Number const & _other) const;
	Number operator - (Number const & _other) const;
	Number operator * (Number const & _other) const;
	Number operator / (Number const & _other) const;

	Number operator += (Number const & _other);
	Number operator -= (Number const & _other);

	bool operator == (Number const & _other) const;
	bool operator < (Number const & _other) const;
	bool operator <= (Number const & _other) const;
	bool operator > (Number const & _other) const { return !(*this <= _other); }
	bool operator >= (Number const & _other) const { return !(*this < _other); }

private:
	void shift_down();
	void shift_up();
	void simplify();

	static void find_common_point(Number & a, Number & b);
};