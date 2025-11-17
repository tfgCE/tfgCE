#include "localisedCharacters.h"

#include "localisedString.h"

#include "..\..\core\other\parserUtils.h"
#include "..\..\core\types\unicode.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_LOCALISED_CHARACTERS

//

using namespace Framework;

//

DEFINE_STATIC_NAME(ignore);

//

LocalisedCharacters* LocalisedCharacters::s_localisedCharacters = nullptr;

void LocalisedCharacters::initialise_static()
{
	an_assert(!s_localisedCharacters);
	s_localisedCharacters = new LocalisedCharacters();
}

void LocalisedCharacters::close_static()
{
	an_assert(s_localisedCharacters);
	delete_and_clear(s_localisedCharacters);
}

IO::Digested const& LocalisedCharacters::get_digested_source()
{
	an_assert(s_localisedCharacters);
	return s_localisedCharacters->digestedSource;
}

void LocalisedCharacters::update_from_localised_strings()
{
#ifdef DEBUG_LOCALISED_CHARACTERS
	output(TXT("[localised charaters] update from localised strings"));
#endif

	an_assert(s_localisedCharacters);

	if (s_localisedCharacters->ignoreSet != NONE)
	{
		for (int i = s_localisedCharacters->sets.get_size() - 1; i >= 0; --i)
		{
			if (s_localisedCharacters->sets[i].id != NAME(ignore))
			{
				s_localisedCharacters->sets.remove_fast_at(i);
			}
		}
	}
	else
	{
		s_localisedCharacters->sets.clear();
	}

	for_every(language, LocalisedStrings::s_localisedStrings->languages)
	{
		Name const languageId = language->id;
		Name const localisedCharacterId = language->localisedCharacterSetId;
		Optional<int> localisedCharacterPriority = language->localisedCharacterSetPriority;
		if (localisedCharacterId.is_valid())
		{
			s_localisedCharacters->process_string(language->name, localisedCharacterId, localisedCharacterPriority); // as displayed, to get its characters right
			for_every_ptr(se, LocalisedStrings::s_localisedStrings->strings)
			{
				if (auto* f = se->get_existing_form_for(languageId))
				{
					if (f->form)
					{
						s_localisedCharacters->process_string(f->form->gather_all_texts(), localisedCharacterId, localisedCharacterPriority);
					}					
				}
			}
		}
	}

	// sort all characters to be in Unicode order to avoid situations when different things are loaded in a bit different order and result in modified localised characters
	Array<uint32> asUnicode;
	for_every(lcset, s_localisedCharacters->sets)
	{
		asUnicode.clear();
		asUnicode.make_space_for(lcset->characters.get_length());
		for_every(ch, lcset->characters.get_data())
		{
			if (*ch == 0)
			{
				break;
			}
			asUnicode.push_back(Unicode::tchar_to_unicode(*ch));
		}
		sort(asUnicode, compare_int);
		tchar* dch = lcset->characters.access_data().begin();
		for_every(ch, asUnicode)
		{
			*dch = Unicode::unicode_to_tchar(*ch);
			++dch;
		}
	}

	sort(s_localisedCharacters->sets);

	// remove characters that already exist in a former set (because we use unified approach right now)
	// if using same font, chinese simplified, traditional, japanese and sometimes even korean, share same characters
	for_count(int, i, s_localisedCharacters->sets.get_size())
	{
		auto& lcset = s_localisedCharacters->sets[i];
		for (int chIdx = 0; chIdx < lcset.characters.get_length(); ++chIdx)
		{
			tchar ch = lcset.characters[chIdx];
			for_count(int, j, i)
			{
				auto const& prevSet = s_localisedCharacters->sets[j];

				if (prevSet.characters.does_contain(ch))
				{
					lcset.characters.access_data().remove_at(chIdx);
					--chIdx;
					break;
				}
			}
		}
	}
}

