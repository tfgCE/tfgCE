#include "string.h"

#include "unicode.h"

#include "..\concurrency\scopedSpinLock.h"

#include "..\containers\arrayStack.h"
#include "..\containers\map.h"
#include "..\containers\list.h"

#include "..\other\parserUtils.h"
#include "..\serialisation\serialiser.h"

#include "..\io\xml.h"

#ifdef AN_LINUX_OR_ANDROID
#include <ctype.h>
#endif

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

String* String::s_empty = nullptr;
String* String::s_space = nullptr;
String* String::s_noBreakSpace = nullptr;
String* String::s_comma = nullptr;
String* String::s_something = nullptr;
String* String::s_degree = nullptr;
String* String::s_newLine = nullptr;

typedef Map<tchar, tchar> tcharRemapper;

tcharRemapper* upperToLowerRemapper = nullptr;
tcharRemapper* lowerToUpperRemapper = nullptr;

String String::spaces(int _amount)
{
	String s = String::of_length(_amount);
	tchar* ch = s.access_data().begin();
	for_count(int, i, _amount)
	{
		*ch = ' ';
		++ch;
	}
	return s;
}

String* String::create_new()
{
	return new String();
}

void String::initialise_static()
{
	an_assert(s_empty == nullptr);
	s_empty = new String();
	an_assert(s_space == nullptr);
	s_space = new String(TXT(" "));
	an_assert(s_noBreakSpace == nullptr);
	s_noBreakSpace = new String(TXT(" "));
	(*s_noBreakSpace)[0] = no_break_space_char();
	an_assert(s_comma == nullptr);
	s_comma = new String(TXT(","));
	an_assert(s_something == nullptr);
	s_something = new String(TXT("ABC"));
	an_assert(s_degree == nullptr);
	s_degree = new String(TXT("'"));
	s_degree->access_data()[0] = 176;
	an_assert(s_newLine == nullptr);
	s_newLine = new String(TXT("\n"));
	an_assert(upperToLowerRemapper == nullptr);
	an_assert(lowerToUpperRemapper == nullptr);
	upperToLowerRemapper = new tcharRemapper();
	lowerToUpperRemapper = new tcharRemapper();
}

void String::close_static()
{
	an_assert(s_empty != nullptr);
	delete_and_clear(s_empty);
	an_assert(s_space != nullptr);
	delete_and_clear(s_space);
	an_assert(s_noBreakSpace != nullptr);
	delete_and_clear(s_noBreakSpace);
	an_assert(s_comma != nullptr);
	delete_and_clear(s_comma);
	an_assert(s_something != nullptr);
	delete_and_clear(s_something);
	an_assert(s_degree != nullptr);
	delete_and_clear(s_degree);
	an_assert(s_newLine != nullptr);
	delete_and_clear(s_newLine);
	an_assert(upperToLowerRemapper != nullptr);
	an_assert(lowerToUpperRemapper != nullptr);
	delete_and_clear(upperToLowerRemapper);
	delete_and_clear(lowerToUpperRemapper);
}

int String::count(tchar ch) const
{
	int i = 0;
	for_every(c, get_data())
	{
		if (*c == ch)
		{
			++i;
		}
	}
	return i;
}

bool String::does_contain(tchar ch) const
{
	for_every(c, get_data())
	{
		if (*c == ch)
		{
			return true;
		}
	}
	return false;
}

void String::split(String const & _splitter, List<String> & _tokens, int _howManySplits) const
{
	int lastStartsAt( 0 );
	int at( 0 );
	int index( 0 );
	int strIdx( 0 );
	int strLen( _splitter.get_length() );
	int splitCount = 0;
	for_every_const(ch, data)
	{
		if (*ch == _splitter[strIdx])
		{
			++ strIdx;
			if (strIdx == strLen)
			{
				String found = get_sub(lastStartsAt, at - lastStartsAt);
				_tokens.push_back(found);
				lastStartsAt = index + 1;
				strIdx = 0;
				at = index + 1;
				++ splitCount;
				if (splitCount == _howManySplits) // will handle 0
				{
					break;
				}
			}
		}
		else
		{
			strIdx = 0;
			at = index + 1;
		}
		++ index;
	}
	String found = get_sub(lastStartsAt);
	_tokens.push_back(found);
}

