#pragma once

#include "..\globalDefinitions.h"
#include "..\memory\memory.h"
#include "..\debug\detectMemoryLeaks.h"

// .inl
#include "..\utils.h"
#include "..\debug\debug.h"

#define TOKEN_LENGTH 128

struct Token
{
public:
	static Token const & empty() { return c_empty; }

	inline Token();
	inline Token(Token const &);
	inline explicit Token(tchar const * const);

	inline int get_length() const { return length; }
	inline void clear() { length = 0; }

	inline tchar const * const to_char() const { return &data[0]; }
	inline tchar & operator [] (uint _index) { return data[_index]; }
	inline tchar const & operator [] (uint _index) const { return data[_index]; }
	inline bool is_empty() const { return length == 0; }

	inline int find_first_of(Token const & _str, int _startAt = 0) const; // NONE if not found
	inline int find_first_of(tchar const _ch, int _startAt = 0) const; // NONE if not found
	inline int find_last_of(tchar const _ch) const; // NONE if not found

	inline Token const & operator += (tchar const _char);

	inline static bool compare_icase(Token const & _a, Token const & _b);
	inline static bool compare_icase(Token const & _a, tchar const * const & _b);

	inline bool operator == (Token const & _other) const;
	inline bool operator == (tchar const * const _text) const;

	inline bool operator != (Token const & _other) const;
	inline bool operator != (tchar const * const _text) const;

	inline bool operator < (Token const & _other) const;

	inline Token const & operator = (Token const & _other);
	inline Token const & operator = (tchar const * const _text);

private:
	static Token const c_empty;

	uint length;
	tchar data[TOKEN_LENGTH + 1];
};

#include "token.inl"
