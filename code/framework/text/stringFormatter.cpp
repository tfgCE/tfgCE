#include "stringFormatter.h"

#include "localisedString.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\other\parserUtils.h"
#include "..\..\core\serialisation\serialiser.h"

#include "stringFormatterSerialisationHandlers.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(ICustomStringFormatter);
REGISTER_FOR_FAST_CAST(IStringFormatterParamsProvider);
REGISTER_FOR_FAST_CAST(CustomStringFormatterStored);

//

BEGIN_SERIALISER_FOR_ENUM(Framework::StringFormatterParam::Type, byte)
	ADD_SERIALISER_FOR_ENUM(0, Framework::StringFormatterParam::String)
	ADD_SERIALISER_FOR_ENUM(1, Framework::StringFormatterParam::Float)
	ADD_SERIALISER_FOR_ENUM(2, Framework::StringFormatterParam::Int)
	ADD_SERIALISER_FOR_ENUM(3, Framework::StringFormatterParam::Custom)
	ADD_SERIALISER_FOR_ENUM(4, Framework::StringFormatterParam::CustomUsingFormat)
END_SERIALISER_FOR_ENUM(Framework::StringFormatterParam::Type)

//

void Framework::make_sure_string_format_is_valid(REF_ String & _format)
{
	if (!_format.is_empty() && !_format.has_prefix(TXT("~")))
	{
		_format = String(TXT("~")) + _format;
	}
}

void Framework::grow_format(REF_ String & _format, String const & _by)
{
	if (! _by.is_empty())
	{
		if (!_by.has_prefix(TXT("~")))
		{
			_format += '~';
		}
		_format += _by;
	}
}

//

void CustomStringFormatterUsingFormat::hard_copy()
{
	iCustom = ICustomStringFormatterPtr(iCustom->get_hard_copy());
}

bool CustomStringFormatterUsingFormat::serialise(Serialiser& _serialiser)
{
	bool result = true;

	result &= StringFormatterSerialisationHandlers::serialise(_serialiser, iCustom);
	String f(format);
	result &= serialise_data(_serialiser, f);
	if (_serialiser.is_reading() && f.get_length())
	{
		int dataSize = sizeof(tchar) * (f.get_length() + 1);
		todo_important(TXT("memory? when it is freed?"));
		tchar * formatStore = (tchar*)allocate_memory(dataSize);
		memory_copy(formatStore, f.get_data().get_data(), dataSize);
		format = formatStore;
		formatMemoryManaged = true;
		result &= serialise_data(_serialiser, f);
	}
	result &= serialise_data(_serialiser, localisedString);

	return result;
}

//

StringFormatParser::StringFormatParser(tchar const * _format)
{
	if (_format)
	{
		int len = (int)tstrlen(_format) + 1;
		format = new tchar[len];
#ifdef AN_CLANG
		tsprintf(format, len, TXT("%S"), _format);
#else
		tsprintf(format, len, TXT("%s"), _format);
#endif
		hasNext = *format == '~';
		current = format;
		if (hasNext)
		{
			++current;
		}
	}
	else
	{
		format = nullptr;
		current = nullptr;
		hasNext = false;
	}
}

StringFormatParser::~StringFormatParser()
{
	if (format)
	{
		delete[] format;
	}
}

tchar const * StringFormatParser::get_next()
{
	if (hasNext)
	{
		tchar const * param = current;

		while (*current != '~' && *current != 0)
		{
			++current;
		}

		hasNext = (*current == '~');
		if (hasNext)
		{
			*current = 0;
			++current;

		}
		return param;
	}
	else
	{
		return nullptr;
	}
}

//

template <typename Class>
Class negate_absolute_percentage(Class _value, bool _negate, bool _absolute, bool _percentage)
{
	if (_percentage) _value = _value * (Class)100;
	if (_absolute) _value = abs(_value);
	if (_negate) _value = -_value;
	return _value;
}

