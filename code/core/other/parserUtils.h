#pragma once

#include "../globalDefinitions.h"

struct String;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace ParserUtils
{
	bool parse_bool(tchar const * _value, bool _defValue = false);
	bool parse_bool_ref(tchar const * _value, REF_ bool & _refValue);
	bool parse_bool_out(tchar const * _value, OUT_ bool & _outValue);
	float parse_float(tchar const * _value, float _defValue = 0.0f);
	bool parse_float_ref(tchar const * _value, REF_ float & _refValue);
	int parse_int(tchar const * _value, int _defValue = 0);
	bool parse_int_ref(tchar const * _value, REF_ int & _refValue);
	int64 parse_int64(tchar const * _value, int64 _defValue = 0);
	bool parse_int64_ref(tchar const * _value, REF_ int64 & _refValue);
	uint parse_hex(tchar const * _value, uint _defValue = 0);
	bool parse_hex_ref(tchar const * _value, REF_ uint & _refValue);
	bool char_is_valid_word_letter(tchar _ch);
	bool char_is_valid_word_letter(char8 _ch);
	bool char_is_number(tchar _ch);
	bool char_is_number(char8 _ch);
	bool is_empty_char(tchar const _ch);
	bool is_empty_char(char8 const _ch);

	bool parse_bool(String const & _value, bool _defValue = false);
	bool parse_bool_ref(String const & _value, REF_ bool & _refValue);
	bool parse_bool_out(String const & _value, OUT_ bool & _outValue);
	float parse_float(String const & _value, float _defValue = 0.0f);
	bool parse_float_ref(String const & _value, REF_ float & _refValue);
	int parse_int(String const & _value, int _defValue = 0);
	int64 parse_int64(String const & _value, int64 _defValue = 0);
	bool parse_int_ref(String const & _value, REF_ int & _refValue);

	template <typename SuperType>
	bool parse_using_to_char(IO::XML::Node const* _node, tchar const* _attrOrChild, typename SuperType::Type& _value);
	template <typename SuperType>
	bool parse_using_to_char(String const & _val, typename SuperType::Type& _value);

	template <typename Container, typename Contained>
	bool load_multiple_from_xml_into(IO::XML::Node const* _node, tchar const* _attrOrChild, Container& _container);

	String single_to_hex(int _value);
	String void_to_hex(void* _value);
	String int_to_hex(int _value);
	String uint_to_hex(uint _value);
	String uint64_to_hex(uint64 _value);
	int hex_to_int(String const & _hex);
	uint hex_to_uint(String const & _hex);

	String int_to_bin(int _value);
	int bin_to_int(String const& _hex);

	String float_to_string_auto_decimals(float _value, int _limit = 8);
};
