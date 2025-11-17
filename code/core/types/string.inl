String::String()
{
	memory_leak_suspect;
	data.push_back((tchar)0);
	forget_memory_leak_suspect;
}

String String::single(tchar const _ch)
{
	String result;
	memory_leak_suspect;
	result.data.set_size(2);
	result.data[0] = _ch;
	result.data[1] = 0;
	forget_memory_leak_suspect;
	return result;
}

String String::of_length(uint const _length)
{
	String result;
	memory_leak_suspect;
	result.data.set_size(_length + 1);
	memory_zero(&result.data[0], _length + 1);
	forget_memory_leak_suspect;
	return result;
}

String::String(String const & _source)
{
	memory_leak_suspect;
	data = _source.data;
	forget_memory_leak_suspect;
}

String::String(tchar const * const _data)
{
	if (_data)
	{
		memory_leak_suspect;
		memory_leak_info(_data);
		data.set_size((uint)(tstrlen(_data) + 1));
		memory_copy(&data[0], _data, get_length() * sizeof(tchar));
		data[get_length()] = (tchar)0;
		forget_memory_leak_info;
		forget_memory_leak_suspect;
	}
	else
	{
		memory_leak_suspect;
		data.set_size(1);
		data[0] = (tchar)0;
		forget_memory_leak_suspect;
	}
}

String String::operator +(String const & _other) const
{
	uint length = get_length();
	uint otherLength = _other.get_length();

	String res = String::of_length(length + otherLength);

	memory_copy(&res.data[0], &data[0], length * sizeof(tchar));
	memory_copy(&res.data[length], &_other.data[0], otherLength * sizeof(tchar));

	res.data[length + otherLength] = (tchar)0;

	return res;
}

String String::operator +(tchar const * const _text) const
{
	if (! _text)
	{
		return *this;
	}
	uint length = get_length();
	uint otherLength = (uint)tstrlen(_text);

	String res = String::of_length(length + otherLength);

	memory_copy(&res.data[0], &data[0], length * sizeof(tchar));
	memory_copy(&res.data[length], _text, otherLength * sizeof(tchar));

	res.data[length + otherLength] = (tchar)0;

	return res;
}

String String::operator +(tchar const _char) const
{
	String res = String::of_length(get_length() + 1);

	memory_copy(&res.data[0], &data[0], get_length() * sizeof(tchar));

	res.data[get_length()] = _char;
	res.data[get_length() + 1] = (tchar)0;

	return res;
}

String const & String::operator +=(String const & _other)
{
	uint length( get_length() );
	memory_leak_suspect;
	data.set_size(length + _other.get_length() + 1);
	forget_memory_leak_suspect;
	memory_copy(&data[length], &_other.data[0], _other.get_length() * sizeof(tchar));
	data[length + _other.get_length()] = (tchar)0;

	return *this;
}

String const & String::operator +=(tchar const * const _text)
{
	if (!_text)
	{
		return *this;
	}
	uint length(get_length());
	uint textLength((uint)tstrlen(_text));
	memory_leak_suspect;
	data.set_size(length + textLength + 1);
	forget_memory_leak_suspect;
	memory_copy(&data[length], _text, textLength * sizeof(tchar));
	data[length + textLength] = (tchar)0;

	return *this;
}

String const & String::operator += (tchar const _char)
{
	uint length( get_length() );
	memory_leak_suspect;
	data.set_size(length + 2);
	forget_memory_leak_suspect;
	data[length] = _char;
	data[length + 1] = (tchar)0;

	return *this;
}

int String::diff_case(String const & _a, String const & _b)
{
	return tstrcmp(&_a.data[0], &_b.data[0]);
}

int String::diff_icase(String const & _a, String const & _b)
{
	return tstrcmpi(&_a.data[0], &_b.data[0]);
}

int String::diff_case(tchar const* _a, tchar const* _b)
{
	return tstrcmp(_a, _b);
}

int String::diff_icase(tchar const * _a, tchar const* _b)
{
	return tstrcmpi(_a, _b);
}

int String::compare_case_sort(void const * _a, void const * _b)
{
	String const * a = plain_cast<String>(_a);
	String const * b = plain_cast<String>(_b);

	return diff_case(*a, *b);
}

int String::compare_icase_sort(void const * _a, void const * _b)
{
	String const * a = plain_cast<String>(_a);
	String const * b = plain_cast<String>(_b);

	return diff_icase(*a, *b);
}

int String::compare_tchar_case_sort(void const * _a, void const * _b)
{
	tchar const * a = plain_cast<tchar const>(_a);
	tchar const * b = plain_cast<tchar const>(_b);

	return diff_case(a, b);
}

int String::compare_tchar_icase_sort(void const * _a, void const * _b)
{
	tchar const* a = plain_cast<tchar const>(_a);
	tchar const* b = plain_cast<tchar const>(_b);

	return diff_icase(a, b);
}

