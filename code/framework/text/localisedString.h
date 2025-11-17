#pragma once

#include "../../core/containers/map.h"
#include "../../core/types/name.h"
#include "../../core/types/optional.h"
#include "../../core/types/string.h"

#include "stringFormatterElement.h"

#include "../library/libraryLoadLevel.h"
#include "../library/usedLibraryStored.h"

namespace IO
{
	class File;
	namespace XML
	{
		class Node;
	};
};

#define todo_localise() /* localise this! */

#define AS_LOC_STR(_id) ::Framework::LocalisedString(_id, true)
#define LOC_STR(_id) ::Framework::LocalisedString(_id, true).get()

#ifdef AN_WINDOWS
	#ifdef BUILD_NON_RELEASE
		#define AN_STORE_LOCALISED_CHARS
	#endif
#endif

namespace Framework
{
	struct LibraryLoadingContext;
	struct LocalisedCharacters;
	struct LocalisedStringEntry;
	struct LocalisedStringForm;
	struct StringFormatterParams;
	class LocalisedStrings;
	class StringFormatter;
	class TexturePart;

	struct LocalisedStringRequestedForm
	{
		static tchar const* form_auto() { return TXT("auto"); }
		static bool is_form_auto(tchar const* _form) { return String::compare_icase(_form, form_auto()); }

		LocalisedStringRequestedForm() {}
		explicit LocalisedStringRequestedForm(int _number) : numberI(_number) {}
		explicit LocalisedStringRequestedForm(tchar const * _form, int _number = 1) : form(_form), numberI(_number) {}
		explicit LocalisedStringRequestedForm(float _number) : numberF(_number) {}
		explicit LocalisedStringRequestedForm(tchar const * _form, float _number) : form(_form), numberF(_number) {}

		LocalisedStringRequestedForm& with_params(StringFormatterParams & _params) { params = &_params; return *this; }
		tchar const * form = nullptr;
		StringFormatterParams * params = nullptr;
		int numberI = 1;
		Optional<float> numberF;
	};

	struct LocalisedString
	{
	public:
		static LocalisedString none;

		LocalisedString();
		LocalisedString(LocalisedString const& _ls);
		LocalisedString(Name const & _id, bool _onlyExisting = false);
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, bool _loadId = true);
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _id, bool _loadId = true);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _fullId, bool _loadId = true);
		bool load_from_xml_child(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc, tchar const * _id, bool _loadId = true);
		bool load_from_xml_child_required(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc, tchar const * _id, bool _loadId = true);
		bool load_from_xml_child(IO::XML::Node const * _node, tchar const * _childName, tchar const * _fullId, bool _loadId = true);
		bool load_from_xml_child_required(IO::XML::Node const * _node, tchar const * _childName, tchar const * _fullId, bool _loadId = true);
		String const & get(LocalisedStringRequestedForm const & _form = LocalisedStringRequestedForm()) const;
		LocalisedStringForm const * get_form(LocalisedStringRequestedForm const & _form = LocalisedStringRequestedForm()) const;
		String const & get_form_for_format_custom() const;

		LocalisedString& operator=(LocalisedString const& _other);

		bool is_valid() const { return id.is_valid(); }
		Name const & get_id() const { return id; }

		LocalisedStringEntry* get_entry() const { return entry; }

	public:
		bool serialise(Serialiser & _serialiser);

	private:
		Name id;
		LocalisedStringEntry* entry;
#ifdef AN_DEVELOPMENT
		mutable Concurrency::SpinLock errorInfoLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
		mutable String errorInfo;