String String::replace(String const & _replace, String const & _with) const
{
	String result = *this;
	int safeStart( 0 );
	while (true)
	{
		int at = result.find_first_of(_replace, safeStart);
		if (at < 0)
		{
			break;
		}
		safeStart = at + _with.get_length();
		result = result.get_left(at) + _with + result.get_sub(at + _replace.get_length());
	}
	return result;
}

bool String::replace_inline(String const & _replace, String const & _with)
{
	bool changed = false;
	int safeStart(0);
	while (true)
	{
		int at = find_first_of(_replace, safeStart);
		if (at < 0)
		{
			break;
		}
		safeStart = at + _with.get_length();
		*this = get_left(at) + _with + get_sub(at + _replace.get_length());
		changed = true;
	}
	return changed;
}

String String::keep_only_chars(String const & _charsToKeep) const
{
	return keep_only_chars(_charsToKeep.to_char());
}

String String::keep_only_chars(tchar const * _charsToKeep) const
{
	String result;
	result.access_data().make_space_for(get_data().get_size());
	for_every(ch, get_data())
	{
		tchar const* keep = _charsToKeep;
		while (*keep)
		{
			if (*ch == *keep)
			{
				result += *ch;
				break;
			}
			++keep;
		}
	}

	return result;
}

String String::compress() const
{
	String result = *this;
	int readIdx( 0 );
	int writeIdx( 0 );
	int emptyChars = 0;
	while (readIdx < result.get_length())
	{
		tchar ch = result[readIdx];
		if (ParserUtils::is_empty_char(ch))
		{
			if (emptyChars == 0)
			{
				result[writeIdx] = ' ';
				++ writeIdx;
			}
			++ emptyChars;
		}
		else
		{
			emptyChars = 0;
			result[writeIdx] = result[readIdx];
			++ writeIdx;
		}
		++ readIdx;
	}
	result[writeIdx] = 0;
	result.data.set_size(writeIdx + 1);
	return result;
}

String String::prune() const
{
	String result = *this;
	int readIdx(0);
	int writeIdx(0);
	while (readIdx < result.get_length())
	{
		tchar ch = result[readIdx];
		if (! ParserUtils::is_empty_char(ch))
		{
			result[writeIdx] = result[readIdx];
			++writeIdx;
		}
		++readIdx;
	}
	result[writeIdx] = 0;
	result.data.set_size(writeIdx + 1);
	return result;
}

String String::trim() const
{
	int leftIdx( 0 );
	while (leftIdx < get_length())
	{
		if (! ParserUtils::is_empty_char(operator[](leftIdx)))
		{
			break;
		}
		++ leftIdx;
	}
	int rightIdx( get_length() - 1 );
	while (rightIdx >= 0)
	{
		if (! ParserUtils::is_empty_char(operator[](rightIdx)))
		{
			break;
		}
		-- rightIdx;
	}
	return get_sub(leftIdx, rightIdx - leftIdx + 1);
}

String String::trim_right() const
{
	int rightIdx(get_length() - 1);
	while (rightIdx >= 0)
	{
		if (!ParserUtils::is_empty_char(operator[](rightIdx)))
		{
			break;
		}
		--rightIdx;
	}
	return get_left(rightIdx + 1);
}

