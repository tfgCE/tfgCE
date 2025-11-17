#include "localisedString.h"

#include "stringFormatter.h"

#include "..\library\library.h"
#include "..\library\libraryLoadingContext.h"
#include "..\library\libraryPrepareContext.h"
#include "..\library\usedLibraryStored.inl"
#include "..\video\texturePart.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\io\file.h"
#include "..\..\core\io\json.h"
#include "..\..\core\io\xml.h"
#include "..\..\core\math\math.h"
#include "..\..\core\other\parserUtils.h"
#include "..\..\core\serialisation\serialiser.h"
#include "..\..\core\system\core.h"
#include "..\..\core\types\unicode.h"

#include <array>
#include <cstdio>
#include <iostream>
#include <string>

#ifdef AN_ANDROID
#include "..\..\core\system\android\androidApp.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

// if not set, will clear manual and replace "auto" with manual
#define KEEP_AUTO_MANUAL_TRANSLATIONS

// when importing tsv file and updating, will fill actual text and context info
#define UPDATE_TEXT_AND_CONTEXT_IN_TSV

//#define OUTPUT_SAVING_LANGUAGE

// use this macro to force update hash for already translated sequences - use in case of changing hash
//#define UPDATE_FOR_HASH_FROM_SOURCE

//

using namespace Framework;

//

// this is used for all the texts loaded from the files
// this should never be changed unless the default language for a game is going to be different than English
#define DEFAULT_LANGUAGE TXT("en")

// suggested languages
// to use a different language by an app, use suggested default language - if it is provided, it will override system language as well
#define SUGGESTED_DEFAULT_LANGUAGE_CHINA TXT("zh-CN")

//

DEFINE_STATIC_NAME_STR(defaultLanguage, DEFAULT_LANGUAGE);
DEFINE_STATIC_NAME_STR(suggestedDefaultLanguageChina, SUGGESTED_DEFAULT_LANGUAGE_CHINA);

//

namespace TSVColums
{
	enum Type
	{
		LLL,
		ID,
		ContextInfo,
		NumberParam,
		HasParam,
		Text,
		TextToTranslate,
		AutoTranslated,
		ManuallyTranslated,
		Comment,

		RequiredCount = ManuallyTranslated + 1
	};
};

//

#ifdef AN_WINDOWS
#ifdef BUILD_NON_RELEASE
bool g_localisedStringsBeingTranslated = false;
#endif
#endif

struct Framework::LocalisedStringTexturePart
{
	Name id;
	Name forLanguage;
	UsedLibraryStored<TexturePart> texturePart;
};

//

LocalisedString LocalisedString::none;

LocalisedString::LocalisedString()
: entry(nullptr)
{
}

LocalisedString::LocalisedString(LocalisedString const& _ls)
: id(_ls.id)
, entry(_ls.entry)
{
}

LocalisedString::LocalisedString(Name const & _id, bool _onlyExisting)
: id(_id)
, entry(nullptr)
{
	if (id.is_valid())
	{
		entry = LocalisedStrings::get_entry(id, _onlyExisting);
	}
	else
	{
		entry = nullptr;
	}
}

LocalisedString& LocalisedString::operator=(LocalisedString const& _other)
{
	id = _other.id;
	entry = _other.entry;

	// don't copy lock and errorInfo, no point in doing so

	return *this;
}

bool LocalisedString::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _id, bool _loadId)
{
	return load_from_xml(_node, ForLibraryLoadingContext(_lc).push_id(_id), _loadId);
}

bool LocalisedString::load_from_xml(IO::XML::Node const * _node, tchar const * _fullId, bool _loadId)
{
	bool result = true;
	if (_node && _node->get_string_attribute(TXT("translation")) == TXT("required"))
	{
		// skip
		// don't load it
	}
	else
	{
		Concurrency::ScopedSpinLock lock(LocalisedStrings::s_localisedStrings->stringsLock, true);
		if (_fullId)
		{
			id = Name(_fullId);
		}
		if (_node)
		{
			int priority = _node->get_int_attribute(TXT("priority"), LocalisedStringEntry::DEFAULT_PRIORITY);
			error_loading_xml_on_assert(priority >= LocalisedStringEntry::DEFAULT_PRIORITY, result, _node, TXT("priority cannot be lower than %i"), LocalisedStringEntry::DEFAULT_PRIORITY);
			if (_loadId)
			{
				id = _node->get_name_attribute(TXT("id"), id);
			}
			if (id.is_valid())
			{
#ifdef AN_DEVELOPMENT
				if (LocalisedStringEntry* e = LocalisedStrings::get_entry(id, true, true))
				{
					if (auto* lfExisting = e->get_existing_form_for(NAME(defaultLanguage)))
					{
						if (lfExisting->form && !lfExisting->form->is_empty())
						{
							if (priority == e->priority)
							{
								if (e->get() != _node->get_text() && ! _node->get_text().is_empty())	// a naive check - if they're the same, just let them be as they are
								{
									warn(TXT("such string (with this priority) already exist, it could be loaded twice or some id is missing and we're loading into same string!\n%S\nfile: %S"), id.to_char(), _node->get_document_info()->get_loaded_from_file().to_char());
								}
							}
						}
					}
				}
#endif
				{
					entry = LocalisedStrings::get_entry(id);
					if (priority >= entry->priority)
					{
						entry->load_default_from_xml(_node);
						entry->priority = max(priority, entry->priority);
					}
				}
			}
			else
			{
				error_loading_xml(_node, TXT("no id provided for localised string"));
				result = false;
			}
		}
		else
		{
			if (id.is_valid())
			{
				entry = LocalisedStrings::get_entry(id);
			}
		}
	}
	return result;
}

bool LocalisedString::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, bool _loadId)
{
	return load_from_xml(_node, _lc.build_id().to_char(), _loadId);
}

bool LocalisedString::load_from_xml_child(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc, tchar const * _id, bool _loadId)
{
	if (IO::XML::Node const * childNode = _node->first_child_named(_childName))
	{
		return load_from_xml(childNode, _lc, _id, _loadId);
	}
	load_from_xml(nullptr, _lc, _id, _loadId); // load id
	return true;
}

bool LocalisedString::load_from_xml_child_required(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc, tchar const * _id, bool _loadId)
{
	if (IO::XML::Node const * childNode = _node->first_child_named(_childName))
	{
		return load_from_xml(childNode, _lc, _id, _loadId);
	}
	else
	{
		error_loading_xml(_node, TXT("no \"%S\" localised string not found, missing node \"%S\""), _id, _childName);
		return false;
	}
}

bool LocalisedString::load_from_xml_child(IO::XML::Node const * _node, tchar const * _childName, tchar const * _fullId, bool _loadId)
{
	if (IO::XML::Node const * childNode = _node->first_child_named(_childName))
	{
		return load_from_xml(childNode, _fullId, _loadId);
	}
	load_from_xml(nullptr, _fullId, _loadId); // load full id
	return true;
}

bool LocalisedString::load_from_xml_child_required(IO::XML::Node const * _node, tchar const * _childName, tchar const * _fullId, bool _loadId)
{
	if (IO::XML::Node const * childNode = _node->first_child_named(_childName))
	{
		return load_from_xml(childNode, _fullId, _loadId);
	}
	else
	{
		error_loading_xml(_node, TXT("no \"%S\" localised string not found, missing node \"%S\""), _fullId, _childName);
		return false;
	}
}

String const & LocalisedString::get(LocalisedStringRequestedForm const & _format) const
{
#ifdef AN_DEVELOPMENT
	if (!entry)
	{
		Concurrency::ScopedSpinLock lock(errorInfoLock);
		error_stop(TXT("<MISSING:\"%S\">"), id.to_char());
		errorInfo = String::printf(TXT("<MISSING:\"%S\">"), id.to_char());
		return errorInfo;
	}
#endif
	return entry ? entry->get(_format) : String::empty();
}

String const & LocalisedString::get_form_for_format_custom() const
{
#ifdef AN_DEVELOPMENT
	if (!entry)
	{
		Concurrency::ScopedSpinLock lock(errorInfoLock);
		error_stop(TXT("<MISSING:%S>"), id.to_char());
		errorInfo = String::printf(TXT("<MISSING:%S>"), id.to_char());
		return errorInfo;
	}
#endif
	return entry ? entry->get_form_for_format_custom() : String::empty();
}

LocalisedStringForm const * LocalisedString::get_form(LocalisedStringRequestedForm const & _form) const
{
#ifdef AN_DEVELOPMENT
	if (!entry)
	{
		Concurrency::ScopedSpinLock lock(errorInfoLock);
		error_stop(TXT("<MISSING:%S>"), id.to_char());
		errorInfo = String::printf(TXT("<MISSING:%S>"), id.to_char());
		return nullptr;
	}
#endif
	return entry? entry->get_form(_form) : nullptr;
}

bool LocalisedString::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, id);
	if (_serialiser.is_reading())
	{
		entry = LocalisedStrings::get_entry(id, true, true);
		an_assert(entry, TXT("we should find such localised string entry among existing"));
	}

	return result;
}

//

LocalisedStringForm::LocalisedStringForm()
{
}

LocalisedStringForm::~LocalisedStringForm()
{
	for_every_ptr(subForm, subForms)
	{
		delete subForm;
	}
}

struct PrepareForTSV
{
	static String string(String const& value)
	{
		String v = value;
		v = v.replace(String(TXT("\t")), String(TXT(" ")));
		if (v.get_left(1) == TXT("=") ||
			v.get_left(1) == TXT("+") ||
			v.get_left(1) == TXT("-"))
		{
			v = String(TXT("\\")) + v;
		}
		return v;
	}
};

