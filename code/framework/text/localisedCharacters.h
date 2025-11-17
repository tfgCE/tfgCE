#pragma once

#include "..\..\core\types\name.h"
#include "..\..\core\types\string.h"
#include "..\..\core\io\digested.h"

template <typename Element> struct Optional;

namespace IO
{
	interface_class Interface;
};

namespace Framework
{
	// this structure is used on demand after the library has been fully loaded (!)
	// updates file where it stores used characters per font
	struct LocalisedCharacters
	{
		static LocalisedCharacters* get() { return s_localisedCharacters; }
		static void initialise_static();
		static void close_static();

		static void update_from_localised_strings(); // goes through all localised strings and recreates sets
		
		static bool load_from(IO::Interface * _io);
		static bool save_to(IO::Interface* _io);

		static IO::Digested const& get_digested_source();

		static String const& get_set(Name const & _id);

		LocalisedCharacters() {}
		~LocalisedCharacters() {}

	private:
		static LocalisedCharacters* s_localisedCharacters;
		struct Set
		{
			Name id; // each language will put into a specific id, also font generators will use designated id
			int priority = 0; // ignore has top priority always

			String characters;

			static int compare(void const* _a, void const* _b);
		};

		Array<Set> sets;

		IO::Digested digestedSource; // when loaded or when saved

		int ignoreSet = 0; // set with ignorable characters, hand filled

		Set* access_set(Name const& _id, Optional<int> const& _priority);

		void process_string(String const& _text, Name const & _setId, Optional<int> const & _priority);

	};

};