String StringFormatterParam::format(tchar const * _format, Name const & _useLocalisedStringId, StringFormatterParams & _params, bool _negate, bool _absolute, bool _percentage)
{
	int formatLen = _format ? (int)tstrlen(_format) : 0;
	// extract string format
	tchar * stringFormat = nullptr;
	if (formatLen)
	{
		allocate_stack_var(tchar, tempStringFormat, formatLen + 1);
		stringFormat = tempStringFormat;
		tchar const * f = _format;
		tchar * ef = stringFormat;
		while (*f != 0)
		{
			if (*f == ':')
			{
				++f;
				// read stringFormat
				while (*f != 0 && *f != '~')
				{
					*ef = *f;
					++f;
					++ef;
				}
				break;
			}
			++f;
		}
		*ef = 0;
	}
	if (stringFormat)
	{
		sanitise_string_format(stringFormat);
	}
	::String result;
	LocalisedStringEntry const * localisedString = nullptr;
	if (_useLocalisedStringId.is_valid())
	{
		localisedString = LocalisedStrings::get_entry(_useLocalisedStringId, true);
		if (!localisedString)
		{
			error_stop(TXT("<MISSING LOCALISED STRING:%S>"), _useLocalisedStringId.to_char());
			return String::printf(TXT("<MISSING LOCALISED STRING:%S>"), _useLocalisedStringId.to_char());
		}
	}
	if (localisedString)
	{
		if (type == Int)
		{
			result = StringFormatter::format(localisedString->get_form(LocalisedStringRequestedForm(_format, negate_absolute_percentage(asInt, _negate, _absolute, _percentage)).with_params(_params)), _params);
		}
		else if (type == Float)
		{
			float useFloat = negate_absolute_percentage(asFloat, _negate, _absolute, _percentage);
			if (stringFormat && ! LocalisedStringRequestedForm::is_form_auto(stringFormat))
			{
				int const MAX = 64;
				allocate_stack_var(tchar, reformatFloat, MAX + 1);
				int length = (int)tstrlen(stringFormat) + 2 + 1;
				allocate_stack_var(tchar, fullFormat, length);
#ifdef AN_CLANG
				tsprintf(fullFormat, length, TXT("%%%Sf"), stringFormat);
#else
				tsprintf(fullFormat, length, TXT("%%%sf"), stringFormat);
#endif
				tsprintf(reformatFloat, MAX, fullFormat, useFloat);
				useFloat = sane_zero(ParserUtils::parse_float(reformatFloat, useFloat));
			}
			result = StringFormatter::format(localisedString->get_form(LocalisedStringRequestedForm(_format, sane_zero(useFloat)).with_params(_params)), _params);
		}
		else if (type == String)
		{
			an_assert(false, TXT("how is this used? what for?"));
			result = StringFormatter::format(localisedString->get_form(LocalisedStringRequestedForm(_format).with_params(_params)), _params);
		}
		else if (type == Custom || type == CustomUsingFormat)
		{
			ICustomStringFormatter const * usingCustom = type == Custom ? iCustom.get() : customUsingFormat.iCustom.get();
			an_assert(usingCustom);
			if (_format && *_format != 0 && !LocalisedStringRequestedForm::is_form_auto(_format))
			{
				::String const & form = usingCustom->get_actual()->get_form_for_format_custom(_format, _params);
				if (form.is_empty())
				{
					result = StringFormatter::format(localisedString->get_form(LocalisedStringRequestedForm(_format).with_params(_params)), _params);
				}
				else
				{
					int length = ((_format ? (int)tstrlen(_format) : 0) + form.get_length() + 1);
					int _formatLength = (int)tstrlen(_format);
					allocate_stack_var(tchar, wholeFormat, length);
					tsprintf(wholeFormat, length, _format);
					tsprintf(&wholeFormat[_formatLength], length - _formatLength, form.to_char());
					result = StringFormatter::format(localisedString->get_form(LocalisedStringRequestedForm(wholeFormat).with_params(_params)), _params);
				}
			}
			else
			{
				result = StringFormatter::format(localisedString->get_form(LocalisedStringRequestedForm(usingCustom->get_actual()->get_form_for_format_custom(nullptr, _params).to_char()).with_params(_params)), _params);
			}
		}
		else
		{
			an_assert(false, TXT("is there something missing?"));
			error_stop(TXT("<INVALID>"));
			result = TXT("<INVALID>");
		}
	}
	else
	{
		if (type == Int)
		{
			if (stringFormat && !LocalisedStringRequestedForm::is_form_auto(stringFormat))
			{
				int length = (int)tstrlen(stringFormat) + 2 + 1;
				allocate_stack_var(tchar, fullFormat, length);
#ifdef AN_CLANG
				tsprintf(fullFormat, length, TXT("%%%Si"), stringFormat);
#else
				tsprintf(fullFormat, length, TXT("%%%si"), stringFormat);
#endif
				result = String::printf(fullFormat, negate_absolute_percentage(asInt, _negate, _absolute, _percentage));
			}
			else
			{
				result = String::printf(TXT("%i"), negate_absolute_percentage(asInt, _negate, _absolute, _percentage));
			}
			if (_percentage)
			{
				result += '%';
			}
		}
		else if (type == Float)
		{
			if (stringFormat && LocalisedStringRequestedForm::is_form_auto(stringFormat))
			{
				result = ParserUtils::float_to_string_auto_decimals(sane_zero(negate_absolute_percentage(asFloat, _negate, _absolute, _percentage)));
			}
			else if (stringFormat)
			{
				int length = (int)tstrlen(stringFormat) + 2 + 1;
				allocate_stack_var(tchar, fullFormat, length);
#ifdef AN_CLANG
				tsprintf(fullFormat, length, TXT("%%%Sf"), stringFormat);
#else
				tsprintf(fullFormat, length, TXT("%%%sf"), stringFormat);
#endif
				result = String::printf(fullFormat, sane_zero(negate_absolute_percentage(asFloat, _negate, _absolute, _percentage)));
			}
			else
			{
				result = String::printf(TXT("%f"), sane_zero(negate_absolute_percentage(asFloat, _negate, _absolute, _percentage)));
			}
			if (_percentage)
			{
				result += '%';
			}
		}
		else if (type == String)
		{
			if (stringFormat)
			{
				int length = (int)tstrlen(stringFormat) + 2 + 1;
				allocate_stack_var(tchar, fullFormat, length);
#ifdef AN_CLANG
				tsprintf(fullFormat, length, TXT("%%%SS"), stringFormat);
#else
				tsprintf(fullFormat, length, TXT("%%%ss"), stringFormat);
#endif
				result = String::printf(fullFormat, asString.to_char());
			}
			else
			{
				result = asString;
			}
		}
		else if (type == Custom)
		{
			an_assert(iCustom.is_set());
			result = iCustom->get_actual()->format_custom(_format, _params);
		}
		else if (type == CustomUsingFormat)
		{
			an_assert(customUsingFormat.iCustom.is_set());
			if (customUsingFormat.inside)
			{
				result = customUsingFormat.iCustom->get_actual()->format_custom(_format, _params);
			}
			else
			{
				customUsingFormat.inside = true; // since now it will be treated as normal custom (check block above)
				ForStringFormatterParams fsfp(_params);
				_params.push_additional_format(_format);
				if (customUsingFormat.localisedString.is_valid())
				{
					result = StringFormatter::format(customUsingFormat.localisedString.get_form(LocalisedStringRequestedForm(_format).with_params(_params)), _params);
				}
				else
				{
					if (!customUsingFormat.format)
					{
						error_stop(TXT("<MISSING CUSTOM FORMAT>"));
						return String::printf(TXT("<MISSING CUSTOM FORMAT>"));
					}
					result = AVOID_CALLING_ACK_ StringFormatter::format(::String(customUsingFormat.format), _params);
				}
				customUsingFormat.inside = false;
			}
		}
		else
		{
			an_assert(false, TXT("is there something missing?"));
			error_stop(TXT("<INVALID>"));
			result = TXT("<INVALID>");
		}
	}

	return result;
}