String String::nice_sentence(int _formatParagraphs) const
{
	ARRAY_STACK(tchar, result, get_length() * 2 + 10); // should be enough
	tchar lastAddedChar = 0;
	int emptyChars = 0;
	int lastEndLineChars = 0;
	for_every(ch, get_data())
	{
		if (*ch == 0)
		{
			break;
		}
		// skip extra spaces between ([ and any next character
		if (lastAddedChar == '(' ||
			lastAddedChar == '[')
		{
			if (*ch == ' ')
			{
				continue;
			}
		}
		// format nice paragraphs too! we want to have two ~, one to end current line, second to have empty line in between
		if (_formatParagraphs >= 0 && (*ch == '~' || *ch == '\n') && lastEndLineChars > _formatParagraphs)
		{
			continue;
		}
		// skip double spaces
		if (ParserUtils::is_empty_char(*ch))
		{
			if (emptyChars == 0)
			{
				lastEndLineChars = (*ch == '~' || *ch == '\n') ? lastEndLineChars + 1 : 0;
				lastAddedChar = *ch;
				result.push_back(*ch);
			}
			++emptyChars;
		}
		else
		{
			emptyChars = 0;
			// remove space if it is in front of a .,~)]
			if (*ch == '.' ||
				*ch == ',' ||
				*ch == '~' ||
				*ch == '\n' ||
				*ch == ')' ||
				*ch == ']')
			{
				if (lastAddedChar == ' ')
				{
					result.set_size(result.get_size() - 1);
				}
			}

			// add space between ., and other character
			if (lastAddedChar == '.' ||
				lastAddedChar == ',')
			{
				if (*ch > '9' &&
					*ch != '.' &&
					*ch != ',' &&
					*ch != '\'' &&
					*ch != '"' &&
					*ch != ';')
				{
					result.push_back(' ');
				}
			}
			lastEndLineChars = *ch == '~' ? lastEndLineChars + 1 : 0;
			lastAddedChar = *ch;
			result.push_back(*ch);
		}
	}
	result.push_back(0);
	// change .. into .
	ARRAY_STACK(tchar, result2, get_length() * 2 + 10); // should be enough
	int dotInRow = 0;
	for_every(ch, result)
	{
		result2.push_back(*ch);
		if (*ch == '.')
		{
			++dotInRow;
		}
		else
		{
			if (dotInRow == 2)
			{
				// remove last dot and remember about new character
				result2.pop_back();
				result2.pop_back();
				result2.push_back(*ch);
			}
			dotInRow = 0;
		}
	}
	return String(result2.get_data());
};

