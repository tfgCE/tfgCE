#include "soundSampleTextCache.h"

#include "..\..\core\math\math.h"
#include "..\..\core\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

String const& Framework::SampleTextCache::get(String const& _text, Optional<float> const & _at, OPTIONAL_ OUT_ bool* _clear) const
{
	if (cachedLocalisedTextsFor != _text)
	{
		cachedLocalisedTextsFor = _text;
		List<String> tokens;
		cachedLocalisedTextsFor.split(String(TXT("{")), tokens);

		cachedLocalisedTexts.clear();
		cachedLocalisedTexts.make_space_for(tokens.get_size());
		for_every(token, tokens)
		{
			int closeAt = token->find_first_of('}');
			CachedText ct;
			if (closeAt != NONE)
			{
				int paramsAt = token->find_first_of(';');
				int textClosesAt = closeAt;
				if (paramsAt != NONE && paramsAt < closeAt)
				{
					String params = token->get_sub(paramsAt, closeAt - paramsAt + 1);
					ct.clear = String::does_contain_icase(params.to_char(), TXT("clear"));
					textClosesAt = paramsAt;
				}
				ct.at = ParserUtils::parse_float(token->get_left(textClosesAt));
				ct.text = token->get_sub(closeAt + 1);
			}
			else
			{
				// syntax error
				ct.text = *token;
			}
			ct.text = ct.text.trim();
			cachedLocalisedTexts.push_back(ct);
		}

		wholeText = String::empty();
		for_every(clt, cachedLocalisedTexts)
		{
			if (!wholeText.is_empty())
			{
				wholeText += String::space();
			}
			wholeText += clt->text;
		}
	}

	if (_at.is_set() && ! cachedLocalisedTexts.is_empty())
	{
		for_every_reverse(ct, cachedLocalisedTexts)
		{
			if (_at.get() >= ct->at)
			{
				assign_optional_out_param(_clear, ct->clear);
				return ct->text;
			}
		}
		assign_optional_out_param(_clear, cachedLocalisedTexts.get_first().clear);
		return cachedLocalisedTexts.is_empty() ? String::empty() : cachedLocalisedTexts.get_first().text;
	}
	else
	{
		assign_optional_out_param(_clear, false);
		return wholeText;
	}
}

Range Framework::SampleTextCache::get_range(String const& _text) const
{
	Range range = Range::empty;

	get(_text, 0.0f);

	Optional<float> lastNonEmptyAt;
	for_every(ct, cachedLocalisedTexts)
	{
		if (ct->text.is_empty())
		{
			if (lastNonEmptyAt.is_set())
			{
				range.include(ct->at);
			}
			lastNonEmptyAt.clear();
		}
		else
		{
			lastNonEmptyAt = ct->at;
			range.include(ct->at);
		}
	}

	return range;
}