bool StringFormatterParam::serialise(Serialiser& _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, id);
	result &= serialise_data(_serialiser, type);
	if (type == String)
	{
		result &= serialise_data(_serialiser, asString);
	}
	if (type == Float)
	{
		result &= serialise_data(_serialiser, asFloat);
	}
	if (type == Int)
	{
		result &= serialise_data(_serialiser, asInt);
	}
	if (type == Custom)
	{
		result &= StringFormatterSerialisationHandlers::serialise(_serialiser, iCustom);
	}
	if (type == CustomUsingFormat)
	{
		result &= serialise_data(_serialiser, customUsingFormat);
	}

	return result;
}

void StringFormatterParam::hard_copy()
{
	if (type == Custom)
	{
		iCustom = ICustomStringFormatterPtr(iCustom->get_hard_copy());
	}
	if (type == CustomUsingFormat)
	{
		customUsingFormat.hard_copy();
	}
}

//

ForStringFormatterParams::ForStringFormatterParams(StringFormatterParams& _params)
: params(_params)
{
}

ForStringFormatterParams::~ForStringFormatterParams()
{
	if (keepParams)
	{
		params.keep_params(keepParamsCount);
	}

	if (addedProviders)
	{
		params.keep_params_providers(keepProvidersCount);
	}

	while (pushedAdditionalFormats > 0)
	{
		--pushedAdditionalFormats;
		params.pop_additional_format();
	}
}