void LocalisedStringForm::add_to_tsv(IO::File* _f, int _libraryLoadLevel, String const& _id, LocalisedStringForm const* _sourceForm, bool _toTranslate) const
{
	// library load level 
	_f->write_text(String::printf(TXT("%i"), _libraryLoadLevel));
	_f->write_text(TXT("\t"));
	// id
	_f->write_text(_id.to_char());
	_f->write_text(TXT("\t"));
	// id
#ifdef BUILD_NON_RELEASE
	if (_sourceForm)
	{
		_f->write_text(PrepareForTSV::string(_sourceForm->contextInfo));
	}
#endif
	_f->write_text(TXT("\t"));
	// numberParam
	if (numberParam.is_valid())
	{
		_f->write_text(numberParam.to_char());
	}
	_f->write_text(TXT("\t"));
	// hasParam
	if (hasParam.is_valid())
	{
		_f->write_text(hasParam.to_char());
	}
	_f->write_text(TXT("\t"));
	// original text
	if (_sourceForm)
	{
		_f->write_text(PrepareForTSV::string(_sourceForm->value));
	}
	_f->write_text(TXT("\t"));
	// text to translate
	if (_sourceForm)
	{
		_f->write_text(PrepareForTSV::string(_sourceForm->valueToTranslate));
	}
	_f->write_text(TXT("\t"));
	// auto translated
	if (!_toTranslate)
	{
		_f->write_text(PrepareForTSV::string(value));
	}
	_f->write_line();
	for_every_ptr(subForm, subForms)
	{
		subForm->add_to_tsv(_f, _libraryLoadLevel, subForm->form, subForm, _toTranslate);
	}
}

struct PrepareForTXT
{
	static String string(String const& value)
	{
		String v = value;
		v = v.replace(String(TXT("\t")), String(TXT(" ")));
		v = v.replace(String(TXT("~")), String(TXT(" ")));
		return v;
	}
};

void LocalisedStringForm::add_to_txt(IO::File* _f) const
{
	String prepared = PrepareForTXT::string(value);
	if (!prepared.is_empty())
	{
		_f->write_text(prepared);
		_f->write_line();
	}
	for_every_ptr(subForm, subForms)
	{
		subForm->add_to_txt(_f);
	}
}

void LocalisedStringForm::add_to_xml(IO::XML::Node * _node) const
{
	if (numberParam.is_valid())
	{
		_node->set_attribute(TXT("numberParam"), numberParam.to_string());
	}
	if (hasParam.is_valid())
	{
		_node->set_attribute(TXT("hasParam"), hasParam.to_string());
	}
	if (!form.is_empty())
	{
		_node->set_attribute(TXT("form"), form);
	}
	_node->add_text(value);
	for_every_ptr(subForm, subForms)
	{
		IO::XML::Node* formNode = _node->add_node(TXT("sub"));
		subForm->add_to_xml(formNode);
	}
}

bool LocalisedStringForm::load_from_xml(IO::XML::Node const * _node, bool _main)
{
	bool result = true;

	isMain = _main;
	numberParam = _node->get_name_attribute(TXT("numberParam"), numberParam);
	hasParam = _node->get_name_attribute(TXT("hasParam"), hasParam);
	form = _node->get_string_attribute(TXT("form"), form);
	make_sure_string_format_is_valid(form);
	if (_node->first_child_named(TXT("text")))
	{
		// read from
		// <something>
		//   <text>text</text>
		//   <textToTranslate>text</textToTranslate>
		// </something>
		value = _node->get_string_attribute_or_from_child(TXT("text"), value);
		valueToTranslate = _node->get_string_attribute_or_from_child(TXT("textToTranslate"), valueToTranslate);
#ifdef BUILD_NON_RELEASE
		contextInfo = _node->get_string_attribute_or_from_child(TXT("translationContextInfo"), contextInfo);
#endif
	}
	else
	{
		// read from
		// <something textToTranslate="text">text</something>
		// <something text="text=" textToTranslate="text"/>
		value = _node->get_string_attribute_or_from_node(TXT("text"), value);
		valueToTranslate = _node->get_string_attribute(TXT("textToTranslate"), valueToTranslate);
#ifdef BUILD_NON_RELEASE
		contextInfo = _node->get_string_attribute(TXT("translationContextInfo"), contextInfo);
#endif
	}
	stringFormatterElements.clear();
	if (!value.is_empty() &&
		! _node->get_bool_attribute(TXT("ignoreFormatting")))
	{
		StringFormatterElement::extract_from(value, OUT_ stringFormatterElements);
	}

	if (!isMain)
	{
		break_format(form, requiredForms);

		result &= parse_number_conditions(_node);
	}

	// load subForms
	for_every(subFormNode, _node->children_named(TXT("sub")))
	{
		LocalisedStringForm * subForm = new LocalisedStringForm();
		if (subForm->load_from_xml(subFormNode, false))
		{
			subForms.push_back(subForm);
		}
		else
		{
			delete subForm;
			error_loading_xml(_node, TXT("problem loading default localised string form"));
			result = false;
		}
	}

	if (value.is_empty() && subForms.is_empty())
	{
		error_loading_xml(_node, TXT("no value or sub forms provided for localised string form"));
		result = false;
	}

	return result;
}

void LocalisedStringForm::break_format(String const & _format, Array<String> & _formats)
{
	if (_format.is_empty())
	{
		return;
	}
	List<String> tokens;
	_format.split(String(TXT("~")), tokens);
	an_assert(!tokens.is_empty(), TXT("format/form should have at least one ~ inside, at the beginning"));
	for_every(token, tokens)
	{
		if (!token->is_empty())
		{
			_formats.push_back(*token);
		}
	}
}

bool LocalisedStringForm::parse_number_conditions(IO::XML::Node const * _node)
{
	bool result = true;

	ARRAY_STACK(int, toRemove, requiredForms.get_size());

	for_every(requiredForm, requiredForms)
	{
		if (requiredForm->has_prefix(TXT("number")) ||
			requiredForm->has_prefix(TXT("-number")) || 
			requiredForm->has_prefix(TXT("|number")))
		{
			NumberCondition condition;
			int startAt = 6;
			if (requiredForm->has_prefix(TXT("-number")))
			{
				startAt = 7;
				condition.negateParam = true;
			}
			if (requiredForm->has_prefix(TXT("|number")))
			{
				startAt = 7;
				condition.absoluteParam = true;
			}
			if (requiredForm->has_prefix(TXT("`number")))
			{
				startAt = 7;
				condition.percentageParam = true;
			}
			String operatorSoFar;
			String actualNumber;
			tchar const * ch = &requiredForm->get_data().get_data()[startAt];
			while (*ch != 0)
			{
				tchar const nCh = *(ch + 1);
				if (*ch == '<' ||
					*ch == '>' ||
					*ch == '=' ||
					*ch == '%')
				{
					operatorSoFar += *ch;
				}
				else
				{
					actualNumber += *ch;
				}
				if (!actualNumber.is_empty() &&
					(nCh == '<' ||
					 nCh == '>' ||
					 nCh == '=' ||
					 nCh == '%' ||
					 nCh == 0))
				{
					// parse!
					if (operatorSoFar == TXT("%"))
					{
						condition.modulo = ParserUtils::parse_float(actualNumber);
					}
					else if (operatorSoFar == TXT("<"))
					{
						condition.less = ParserUtils::parse_float(actualNumber);
					}
					else if (operatorSoFar == TXT("<="))
					{
						condition.lessOrEqual = ParserUtils::parse_float(actualNumber);
					}
					else if (operatorSoFar == TXT(">"))
					{
						condition.greater = ParserUtils::parse_float(actualNumber);
					}
					else if (operatorSoFar == TXT(">="))
					{
						condition.greaterOrEqual = ParserUtils::parse_float(actualNumber);
					}
					else if (operatorSoFar == TXT("=="))
					{
						condition.equal = ParserUtils::parse_float(actualNumber);
					}
					else
					{
						error_loading_xml(_node, TXT("could not parse operator \"%S\""), operatorSoFar.to_char());
						result = false;
					}

					// ready for next
					operatorSoFar = String::empty();
					actualNumber = String::empty();
				}
				++ch;
			}
			numberConditions.push_back(condition);
			toRemove.push_back(for_everys_index(requiredForm));
		}
	}

	for_every_reverse(tr, toRemove)
	{
		requiredForms.remove_at(*tr);
	}

	return result;
}

String LocalisedStringForm::gather_all_texts() const
{
	String result = value;
	for_every_ptr(sf, subForms)
	{
		result += TXT(" ");
		result += sf->gather_all_texts();
	}
	return result;
}

String LocalisedStringForm::gather_all_texts_for_hash() const
{
	String result = value + valueToTranslate;
	for_every_ptr(sf, subForms)
	{
		result += valueToTranslate;
	}
	return result;
}

LocalisedStringForm* LocalisedStringForm::create_copy()
{
	LocalisedStringForm* formCopy = new LocalisedStringForm();
	
	*formCopy = *this;

	formCopy->subForms.clear();
	for_every_ptr(sf, subForms)
	{
		formCopy->subForms.push_back(sf->create_copy());
	}

	return formCopy;
}