void LocalisedCharacters::process_string(String const& _text, Name const& _setId, Optional<int> const& _priority)
{
	Set* lcset = access_set(_setId, _priority);
	Set* ignoreSet = s_localisedCharacters->ignoreSet != NONE ? &s_localisedCharacters->sets[s_localisedCharacters->ignoreSet] : nullptr;
	
	for_every(ch, _text.get_data())
	{
		if (*ch < 127 ||
			*ch == String::no_break_space_char())
		{
			continue;
		}

		if (ignoreSet &&
			ignoreSet->characters.does_contain(*ch))
		{
			continue;
		}

		bool alreadyInOtherSet = false;
		for_every(ls, sets)
		{
			if (ls->priority <= lcset->priority)
			{
				continue;
			}
			if (ls->characters.does_contain(*ch))
			{
				alreadyInOtherSet = true;
				break;
			}
		}
		if (alreadyInOtherSet)
		{
			continue;
		}

		if (!lcset->characters.does_contain(*ch))
		{
#ifdef DEBUG_LOCALISED_CHARACTERS
			output(TXT("[localised charaters] add \"%c\" to \"%S\""), *ch, lcset->id.to_char());
#endif
			lcset->characters += *ch;
		}
	}
}

bool LocalisedCharacters::load_from(IO::Interface * _io)
{
	bool result = true;

	an_assert(s_localisedCharacters);

	s_localisedCharacters->sets.clear();

	s_localisedCharacters->digestedSource.digest(_io);

	List<String> lines;
	_io->read_lines(lines);

	s_localisedCharacters->ignoreSet = NONE;

	int linesCount = lines.get_size();
	int setsCount = linesCount / 2;
	for (int i = 0; i < setsCount; ++ i)
	{
		s_localisedCharacters->sets.grow_size(1);
		auto& lcSet = s_localisedCharacters->sets.get_last();
		{
			String const& l = lines[i * 2];
			int bAt = l.find_last_of(':');
			if (bAt != NONE)
			{
				lcSet.id = Name(l.get_left(bAt));
				lcSet.priority = ParserUtils::parse_int(l.get_sub(bAt + 1));
			}
			else
			{
				lcSet.id = Name(l);
				lcSet.priority = 0;
			}
		}
		lcSet.characters = lines[i * 2 + 1];
		if (lcSet.id == NAME(ignore))
		{
			s_localisedCharacters->ignoreSet = i;
		}
	}
	
	output(TXT("loaded localised characters, set count %i"), s_localisedCharacters->sets.get_size());
	for_every(lcset, s_localisedCharacters->sets)
	{
		output(TXT("  set %i:%S, char count:%i"), for_everys_index(lcset), lcset->id.to_char(), lcset->characters.get_length());
		//output(TXT("    \"%S\""), lcset->characters.to_char());
	}

	return result;
}

bool LocalisedCharacters::save_to(IO::Interface * _io)
{
	bool result = true;

	an_assert(s_localisedCharacters);

	for_every(lcset, s_localisedCharacters->sets)
	{
		_io->write_line(String::printf(TXT("%S:%i"), lcset->id.to_string().to_char(), lcset->priority));
		_io->write_line(lcset->characters);
	}

	s_localisedCharacters->digestedSource.digest(_io);

	return result;
}

String const& LocalisedCharacters::get_set(Name const& _id)
{
	an_assert(s_localisedCharacters);

	for_every(lcset, s_localisedCharacters->sets)
	{
		if (lcset->id == _id)
		{
			return lcset->characters;
		}
	}
	
	return String::empty();
}

LocalisedCharacters::Set* LocalisedCharacters::access_set(Name const& _id, Optional<int> const& _priority)
{
	for_every(lcs, s_localisedCharacters->sets)
	{
		if (lcs->id == _id)
		{
			if (_priority.is_set())
			{
				lcs->priority = max(lcs->priority, _priority.get());
			}
			return lcs;
		}
	}

	s_localisedCharacters->sets.grow_size(1);
	Set* lcset = &s_localisedCharacters->sets.get_last();
	lcset->id = _id;
	lcset->priority = _priority.get(0);

	return lcset;
}

//

int LocalisedCharacters::Set::compare(void const* _a, void const* _b)
{
	Set const* a = (Set const*)(_a);
	Set const* b = (Set const*)(_b);
	if (a->id == NAME(ignore)) return A_BEFORE_B;
	if (b->id == NAME(ignore)) return B_BEFORE_A;
	if (a->priority > b->priority) return A_BEFORE_B;
	if (a->priority < b->priority) return B_BEFORE_A;
	return A_AS_B;
}