void ForStringFormatterParams::keep_params()
{
	if (!keepParams)
	{
		keepParams = true;
		keepParamsCount = params.get_param_count();
	}
}

void ForStringFormatterParams::add_params_provider(IStringFormatterParamsProvider const * _provider)
{
	if (!addedProviders)
	{
		addedProviders = true;
		keepProvidersCount = params.get_params_providers_count();
	}
	params.add_params_provider(_provider);
}

void ForStringFormatterParams::add_params_provider(ProvideStringFormatterParams _provider)
{
	if (!addedProviders)
	{
		addedProviders = true;
		keepProvidersCount = params.get_params_providers_count();
	}
	params.add_params_provider(_provider);
}

void ForStringFormatterParams::push_additional_format(tchar const * _format)
{
	++ pushedAdditionalFormats;
	params.push_additional_format(_format);
}

//

StringFormatterParams::StringFormatterParams()
{
	SET_EXTRA_DEBUG_INFO(additionalFormatStack, TXT("StringFormatterParams.additionalFormatStack"));
	SET_EXTRA_DEBUG_INFO(paramsProviders, TXT("StringFormatterParams.paramsProviders"));
	params.make_space_for(20);
}

void StringFormatterParams::clear()
{
	params.clear();
	additionalFormatStack.clear();
	paramsProviders.clear();
}

void StringFormatterParams::prune()
{
	params.prune();
}

void StringFormatterParams::remove_params_at(int _at, int _count)
{
	params.remove_at(_at, _count);
}

StringFormatterParams & StringFormatterParams::add_params_from(StringFormatterParams const & _params)
{
	params.make_space_for_additional(_params.params.get_size());
	for_every(param, _params.params)
	{
		add(*param);
	}
	return *this;
}

StringFormatterParam const * StringFormatterParams::find_existing_only(Name const & _id) const
{
	for_every_reverse(param, params)
	{
		if (param->get_id() == _id)
		{
			return param;
		}
	}
	return nullptr;
}

