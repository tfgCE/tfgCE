#include "stringFormatterElement.h"

#include "..\..\core\containers\arrayStack.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

void StringFormatterElement::extract_from(String const & _string, std::function<void(bool _isParam, tchar const * _text, tchar const * _paramName, tchar const * _format, tchar const * _parameterLocalisedStringId, bool _negateParam, bool _absoluteParam, bool _percentageParam, int _uppercaseCount)> _on_param)
{
	// this doubles string formatter function
	String text;
	text.access_data().make_space_for(_string.get_length() + 1);

	ARRAY_STACK(tchar, parameterRead, _string.get_length() + 1);
	ARRAY_STACK(tchar, parameterName, _string.get_length() + 1);
	ARRAY_STACK(tchar, parameterLocalisedStringId, _string.get_length() + 1);
	bool parameterOpen = false;
	for_every(ch, _string.get_data())
	{
		if (*ch == '%')
		{
			if (parameterOpen)
			{
				if (parameterRead.is_empty())
				{
					text += '%'; // just add percentage sign
				}
				else
				{
					parameterRead.push_back(0);
					parameterName.clear();
					parameterLocalisedStringId.clear();
					bool loadLocalisedStringId = false;
					int uppercaseCount = 0;
					bool anythingIn = false;
					tchar const * parameterFormat = &parameterRead.get_last(); // points at 0 at the end
					for_every(pCh, parameterRead)
					{
						if (*pCh == 0)
						{
							// stop
							break;
						}
						if (*pCh == '^' && !anythingIn)
						{
							++uppercaseCount;
							// just ignore for now
							continue;
						}
						anythingIn = true;
						if (*pCh == '@')
						{
							loadLocalisedStringId = true;
							continue; // continue with next character
						}
						if (*pCh == '~')
						{
							parameterFormat = pCh; // will point at format ~
							break;
						}
						else
						{
							if (loadLocalisedStringId)
							{
								parameterLocalisedStringId.push_back(*pCh); // add next char
							}
							else
							{
								parameterName.push_back(*pCh); // add next char
							}
						}
					}
					parameterLocalisedStringId.push_back(0); // close it
					parameterName.push_back(0); // close it

					bool negateParam = false;
					bool absoluteParam = false;
					bool percentageParam = false;
					tchar * useParameterName = parameterName.get_data();
					if (*useParameterName == '-')
					{
						++useParameterName;
						negateParam = true;
					}
					if (*useParameterName == '|')
					{
						++useParameterName;
						absoluteParam = true;
					}
					if (*useParameterName == '`')
					{
						++useParameterName;
						percentageParam = true;
					}

					// we've read text earlier, use it
					if (!text.is_empty())
					{
						_on_param(false, text.get_data().get_data(), nullptr, nullptr, nullptr, false, false, false, 0);
						text = String::empty();
					}

					_on_param(true, nullptr, tstrlen(useParameterName) > 0? useParameterName : nullptr, parameterFormat, parameterLocalisedStringId.get_size() > 1 ? parameterLocalisedStringId.get_data() : nullptr, negateParam, absoluteParam, percentageParam, uppercaseCount);

				}
				parameterOpen = false;
			}
			else
			{
				parameterOpen = true;
				parameterRead.clear();
			}
		}
		else if (*ch != 0)
		{
			if (parameterOpen)
			{
				parameterRead.push_back(*ch);
			}
			else
			{
				text += *ch;
			}
		}
	}

	if (parameterOpen)
	{
		parameterRead.push_back(0);
		error(TXT("parameter \"%S\" not closed in \"%S\". if you want to put %% sign, use %%%% or add \"ignoreFormatting=\"true\"\"?"), parameterRead.get_data(), _string.to_char());
	}

	if (!text.is_empty())
	{
		_on_param(false, text.get_data().get_data(), nullptr, nullptr, nullptr, false, false, false, 0);
	}
}

void StringFormatterElement::extract_from(String const & _string, OUT_ Array<StringFormatterElement> & _elements)
{
	extract_from(_string,
		[&_elements](bool _isParam, tchar const * _text, tchar const * _paramName, tchar const * _format, tchar const * _parameterLocalisedStringId, bool _negateParam, bool _absoluteParam, bool _percentageParam, int _uppercaseCount)
	{
		if (!_isParam)
		{
			StringFormatterElement element;
			element.text = _text;
			_elements.push_back(element);
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
			_elements.push_back(element);
		}
	});
}