bool String::compare_icase(String const & _a, String const & _b)
{
	return tstrcmpi(&_a.data[0], &_b.data[0]) == 0;
}

bool String::compare_icase(String const & _a, tchar const * const _b)
{
	return tstrcmpi(&_a.data[0], _b) == 0;
}

bool String::compare_icase(tchar const * const _a, tchar const * const _b)
{
	return tstrcmpi(_a, _b) == 0;
}

bool String::compare_icase_left(tchar const * const _a, tchar const * const _b, int _firstCharactersCount)
{
	int aLen = (int)tstrlen(_a);
	int bLen = (int)tstrlen(_b);
	int len = min(aLen, bLen);
	if (len < _firstCharactersCount)
	{
		return false;
	}
	an_assert(_firstCharactersCount < 256);
	tchar aTemp[256];
	tchar bTemp[256];
	int dataSize = sizeof(tchar) * (_firstCharactersCount + 1);
	memory_copy(aTemp, _a, dataSize);
	memory_copy(bTemp, _b, dataSize);
	aTemp[_firstCharactersCount] = 0;
	bTemp[_firstCharactersCount] = 0;
	return tstrcmpi(aTemp, bTemp) == 0;
}

bool String::operator ==(String const & _other) const
{
	return tstrcmp(&data[0], &_other.data[0]) == 0;
}

bool String::operator ==(tchar const * const _text) const
{
	return tstrcmp(&data[0], _text) == 0;
}

bool String::operator !=(String const & _other) const
{
	return tstrcmp(&data[0], &_other.data[0]) != 0;
}

bool String::operator !=(tchar const * const _text) const
{
	return tstrcmp(&data[0], _text) != 0;
}

bool String::operator <(String const & _other) const
{
	return tstrcmp(&data[0], &_other.data[0]) < 0;
}

String const & String::operator =(String const & _other)
{
	if (_other.data.is_empty())
	{
		data.set_size(1);
		data[0] = (tchar)0;
	}
	else
	{
		uint otherLength = _other.get_length();
		memory_leak_suspect;
		memory_leak_info(_other.to_char());
		data.set_size(otherLength + 1);
		forget_memory_leak_info;
		forget_memory_leak_suspect;
		memory_copy(&data[0], &_other.data[0], otherLength * sizeof(tchar));
		data[otherLength] = (tchar)0;
	}

	return *this;
}

String const & String::operator =(tchar const * const _text)
{
	if (!_text)
	{
		return *this;
	}

	uint textLength((uint)tstrlen(_text));
	memory_leak_suspect;
	memory_leak_info(_text);
	data.set_size(textLength + 1);
	forget_memory_leak_info;
	forget_memory_leak_suspect;
	memory_copy(&data[0], _text, textLength * sizeof(tchar));
	data[textLength] = (tchar)0;

	return *this;
}

bool String::has_prefix(String const & _prefix) const
{
	if (_prefix.get_length() > get_length())
	{
		return false;
	}

	tchar const * prefixCh = _prefix.data.get_data();
	for_every_const(ch, data)
	{
		if (*ch != *prefixCh)
		{
			return false;
		}
		++prefixCh;
		if (*prefixCh == 0)
		{
			break;
		}
	}

	return true;
}

bool String::has_prefix(tchar const * _prefix) const
{
	if ((int)tstrlen(_prefix) > get_length())
	{
		return false;
	}

	tchar const * prefixCh = _prefix;
	for_every_const(ch, data)
	{
		if (*ch != *prefixCh)
		{
			return false;
		}
		++prefixCh;
		if (*prefixCh == 0)
		{
			break;
		}
	}

	return true;
}

String String::without_prefix(String const & _prefix) const
{
	if (has_prefix(_prefix))
	{
		return get_sub(_prefix.get_length());
	}
	else
	{
		return *this;
	}
}

String String::without_prefix(tchar const * _prefix) const
{
	if (has_prefix(_prefix))
	{
		return get_sub((uint)tstrlen(_prefix));
	}
	else
	{
		return *this;
	}
}

bool String::has_suffix(String const & _suffix) const
{
	if (_suffix.get_length() > get_length())
	{
		return false;
	}

	tchar const * suffixCh = _suffix.data.get_data();
	tchar const * ch = &data[get_length() - _suffix.get_length()];
	for (int i = 0; i < _suffix.get_length(); ++ i)
	{
		if (*ch != *suffixCh)
		{
			return false;
		}
		++suffixCh;
		++ch;
	}

	return true;
}

bool String::has_suffix(tchar const * _suffix) const
{
	int suffixLen = (int)tstrlen(_suffix);
	if (suffixLen > get_length())
	{
		return false;
	}

	tchar const * suffixCh = _suffix;
	tchar const * ch = &data[get_length() - suffixLen];
	for (int i = 0; i < suffixLen; ++i)
	{
		if (*ch != *suffixCh)
		{
			return false;
		}
		++suffixCh;
		++ch;
	}

	return true;
}

