Token::Token()
:	length( 0 )
{
}

Token::Token(Token const & _source)
{
	length = _source.length;
	memory_copy(data, _source.data, length * sizeof(tchar));
}

Token::Token(tchar const * const _data)
{
	an_assert(tstrlen(_data) <= TOKEN_LENGTH, TXT("data cannot exceed max token length TOKEN_LENGTH"));
	length = (uint)(tstrlen(_data));
	memory_copy(&data[0], _data, (length + 1) * sizeof(tchar));
}

Token const & Token::operator += (tchar const _char)
{
	an_assert(length <= TOKEN_LENGTH - 1, TXT("data cannot exceed max token length TOKEN_LENGTH"));
	data[length] = _char;
	data[length + 1] = (tchar)0;
	++ length;

	return *this;
}

bool Token::compare_icase(Token const & _a, Token const & _b)
{
	return tstrcmpi(&_a.data[0], &_b.data[0]) == 0;
}

bool Token::compare_icase(Token const & _a, tchar const * const & _b)
{
	return tstrcmpi(&_a.data[0], _b) == 0;
}

bool Token::operator ==(Token const & _other) const
{
	return tstrcmp(&data[0], &_other.data[0]) == 0;
}

bool Token::operator ==(tchar const * const _text) const
{
	return tstrcmp(&data[0], _text) == 0;
}

bool Token::operator !=(Token const & _other) const
{
	return tstrcmp(&data[0], &_other.data[0]) != 0;
}

bool Token::operator !=(tchar const * const _text) const
{
	return tstrcmp(&data[0], _text) != 0;
}

bool Token::operator <(Token const & _other) const
{
	return tstrcmp(&data[0], &_other.data[0]) < 0;
}

Token const & Token::operator =(Token const & _other)
{
	length = _other.length;
	memory_copy(data, _other.data, length * sizeof(tchar));

	return *this;
}

Token const & Token::operator =(tchar const * const _text)
{
	an_assert(tstrlen(_text) <= TOKEN_LENGTH, TXT("data cannot exceed max token length TOKEN_LENGTH"));
	length = (uint)tstrlen(_text);
	memory_copy(&data[0], _text, (length + 1) * sizeof(tchar));

	return *this;
}

int Token::find_last_of(tchar const _ch) const
{
	int index( get_length() - 1 );
	for (tchar const * ch = &data[index]; index >= 0; -- index, -- ch)
	{
		if (*ch == _ch)
		{
			return index;
		}
	}
	return NONE;
}

int Token::find_first_of(Token const & _str, int _startAt) const
{
	int at( _startAt );
	int index( 0 );
	int strIdx( 0 );
	int strLen( _str.get_length() );
	for (tchar const * ch = data; index < (int)length; ++ index, ++ ch)
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

int Token::find_first_of(tchar const _ch, int _startAt) const
{
	int index( _startAt );
	for (tchar const * ch = data + _startAt; index < (int)length; ++ index, ++ ch)
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