StringFormatterParam * StringFormatterParams::find_or_add(Name const & _id)
{
	if (auto * param = find(_id))
	{
		return param;
	}
	params.push_back(StringFormatterParam(_id, 0));
	return &(params.get_last());
}

StringFormatterParam * StringFormatterParams::find(Name const & _id)
{
	for_every_reverse(param, params)
	{
		if (param->get_id() == _id)
		{
			return param;
		}
	}
	if (_id.is_valid())
	{
		return add_from_providers(_id);
	}
	else
	{
		return nullptr;
	}
}

StringFormatterParam * StringFormatterParams::find(tchar const * _id)
{
	if (_id == 0)
	{
		for_every_reverse(param, params)
		{
			if (! param->get_id().is_valid())
			{
				return param;
			}
		}
	}
	for_every_reverse(param, params)
	{
		if (String::compare_icase(param->get_id().to_char(), _id))
		{
			return param;
		}
	}
	if (_id)
	{
		return add_from_providers(Name(_id));
	}
	else
	{
		return nullptr;
	}
}

StringFormatterParam * StringFormatterParams::add_from_providers(Name const & _id)
{
	for_every_reverse(provider, paramsProviders)
	{
		bool added = false;
		if (provider->provider) added = provider->provider->requested_string_formatter_param(REF_ *this, _id);
		if (!added && provider->provideFunc) added = provider->provideFunc(REF_ *this, _id);
		if (added)
		{
			for_every_reverse(param, params)
			{
				if (param->get_id() == _id)
				{
					return param;
				}
			}
			an_assert(false, TXT("param was requested, provider told it added, but there is no such param!"));
		}
	}
	return nullptr;
}

bool StringFormatterParams::serialise(Serialiser& _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, dateTimeFormat);
	result &= serialise_data(_serialiser, defaultParamId);
	result &= serialise_data(_serialiser, params);

	an_assert(additionalFormatStack.is_empty());
	an_assert(paramsProviders.is_empty());

	return result;
}

void StringFormatterParams::hard_copy()
{
	for_every(param, params)
	{
		param->hard_copy();
	}

	remove_params_providers();
}

//

String StringFormatter::format_sentence_loc_str(LocalisedString const& _localisedString, StringFormatterParams& _params, int _formatParagraphs)
{
	return format_loc_str(_localisedString, _params).nice_sentence(_formatParagraphs);
}

String StringFormatter::format_sentence_loc_str(Name const & _localisedStringID, StringFormatterParams & _params, int _formatParagraphs)
{
	return format_loc_str(_localisedStringID, _params).nice_sentence(_formatParagraphs);
}

String StringFormatter::format_sentence_loc_str(Name const & _localisedStringID, tchar const * _format, StringFormatterParams & _params, int _formatParagraphs)
{
	return format_loc_str(_localisedStringID, _format, _params).nice_sentence(_formatParagraphs);
}

String StringFormatter::format_loc_str(Name const & _localisedStringID, StringFormatterParams & _params)
{
	return format_loc_str(_localisedStringID, nullptr, _params);
}

String StringFormatter::format_loc_str(LocalisedString const& _localisedString, StringFormatterParams& _params)
{
	return format_loc_str(_localisedString, nullptr, _params);
}

String StringFormatter::format_loc_str(Name const & _localisedStringID, tchar const * _format, StringFormatterParams & _params)
{
	if (LocalisedStringEntry const * localisedStringEntry = LocalisedStrings::get_entry(_localisedStringID, true))
	{
		if (LocalisedStringForm const * localisedStringForm = localisedStringEntry->get_form(LocalisedStringRequestedForm(_format).with_params(_params)))
		{
			if (localisedStringForm->get_string_formatter_elements().is_empty())
			{
				return localisedStringForm->value;
			}
			else
			{
				return format(localisedStringForm->get_string_formatter_elements(), _format, _params);
			}
		}
		else
		{
			error_stop(TXT("<MISSING LOCALISED STRING FORM FOR:%S>"), _localisedStringID.to_char());
			return String::printf(TXT("<MISSING LOCALISED STRING FORM FOR:%S>"), _localisedStringID.to_char());
		}
	}
	else
	{
		error_stop(TXT("<MISSING LOCALISED STRING:%S>"), _localisedStringID.to_char());
		return String::printf(TXT("<MISSING LOCALISED STRING:%S>"), _localisedStringID.to_char());
	}
}

