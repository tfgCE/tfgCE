#pragma once

#include "io.h"

namespace IO
{
	/**
	 *	Asset files are available only on android
	 *	And it can only be read
	 */
#ifdef AN_ANDROID
	class AssetFile
	: public Interface
	{
	public:
		AssetFile(String const & _fileName);
		AssetFile();
		~AssetFile();

		static bool does_exist(String const & _fileName);
		static uint get_size_of(String const & _fileName);

		bool is_open() const;

		bool open(String const &); // open to read

		void close();

	protected:
		static uint read_data_from_asset(Interface * io, void * const _data, uint const _numBytes);
		static uint go_to_in_asset(Interface * io, uint const _newLoc);
		static uint get_loc_in_asset(Interface * io);

	private:
		enum Constants
		{
			bufSize = 131072 // 128kB 16384
		};

		// ---

		AAsset* asset = nullptr; // asset file handle

		byte buf[bufSize];
		byte* bufPtr; // pointer to buffer
		uint bufLen; // how much is left inside of buffer

		// ---

		uint read_buffer();
	};
#else
	class AssetFile
	: public Interface
	{
	public:
		AssetFile(String const& _fileName): Interface() {}
		AssetFile() {}
		~AssetFile() {}

		static bool does_exist(String const& _fileName) { return false; }
		static uint get_size_of(String const& _fileName) { return 0; }

		bool is_open() const { return false; }

		bool open(String const&) { return false; }
	};
#endif
};