LocalisedStringForm const * LocalisedStringForm::get_form(Name const& _stringId, LocalisedStringRequestedForm const & _format) const
{
	LocalisedStringRequestedForm format = _format;
	if (hasParam.is_valid())
	{
		if (format.params)
		{
			if (auto * param = format.params->find(hasParam))
			{
				// ok
			}
			else
			{
				return nullptr;
			}
		}
		else
		{
			an_assert(false, TXT("no params"));
#ifdef AN_DEVELOPMENT
			Concurrency::ScopedSpinLock lock(errorInfoLock);
			error_stop(TXT("<NO PARAMS>"));
			errorInfo = String::printf(TXT("<NO PARAMS>"));
#endif
			return nullptr;
		}
	}
	if (numberParam.is_valid())
	{
		if (format.params)
		{
			if (auto * param = format.params->find(numberParam))
			{
				if (param->get_type() == StringFormatterParam::Int)
				{
					format.numberI = param->get_as_int();
					format.numberF.clear();
				}
				else if (param->get_type() == StringFormatterParam::Float)
				{
					format.numberF = sane_zero(param->get_as_float());
					format.numberI = (int)format.numberF.get();
				}
				else
				{
					an_assert(false, TXT("invalid param: %S"), numberParam.to_char());
#ifdef AN_DEVELOPMENT
					Concurrency::ScopedSpinLock lock(errorInfoLock);
					error_stop(TXT("<INVALID PARAM:%S>"), numberParam.to_char());
					errorInfo = String::printf(TXT("<INVALID PARAM:%S>"), numberParam.to_char());
#endif
					return nullptr;
				}
			}
			else
			{
				an_assert(false, TXT("no param: %S"), numberParam.to_char());
#ifdef AN_DEVELOPMENT
				Concurrency::ScopedSpinLock lock(errorInfoLock);
				error_stop(TXT("<NO PARAM:%S>"), numberParam.to_char());
				errorInfo = String::printf(TXT("<NO PARAM:%S>"), numberParam.to_char());
#endif
				return nullptr;
			}
		}
		else
		{
			error_dev_investigate(TXT("there is a missing param (number param) \"%S\" for string id \"%S\": %S"), numberParam.to_char(), _stringId.to_char(), _format.form ? _format.form : TXT("--"));
#ifdef AN_DEVELOPMENT
			Concurrency::ScopedSpinLock lock(errorInfoLock);
			error_stop(TXT("<NO PARAMS>"));
			errorInfo = String::printf(TXT("<NO PARAMS>"));
#endif
			return nullptr;
		}
	}
	if (!isMain)
	{
		if (!numberConditions.is_empty())
		{
			int number = format.numberI;
			for_every(condition, numberConditions)
			{
				if (format.numberF.is_set())
				{
					if (!condition->is_ok(format.numberF.get()))
					{
						// number condition not ok
						return nullptr;
					}
				}
				else
				{
					if (!condition->is_ok(number))
					{
						// number condition not ok
						return nullptr;
					}
				}
			}
		}
		if (format.form)
		{
			for_every(requiredForm, requiredForms)
			{
				if (!String::does_contain(format.form, requiredForm->to_char()))
				{
					// format not found
					return nullptr;
				}
			}
		}
		else if (!requiredForms.is_empty())
		{
			// requires format but format was not provided
			return nullptr;
		}
	}
	for_every_ptr(subForm, subForms)
	{
		if (LocalisedStringForm const * form = subForm->get_form(_stringId, format))
		{
			return form;
		}
	}
	if (!value.is_empty())
	{
		return this;
	}
	return nullptr;
}

String const & LocalisedStringForm::get(Name const & _stringId, LocalisedStringRequestedForm const & _format) const
{
	if (LocalisedStringForm const * form = get_form(_stringId, _format))
	{
		return form->value;
	}
	else
	{
#ifdef AN_DEVELOPMENT
		return errorInfo;
#else
		return String::empty();
#endif
	}
}

//

bool LocalisedStringForm::NumberCondition::is_ok(int _number) const
{
	if (absoluteParam)
	{
		_number = abs(_number);
	}
	if (percentageParam)
	{
		_number = _number * 100;
	}
	if (negateParam)
	{
		_number = -_number;
	}
	if (modulo.is_set())
	{
		_number = mod_raw(_number, (int)modulo.get());
	}
	if (equal.is_set() && _number != (int)equal.get())
	{
		return false;
	}
	if (less.is_set() && _number >= (int)less.get())
	{
		return false;
	}
	if (lessOrEqual.is_set() && _number > (int)lessOrEqual.get())
	{
		return false;
	}
	if (greater.is_set() && _number <= (int)greater.get())
	{
		return false;
	}
	if (greaterOrEqual.is_set() && _number < (int)greaterOrEqual.get())
	{
		return false;
	}
	return true;
}

bool LocalisedStringForm::NumberCondition::is_ok(float _number) const
{
	if (absoluteParam)
	{
		_number = abs(_number);
	}
	if (percentageParam)
	{
		_number = _number * 100.0f;
	}
	if (negateParam)
	{
		_number = -_number;
	}
	if (modulo.is_set())
	{
		_number = mod_raw(_number, modulo.get());
	}
	if (equal.is_set() && _number != equal.get())
	{
		return false;
	}
	if (less.is_set() && _number >= less.get())
	{
		return false;
	}
	if (lessOrEqual.is_set() && _number > lessOrEqual.get())
	{
		return false;
	}
	if (greater.is_set() && _number <= greater.get())
	{
		return false;
	}
	if (greaterOrEqual.is_set() && _number < greaterOrEqual.get())
	{
		return false;
	}
	return true;
}

//

LocalisedStringEntry::LocalisedStringEntry()
{
}

LocalisedStringEntry::~LocalisedStringEntry()
{
	for_every(lf, languageForms)
	{
		delete_and_clear(lf->form);
	}
}

String const & LocalisedStringEntry::get(LocalisedStringRequestedForm const & _format) const
{
	if (form)
	{
		String const & result = form->get(id, _format);
		if (!result.is_empty())
		{
			return result;
		}
	}

#ifdef AN_DEVELOPMENT
	if (!formReplacement.is_empty())
	{
		error_stop(formReplacement.to_char());
	}
#endif
	return formReplacement;
}

LocalisedStringForm const * LocalisedStringEntry::get_form(LocalisedStringRequestedForm const & _format) const
{
	LocalisedStringForm const * result = form ? form->get_form(id, _format) : nullptr;
	return result ? result : form;
}

String const & LocalisedStringEntry::get_form_for_format_custom() const
{
	if (form)
	{
		return form->get_form_for_format_custom();
	}

	return String::empty();
}

LocalisedStringEntry::FormHash LocalisedStringEntry::calculate_hash_for(String const& _string)
{
	FormHash hashValue = 0;
	int idxKey = 0;
	for_every(ch, _string.get_data())
	{
		if (*ch == 0)
		{
			break;
		}
		FormHash chHashValue = (FormHash)Unicode::tchar_to_unicode(*ch);
		// 1111 1111  1111 1111  1111 1111  1111 1111
		// ^^^. ....  .... ....  .... ....  .... ....
		hashValue = ((hashValue & (0xe00000000)) >> 29)
				  | ((hashValue & (0x1ffffffff)) << 3);
		hashValue = hashValue ^ chHashValue;
		hashValue = hashValue ^ idxKey;
		idxKey += chHashValue / 4;
	}
	return hashValue;
}

LocalisedStringEntry::LanguageForm const* LocalisedStringEntry::get_existing_form_for(Name const& _language) const
{
	int languageIdx = LocalisedStrings::find_language_index(_language);
	if (languageForms.is_index_valid(languageIdx))
	{
		auto& lf = languageForms[languageIdx];
		if (lf.form)
		{
			return &lf;
		}
	}
	return nullptr;
}

LocalisedStringEntry::LanguageForm const* LocalisedStringEntry::get_existing_form_for(int _languageIdx) const
{
	if (languageForms.is_index_valid(_languageIdx))
	{
		auto& lf = languageForms[_languageIdx];
		if (lf.form)
		{
			return &lf;
		}
	}
	return nullptr;
}

LocalisedStringEntry::LanguageForm& LocalisedStringEntry::access_form_for(Name const& _language)
{
	Concurrency::ScopedSpinLock lock(loadLock, true);
	int languageIdx = LocalisedStrings::find_language_index(_language);
	if (!languageForms.is_index_valid(languageIdx))
	{
		languageForms.set_size(languageIdx + 1);
	}
	LanguageForm& lf = languageForms[languageIdx];
	lf.language = _language;
	if (!lf.form)
	{
		lf.form = new LocalisedStringForm();
	}
	return lf;
}

void LocalisedStringEntry::update_library_load_level()
{
	if (auto* l = Library::get_current())
	{
		libraryLoadLevel = l->get_current_library_load_level();
	}
	else
	{
		libraryLoadLevel = 0;
	}
}

bool LocalisedStringEntry::load_from_xml(IO::XML::Node const * _node, Name const & _language)
{
	bool result = true;

	if (_node && _node->get_string_attribute(TXT("translation")) == TXT("required"))
	{
		// skip
		return true;
	}

	Concurrency::ScopedSpinLock lock(loadLock, true);

	id = _node->get_name_attribute(TXT("id"), id);
	if (!id.is_valid())
	{
		error_loading_xml(_node, TXT("no id provided for localised string"));
		result = false;
	}

#ifdef BUILD_NON_RELEASE
	if (auto* attr = _node->get_attribute(TXT("autoTranslation")))
	{
		noAutoTranslation = attr->get_as_string() == TXT("no");
	}
#endif

	if (_language == NAME(defaultLanguage))
	{
		update_library_load_level();
	}

	if (_node->has_children() || _node->has_text()) // if doesn't have any child, allow not loading
	{
		LanguageForm & lf = access_form_for(_language);
		LocalisedStringForm*& loadedForm = lf.form;
		if (loadedForm->load_from_xml(_node))
		{
#ifdef BUILD_NON_RELEASE
			if (_language == NAME(defaultLanguage))
			{
				lf.formHash = calculate_hash_for(loadedForm->gather_all_texts_for_hash());
			}
			else
			{
				auto const& hashString = _node->get_string_attribute(TXT("forHash"));
				if (!hashString.is_empty())
				{
					lf.formHash = ParserUtils::parse_hex(hashString.to_char());
				}
			}
#endif
			// loaded fine, proceed
			if (_language == LocalisedStrings::get_language())
			{
				// set value if language matches
				form = lf.form;
			}
		}
		else
		{
			error_loading_xml(_node, TXT("invalid data provided for localised string"));
			result = false;
		}
	}

	return result;
}