String StringFormatter::format_loc_str(LocalisedString const & _localisedString, tchar const * _format, StringFormatterParams & _params)
{
	if (LocalisedStringEntry const * localisedStringEntry = _localisedString.get_entry())
	{
		if (LocalisedStringForm const * localisedStringForm = localisedStringEntry->get_form(LocalisedStringRequestedForm(_format).with_params(_params)))
		{
			if (localisedStringForm->get_string_formatter_elements().is_empty())
			{
				return localisedStringForm->value;
			}
			else
			{
				return format(localisedStringForm->get_string_formatter_elements(), _format, _params);
			}
		}
		else
		{
			error_stop(TXT("<MISSING LOCALISED STRING FORM FOR:%S>"), _localisedString.get_id().to_char());
			return String::printf(TXT("<MISSING LOCALISED STRING FORM FOR:%S>"), _localisedString.get_id().to_char());
		}
	}
	else
	{
		error_stop(TXT("<MISSING LOCALISED STRING:%S>"), _localisedString.get_id().to_char());
		return String::printf(TXT("<MISSING LOCALISED STRING:%S>"), _localisedString.get_id().to_char());
	}
}

String StringFormatter::format(LocalisedStringForm const * _form, StringFormatterParams & _params)
{
	if (_form)
	{
		if (_form->get_string_formatter_elements().is_empty())
		{
			return _form->value;
		}
		else
		{
			return format(_form->get_string_formatter_elements(), nullptr, _params);
		}
	}
	else
	{
		error_stop(TXT("<MISSING LOCALISED STRING FORM>"));
		return String::printf(TXT("<MISSING LOCALISED STRING FORM>"));
	}
}

String StringFormatter::format_sentence(String const & _string, StringFormatterParams & _params, int _formatParagraphs)
{
	return format(_string, _params).nice_sentence(_formatParagraphs);
}