#endif
	};

	struct LocalisedStringForm
	{
		LocalisedStringForm();
		~LocalisedStringForm();

		bool is_empty() const { return value.is_empty() && subForms.is_empty(); }

		void add_to_tsv(IO::File * _f, int _libraryLoadLevel, String const & _id = String::empty(), LocalisedStringForm const * _sourceForm = nullptr, bool _toTranslate = false) const;
		void add_to_txt(IO::File * _f) const;
		void add_to_xml(IO::XML::Node * _node) const;
		bool load_from_xml(IO::XML::Node const * _node, bool _main = true);
		
		String const & get(Name const& _stringId, LocalisedStringRequestedForm const & _form) const;
		LocalisedStringForm const * get_form(Name const& _stringId, LocalisedStringRequestedForm const & _form) const; // _forString only used as an information
		String const & get_form_for_format_custom() const { return form; }

		Array<StringFormatterElement> const & get_string_formatter_elements() const { return stringFormatterElements; }

		String gather_all_texts() const; // for localised characters and for "source" info
		String gather_all_texts_for_hash() const; // this is to catch changes, includes both value and valueToTranslate
		
	private: friend struct LocalisedStringEntry;
		LocalisedStringForm* create_copy();

	private: friend class LocalisedStrings;
#ifdef BUILD_NON_RELEASE
		   bool justTranslated = false;
#endif

	private:
		// requiredForms are translated into number condition when they start with number (remember about initial ~)
		// all operators are combined together one after another, therefore a number condition may look like this:
		// ~number%10>=2<=4 for all numbers that end with 2, 3 or 4
		struct NumberCondition
		{
			bool negateParam = false;
			bool absoluteParam = false;
			bool percentageParam = false;
			Optional<float> equal;
			Optional<float> less;
			Optional<float> lessOrEqual;
			Optional<float> greater;
			Optional<float> greaterOrEqual;
			Optional<float> modulo;

			bool is_ok(int _number) const;
			bool is_ok(float _number) const;
		};
		Array<LocalisedStringForm*> subForms; // sub forms

		bool isMain = false; // for main form is form of whole thing (so it can be used by other words, eg. "kobieta" may have form "rodzaj_zenski")
		String value; // actual value, this might be word, sentence or string format, eg. "Hello %character~firstName%"
		String valueToTranslate; // optional, will be used in translation
#ifdef BUILD_NON_RELEASE
		String contextInfo; // for the translator
#endif
		Array<StringFormatterElement> stringFormatterElements; // extracted with string formatter
		String form; // format parameters, eg. ~masculinum
		Array<String> requiredForms; // has to match all of them, number conditions are removed (not used for main)
		Name numberParam; // override_ for numberConditions;
		Name hasParam; // has parameter defined
		Array<NumberCondition> numberConditions; // (not used for main) (start with "number", eg. "number>=4" if value provided is greater or equal to 4

#ifdef AN_DEVELOPMENT
		mutable Concurrency::SpinLock errorInfoLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
		mutable String errorInfo;
#endif

		void break_format(String const & _format, Array<String> & _formats);
		bool parse_number_conditions(IO::XML::Node const * _node);

		friend class StringFormatter;
	};

	struct LocalisedStringEntry
	{
	public:
		static int const DEFAULT_PRIORITY = 0; // and lowest at the same time

	public:
		LocalisedStringEntry();
		~LocalisedStringEntry();
		bool load_from_xml(IO::XML::Node const * _node, Name const & _language);
		bool load_default_from_xml(IO::XML::Node const * _node); // only if default was not loaded with "load_from_xml"

		String const & get(LocalisedStringRequestedForm const & _format = LocalisedStringRequestedForm()) const;
		LocalisedStringForm const * get_form(LocalisedStringRequestedForm const & _form) const;
		LocalisedStringForm const * get_main_form() const { return form; }
		String const & get_form_for_format_custom() const;

		bool update_for_language(Name const & _language, bool _mayFail = false);
		bool update_for_language(Name const & _language, int _languageIdx, bool _mayFail = false);

		bool is_empty() const { return !form || form->is_empty(); }

		bool does_require_translation_to(Name const& _language) const;
		void translate_to(Name const& _language, bool _onlyUpdateHashes = false);

	private:
		Concurrency::SpinLock loadLock = Concurrency::SpinLock(TXT("Framework.LocalisedStringEntry.loadLock"));

		typedef uint32 FormHash;

		struct LanguageForm
		{
			Name language;
#ifdef BUILD_NON_RELEASE
			FormHash formHash; // for default language this is form's hash, for non-default this is what was used to auto translate
#endif
			LocalisedStringForm* form = nullptr;
		};

		LibraryLoadLevel::Type libraryLoadLevel = 0;
		Name id;
		Name forLanguage;
		int priority = DEFAULT_PRIORITY - 1; // if lower, means empty and allow loading
#ifdef BUILD_NON_RELEASE
		bool noAutoTranslation = false; // means that we're still going to create an entry, it will be just the same
#endif
		LocalisedStringForm const * form = nullptr; // value for current language
		String formReplacement;
		Array<LanguageForm> languageForms; // values for all languages, index is language index

		static FormHash calculate_hash_for(String const& _string);

		LanguageForm const* get_existing_form_for(Name const& _language) const;
		LanguageForm const* get_existing_form_for(int _languageIdx) const;
		LanguageForm& access_form_for(Name const& _language);

		void update_library_load_level();

		friend struct LocalisedCharacters;
		friend struct LocalisedString;
		friend class LocalisedStrings;
	private:
		LocalisedStringEntry(LocalisedStringEntry const & _source);
		LocalisedStringEntry & operator = (LocalisedStringEntry const & _source);
	};

	struct LocalisedStringTexturePart;

	class LocalisedStrings
	{
	public:
		struct Language
		{
			Name id;
			Name localisedCharacterSetId;
			String sortId;
			Optional<int> localisedCharacterSetPriority;
			String name; // as presented, it should be in the language
			Optional<float> variableSpaceWidthPt;
		};
		struct SortedLanguageLookup
		{
			int idx;
			// taken from Language
			Name id;
			String sortId;
			String name;

			static inline int compare(void const* _a, void const* _b)
			{
				SortedLanguageLookup const & a = *plain_cast<SortedLanguageLookup>(_a);
				SortedLanguageLookup const & b = *plain_cast<SortedLanguageLookup>(_b);
				return String::compare_icase_sort(&a.sortId, &b.sortId);
			}
		};
		static void initialise_static();
		static void close_static();

		static void drop_assets();

		static int get_langauge_change_count();
		static void set_language(Name const & _language, bool _force = false);
		static Name const & get_language();
		static Name const & get_default_language();
		static Name const & get_system_language();
		static Name const & get_suggested_default_language(); // only if suggested is different to default

		static Array<Language> const & get_languages() { return s_localisedStrings->languages; }
		static Array<SortedLanguageLookup> get_sorted_languages();
		static Language * access_language(Name const & _id, bool _onlyExisting);
		static int add_language(Name const & _id);
		static int find_language_index(Name const & _id);

		static LocalisedStringEntry* get_entry(Name const & _id, bool _onlyExisting = false, bool _loading = false);
		static TexturePart* get_texture_part(Name const & _id, Name const & _languageToLoad);

		static bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		static bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		static bool unload(LibraryLoadLevel::Type _libraryLoadLevel);

		static void save_to_file(String const & _fileName, Optional<LibraryLoadLevel::Type> const& _libraryLoadLevel = NP, Optional<bool> const& _withSource = NP, Optional<bool> const& _breakIntoParts = NP, Optional<bool> const& _simpleHeader = NP, Optional<bool> const & _andReload = NP);
		static void save_all_languages_to_dir(String const & _dirName);
		static void save_language_to_dir(String const& _dirName, Name const& _language, bool _andReload = false);
		static bool save_language_to_tsv(IO::File * _f, Optional<LibraryLoadLevel::Type> const & _libraryLoadLevel = NP);
		static bool save_language_to_txt(IO::File * _f, Optional<LibraryLoadLevel::Type> const & _libraryLoadLevel = NP);
		static bool save_language_to_xml(IO::XML::Node * _node, Optional<LibraryLoadLevel::Type> const & _libraryLoadLevel = NP, Optional<bool> const& _withSource = NP, Optional<bool> const& _breakIntoParts = NP, Optional<bool> const& _simpleHeader = NP);
		
		static bool load_all_languages_from_dir_tsv_and_save(String const& _inputDirName, String const& _outputDirName); // returns true if somethign got imported

		static void translate_all_requiring(String const& _dirName, bool _onlyUpdateHashes = false);

	private:
		static LocalisedStrings* s_localisedStrings;

		Concurrency::SpinLock stringsLock = Concurrency::SpinLock(TXT("Framework.LocalisedStrings.stringsLock"));
		Concurrency::SpinLock languagesLock = Concurrency::SpinLock(TXT("Framework.LocalisedStrings.languagesLock"));

		Name language; // currently selected language
		int languageChangeCount = 0;
		Array<Language> languages;
		Map<Name, LocalisedStringEntry*> strings;
		List<LocalisedStringTexturePart> textureParts;

		~LocalisedStrings();

		static bool load_entries_from_xml(IO::XML::Node const * _node, Name const & _languageToLoad, LibraryLoadingContext & _lc);

		static LocalisedStringTexturePart* get_texture_part_to_load(Name const & _id, Name const & _languageToLoad);

		friend struct LocalisedString;
		friend struct LocalisedStringEntry;
		friend struct LocalisedCharacters;
	};

};