bool LocalisedStringEntry::load_default_from_xml(IO::XML::Node const * _node)
{
	Concurrency::ScopedSpinLock lock(loadLock, true);

	bool result = true;

#ifdef BUILD_NON_RELEASE
	if (auto* attr = _node->get_attribute(TXT("autoTranslation")))
	{
		noAutoTranslation = attr->get_as_string() == TXT("no");
	}
#endif

	if (_node->has_children() || _node->has_text()) // if doesn't have any child, allow not loading
	{
		LanguageForm& lf = access_form_for(NAME(defaultLanguage));
		LocalisedStringForm*& loadedForm = lf.form;
		if (loadedForm->load_from_xml(_node))
		{
#ifdef BUILD_NON_RELEASE
			lf.formHash = calculate_hash_for(loadedForm->gather_all_texts_for_hash());
#endif
			// loaded fine, proceed to setup for selected language (if not loaded, allow to fail)
			update_for_language(LocalisedStrings::get_language(), true);
		}
		else
		{
			error_loading_xml(_node, TXT("invalid default data provided for localised string"));
			result = false;
		}
	}

	return result;
}

bool LocalisedStringEntry::update_for_language(Name const& _language, bool _mayFail)
{
	if (forLanguage == _language)
	{
		return true;
	}

	int languageIdx = LocalisedStrings::find_language_index(_language);

	return update_for_language(_language, languageIdx, _mayFail);
}

bool LocalisedStringEntry::update_for_language(Name const& _language, int _languageIdx, bool _mayFail)
{
	if (forLanguage == _language)
	{
		return true;
	}

	Concurrency::ScopedSpinLock lock(LocalisedStrings::s_localisedStrings->stringsLock, true);

	if (form)
	{
		formReplacement = String::printf(TXT("<NOT DEFINED:%S>"), id.to_char());
	}
	if (auto * lf = get_existing_form_for(_languageIdx))
	{
		an_assert(lf->form);
		forLanguage = _language;
		form = lf->form;
		return true;
	}
	else
	{
#ifdef AN_WINDOWS
#ifdef BUILD_NON_RELEASE
		if (!g_localisedStringsBeingTranslated)
#endif
#endif
		if (! _mayFail)
		{
			if (!languageForms.is_empty()) // if empty, maybe it should remain empty?
			{
				warn(TXT("for language \"%S\" not defined string \"%S\""), _language.to_char(), id.to_char());
			}
		}
		if (auto* lf = get_existing_form_for(NAME(defaultLanguage)))
		{
			// go with the default language to show anything
			an_assert(lf->form);
			forLanguage = _language;
			form = lf->form;
		}
		return false;
	}
}

bool LocalisedStringEntry::does_require_translation_to(Name const& _language) const
{
	LanguageForm const * fromForm = nullptr;
	LanguageForm const * toForm = nullptr;
	for_every(lf, languageForms)
	{
		if (lf->language == NAME(defaultLanguage))
		{
			fromForm = lf;
		}
		if (lf->language == _language)
		{
			toForm = lf;
		}
	}
	if (fromForm)
	{
		if (toForm)
		{
#ifdef BUILD_NON_RELEASE
			if (toForm->formHash != 0 && // if not provided, won't be translated
				toForm->formHash != fromForm->formHash)
			{
				return true;
			}
#endif
			return false;
		}
		else
		{
			// such an entry doesn't even exist!
			return true;
		}
	}
	return false;
}

void LocalisedStringEntry::translate_to(Name const& _language, bool _onlyUpdateHashes)
{
#ifdef AN_WINDOWS
#ifdef BUILD_NON_RELEASE
	String curlPath(TXT("..\\..\\externals\\tools\\curl\\"));
	String curlExec(TXT("curl.exe"));
	String translateBatch(TXT("translate.bat"));
	String translateRequestJson(TXT("translate.json"));
	String translateOutputJson(TXT("translated.json"));
	String currentPath;

	{
		tchar cp[513];
		if (_wgetcwd(cp, 512))
		{
			currentPath = cp;
		}
		else
		{
			error(TXT("couldn't get current path"));
			return;
		}
	}

	struct Translate
	{
		String toTranslate;
		String toTranslateOrg;
		String translated;
		LocalisedStringForm* form;
		Array<String> replacedFormats; // changed %something% into %0%

		static void populate(LocalisedStringForm* _form, REF_ Array<Translate>& _translate)
		{
			String const& value = !_form->valueToTranslate.is_empty() ? _form->valueToTranslate : _form->value;
			if (!value.is_empty())
			{
				Translate t;
				t.toTranslate = value;
				t.toTranslateOrg = value;
				t.form = _form;
				_translate.push_back(t);
			}
			for_every_ptr(sf, _form->subForms)
			{
				populate(sf, REF_ _translate);
			}
		}
	};
	Array<Translate> translate;
	if (IO::File::does_exist(curlPath + curlExec) &&
		IO::File::does_exist(curlPath + translateBatch))
	{
		{
			auto& toForm = access_form_for(_language);
			LanguageForm const * fromForm = get_existing_form_for(NAME(defaultLanguage));
			if (fromForm && fromForm->form)
			{
				if (! _onlyUpdateHashes)
				{
					delete_and_clear(toForm.form);
					toForm.form = fromForm->form->create_copy();
				}
				toForm.formHash = fromForm->formHash; // update hash so we know what we've updated
				toForm.form->justTranslated = true;
				Translate::populate(toForm.form, REF_ translate);
			}
		}

		if (_onlyUpdateHashes)
		{
			// nothing
		}
		else if (noAutoTranslation)
		{
			// we created a copy, keep it as it is
			output(TXT("[translation] no translating for id: \"%S\""), id.to_char());
		}
		else
		{
			for_every(t, translate)
			{
				if (t->toTranslate.does_contain('%') ||
					t->toTranslate.does_contain('"') ||
					t->toTranslate.does_contain('{'))
				{
					bool percentageSingsAreFormatting = ! t->form->stringFormatterElements.is_empty(); // if it is empty, means that % is just a percentage sign
					String toTranslate;
					int lastOpen = NONE;
					int lastBracketOpen = NONE;
					for (int i = 0; i < t->toTranslate.get_length(); ++i)
					{
						tchar tch = t->toTranslate[i];
						if (lastBracketOpen == NONE &&
							tch == '{')
						{
							lastBracketOpen = i;
							continue;
						}
						if (lastBracketOpen != NONE)
						{
							if (tch == '}')
							{
								toTranslate += String::printf(TXT("%%%i%%"), t->replacedFormats.get_size());
								t->replacedFormats.push_back(t->toTranslate.get_sub(lastBracketOpen, i - lastBracketOpen + 1));
								lastBracketOpen = NONE;
								continue;
							}
							// skip anything else
							continue;
						}

						if (tch == '%')
						{
							if (! percentageSingsAreFormatting)
							{
								toTranslate += String::printf(TXT("%%%i%%"), t->replacedFormats.get_size());
								t->replacedFormats.push_back(String(TXT("%%")));
								continue;
							}
							else
							{
								if (lastOpen == NONE)
								{
									lastOpen = i;
									continue;
								}
								else
								{
									toTranslate += String::printf(TXT("%%%i%%"), t->replacedFormats.get_size());
									t->replacedFormats.push_back(t->toTranslate.get_sub(lastOpen, i - lastOpen + 1));
									lastOpen = NONE;
									continue;
								}
							}
						}
						if (lastOpen != NONE)
						{
							// eat any other character
							continue;
						}
						if (tch == '"')
						{
							toTranslate += String::printf(TXT("%%%i%%"), t->replacedFormats.get_size());
							t->replacedFormats.push_back(String(TXT("\"")));
							continue;
						}
						toTranslate += tch;
					}
					t->toTranslate = toTranslate;
				}
			}

			for_every(t, translate)
			{
				IO::File* fRequest = new IO::File();
				if (fRequest->create(curlPath + translateRequestJson))
				{
					fRequest->set_type(IO::InterfaceType::UTF8);
					RefCountObjectPtr<IO::JSON::Document> json;
					json = new IO::JSON::Document();
					json->set(TXT("q"), t->toTranslate);
					json->set(TXT("source"), NAME(defaultLanguage).to_string()); 
					json->set(TXT("target"), _language.to_string()); 
					json->set(TXT("format"), String(TXT("text")));
					String jsonString = json->to_string();
					fRequest->write_text(jsonString);
				}
				delete fRequest;

				struct Command
				{
					static String run(String const& cmd)
					{
						ArrayStatic<tchar, 257> buffer;
						String result;
						FILE* pipe = _wpopen(cmd.to_char(), TXT("r"));
						if (!pipe)
						{
							throw std::runtime_error("popen() failed!");
						}
						else
						{
							buffer.set_size(buffer.get_max_size());
							while (true)
							{
								memory_zero(buffer.begin(), buffer.get_data_size());
								if (fgetws(buffer.get_data(), buffer.get_size() - 1, pipe) != nullptr)
								{
									// the last one should be zeroed
									result += buffer.get_data();
								}
								else
								{
									break;
								}
							}
							_pclose(pipe);
						}
						return result;
					}
				};

				output(TXT("[translation] id: \"%S\""), id.to_char());
				output(TXT("[translation] translate: \"%S\""), t->toTranslate.to_char());

				_wchdir(currentPath.to_char());
				_wchdir(curlPath.to_char());
				Command::run(translateBatch);
				_wchdir(currentPath.to_char());

				IO::File* fResponse = new IO::File();
				if (fResponse->open(curlPath + translateOutputJson))
				{
					fResponse->set_type(IO::InterfaceType::UTF8);
					List<String> lines;
					fResponse->read_lines(REF_ lines);
					String jsonString;
					for_every(l, lines)
					{
						jsonString += *l;
						jsonString += String::space();
					}
					RefCountObjectPtr<IO::JSON::Document> json;
					json = new IO::JSON::Document();
					bool translated = false;
					if (json->parse(jsonString))
					{
						if (auto* dataNode = json->get_sub(TXT("data")))
						{
							if (auto* translationsArray = dataNode->get_array_elements(TXT("translations")))
							{
								for_every(translationNode, *translationsArray)
								{
									String translatedText = translationNode->get(TXT("translatedText"));
									translatedText.replace_inline(String(TXT("\\003c")), String(TXT("<")));
									translatedText.replace_inline(String(TXT("\\003e")), String(TXT(">")));
									t->translated = translatedText;
									translated = true;
									output(TXT("[translation] translated to \"%S\": \"%S\""), _language.to_char(), t->translated.to_char());
								}
							}
						}
					}
				}
				delete fResponse;
			}

			for_every(t, translate)
			{
				String translated;
				if (t->replacedFormats.is_empty())
				{
					translated = t->translated;
				}
				else
				{
					int lastOpen = NONE;
					int rfIdx = 0;
					for (int i = 0; i < t->translated.get_length(); ++i)
					{
						tchar tch = t->translated[i];
						if (tch == '%')
						{
							if (lastOpen == NONE)
							{
								lastOpen = i;
								continue;
							}
							else
							{
								int idx = ParserUtils::parse_int(t->translated.get_sub(lastOpen + 1, i - (lastOpen + 1)));
								if (t->replacedFormats.is_index_valid(idx))
								{
									translated += t->replacedFormats[idx];
								}
								++rfIdx;
								lastOpen = NONE;
								continue;
							}
						}
						if (lastOpen != NONE)
						{
							// eat any other character
							continue;
						}
						translated += tch;
					}
				}
				output(TXT("[translation] store \"%S\": \"%S\""), id.to_char(), translated.to_char());
				t->form->value = translated;
			}

			/*
			output(TXT(" id:\"%S\" is"), id.to_char());
			for_every(lf, languageForms)
			{
				output(TXT(" \"%S\": \"%S\""), lf->language.to_char(), lf->form? lf->form->gather_all_texts().to_char() : TXT("[no form!]"));
			}
			*/

		}
	}
#endif
#endif
}

