#pragma once

#include "..\debug\detectMemoryLeaks.h"
#include "..\containers\array.h"
#include "..\containers\list.h"
#include "..\math\mathUtils.h"
#include "..\other\registeredType.h"
#ifdef AN_CLANG
#include "..\other\registeredType.h"
#endif

// .inl
#include "..\casts.h"
#include "..\utils.h"

class Serialiser;

template <typename Element> class List;

inline void sanitise_string_format(tchar* _format)
{
#ifndef AN_CLANG
	tchar* ch = _format;
	if (*ch != 0)
	{
		tchar* nch = _format + 1;
		while (*nch != 0)
		{
			if (*ch == '%' &&
				*nch == 'S')
			{
				*nch = 's';
			}
			++ch;
			++nch;
		}
	}
#else
#ifdef AN_DEVELOPMENT
	tchar* ch = _format;
	if (*ch != 0)
	{
		tchar* nch = _format + 1;
		while (*nch != 0)
		{
			if (*ch == '%' &&
				*nch == 's')
			{
				error_stop(TXT("string should have capital S as a param!"));
			}
			++ch;
			++nch;
		}
	}
#endif
#endif
}

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

struct String
{
public:
	static tchar no_break_space_char() { return 160; }

	static String const & empty() { an_assert(s_empty); return *s_empty; }
	static String const & space() { an_assert(s_space); return *s_space; }
	static String const & no_break_space() { an_assert(s_noBreakSpace); return *s_noBreakSpace; }
	static String const & comma() { an_assert(s_comma); return *s_comma; }
	static String const & something() { an_assert(s_something); return *s_something; }
	static String const & degree() { an_assert(s_degree); return *s_degree; }
	static String const & new_line() { an_assert(s_newLine); return *s_newLine; }
	static String spaces(int _amount);
	
	static void use_degree(String const& _degree) { an_assert(s_degree); *s_degree = _degree; }

	static void initialise_static();
	static void close_static();

	static String* create_new();

	static bool load_lower_upper_pairs_from_xml(IO::XML::Node const * _node);

	inline String();
	inline static String single(tchar const _ch);
	inline static String of_length(uint const _length);
	inline String(String const &);
	inline explicit String(tchar const * const);

	template<typename AnyChar>
	inline static String convert_from(AnyChar* _in);
	template<typename AnyChar>
	inline void convert_to(AnyChar* _out, int _maxLength) const;
	template<typename AnyChar>
	inline bool is_same_as(AnyChar const * _other) const;

	inline int get_length() const { return data.get_size() - 1; }
	inline Array<tchar> const & get_data() const { return data; }
	inline Array<tchar> & access_data() { return data; }

	static String from_char8(char8 const * _text);
	static String from_char8_utf(char8 const * _text);
	Array<char8> to_char8_array() const;
	Array<char8> to_char8_utf_array() const;
	void to_char8_array(OUT_ REF_ Array<char8>& asChar8) const;

	inline tchar const * const to_char() const { return &data[0]; }
	inline tchar & operator [] (uint _index) { return data[_index]; }
	inline tchar const & operator [] (uint _index) const { return data[_index]; }
	inline bool is_empty() const { return data.get_size() <= 1; }

	static String printf(tchar const * const _format, ...);

	inline void change_no_break_spaces_into_spaces(); // in this string
	inline void change_consecutive_spaces_to_no_break_spaces(); // in this string

	/*
	inline float const GetAsFloat() const;
    inline int const GetAsInt() const;
	*/
	inline String get_left(uint _howMany) const;
	inline String get_right(uint _howMany) const;
	inline String get_sub(int _begin, int _howMany = NONE) const;
	void keep_left_inline(int _keep);
	void cut_right_inline(int _by);
	/*
	inline void Format(tchar const * const _format, ...);
	*/

	String first_to_upper() const;
	String to_upper() const;
	String to_lower() const;
	static tchar to_upper(tchar ch);
	static tchar to_lower(tchar ch);

	static String combine_lines(List<String> const& _lines);
	static String combine_lines(List<String> const& _lines, String const& _newLineAs);

