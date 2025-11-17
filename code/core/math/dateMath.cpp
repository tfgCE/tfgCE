#include "math.h"

int date_to_day_number(int _year, int _month, int _day)
{
	_month = (_month + 9) % 12;
	_year = _year - _month/10;
	return 365*_year + _year/4 - _year/100 + _year/400 + (_month*306 + 5)/10 + ( _day - 1 );
}

void day_number_to_date(int _dayNumber, REF_ int & _year, REF_ int & _month, REF_ int & _day)
{
	_year = (int)((10000 * (int64)_dayNumber + 14780) / 3652425);
	int ddd = _dayNumber - (365*_year + _year/4 - _year/100 + _year/400);
	if (ddd < 0)
	{
		-- _year;
		ddd = _dayNumber - (365*_year + _year/4 - _year/100 + _year/400);
	}
	int mi = (100*ddd + 52)/3060;
	_year = _year + (mi + 2)/12;
	_month = (mi + 2)%12 + 1;
	_day = ddd - (mi*306 + 5)/10 + 1;
}
