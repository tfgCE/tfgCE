#include "mc_lcdLetters.h"

#include "..\modules.h"
#include "..\moduleDataImpl.inl"
#include "..\moduleAppearanceImpl.inl"

#include "..\..\library\library.h"

using namespace Framework;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(materialIndex);
DEFINE_STATIC_NAME(letterCount);
DEFINE_STATIC_NAME(content);
DEFINE_STATIC_NAME(defaultContent);
DEFINE_STATIC_NAME(firstLetterParamIndex);
DEFINE_STATIC_NAME(letterColour);
DEFINE_STATIC_NAME(letterColourParamName);
DEFINE_STATIC_NAME(letterPowerParamName);
DEFINE_STATIC_NAME(lcdLetterType);

// material param names
DEFINE_STATIC_NAME(lcdColours);
DEFINE_STATIC_NAME(lcdPower);

//

Framework::LCDLetter::Type Framework::LCDLetter::parse(String const & _value)
{
	if (_value == TXT("s7")	|| _value == TXT("segment7"))		return Segment7;
	if (_value == TXT("d")	|| _value == TXT("directional"))	return Directional;

	error(TXT("LCD Letter type \"%S\" not recognised"), _value.to_char());
	return Segment7;
}

//

Framework::LCDLetters* Framework::LCDLetters::s_lcdLetters = nullptr;
Framework::LCDLetters::Segment7Letter Framework::LCDLetters::s_segment7Empty;

void Framework::LCDLetters::initialise_static()
{
	an_assert(!s_lcdLetters);
	s_lcdLetters = new LCDLetters();
}

void Framework::LCDLetters::close_static()
{
	an_assert(s_lcdLetters);
	delete_and_clear(s_lcdLetters);
}

bool Framework::LCDLetters::load_from_xml(IO::XML::Node const * _node)
{
	an_assert(s_lcdLetters);
	bool result = true;

	for_every(node, _node->children_named(TXT("lcd")))
	{
		String letters = node->get_string_attribute(TXT("letter"));
		if (letters.is_empty())
		{
			error_loading_xml(node, TXT("no letters provided"));
			result = false;
		}
		if (node->get_string_attribute(TXT("type")) == TXT("s7"))
		{
			Segment7Letter s7l = parse(node->get_text());
			for_every(l, letters.get_data())
			{
				if (*l != 0)
				{
					s_lcdLetters->segment7Letters[*l] = s7l;
				}
			}
		}
	}

	return result;
}

Framework::LCDLetters::Segment7Letter const & Framework::LCDLetters::get_segment_7(tchar const & _ch)
{
	an_assert(s_lcdLetters);

	if (auto* e = s_lcdLetters->segment7Letters.get_existing(_ch))
	{
		return *e;
	}
	else
	{
		return s_segment7Empty;
	}
}

Framework::LCDLetters::Segment7Letter Framework::LCDLetters::parse(String const & _text)
{
	String t = _text.keep_only_chars(TXT("-*"));
	Segment7Letter s7l;
	tchar const * tc = t.get_data().begin();
	for_every(s, s7l.on)
	{
		if (*tc == '*')
		{
			*s = true;
		}
		++tc;
	}

	return s7l;
}

//


REGISTER_FOR_FAST_CAST(CustomModules::LCDLetters);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new CustomModules::LCDLetters(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & CustomModules::LCDLetters::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("lcdLetters")), create_module);
}

CustomModules::LCDLetters::LCDLetters(Framework::IModulesOwner* _owner)
: base(_owner)
, letterColourParamName(NAME(lcdColours))
, letterPowerParamName(NAME(lcdPower))
{
}

CustomModules::LCDLetters::~LCDLetters()
{
}

void CustomModules::LCDLetters::reset()
{
	base::reset();
}

void CustomModules::LCDLetters::activate()
{
	base::activate();
	update();
}

void CustomModules::LCDLetters::initialise()
{
	base::initialise();
}

void CustomModules::LCDLetters::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		int tempMaterialIndex = _moduleData->get_parameter<int>(this, NAME(materialIndex), NONE);
		if (tempMaterialIndex != NONE)
		{
			materialIndex = tempMaterialIndex;
		}
		defaultContent = _moduleData->get_parameter<String>(this, NAME(content), defaultContent);
		defaultContent = _moduleData->get_parameter<String>(this, NAME(defaultContent), defaultContent);
		content = defaultContent;
		letterCount = max(1, content.get_length());
		letterCount = _moduleData->get_parameter<int>(this, NAME(letterCount), letterCount);

		firstLetterParamIndex = _moduleData->get_parameter<int>(this, NAME(firstLetterParamIndex), firstLetterParamIndex);
		letterColour = _moduleData->get_parameter<Colour>(this, NAME(letterColour), letterColour);

		letterColourParamName = _moduleData->get_parameter<Name>(this, NAME(letterColourParamName), letterColourParamName);
		letterPowerParamName = _moduleData->get_parameter<Name>(this, NAME(letterPowerParamName), letterPowerParamName);

		String typeRead = _moduleData->get_parameter<String>(this, NAME(lcdLetterType));
		if (!typeRead.is_empty())
		{
			type = LCDLetter::parse(typeRead);
		}
	}
}