int String::find_last_of(tchar const _ch, int _startAt) const
{
	if (_startAt == NONE || _startAt >= get_length())
	{
		int index(get_length());
		for_every_reverse_const(ch, data)
		{
			if (*ch == _ch)
			{
				return index;
			}
			--index;
		}
	}
	else
	{
		int index = _startAt;
		tchar const* ch = &data[index];
		for (int index = _startAt; index >= 0; -- index, -- ch)
		{
			if (*ch == _ch)
			{
				return index;
			}
		}
	}
	return NONE;
}

int String::find_first_of(String const & _str, int _startAt) const
{
	int at( _startAt );
	int index( 0 );
	int strIdx( 0 );
	int strLen( _str.get_length() );
	for_every_const(ch, data)
	{
		if (index >= _startAt)
		{
			if (*ch == _str[strIdx])
			{
				++ strIdx;
				if (strIdx == strLen)
				{
					return at;
				}
			}
			else
			{
				strIdx = 0;
				at = index + 1;
			}
		}
		++ index;
	}
	return NONE;
}

int String::find_first_of(tchar const _ch, int _startAt) const
{
	int index( _startAt );
	int length( get_length() );
	for (tchar const* ch = data.get_data() + _startAt; index < (int)length; ++index, ++ch)
	{
		if (index >= _startAt)
		{
			if (*ch == _ch)
			{
				return index;
			}
		}
	}
	return NONE;
}

String String::get_left(uint _howMany) const
{
	int length = get_length();
	_howMany = min(length, (int)_howMany);
	String res = String::of_length(max(0, (int)_howMany));
	tchar * resCh = res.data.get_data();
	tchar const * ch = data.get_data();
	for (int i = 0; i < (int)_howMany; ++ i, ++ resCh, ++ ch)
	{
		*resCh = *ch;
	}
	*resCh = (tchar)0;
	return res;
}

String String::get_right(uint _howMany) const
{
	int length = get_length();
	_howMany = min(length, (int)_howMany);
	String res = String::of_length(max(0, (int)_howMany));
	tchar * resCh = res.data.get_data();
	tchar const * ch = data.get_data() + length - _howMany;
	for (int i = 0; i < (int)_howMany; ++ i, ++ resCh, ++ ch)
	{
		*resCh = *ch;
	}
	*resCh = (tchar)0;
	return res;
}

String String::get_sub(int _begin, int _howMany) const
{
	int length = get_length();
	if (_begin < 0)
	{
		_begin = (int)length + _begin;
		if (_begin < 0)
		{
			_begin = 0;
		}
	}
	if (_begin >= length)
	{
		return String::empty();
	}
	_howMany = _howMany < 0? min(length - _begin, length) : min(length - _begin, _howMany);
	String res = String::of_length( max(0, _howMany) );
	tchar * resCh = res.data.get_data();
	tchar const * ch = &data[_begin];
	for (int i = 0; i < _howMany; ++ i, ++ resCh, ++ ch)
	{
		*resCh = *ch;
	}
	*resCh = (tchar)0;
	memory_leak_suspect;
	res.data.set_size((uint)(resCh - &res.data[0] + 1));
	forget_memory_leak_suspect;
	return res;
}

template<typename AnyChar>
String String::convert_from(AnyChar* _in)
{
	String result;
	AnyChar* inc = (AnyChar*)_in;
	while (*inc != 0)
	{
		result += tchar(*inc);
		++inc;
	}
	return result;
}

template<typename AnyChar>
void String::convert_to(AnyChar* _out, int _maxLength) const
{
	int len = 0;
	for_every(ch, get_data())
	{
		*_out = AnyChar(*ch);
		++len;
		if (len == _maxLength)
		{
			// to fill current one with 0
			break;
		}
		++_out;
	}
	*_out = 0;
}

template<typename AnyChar>
bool String::is_same_as(AnyChar const * _other) const
{
	for_every(ch, get_data())
	{
		if (*_other != AnyChar(*ch))
		{
			return false;
		}
		++_other;
	}
	return true;
}

void String::change_no_break_spaces_into_spaces()
{
	int index( 0 );
	int length( get_length() );
	for (tchar * ch = data.get_data(); index < (int)length; ++index, ++ch)
	{
		if (*ch == no_break_space_char())
		{
			*ch = ' ';
		}
	}
}

void String::change_consecutive_spaces_to_no_break_spaces()
{
	int index( 0 );
	int length( get_length() );
	bool convertSpaces = false;
	for (tchar * ch = data.get_data(); index < (int)length; ++index, ++ch)
	{
		if (*ch == 32)
		{
			// first or last are spaces
			if (index == 0 ||
				index == length - 1)
			{
				convertSpaces = true;
			}
			// two spaces after one another
			if (index < length - 1 &&
				data[index + 1] == ' ')
			{
				convertSpaces = true;
			}
			if (convertSpaces)
			{
				*ch = no_break_space_char();
			}
			convertSpaces = true; // convert all following
		}
		else
		{
			convertSpaces = false;
		}
	}
}
