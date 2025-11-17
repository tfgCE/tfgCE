#pragma once

#include "io.h"

namespace IO
{
	class File
	: public Interface
	{
	public:
		File(String const & _fileName);
		File();
		~File();

		File* temp_ptr() { return this; }

		static bool does_exist(String const & _fileName);
		static uint get_size_of(String const & _fileName);

		static void rename_file(String const& _from, String const& _to);
		static void delete_file(String const& _file);
		void copy_to_file(String const& _file); // copies content to a different file

		bool is_open() const;

		bool open(String const &); // open to read
		bool open_to_write(String const &); // open existing to write
		bool create(String const &); // create new to write
		bool append_to(String const &); // creates new or appends to existing

		void close();

		void flush() { if (isWriting) write_buffer(); }

	protected:
		static uint read_data_from_file(Interface * io, void * const _data, uint const _numBytes);
		static uint write_data_to_file(Interface * io, void const * const _data, uint const _numBytes);
		static uint go_to_in_file(Interface * io, uint const _newLoc);
		static uint get_loc_in_file(Interface * io);

	private:
		enum Constants
		{
			noFile = -1,
			bufSize = 131072 // 128kB 16384
		};

		// ---

		int file; // file handle

		bool isWriting;	// is it writing now

		byte buf[bufSize];
		byte* bufPtr; // pointer to buffer
		uint bufLen; // how much is left inside of buffer

		// ---

		uint read_buffer();
		uint write_buffer();
	};
};