bool String::serialise(Serialiser & _serialiser)
{
	bool as8bitCharArray = false;
	if (_serialiser.is_reading())
	{
		serialise_data(_serialiser, as8bitCharArray);
	}
	else
	{
		as8bitCharArray = 1;
		for_every(ch, data)
		{
			if (
#ifndef AN_CLANG
				* ch < 0 ||
#endif
				* ch > 127)
			{
				as8bitCharArray = 0;
				break;
			}
		}
		serialise_data(_serialiser, as8bitCharArray);
	}
	if (as8bitCharArray)
	{
		// no need to encode characters
		if (_serialiser.is_reading())
		{
			Array<char8> as8;
			if (serialise_plain_data_array(_serialiser, as8))
			{
				if (as8.is_empty())
				{
					operator=(String::empty());
				}
				else
				{
					data.set_size(as8.get_size() + 1); // ending null is not stored
					memory_zero(data.get_data(), sizeof(tchar) * data.get_size());
					tchar* dst = data.get_data();
					char8 const* src = as8.get_data();
					for_count(int, i, data.get_size())
					{
#ifdef AN_LITTLE_ENDIAN
						memory_copy(dst, src, sizeof(char8));
#else
	#error implement
#endif
						++dst;
						++src;
					}
					data.get_last() = 0;
				}
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			ARRAY_STACK(char8, as8, data.get_size());
			if (!is_empty())
			{
				as8.set_size(data.get_size() - 1); // don't store ending null
				char8* dst = as8.get_data();
				tchar const* src = data.get_data();
				for_count(int, i, as8.get_size())
				{
#ifdef AN_LITTLE_ENDIAN
					memory_copy(dst, src, sizeof(char8));
#else
#error implement
#endif
					++dst;
					++src;
				}
			}
			return serialise_plain_data_array(_serialiser, as8);
		}
	}
	else
	{
		int length;
		if (_serialiser.is_reading())
		{
			serialise_data(_serialiser, length);
			data.set_size(length + 1);
			data[length] = 0;
		}
		else 
		{
			an_assert(_serialiser.is_writing());
			length = get_length();
			serialise_data(_serialiser, length);
		}

		// encode characters in a fashion similar to unicode but we use all the bits available instead of just 6 per character, we could fit a bit better this way
		tchar* ch = data.begin();
		for (int i = 0; i < length; ++i, ++ch)
		{
			byte asByte;
			if (_serialiser.is_reading())
			{
				serialise_data(_serialiser, asByte);
				if ((asByte & 0xF0) == 0xE0)
				{
					// 1110 xxxx +3B
					// read three bytes (total four)
					uint32 asUnicode = (uint32)(asByte & 0x0F);
					serialise_data(_serialiser, asByte);
					asUnicode = asUnicode * 256 + (uint32)asByte;
					serialise_data(_serialiser, asByte);
					asUnicode = asUnicode * 256 + (uint32)asByte;
					serialise_data(_serialiser, asByte);
					asUnicode = asUnicode * 256 + (uint32)asByte;
					*ch = Unicode::unicode_to_tchar(asUnicode);
				}
				else if ((asByte & 0xE0) == 0xC0)
				{
					// 110x xxxx +2B
					// read two bytes (total three)
					uint32 asUnicode = (uint32)(asByte & 0x1F);
					serialise_data(_serialiser, asByte);
					asUnicode = asUnicode * 256 + (uint32)asByte;
					serialise_data(_serialiser, asByte);
					asUnicode = asUnicode * 256 + (uint32)asByte;
					*ch = Unicode::unicode_to_tchar(asUnicode);
				}
				else if ((asByte & 0xC0) == 0x80)
				{
					// 10xx xxxx +1B
					// read one byte (total two)
					uint32 asUnicode = (uint32)(asByte & 0x3F);
					serialise_data(_serialiser, asByte);
					asUnicode = asUnicode * 256 + (uint32)asByte;
					*ch = Unicode::unicode_to_tchar(asUnicode);
				}
				else
				{
					// 0xxx xxxx
					*ch = (tchar)asByte;
				}
			}
			else
			{
				if (
#ifndef AN_CLANG
					*ch >= 0 &&
#endif
					*ch < 128)
				{
					asByte = (byte)*ch;
					serialise_data(_serialiser, asByte);
				}
				else
				{
					uint32 asUnicode = (uint32)Unicode::tchar_to_unicode(*ch);
					if (asUnicode < 16384)
					{
						// 14 bits (2^(14) = 16384)
						asByte = (byte)((asUnicode & 0x00003F00) >> 8) | 0x80;
						serialise_data(_serialiser, asByte);
						asByte = (byte)((asUnicode & 0x000000FF) >> 0);
						serialise_data(_serialiser, asByte);
					}
					else if (asUnicode < 4194304)
					{
						// 21 bits (2^(21) = 2097152)
						asByte = (byte)((asUnicode & 0x007F0000) >> 16) | 0xC0;
						serialise_data(_serialiser, asByte);
						asByte = (byte)((asUnicode & 0x0000FF00) >> 8);
						serialise_data(_serialiser, asByte);
						asByte = (byte)((asUnicode & 0x000000FF) >> 0);
						serialise_data(_serialiser, asByte);
					}
					else 
					{
						// 28 bits 
						asByte = (byte)((asUnicode & 0x0F000000) >> 24) | 0xE0;
						serialise_data(_serialiser, asByte);
						asByte = (byte)((asUnicode & 0x00FF0000) >> 16);
						serialise_data(_serialiser, asByte);
						asByte = (byte)((asUnicode & 0x0000FF00) >> 8);
						serialise_data(_serialiser, asByte);
						asByte = (byte)((asUnicode & 0x000000FF) >> 0);
						serialise_data(_serialiser, asByte);
					}
				}
			}
			if (*ch == 0)
			{
				break;
			}
		}
		return true;
	}
}

String String::from_char8(char8 const * _text)
{
	if (!_text)
	{
		return String();
	}
#ifdef AN_WIDE_CHAR
	const size_t size = strlen(_text) + 1;
	String result;
	result.access_data().set_size((uint)size);
#ifdef AN_LINUX_OR_ANDROID
	mbstowcs(result.access_data().begin(), _text, size);
#else
#ifdef AN_WINDOWS
	size_t converted;
	mbstowcs_s(&converted, result.access_data().get_data(), size, _text, size);
	//an_assert(converted == strlen(_text));
#else
#error implement
#endif
#endif
	an_assert(result.get_length() == strlen(_text));
	return result;
#else
	return String(_text);
#endif
}

String String::from_char8_utf(char8 const * _text)
{
	if (!_text)
	{
		return String();
	}
#ifdef AN_WIDE_CHAR
	String result;
	char8 const * p = _text;
	while (*p != 0)
	{
		if (*p < 0 || *p > 127)
		{
			if (*(p + 1) != 0)
			{
				tchar t[5];
#ifdef AN_WINDOWS
				MultiByteToWideChar(CP_UTF8, 0, p, 2, t, 1);
#else
#ifdef AN_LINUX_OR_ANDROID
				mbtowc(t, p, 1);
#else
#error TODO implement utf8 to wide char
#endif
#endif
				result += Unicode::unicode_to_tchar(t[0]); // not true unicode but we have to encode as tchar
				++p;
				++p;
				continue;
			}
		}
		tchar t = *p;
		result += t;
		++p;
	}
	return result;
#else
	return String(_text);
#endif
}

Array<char8> String::to_char8_array() const
{
#ifdef AN_WIDE_CHAR
	Array<char8> asChar8;
	int size = get_length() + 1; // to get zero at the end
	asChar8.set_size(size);
	char8* dst = asChar8.begin();
	tchar const* src = get_data().begin_const();
	while (size)
	{
		if (
#ifndef AN_CLANG
			*src >= 0 &&
#endif
			*src <= 128)
		{
			*dst = (char8)(*src);
		}
		else
		{
			*dst = '?';
		}
		--size;
		++dst;
		++src;
	}
	return asChar8;
#else
	return data;
#endif
}

void String::to_char8_array(OUT_ REF_ Array<char8> & asChar8) const
{
#ifdef AN_WIDE_CHAR
	int size = get_length() + 1; // to get zero at the end
	asChar8.set_size(size);
	char8* dst = asChar8.begin();
	tchar const* src = get_data().begin_const();
	while (size)
	{
		if (
#ifndef AN_CLANG
			* src >= 0 &&
#endif
			* src <= 128)
		{
			*dst = (char8)(*src);
		}
		else
		{
			*dst = '?';
		}
		--size;
		++dst;
		++src;
	}
#else
	#error implement
#endif
}

Array<char8> String::to_char8_utf_array() const
{
#ifdef AN_WIDE_CHAR
	Array<char8> asChar8;
	int size = get_length() + 1; // to get zero at the end
	asChar8.set_size(size);
	char8* dst = asChar8.begin();
	tchar const* src = get_data().begin_const();
	while (size)
	{
		if (
#ifndef AN_CLANG
			*src >= 0 &&
#endif
			*src < 128)
		{
			*dst = (char8)(*src);
		}
		else
		{
			if (*src != 0)
			{
				char t[6];
				tchar srcCh = (tchar)Unicode::tchar_to_unicode(*src); // not true unicode but we have to decode to tchar
#ifdef AN_WINDOWS
				int count = WideCharToMultiByte(CP_UTF8, 0, &srcCh, 1, t, 5, nullptr, nullptr);
#else
#ifdef AN_LINUX_OR_ANDROID
				int count = wctomb(t, srcCh);
#else
#error TODO implement utf8 to wide char
#endif
#endif
				for_count(int, i, count)
				{
					*dst = t[i];
					++dst;
				}
				++src;
				--size;
				continue;
			}
			*dst = '?';
		}
		--size;
		++dst;
		++src;
	}
	return asChar8;
#else
	return data;
#endif
}

String String::first_to_upper() const
{
	String result = *this;

	if (!result.is_empty())
	{
		tchar* ch = result.access_data().begin();
		*ch = to_upper(*ch);
	}

	return result;
}

tchar String::to_upper(tchar ch)
{
	ch = toupper(ch);
	if (auto* newCh = lowerToUpperRemapper->get_existing(ch))
	{
		ch = *newCh;
	}
	return ch;
}

tchar String::to_lower(tchar ch)
{
	ch = tolower(ch);
	if (auto* newCh = upperToLowerRemapper->get_existing(ch))
	{
		ch = *newCh;
	}
	return ch;
}

String String::to_upper() const
{
	String result = *this;

	for_every(ch, result.data)
	{
		*ch = to_upper(*ch);
	}

	return result;
}

String String::to_lower() const
{
	String result = *this;

	for_every(ch, result.data)
	{
		*ch = to_lower(*ch);
	}

	return result;
}

bool String::load_lower_upper_pairs_from_xml(IO::XML::Node const * _node)
{
	static Concurrency::SpinLock loadLock = Concurrency::SpinLock(TXT("String.load_lower_upper_pairs_from_xml.loadLock"));
	Concurrency::ScopedSpinLock lock(loadLock);

	String lowerUpperPairs = _node->get_text();
	// remove all white spaces
	lowerUpperPairs = lowerUpperPairs.replace(String(TXT("\t")), String::empty());
	lowerUpperPairs = lowerUpperPairs.replace(String(TXT("\r")), String::empty());
	lowerUpperPairs = lowerUpperPairs.replace(String(TXT("\n")), String::empty());
	lowerUpperPairs = lowerUpperPairs.replace(String::space(), String::empty());
	for (int idx = 0; idx < lowerUpperPairs.get_length() - 1; idx += 2)
	{
		tchar lower = lowerUpperPairs.get_data()[idx];
		tchar upper = lowerUpperPairs.get_data()[idx + 1];
		(*lowerToUpperRemapper)[lower] = upper;
		(*upperToLowerRemapper)[upper] = lower;
	}

	return true;
}

void String::fix_length()
{
	for_every(ch, data)
	{
		if (*ch == 0)
		{
			data.set_size(for_everys_index(ch) + 1);
			break;
		}
	}
}

bool String::does_contain(tchar const * _text, tchar const * _hasToBeContained)
{
	if (!_text)
	{
		return false;
	}
	int textLength = (int)tstrlen(_text);
	int reqLength = (int)tstrlen(_hasToBeContained);

	tchar const * tCh = _text;
	for_count(int, i, textLength - reqLength + 1)
	{
		bool allMatch = true;
		tchar const * tChR = tCh;
		tchar const * rCh = _hasToBeContained;
		for_count(int, j, reqLength)
		{
			if (*tChR != *rCh)
			{
				allMatch = false;
				break;
			}
			++tChR;
			++rCh;
		}
		if (allMatch)
		{
			return true;
		}
		++tCh;
	}
	return false;
}

bool String::does_contain_icase(tchar const * _text, tchar const * _hasToBeContained)
{
	if (!_text)
	{
		return false;
	}
	int textLength = (int)tstrlen(_text);
	int reqLength = (int)tstrlen(_hasToBeContained);

	tchar const * tCh = _text;
	for_count(int, i, textLength - reqLength + 1)
	{
		bool allMatch = true;
		tchar const * tChR = tCh;
		tchar const * rCh = _hasToBeContained;
		for_count(int, j, reqLength)
		{
			tchar a[] = { *tChR, 0 };
			tchar b[] = { *rCh, 0 };
			if (! compare_icase(a, b))
			{
				allMatch = false;
				break;
			}
			++tChR;
			++rCh;
		}
		if (allMatch)
		{
			return true;
		}
		++tCh;
	}
	return false;
}

void String::cut_right_inline(int _by)
{
	int len = get_length();
	_by = min(_by, get_length());
	len -= _by;
	data[len] = 0;
	data.set_size(len + 1);
}

void String::keep_left_inline(int _keep)
{
	_keep = min(_keep, get_length());
	data[_keep] = 0;
	data.set_size(_keep + 1);
}

String String::pad_left(int _toLength) const
{
	int required = _toLength - get_length();
	if (required > 0)
	{
		return String::spaces(required) + *this;
	}
	return *this;
}

String String::pad_left_with(int _toLength, tchar _ch) const
{
	int required = _toLength - get_length();
	if (required > 0)
	{
		String result = String::spaces(required) + *this;
		for_count(int, i, required)
		{
			result[i] = _ch;
		}
		return result;
	}
	return *this;
}

String String::combine_lines(List<String> const& _lines)
{
	return combine_lines(_lines, new_line());
}

String String::combine_lines(List<String> const& _lines, String const & _newLineAs)
{
	String result;
	for_every(l, _lines)
	{
		if (!result.is_empty())
		{
			result += _newLineAs;
		}
		result += *l;
	}
	return result;
}

// reuse temp buffers
// it is highly unlikely we're going to be short on the buffers. we would have to have dozens of concurrent threads creating strings one after another
#define stringPrintfTempBuffers_count 64
#define stringPrintfTempBuffers_size 16384
tchar stringPrintfTempBuffers[stringPrintfTempBuffers_count][stringPrintfTempBuffers_size];
Concurrency::Counter stringPrintfTempBuffersIdx;

String String::printf(tchar const* const _format, ...)
{
	va_list list;
	va_start(list, _format);
	tchar* tempBuffer = stringPrintfTempBuffers[mod(stringPrintfTempBuffersIdx.increase(), stringPrintfTempBuffers_count)];
#ifndef AN_CLANG
	int textsize = (int)tstrlen(_format) + 1;
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _format, memsize);
	sanitise_string_format(format);
	tvsprintf(tempBuffer, stringPrintfTempBuffers_size - 1, format, list);
#else
	tvsprintf(tempBuffer, stringPrintfTempBuffers_size - 1, _format, list);
#endif
	return String(tempBuffer);
}

bool String::save_to_xml(IO::XML::Node* _node) const
{
	_node->add_text(*this);
	return true;
}