	int count(tchar ch) const;
	bool does_contain(tchar ch) const;
	void split(String const & _splitter, List<String> & _tokens, int _howManySplits = 0) const;
	String replace(String const & _replace, String const & _with) const;
	bool replace_inline(String const & _replace, String const & _with);
	String keep_only_chars(String const & _charsToKeep) const;
	String keep_only_chars(tchar const * _charsToKeep) const;
	String compress() const; // remove double spaces, end chars, tabs
	String prune() const; // remove all spaces, end chars, tabs
	String trim() const;
	String trim_right() const;
	String pad_left(int _toLength) const;
	String pad_left_with(int _toLength, tchar _ch) const;
	String nice_sentence(int _formatParagraphs = NONE) const; // compress, remove or add required spaces, _formatParagraphs means how many empty lines do we want to have when handling ~, if 0 it will be just one ~, if 1, we allow ~~, if 2->~~~ and so on

	void fix_length(); // fix length in case 0 is not at the end

	inline bool has_prefix(String const & _prefix) const;
	inline bool has_prefix(tchar const * _prefix) const;
	inline String without_prefix(String const & _prefix) const;
	inline String without_prefix(tchar const * _prefix) const;

	inline bool has_suffix(String const & _suffix) const;
	inline bool has_suffix(tchar const * _suffix) const;

	inline bool does_contain(String const & _hasToBeContained) const { return does_contain(_hasToBeContained.to_char()); }
	inline bool does_contain_icase(String const & _hasToBeContained) const { return does_contain_icase(_hasToBeContained.to_char()); }
	inline bool does_contain(tchar const* _hasToBeContained) const { return does_contain(to_char(), _hasToBeContained); }
	inline bool does_contain_icase(tchar const* _hasToBeContained) const { return does_contain_icase(to_char(), _hasToBeContained); }
	static bool does_contain(tchar const * _text, tchar const * _hasToBeContained);
	static bool does_contain_icase(tchar const * _text, tchar const * _hasToBeContained);

	inline int find_first_of(String const & _str, int _startAt = 0) const; // NONE if not found
	inline int find_first_of(tchar const _ch, int _startAt = 0) const; // NONE if not found
	inline int find_last_of(tchar const _ch, int _startAt = NONE) const; // NONE if not found

	inline String operator + (String const & _other) const;
	inline String operator + (tchar const * const _text) const;
	inline String operator + (tchar const _char) const;
	inline String const & operator += (String const & _other);
	inline String const & operator += (tchar const * const _text);
	inline String const & operator += (tchar const _char);

	inline static bool compare_icase(String const & _a, String const & _b);
	inline static bool compare_icase(String const & _a, tchar const * const _b);
	inline static bool compare_icase(tchar const * const _a, tchar const * const _b);
	inline static bool compare_icase_left(tchar const * const _a, tchar const * const _b, int _firstCharactersCount); // both have to have at least this amount of characters

	inline static int diff_case(String const & _a, String const & _b);
	inline static int diff_icase(String const & _a, String const & _b);
	inline static int diff_case(tchar const * _a, tchar const* _b);
	inline static int diff_icase(tchar const* _a, tchar const* _b);
	inline static int compare_case_sort(void const * _a, void const * _b);
	inline static int compare_icase_sort(void const * _a, void const * _b);
	inline static int compare(void const * _a, void const * _b) { return compare_icase_sort(_a, _b); }
	inline static int compare_tchar_case_sort(void const * _a, void const * _b);
	inline static int compare_tchar_icase_sort(void const * _a, void const * _b);
	inline static int compare_tchar(void const * _a, void const * _b) { return compare_tchar_icase_sort(_a, _b); }

	inline bool operator == (String const & _other) const;
	inline bool operator == (tchar const * const _text) const;

	inline bool operator != (String const & _other) const;
	inline bool operator != (tchar const * const _text) const;

	inline bool operator < (String const & _other) const;

	inline String const & operator = (String const & _other);
	inline String const & operator = (tchar const * const _text);

	bool serialise(Serialiser & _serialiser);

	bool save_to_xml(IO::XML::Node* _node) const;

private:
	static String* s_empty;
	static String* s_space;
	static String* s_noBreakSpace;
	static String* s_comma;
	static String* s_something;
	static String* s_degree;
	static String* s_newLine;

	Array<tchar> data;
};

#include "string.inl"

DECLARE_REGISTERED_TYPE(String);

template <typename Type>
inline String as_string(Type const& _value)
{
	return _value.to_string();
}

