#pragma once

#include "..\globalDefinitions.h"
#include "..\types\string.h"
#include "..\types\token.h"
#include "..\containers\list.h"
#include "..\endianUtils.h"

template <typename Element> struct Optional;

#ifdef AN_ANDROID
#include "android\asset_manager.h"
#endif
namespace IO
{
#ifdef AN_ANDROID
	AAssetManager* get_asset_manager();
#endif

	void initialise_static();
	void close_static();

	String const & get_main_directory();
	void go_to_main_directory();
	void go_to_directory(String const & _directory);

	String get_user_game_data_directory(); // empty for current but can be also external for android, etc
	String get_asset_data_directory(); // empty but used to know from where we want to read

	String get_auto_directory_name(); // just _auto

	void validate_auto_directory(); // make sure it is created for the right build number

	tchar const* get_directory_separator();

	String sanitise_filepath(String const& _filePath);
	String unify_filepath(String const& _filePath);
	
	String safe_filename(String const& _fileName);

	namespace InterfaceType
	{
		enum Type
		{
			Binary,
			UTF8,
			ANSI,

			Text = UTF8,
		};
	};

	class Interface
	{
			/**
			 *	Interface object that works as input/output
			 */
		public:
			//
			void set_type(InterfaceType::Type _type) { type = _type; }
			bool is_type_text() const { return type == InterfaceType::Text || type == InterfaceType::UTF8 || type == InterfaceType::ANSI; };

			// read/write one data using endian adjustment
			uint read_single(void * const _data, uint const _numBytes);
			uint write_single(void * const _data, uint const _numBytes);
			uint write_single(void const * const _data, uint const _numBytes);

			//
			uint read_into(Array<int8> & _array);
			uint read(void * const _data, uint const _numBytes);
			uint write(void const * const _data, uint const _numBytes);
			uint go_to(uint const _where);
			uint get_loc();
			uint read_into_string(OUT_ String & _string);
			uint read_into_string(OUT_ String & _string, Optional<int> const & _size);

			//
			uint read_lines(List<String> & _lines);

			//
			bool read_token(Token & _token, bool _readEmptySymbol = false);
			bool read_int(int & _value);
			bool read_float(float & _value);
			bool read_next_line();
			void clear_last_read_char() { hasLastChar = false; }

			//
			tchar const read_char(int * _lineNumber = nullptr); // read just one character
			uint omit_spaces(bool _startingWithLastRead = false, int * _lineNumber = nullptr);
			uint omit_spaces_and_special_characters(bool _startingWithLastRead = false, int * _lineNumber = nullptr);

			tchar const get_last_read_char() const { an_assert(is_type_text(), TXT("should be used only for text interfaces"));  return lastReadChar; }
			bool has_last_read_char() const { an_assert(is_type_text(), TXT("should be used only for text interfaces"));  return hasLastChar; }
			uint write_text(String const & _string);
			uint write_text(tchar const * const _text);
			uint write_text(char8 const * const _text);
			uint write_line(String const& _string) { int a = write_text(_string); int b = write_next_line(); return a + b; }
			uint write_line(tchar const * const _text) { int a = write_text(_text); int b = write_next_line(); return a + b; }
			uint write_line(char8 const * const _text) { int a = write_text(_text); int b = write_next_line(); return a + b; }
			uint write_line() { return write_next_line(); }
			uint write_next_line() { return write_text(TXT("\n")); }

		protected:
			// everything is saved as little endian and it would be great if those methods used adjust endian when reading/writing
			typedef uint (*ReadFunc)(Interface * io, void * const _data, uint const _numBytes);
			typedef uint (*WriteFunc)(Interface * io, void const * const _data, uint const _numBytes);
			typedef uint (*GoToFunc)(Interface * io, uint const _where);
			typedef uint (*GetLocFunc)(Interface * io);

			ReadFunc read_func;
			WriteFunc write_func;
			GoToFunc go_to_func;
			GetLocFunc get_loc_func;

			Interface(ReadFunc _read_func, WriteFunc _write_func, GoToFunc _go_to_func, GetLocFunc _get_loc_func);
			explicit Interface();

		private:
			// those work only if read_char was used
			tchar lastReadChar;
			bool hasLastChar;

			InterfaceType::Type type = InterfaceType::Binary;
	};

};