void StringFormatter::process_element(StringFormatterElement const & _element, OUT_ String & _result, StringFormatterParams & _params)
{
	if (!_element.isParam)
	{
		_result += _element.text;
	}
	else
	{
		ForStringFormatterParams fsfp(_params);

		tchar const * parameterFormat = _element.format.get_data().get_data();
		int parameterFormatLength = _element.format.get_length();
		int newParameterFormatLength = parameterFormatLength;
		bool additionalFormatNonEmpty = !_params.additionalFormatStack.is_empty() && (*_params.additionalFormatStack.get_last());
		if (additionalFormatNonEmpty)
		{
			newParameterFormatLength += (int)tstrlen(_params.additionalFormatStack.get_last());
		}
		allocate_stack_var(tchar, newParameterFormat, 1 + newParameterFormatLength);

		// create new parameter format if needed (using additional format)
		String resultParam;
		if (parameterFormatLength != newParameterFormatLength)
		{
			// build whole format using last additional format (it should have all formats that we gathered on the way)
			tchar * dest = newParameterFormat;
			memory_copy(dest, parameterFormat, sizeof(tchar) * parameterFormatLength);
			dest += parameterFormatLength;
			{
				tchar const * additionalFormat = _params.additionalFormatStack.get_last();
				int length = (int)tstrlen(additionalFormat);
				memory_copy(dest, additionalFormat, sizeof(tchar) * length);
				dest += length;
			}
			*dest = 0;
			// use this one
			parameterFormat = newParameterFormat;
			fsfp.push_additional_format(nullptr); // to mark that this format is already incorporated
		}

		if (_element.param == TXT("percentage-sign"))
		{
			resultParam += TXT("%");
		}
		else if (!_element.param.is_valid()) // not for param, check default
		{
			Name const & defaultParamName = _params.get_default_param_id();
			if (defaultParamName.is_valid())
			{
				// use default param!
				if (auto * param = _params.find(defaultParamName))
				{
					resultParam += param->format(parameterFormat, _element.useLocStrId, _params, _element.negateParam, _element.absoluteParam, _element.percentageParam);
				}
				else
				{
					error_stop(TXT("<MISSING DEFAULT PARAM:%S>"), defaultParamName);
					resultParam += String::printf(TXT("<MISSING DEFAULT PARAM:%S>"), defaultParamName);
				}
			}
			else
			{
				if (_element.useLocStrId.is_valid())
				{
					if (LocalisedStringEntry const * localisedString = LocalisedStrings::get_entry(_element.useLocStrId, true))
					{
						resultParam += StringFormatter::format(localisedString->get(LocalisedStringRequestedForm(parameterFormat).with_params(_params)), _params);
					}
					else
					{
						error_stop(TXT("<MISSING LOCALISED STRING:%S>"), _element.useLocStrId.to_char());
						resultParam += String::printf(TXT("<MISSING LOCALISED STRING:%S>"), _element.useLocStrId.to_char());
					}
				}
				else
				{
					error_stop(TXT("<NO PARAM AND NO LOCSTRINGID>"));
					resultParam += String::printf(TXT("<NO PARAM AND NO LOCSTRINGID>"));
				}
			}
		}
		else if (auto * param = _params.find(_element.param))
		{
			// set it as default
			Name prevDefaultParam = _params.replace_default_param_id(_element.param);
			resultParam += param->format(parameterFormat, _element.useLocStrId, _params, _element.negateParam, _element.absoluteParam, _element.percentageParam);
			_params.set_default_param_id(prevDefaultParam); // restore
		}
		else
		{
			error_stop(TXT("<MISSING PARAM:%S>"), _element.param.to_char());
			resultParam += String::printf(TXT("<MISSING PARAM:%S>"), _element.param.to_char());
		}

		if (_element.uppercaseCount > 1)
		{
			resultParam = resultParam.to_upper();
		}
		else if (_element.uppercaseCount == 1)
		{
			resultParam = resultParam.get_left(1).to_upper() + resultParam.get_sub(1);
		}
		_result += resultParam;
	}
}

String StringFormatter::format(Array<StringFormatterElement> const & _elements, tchar const * _format, StringFormatterParams & _params)
{
	todo_note(TXT("push format onto stack?"));
	String result;
	result.access_data().make_space_for(1024);
	for_every(element, _elements)
	{
		process_element(*element, result, _params);
	}
	return result;
}

String StringFormatter::format(String const & _string, StringFormatterParams & _params)
{
	String result;
	result.access_data().make_space_for(_string.get_length() + 1);

	StringFormatterElement::extract_from(_string,
		[&result, &_params](bool _isParam, tchar const * _text, tchar const * _paramName, tchar const * _format, tchar const * _parameterLocalisedStringId, bool _negateParam, bool _absoluteParam, bool _percentageParam, int _uppercaseCount)
	{
		if (!_isParam)
		{
			result += _text;
		}
		else
		{
			StringFormatterElement element;
			element.isParam = true;
			element.param = _paramName ? Name(_paramName) : Name::invalid();
			element.useLocStrId = _parameterLocalisedStringId ? Name(_parameterLocalisedStringId) : Name::invalid();
			element.negateParam = _negateParam;
			element.absoluteParam = _absoluteParam;
			element.percentageParam = _percentageParam;
			element.uppercaseCount = _uppercaseCount;
			element.format = _format;
			process_element(element, OUT_ result, _params);
		}
	});

	return result;
}
