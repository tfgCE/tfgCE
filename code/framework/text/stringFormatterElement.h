#pragma once

#include "..\..\core\types\name.h"
#include "..\..\core\types\string.h"

#include <functional>

namespace Framework
{
	struct StringFormatterElement
	{
		//
		String text;
		//
		bool isParam = false;
		Name param;
		Name useLocStrId;
		String format;
		bool negateParam = false;
		bool absoluteParam = false;
		bool percentageParam = false;
		int uppercaseCount = 0;
		//

		static void extract_from(String const & _string, OUT_ Array<StringFormatterElement> & _elements);
		static void extract_from(String const & _string, std::function<void(bool _isParam, tchar const * _text, tchar const * _paramName, tchar const * _format, tchar const * _parameterLocalisedStringId, bool _negateParam, bool _absoluteParam, bool _percentageParam, int _uppercaseCount)> _on_param);
	};
};