//

LocalisedStrings* LocalisedStrings::s_localisedStrings = nullptr;

void LocalisedStrings::initialise_static()
{
	an_assert(s_localisedStrings == nullptr);
	s_localisedStrings = new LocalisedStrings();
	s_localisedStrings->language = NAME(defaultLanguage);
	s_localisedStrings->add_language(NAME(defaultLanguage));
}

void LocalisedStrings::close_static()
{
	an_assert(s_localisedStrings != nullptr);
	delete_and_clear(s_localisedStrings);
}

void LocalisedStrings::drop_assets()
{
	an_assert(s_localisedStrings != nullptr);
	s_localisedStrings->textureParts.clear();
}

LocalisedStrings::~LocalisedStrings()
{
	for_every_ptr(string, strings)
	{
		delete string;
	}
	strings.clear();
}

int LocalisedStrings::get_langauge_change_count()
{
	an_assert(s_localisedStrings != nullptr);
	return s_localisedStrings->languageChangeCount;
}

void LocalisedStrings::set_language(Name const & _language, bool _force)
{
	an_assert(s_localisedStrings != nullptr);
	if (s_localisedStrings->language == _language && ! _force)
	{
		return;
	}
	output(TXT("change language to \"%S\" (from \"%S\")"), _language.to_char(), s_localisedStrings->language.to_char());
	s_localisedStrings->language = _language;
	int languageIdx = LocalisedStrings::find_language_index(_language);
	for_every_ptr(string, s_localisedStrings->strings)
	{
		string->update_for_language(s_localisedStrings->language, languageIdx);
	}

	for_every(lang, Framework::LocalisedStrings::get_languages())
	{
		if (lang->id == _language)
		{
			::System::Font::set_variable_space_width_pt(lang->variableSpaceWidthPt);
		}
	}

	++s_localisedStrings->languageChangeCount;
}

Name const & LocalisedStrings::get_language()
{
	an_assert(s_localisedStrings != nullptr);
	return s_localisedStrings->language;
}

Name const & LocalisedStrings::get_default_language()
{
	return NAME(defaultLanguage);
}

Name const & LocalisedStrings::get_suggested_default_language()
{
#ifdef AN_CHINA
	return NAME(suggestedDefaultLanguageChina);
#endif
	return Name::invalid();
}

Name const & LocalisedStrings::get_system_language()
{
	tchar const* langID = ::System::Core::get_system_language();
	// for china force default language which should be Chinese-simplified (see above)
#ifdef AN_CHINA
	langID = NAME(defaultLanguage).to_char();
#endif
	if (langID)
	{
		for_every(lang, Framework::LocalisedStrings::get_languages())
		{
			if (String::compare_icase(lang->id.to_char(), langID))
			{
				return lang->id;
			}
		}
		// lang->id should be the prefix
		for_every(lang, Framework::LocalisedStrings::get_languages())
		{
			int lIdLen = (int)tstrlen(lang->id.to_char());
			if (String::compare_icase_left(lang->id.to_char(), langID, lIdLen))
			{
				return lang->id;
			}
		}
	}
	return Name::invalid();
}

LocalisedStrings::Language * LocalisedStrings::access_language(Name const & _id, bool _onlyExisting)
{
	Concurrency::ScopedSpinLock lock(s_localisedStrings->languagesLock, true);
	an_assert(s_localisedStrings != nullptr);
	for_every(lang, s_localisedStrings->languages)
	{
		if (lang->id == _id)
		{
			return lang;
		}
	}
	if (!_onlyExisting)
	{
		s_localisedStrings->languages.push_back(Language());
		s_localisedStrings->languages.get_last().id = _id;
		return &s_localisedStrings->languages.get_last();
	}
	return nullptr;
}

int LocalisedStrings::add_language(Name const & _id)
{
	Concurrency::ScopedSpinLock lock(s_localisedStrings->languagesLock, true);
	an_assert(s_localisedStrings != nullptr);
	for_every(lang, s_localisedStrings->languages)
	{
		if (lang->id == _id)
		{
			return for_everys_index(lang);
		}
	}
	s_localisedStrings->languages.push_back(Language());
	s_localisedStrings->languages.get_last().id = _id;
	return s_localisedStrings->languages.get_size() - 1;
}

int LocalisedStrings::find_language_index(Name const & _id)
{
	Concurrency::ScopedSpinLock lock(s_localisedStrings->languagesLock, true);
	an_assert(s_localisedStrings != nullptr);
	for_every(lang, s_localisedStrings->languages)
	{
		if (lang->id == _id)
		{
			return for_everys_index(lang);
		}
	}
	return NONE;
}


LocalisedStringEntry* LocalisedStrings::get_entry(Name const & _id, bool _onlyExisting, bool _loading)
{
	Concurrency::ScopedSpinLock lock(s_localisedStrings->stringsLock, true);
	todo_note(TXT("solve this properly! try to avoid lock like this"));
	an_assert(s_localisedStrings != nullptr);
	if (auto pEntry = s_localisedStrings->strings.get_existing(_id))
	{
		return *pEntry;
	}
	else if (! _onlyExisting)
	{
		LocalisedStringEntry* newEntry = new LocalisedStringEntry();
		newEntry->id = _id;
		newEntry->update_library_load_level();
		s_localisedStrings->strings[_id] = newEntry;
		return newEntry;
	}
	else if (! _loading)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (Framework::Library::may_ignore_missing_references())
		{
			// don't show anything
		}
		else
		{
			error_dev_ignore(TXT("could not find existing string \"%S\", might be ok as maybe it should not be translated any more"), _id.to_char());
		}
		/*
		output(TXT("existing:"));
		int keyIdx = 0;
		for_every_ptr(st, s_localisedStrings->strings)
		{
			output(TXT("  %i [%S] = \"%S\""), keyIdx, for_everys_map_key(st).to_char(), st->get().to_char());
			++keyIdx;
		}
		*/
#endif
		return nullptr;
	}
	return nullptr;
}

TexturePart* LocalisedStrings::get_texture_part(Name const & _id, Name const & _languageToLoad)
{
	for_every(tp, s_localisedStrings->textureParts)
	{
		if (tp->id == _id && tp->forLanguage == _languageToLoad)
		{
			return tp->texturePart.get();
		}
	}
	return nullptr;
}

LocalisedStringTexturePart* LocalisedStrings::get_texture_part_to_load(Name const & _id, Name const & _languageToLoad)
{
	for_every(tp, s_localisedStrings->textureParts)
	{
		if (tp->id == _id && tp->forLanguage == _languageToLoad)
		{
			return tp;
		}
	}
	LocalisedStringTexturePart tp;
	tp.id = _id;
	tp.forLanguage = _languageToLoad;
	s_localisedStrings->textureParts.push_back(tp);
	return &(s_localisedStrings->textureParts.get_last());
}