void CustomModules::LCDLetters::set_content(tchar const * _content, bool _update)
{
	if ((int)tstrlen(_content) > letterCount)
	{
		content.access_data().set_size(letterCount + 1);
		tchar * dest = content.access_data().begin();
		tchar const * src = _content;
		for_count(int, i, letterCount)
		{
			*dest = *src;
			++ dest;
			++ src;
		}
		*dest = 0;
	}
	else
	{
		content = _content;
	}
	if (_update)
	{
		update();
	}
}

void CustomModules::LCDLetters::set_colour(Colour const & _colour, bool _update)
{
	letterColour = _colour;
	if (_update)
	{
		update();
	}
}

void CustomModules::LCDLetters::set_colour(int _letterIndex, Optional<Colour> const & _colour, bool _update)
{
	while (letterColours.get_size() <= _letterIndex)
	{
		letterColours.push_back(NP);
	}
	letterColours[_letterIndex] = _colour;
	if (_update)
	{
		update();
	}
}

void CustomModules::LCDLetters::clear_individual_colours(bool _update)
{
	for_every(lc, letterColours)
	{
		lc->clear();
	}
	if (_update)
	{
		update();
	}
}

void CustomModules::LCDLetters::update()
{
	{
		Concurrency::ScopedSpinLock lock(paramsLock);

		targetPowerParams.clear();
		targetColourParams.clear();

		if (type == LCDLetter::Segment7)
		{
			for_count(int, l, letterCount)
			{
				int letterAt = firstLetterParamIndex + 7 * l;
				while (targetPowerParams.get_size() < letterAt + 7)
				{
					targetPowerParams.push_back(0.0f);
					targetColourParams.push_back(Colour::black);
				}
				Framework::LCDLetters::Segment7Letter const & s7l = Framework::LCDLetters::get_segment_7(l <= content.get_length() ? content.get_data()[l] : ' ');
				float* powerParam = &targetPowerParams[letterAt];
				Colour* colourParam = &targetColourParams[letterAt];
				Colour useColour = letterColour;
				if (l < letterColours.get_size())
				{
					useColour = letterColours[l].get(useColour);
				}
				bool const* s7ls = s7l.on.begin();
				for_count(int, s, 7)
				{
					*powerParam = *s7ls ? 1.0f : 0.0f;
					*colourParam = useColour;

					++s7ls;
					++powerParam;
					++colourParam;
				}
			}
		}
		else if (type == LCDLetter::Directional)
		{
			for_count(int, l, letterCount)
			{
				int letterAt = firstLetterParamIndex + l;
				while (targetPowerParams.get_size() <= letterAt)
				{
					targetPowerParams.push_back(0.0f);
					targetColourParams.push_back(Colour::black);
				}
				float* powerParam = &targetPowerParams[letterAt];
				Colour* colourParam = &targetColourParams[letterAt];
				Colour useColour = letterColour;
				if (l < letterColours.get_size())
				{
					useColour = letterColours[l].get(useColour);
				}
				tchar c = l < content.get_length() ? content.get_data()[l] : ' ';
				*powerParam = c != ' ' ? 1.0f : 0.0f;
				*colourParam = useColour;
			}
		}
		else
		{
			todo_important(TXT("implement_"));
		}
	}

	if (blendTime == 0.0f)
	{
		update_blend(0.0f);
	}
	else
	{
		mark_requires(all_customs__advance_post);
	}
}

void CustomModules::LCDLetters::advance_post(float _deltaTime)
{
	update_blend(_deltaTime);
}

void CustomModules::LCDLetters::update_blend(float _deltaTime)
{
	Concurrency::ScopedSpinLock lock(paramsLock);

	if (powerParams.get_size() < targetPowerParams.get_size())
	{
		powerParams.make_space_for(targetPowerParams.get_size());
		while (powerParams.get_size() < targetPowerParams.get_size())
		{
			powerParams.push_back(0.0f);
		}
	}
	else
	{
		powerParams.set_size(targetPowerParams.get_size());
	}
	if (colourParams.get_size() < targetColourParams.get_size())
	{
		colourParams.make_space_for(targetColourParams.get_size());
		while (colourParams.get_size() < targetColourParams.get_size())
		{
			colourParams.push_back(Colour::black.with_alpha(0.0f));
		}
	}
	else
	{
		colourParams.set_size(targetColourParams.get_size());
	}

	bool furtherBlendRequired = false;

	{
		auto * pp = powerParams.begin();
		for_every(tpp, targetPowerParams)
		{
			*pp = blend_to_using_time(*pp, *tpp, blendTime, _deltaTime);
			furtherBlendRequired |= (abs(*pp - *tpp) > 0.001f);
			++pp;
		}
	}

	{
		auto * cp = colourParams.begin();
		for_every(tcp, targetColourParams)
		{
			*cp = blend_to_using_time(*cp, *tcp, blendTime, _deltaTime);
			//cp->a = blend_to_using_time(cp->a, tcp->a, blendTime, _deltaTime); // alpha needs a separate blend
			furtherBlendRequired |= ((*cp - *tcp).get_length() > 0.001f);
			++cp;
		}
	}

	for_every_ptr(appearance, get_owner()->get_appearances())
	{
		appearance->set_shader_param(letterPowerParamName, powerParams, materialIndex.get(NONE));
		appearance->set_shader_param(letterColourParamName, colourParams, materialIndex.get(NONE));
	}

	if (!furtherBlendRequired)
	{
		mark_no_longer_requires(all_customs__advance_post);
	}
}
