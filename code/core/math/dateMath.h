#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

int date_to_day_number(int _year, int _month, int _day);

void day_number_to_date(int _dayNumber, REF_ int & _year, REF_ int & _month, REF_ int & _day);