bool LocalisedStrings::load_entries_from_xml(IO::XML::Node const * _node, Name const & _languageToLoad, LibraryLoadingContext & _lc)
{
	bool result = true;

	int priority = _node->get_int_attribute(TXT("priority"), LocalisedStringEntry::DEFAULT_PRIORITY);
	error_loading_xml_on_assert(priority >= LocalisedStringEntry::DEFAULT_PRIORITY, result, _node, TXT("priority cannot be lower than %i"), LocalisedStringEntry::DEFAULT_PRIORITY);

	for_every(entryNode, _node->children_named(TXT("texturePart")))
	{
		Name id = entryNode->get_name_attribute(TXT("id"));
		if (id.is_valid())
		{
			LocalisedStringTexturePart* texturePart = get_texture_part_to_load(id, _languageToLoad);
			result &= texturePart->texturePart.load_from_xml(entryNode, TXT("use"), _lc);
		}
		else
		{
			error_loading_xml(entryNode, TXT("no id defined for localised string in localised strings"));
			result = false;
		}
	}

	for_every(entryNode, _node->children_named(TXT("string")))
	{
		if (entryNode && entryNode->get_string_attribute(TXT("translation")) == TXT("required"))
		{
			// skip
			continue;
		}
		Name id = entryNode->get_name_attribute(TXT("id"));
		if (id.is_valid())
		{
			LocalisedStringEntry* entry = get_entry(id);
			if (priority > entry->priority ||
				! entry->get_existing_form_for(_languageToLoad))
			{
				result &= entry->load_from_xml(entryNode, _languageToLoad);
				entry->priority = max(priority, entry->priority);
			}
		}
		else
		{
			error_loading_xml(entryNode, TXT("no id defined for localised string in localised strings"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("group")))
	{
		result &= load_entries_from_xml(node, _languageToLoad, _lc);
	}

	return result;
}

bool LocalisedStrings::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (!_node || _node->get_name() != TXT("localisedStrings"))
	{
		return false;
	}

	bool result = true;
	Name languageToLoad = _node->get_name_attribute(TXT("language"), _node->get_name_attribute(TXT("lang")));
	if (!languageToLoad.is_valid())
	{
		languageToLoad = NAME(defaultLanguage);
	}
	int idx = add_language(languageToLoad);
	
	s_localisedStrings->languages[idx].name = _node->get_string_attribute(TXT("name"), s_localisedStrings->languages[idx].name);
	s_localisedStrings->languages[idx].localisedCharacterSetId = _node->get_name_attribute(TXT("localisedCharacterSetId"), s_localisedStrings->languages[idx].localisedCharacterSetId);
	s_localisedStrings->languages[idx].sortId = _node->get_string_attribute(TXT("sortId"), s_localisedStrings->languages[idx].sortId);
	s_localisedStrings->languages[idx].localisedCharacterSetPriority.load_from_xml(_node, TXT("localisedCharacterSetPriority"));
	s_localisedStrings->languages[idx].variableSpaceWidthPt.load_from_xml(_node, TXT("variableSpaceWidthPt"));

	result &= load_entries_from_xml(_node, languageToLoad, _lc);

	return result;
}

bool LocalisedStrings::save_language_to_tsv(IO::File* _f, Optional<LibraryLoadLevel::Type> const& _libraryLoadLevel)
{
	an_assert(s_localisedStrings);

	bool result = true;

	_f->write_line(TXT("load level\tstring id\tcontext info\tnumber param\thas param\ttext\talt text to translate\ttranslated\tmanually translated\tcomment"));

	// first sort them by building temporary map
	Map<String, LocalisedStringEntry*> sortedStringEntries;
	for_every(stringEntry, s_localisedStrings->strings)
	{
		sortedStringEntries[for_everys_map_key(stringEntry).to_string()] = *stringEntry;
	}

	for_every_ptr(stringEntry, sortedStringEntries)
	{
		if (_libraryLoadLevel.is_set() &&
			_libraryLoadLevel.get() != stringEntry->libraryLoadLevel)
		{
			continue;
		}
		if (stringEntry->languageForms.is_empty())
		{
			continue;
		}
		auto* languageForm = stringEntry->get_existing_form_for(s_localisedStrings->language);
		auto* sourceForm = stringEntry->get_existing_form_for(NAME(defaultLanguage));
		if (!sourceForm)
		{
			// if there's no source form, don't save it, it might be something older
			continue;
		}
		if (languageForm)
		{
			languageForm->form->add_to_tsv(_f, stringEntry->libraryLoadLevel, stringEntry->id.to_string(), sourceForm->form);
		}
	}

	return result;
}

bool LocalisedStrings::save_language_to_txt(IO::File* _f, Optional<LibraryLoadLevel::Type> const& _libraryLoadLevel)
{
	an_assert(s_localisedStrings);

	bool result = true;

	// first sort them by building temporary map
	Map<String, LocalisedStringEntry*> sortedStringEntries;
	for_every(stringEntry, s_localisedStrings->strings)
	{
		sortedStringEntries[for_everys_map_key(stringEntry).to_string()] = *stringEntry;
	}

	for_every_ptr(stringEntry, sortedStringEntries)
	{
		if (_libraryLoadLevel.is_set() &&
			_libraryLoadLevel.get() != stringEntry->libraryLoadLevel)
		{
			continue;
		}
		if (stringEntry->languageForms.is_empty())
		{
			continue;
		}
		auto* languageForm = stringEntry->get_existing_form_for(s_localisedStrings->language);
		auto* sourceForm = stringEntry->get_existing_form_for(NAME(defaultLanguage));
		if (!sourceForm)
		{
			// if there's no source form, don't save it, it might be something older
			continue;
		}
		if (languageForm)
		{
			languageForm->form->add_to_txt(_f);
		}
	}

	return result;
}

bool LocalisedStrings::save_language_to_xml(IO::XML::Node * _node, Optional<LibraryLoadLevel::Type> const& _libraryLoadLevel, Optional<bool> const & _withSource, Optional<bool> const& _breakIntoParts, Optional<bool> const& _simpleHeader)
{
	an_assert(s_localisedStrings);

	bool result = true;

	// at this point it, the language should be already selected using LocalisedStrings::set_language
#ifdef OUTPUT_SAVING_LANGUAGE
	output(TXT("LANGUAGE \"%S\""), s_localisedStrings->language.to_string().to_char());
#endif
	_node->set_attribute(TXT("language"), s_localisedStrings->language.to_string());
	if (!_simpleHeader.get(false))
	{
		if (auto* lang = access_language(s_localisedStrings->language, true))
		{
			_node->set_attribute(TXT("name"), lang->name);
			_node->set_attribute(TXT("sortId"), lang->sortId);
			_node->set_attribute(TXT("localisedCharacterSetId"), lang->localisedCharacterSetId.to_string());
			if (lang->localisedCharacterSetPriority.is_set())
			{
				_node->set_int_attribute(TXT("localisedCharacterSetPriority"), lang->localisedCharacterSetPriority.get());
			}
			if (lang->variableSpaceWidthPt.is_set())
			{
				_node->set_float_attribute(TXT("variableSpaceWidthPt"), lang->variableSpaceWidthPt.get());
			}
		}
	}

	// first sort them by building temporary map
	Map<String, LocalisedStringEntry*> sortedStringEntries;
	for_every(stringEntry, s_localisedStrings->strings)
	{
		sortedStringEntries[for_everys_map_key(stringEntry).to_string()] = *stringEntry;
	}

	bool breakIntoParts = _breakIntoParts.get(false);
	// translationIdx:
	//	0 just translated
	//	1 already translated
	//	2 translation required
	for_count(int, translationIdx, breakIntoParts ? 3 : 1)
	{
		if (breakIntoParts)
		{
			if (translationIdx == 0) _node->add_node(TXT(".just_processed"));
			if (translationIdx == 1) _node->add_node(TXT(".already_translated"));
			if (translationIdx == 2) _node->add_node(TXT(".translation_required"));
		}
		for_every_ptr(stringEntry, sortedStringEntries)
		{
			if (_libraryLoadLevel.is_set() &&
				_libraryLoadLevel.get() != stringEntry->libraryLoadLevel)
			{
				continue;
			}
			if (stringEntry->languageForms.is_empty())
			{
				continue;
			}
			auto* languageForm = stringEntry->get_existing_form_for(s_localisedStrings->language);
			auto* sourceForm = stringEntry->get_existing_form_for(NAME(defaultLanguage));
			bool toTranslate = false;
#ifdef BUILD_NON_RELEASE
			if (! stringEntry->noAutoTranslation)
#endif
			{
				if (!languageForm)
				{
					languageForm = sourceForm;
					toTranslate = true;
				}
				else if (sourceForm)
				{
#ifdef BUILD_NON_RELEASE
					if (languageForm->formHash != sourceForm->formHash)
					{
						toTranslate = true;
					}
#endif
				}
			}
			if (!sourceForm)
			{
				// if there's no source form, don't save it, it might be something older
				continue;
			}
			if (breakIntoParts)
			{
#ifdef BUILD_NON_RELEASE
				bool processThis = (translationIdx == 0 && !toTranslate && (languageForm && languageForm->form && languageForm->form->justTranslated)) ||
								   (translationIdx == 1 && !toTranslate && (! languageForm || ! languageForm->form || ! languageForm->form->justTranslated)) ||
								   (translationIdx == 2 && toTranslate);
#else
				bool processThis = (translationIdx == 1 && !toTranslate) ||
								   (translationIdx == 2 && toTranslate);
#endif
				if (! processThis)
				{
					continue;
				}
			}
			if (languageForm)
			{
				IO::XML::Node* node = _node->add_node(TXT("string"));
				node->set_attribute(TXT("id"), stringEntry->id.to_string());
#ifdef BUILD_NON_RELEASE
				if (languageForm->formHash != 0)
				{
					if (s_localisedStrings->language == NAME(defaultLanguage))
					{
						node->set_attribute(TXT("hash"), ParserUtils::uint_to_hex(languageForm->formHash));
					}
					else
					{
						node->set_attribute(TXT("forHash"), ParserUtils::uint_to_hex(
#ifdef UPDATE_FOR_HASH_FROM_SOURCE
							sourceForm->formHash
#else
							languageForm->formHash
#endif
							));
						if (_withSource.get(false))
						{
							if (sourceForm->form)
							{
								node->set_attribute(TXT("source"), sourceForm->form->gather_all_texts());
							}
						}
					}
				}
#endif
				if (languageForm->form &&
					languageForm->form->stringFormatterElements.is_empty() &&
					languageForm->form->gather_all_texts().does_contain('%'))
				{
					node->set_bool_attribute(TXT("ignoreFormatting"), true);
				}
#ifdef BUILD_NON_RELEASE
				if (stringEntry->noAutoTranslation)
				{
					node->set_attribute(TXT("autoTranslation"), TXT("no"));
				}
#endif
				if (toTranslate)
				{
					node->set_attribute(TXT("translation"), TXT("required"));
				}
				if (languageForm->form &&
					!languageForm->form->is_empty())
				{
					languageForm->form->add_to_xml(node);
				}
#ifdef OUTPUT_SAVING_LANGUAGE
				output(TXT(" %S"), node->get_text().to_char());
#endif
			}
		}
	}

#ifdef OUTPUT_SAVING_LANGUAGE
	output(TXT("END OF LANGUAGE"));
#endif

	return result;
}

void LocalisedStrings::save_to_file(String const & _fileName, Optional<LibraryLoadLevel::Type> const& _libraryLoadLevel, Optional<bool> const& _withSource, Optional<bool> const& _breakIntoParts, Optional<bool> const& _simpleHeader, Optional<bool> const& _andReload)
{
	an_assert(s_localisedStrings);

	String filePath = IO::get_user_game_data_directory() + _fileName;
	String filePathBak = filePath + TXT(".bak");
	if (IO::File::does_exist(filePath))
	{
		IO::File* f = new IO::File();
		if (f->open(filePath))
		{
			f->IO::File::copy_to_file(filePathBak);
		}
		delete f;
	}

	{
		IO::File* f = new IO::File();
		if (f->create(filePath))
		{
			f->set_type(IO::InterfaceType::UTF8);

			{
				if (_fileName.get_right(4).to_lower() == TXT(".txt"))
				{
					save_language_to_txt(f, _libraryLoadLevel);
				}
				else if (_fileName.get_right(4).to_lower() == TXT(".tsv"))
				{
					save_language_to_tsv(f, _libraryLoadLevel);
				}
				else
				{
					IO::XML::Document doc;
					IO::XML::Node* libraryNode = doc.get_root()->add_node(TXT("library"));
					libraryNode->set_attribute(String(TXT("group")), String(TXT("languages"))); // hardcoded

					IO::XML::Node* localisedStringsNode = libraryNode->add_node(TXT("localisedStrings"));
					save_language_to_xml(localisedStringsNode, _libraryLoadLevel, _withSource, _breakIntoParts, _simpleHeader);

					// set a bigger priority
					if (!_simpleHeader.get(false))
					{
						// actually we don't use priorities now for saving
						//localisedStringsNode->set_int_attribute(TXT("priority"), 10);
					}

#ifdef BUILD_NON_RELEASE
					if (_andReload.get(false))
					{
						localisedStringsNode->set_int_attribute(TXT("priority"), 10000); // to force reload
						Framework::LibraryLoadingContext lc(Library::get_current(), IO::Utils::get_directory(filePath), Framework::LibrarySource::Files, Framework::LibraryLoadingMode::Load);
						load_from_xml(localisedStringsNode, lc);
					}
#endif

					doc.set_indent_tab();
					doc.save_xml(f, false, NP);
				}
			}

			output_colour_system();
			output(TXT("language \"%S\" [%i]%S%S saved to %S"), s_localisedStrings->language.to_char(), _libraryLoadLevel.get(0), _withSource.get(false)? TXT("+src") : TXT(""), _breakIntoParts.get(false)? TXT("+parts") : TXT(""), filePath.to_char());
			output_colour();
		}
		delete f;
	}

	IO::File::delete_file(filePathBak);
}

bool LocalisedStrings::load_all_languages_from_dir_tsv_and_save(String const& _inputDirName, String const& _outputDirName)
{
	an_assert(s_localisedStrings);

	bool somethingGotImported = false;

	Name storedLanguage = get_language();

	String dirName = _inputDirName;
	if (!dirName.is_empty() &&
		dirName.get_right(1) != TXT("\\") &&
		dirName.get_right(1) != TXT("/"))
	{
		dirName += IO::get_directory_separator();
	}

	List<String> lines;
	List<String> parts;
	Array<Name> translatedIds;

	for_every(language, s_localisedStrings->languages)
	{
		bool somethingInLanguageImported = false;

		// use different names to avoid saving/resaving
		String fileName = dirName + language->id.to_string() + TXT(".tsv");
		if (!IO::File::does_exist(fileName))
		{
			fileName = dirName + language->id.to_string() + TXT(" - tea for god.tsv");
		}
		if (!IO::File::does_exist(fileName))
		{
			fileName = dirName + language->id.to_string() + TXT(" pico - tea for god.tsv");
		}
		if (!IO::File::does_exist(fileName))
		{
			fileName = dirName + language->id.to_string() + TXT(" - arkusz 1.tsv");
		}
		if (IO::File::does_exist(fileName))
		{
			IO::File* f = new IO::File();
			IO::File* fout = new IO::File();
			String fileNameOut = dirName + language->id.to_string() + TXT(" altered.tsv");
			if (fout)
			{
				fout->create(fileNameOut);
				if (fout->is_open())
				{
					fout->set_type(IO::InterfaceType::UTF8);
				}
			}
			if (f->open(fileName))
			{
				f->set_type(IO::InterfaceType::UTF8);
				set_language(language->id);
				lines.clear();
				translatedIds.clear();
				f->read_lines(lines);
				bool firstLine = true;
				Name mainId;
				for_every(line, lines)
				{
					if (firstLine)
					{
						if (fout && fout->is_open())
						{
							fout->write_text(*line); // just copy the first line
							fout->write_line();
						}
					}
					else
					{
						parts.clear();
						line->split(String(TXT("\t")), parts);
						if (parts.get_size() > TSVColums::ManuallyTranslated)
						{
							String idForm = parts[1];
							bool thisIsMainForm = false;
							if (!(idForm.find_first_of('~') == 0))
							{
								mainId = Name(idForm);
								thisIsMainForm = true;
							}
							if (thisIsMainForm)
							{
								translatedIds.push_back_unique(mainId);
							}
							String numberParam = parts[TSVColums::NumberParam];
							String hasParam = parts[TSVColums::HasParam];
							String translatedTo = parts[TSVColums::ManuallyTranslated];
							if (!translatedTo.is_empty())
							{
#ifndef KEEP_AUTO_MANUAL_TRANSLATIONS
								parts[TSVColums::AutoTranslated] = translatedTo; // update for output
#endif
								if (translatedTo.get_left(2) == TXT("\\=") ||
									translatedTo.get_left(2) == TXT("\\+") ||
									translatedTo.get_left(2) == TXT("\\-") ||
									translatedTo.get_left(2) == TXT("/=") ||
									translatedTo.get_left(2) == TXT("/+") ||
									translatedTo.get_left(2) == TXT("/-"))
								{
									translatedTo = translatedTo.get_sub(1);
								}
								if (auto* e = get_entry(mainId, true))
								{
									bool eTranslated = false;
									LocalisedStringEntry::LanguageForm& lf = e->access_form_for(language->id);
									if (thisIsMainForm)
									{
										if (lf.form->value != translatedTo)
										{
											lf.form->value = translatedTo;
											eTranslated = true;
#ifdef BUILD_NON_RELEASE
											lf.form->justTranslated = true;
#endif
										}
										lf.form->subForms.clear(); // remove all sub forms, we will read them next!
#ifdef BUILD_NON_RELEASE
										if (fout && fout->is_open())
										{
											// update context info
											if (auto* sourceForm = e->get_existing_form_for(NAME(defaultLanguage)))
											{
												if (sourceForm->form)
												{
													parts[TSVColums::ContextInfo] = sourceForm->form->contextInfo;
												}
											}
										}
#endif
									}
									else
									{
										bool found = false;
										for_every_ptr(subForm, lf.form->subForms)
										{
											if (subForm->form == idForm)
											{
												if (subForm->value != translatedTo)
												{
													subForm->value = translatedTo;
													eTranslated = true;
#ifdef BUILD_NON_RELEASE
													lf.form->justTranslated = true;
#endif
												}
												found = true;
												break;
											}
										}
										if (!found)
										{
											LocalisedStringForm* subForm = new LocalisedStringForm();
											subForm->form = idForm;
											subForm->value = translatedTo;
											if (!numberParam.is_empty())
											{
												subForm->numberParam = Name(numberParam);
											}
											if (!hasParam.is_empty())
											{
												subForm->hasParam = Name(hasParam);
											}
											lf.form->subForms.push_back(subForm);
											eTranslated = true;
										}
									}

									// just update hash
									if (eTranslated)
									{
										e->translate_to(language->id, true);
										somethingInLanguageImported = true;
									}
								}
#ifndef KEEP_AUTO_MANUAL_TRANSLATIONS
								parts[TSVColums::ManuallyTranslated] = String::empty(); // for the output we want to remove the last manual translation
#endif
							}
#ifdef UPDATE_TEXT_AND_CONTEXT_IN_TSV
							// find original form
							if (auto* e = get_entry(mainId, true))
							{
								if (auto* sourceForm = e->get_existing_form_for(NAME(defaultLanguage)))
								{
									if (sourceForm->form)
									{
										if (thisIsMainForm)
										{
											String text = PrepareForTSV::string(sourceForm->form->value);
											String textToTranslate = PrepareForTSV::string(sourceForm->form->valueToTranslate);
											if (text != parts[TSVColums::Text] ||
												textToTranslate != parts[TSVColums::TextToTranslate])
											{
												String add = String(TXT("!ORIGINAL TEXT CHANGED!"));
												if (!textToTranslate.does_contain(add))
												{
													textToTranslate = add + String::space() + textToTranslate;
												}
											}
											parts[TSVColums::Text] = text;
											parts[TSVColums::TextToTranslate] = textToTranslate;
#ifdef BUILD_NON_RELEASE
											parts[TSVColums::ContextInfo] = PrepareForTSV::string(sourceForm->form->contextInfo);
#endif
										}
										else
										{
											for_every_ptr(subForm, sourceForm->form->subForms)
											{
												if (subForm->form == idForm)
												{
													parts[TSVColums::Text] = PrepareForTSV::string(subForm->value);
													parts[TSVColums::TextToTranslate] = PrepareForTSV::string(subForm->valueToTranslate);
#ifdef BUILD_NON_RELEASE
													parts[TSVColums::ContextInfo] = PrepareForTSV::string(subForm->contextInfo);
#endif
													break;
												}
											}
										}
									}
								}
							}
#endif
							if (fout && fout->is_open())
							{
								String line;
								for_every(p, parts)
								{
									// this will allow to not override manually translated or comments
									if (for_everys_index(p) == TSVColums::ManuallyTranslated)
									{
										break;
									}
									if (!line.is_empty())
									{
										line += TXT("\t");
									}
									line += *p;
								}
								fout->write_text(line.to_char());
								fout->write_line();
							}
						}
					}
					firstLine = false;
				}
				if (fout && fout->is_open())
				{
					// first sort them by building temporary map
					Map<String, LocalisedStringEntry*> sortedStringEntries;
					for_every(stringEntry, s_localisedStrings->strings)
					{
						if (!translatedIds.does_contain((*stringEntry)->id))
						{
							sortedStringEntries[for_everys_map_key(stringEntry).to_string()] = *stringEntry;
						}
					}

					for_every_ptr(stringEntry, sortedStringEntries)
					{
						if (stringEntry->languageForms.is_empty())
						{
							continue;
						}
						auto* languageForm = stringEntry->get_existing_form_for(s_localisedStrings->language);
						auto* sourceForm = stringEntry->get_existing_form_for(NAME(defaultLanguage));
						if (!sourceForm)
						{
							// if there's no source form, don't save it, it might be something older
							continue;
						}
						if (languageForm)
						{
							languageForm->form->add_to_tsv(fout, stringEntry->libraryLoadLevel, stringEntry->id.to_string(), sourceForm->form);
						}
						else
						{
							sourceForm->form->add_to_tsv(fout, stringEntry->libraryLoadLevel, stringEntry->id.to_string(), sourceForm->form, true);
						}
					}
				}
				if (somethingInLanguageImported)
				{
					save_language_to_dir(_outputDirName, language->id, true);
				}
			}
			delete f;
			if (fout)
			{
				delete fout;
			}
		}

		somethingGotImported |= somethingInLanguageImported;
	}

	set_language(storedLanguage);

	return somethingGotImported;
}

void LocalisedStrings::save_all_languages_to_dir(String const & _dirName)
{
	an_assert(s_localisedStrings);

	Name storedLanguage = get_language();

	String dirName = _dirName;
	if (! dirName.is_empty() &&
		dirName.get_right(1) != TXT("\\") &&
		dirName.get_right(1) != TXT("/"))
	{
		dirName += IO::get_directory_separator();
	}

	Array<LibraryLoadLevel::Type> libraryLoadLevels;
	for_every_ptr(stringEntry, s_localisedStrings->strings)
	{
		libraryLoadLevels.push_back_unique(stringEntry->libraryLoadLevel);
	}

	for_every(language, s_localisedStrings->languages)
	{
		set_language(language->id);
		bool simpleHeader = false;
		for_every(lll, libraryLoadLevels)
		{
			save_to_file(dirName + language->id.to_string() + TXT(".tsv"));
			save_to_file(dirName + language->id.to_string() + TXT(".txt"));
			save_to_file(dirName + language->id.to_string() + String::printf(TXT(" [lll_%i]"), *lll) + TXT(".xml"), *lll, false, false, simpleHeader);
			save_to_file(dirName + language->id.to_string() + String::printf(TXT(" [lll_%i] src"), *lll) + TXT(".xml"), *lll, true, false, simpleHeader);
			save_to_file(dirName + language->id.to_string() + String::printf(TXT(" [lll_%i] src parts"), *lll) + TXT(".xml"), *lll, true, true, simpleHeader);
			simpleHeader = true;
		}
	}

	set_language(storedLanguage);
}

void LocalisedStrings::save_language_to_dir(String const & _dirName, Name const & _language, bool _andReload)
{
	an_assert(s_localisedStrings);

	Name storedLanguage = get_language();

	String dirName = _dirName;
	if (! dirName.is_empty() &&
		dirName.get_right(1) != TXT("\\") &&
		dirName.get_right(1) != TXT("/"))
	{
		dirName += IO::get_directory_separator();
	}

	Array<LibraryLoadLevel::Type> libraryLoadLevels;
	for_every_ptr(stringEntry, s_localisedStrings->strings)
	{
		libraryLoadLevels.push_back_unique(stringEntry->libraryLoadLevel);
	}

	{
		set_language(_language, true);
		bool simpleHeader = false;
		for_every(lll, libraryLoadLevels)
		{
			save_to_file(dirName + _language.to_string() + TXT(".tsv"));
			save_to_file(dirName + _language.to_string() + TXT(".txt"));
			save_to_file(dirName + _language.to_string() + String::printf(TXT(" [lll_%i]"), *lll) + TXT(".xml"), *lll, false, false, simpleHeader);
			save_to_file(dirName + _language.to_string() + String::printf(TXT(" [lll_%i] src"), *lll) + TXT(".xml"), *lll, true, false, simpleHeader);
			save_to_file(dirName + _language.to_string() + String::printf(TXT(" [lll_%i] src parts"), *lll) + TXT(".xml"), *lll, true, true, simpleHeader);
			simpleHeader = true;
		}

		if (_andReload)
		{
			save_to_file(dirName + _language.to_string() + TXT(".xml"), NP, false, false, simpleHeader, _andReload);
		}
	}

	set_language(storedLanguage);
}

bool LocalisedStrings::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(tp, s_localisedStrings->textureParts)
	{
		result &= tp->texturePart.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

bool LocalisedStrings::unload(LibraryLoadLevel::Type _libraryLoadLevel)
{
	bool result = true;

	for_every(tp, s_localisedStrings->textureParts)
	{
		tp->texturePart.clear();
	}

	return result;
}

void LocalisedStrings::translate_all_requiring(String const& _dirName, bool _onlyUpdateHashes)
{
	Name storedLanguage = get_language();

#ifdef AN_WINDOWS
#ifdef BUILD_NON_RELEASE
	g_localisedStringsBeingTranslated = true;
#endif
#endif

	Map<String, LocalisedStringEntry*> sortedStringEntries;
	for_every(stringEntry, s_localisedStrings->strings)
	{
		sortedStringEntries[for_everys_map_key(stringEntry).to_string()] = *stringEntry;
	}

	an_assert(s_localisedStrings);
	for_every(language, s_localisedStrings->languages)
	{
		if (language->id != NAME(defaultLanguage))
		{
			int translated = 0;

			for_every_ptr(entry, sortedStringEntries)
			{
				if (entry->does_require_translation_to(language->id) || (_onlyUpdateHashes
#ifdef BUILD_NON_RELEASE
					&& ! entry->noAutoTranslation
#endif
						))
				{
					entry->translate_to(language->id, _onlyUpdateHashes);
					++translated;

					if (translated > 50 && ! _onlyUpdateHashes)
					{
						save_language_to_dir(_dirName, language->id);
						translated = 0;
					}
				}
			}

			if (translated)
			{
				save_language_to_dir(_dirName, language->id);
			}
		}
	}

#ifdef AN_WINDOWS
#ifdef BUILD_NON_RELEASE
	g_localisedStringsBeingTranslated = false;
#endif
#endif

	set_language(storedLanguage);
}

Array<LocalisedStrings::SortedLanguageLookup> LocalisedStrings::get_sorted_languages()
{
	Array<SortedLanguageLookup> result;
	result.make_space_for(s_localisedStrings->languages.get_size());
	for_every(l, s_localisedStrings->languages)
	{
		SortedLanguageLookup sll;
		sll.idx = for_everys_index(l);
		sll.id = l->id;
		sll.sortId = l->sortId;
		sll.name = l->name;
		result.push_back(sll);
	}
	sort(result);
	return result;
}