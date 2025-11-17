#include "textBlockInstance.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"
#include "..\..\core\serialisation\serialiser.h"

using namespace Framework;

TextBlockInstance::TextBlockInstance()
{
}

TextBlockInstance::~TextBlockInstance()
{
	delete_and_clear(stringFormatterParams);
}

String TextBlockInstance::format() const
{
	return element.get() && stringFormatterParams ? element->format_custom(usingFormat.to_char(), *stringFormatterParams).nice_sentence(element->textBlock->get_allow_empty_lines()) : String::empty();
}

String TextBlockInstance::format_custom(tchar const * _format, StringFormatterParams & _params) const
{
	// most likely this is called through format() and we should just accept both _format and _params
	return element.get() && stringFormatterParams ? element->format_custom(_format, _params).nice_sentence(element->textBlock->get_allow_empty_lines()) : String::empty();
}

TextBlockInstance::ElementPtr TextBlockInstance::build_element_from_piece(PieceCombiner::PieceInstance<TextBlockGenerator> const * _pieceInstance)
{
	Element* element = new Element();

	element->textBlock = _pieceInstance->get_piece()->def.textBlock.get();
	for_every_ptr(connector, _pieceInstance->get_owned_connectors())
	{
		if (connector->get_connector()->def.paramId.is_valid())
		{
			ElementPtr paramElement(build_element_from_piece(connector->get_owner_on_other_end()));
			if (paramElement.is_set())
			{
				element->params.push_back(Param(connector->get_connector()->def.paramId, paramElement.get()));
			}
		}
	}

	return ElementPtr(element);
}

TextBlockInstancePtr TextBlockInstance::build_from_piece(PieceCombiner::PieceInstance<TextBlockGenerator> const * _pieceInstance, tchar const * _format, StringFormatterParams& _params)
{
	if (!_pieceInstance)
	{
		return TextBlockInstancePtr();
	}

	TextBlockInstance* tbi = new TextBlockInstance();
	tbi->stringFormatterParams = new StringFormatterParams();
	*(tbi->stringFormatterParams) = _params;
	tbi->stringFormatterParams->prune();
	tbi->usingFormat = _format;

	tbi->element = build_element_from_piece(_pieceInstance);

	// this is main piece, format it to gather params and then create hard copy of params (dump providers... and create copies of some parameters)
	tbi->format();
	tbi->stringFormatterParams->hard_copy();

	return TextBlockInstancePtr(tbi);
}

TextBlockInstancePtr TextBlockInstance::build_from_text_block(TextBlock * _textBlock, tchar const * _format, StringFormatterParams & _params)
{
	if (!_textBlock)
	{
		return TextBlockInstancePtr();
	}

	TextBlockInstance* tbi = new TextBlockInstance();
	tbi->stringFormatterParams = new StringFormatterParams();
	*(tbi->stringFormatterParams) = _params;
	tbi->stringFormatterParams->prune(); 
	tbi->usingFormat = _format;

	tbi->element = new Element();
	tbi->element->textBlock = _textBlock;

	// this is main piece, format it to gather params and then create hard copy of params (dump providers... and create copies of some parameters)
	tbi->format();
	tbi->stringFormatterParams->hard_copy();

	return TextBlockInstancePtr(tbi);
}

bool TextBlockInstance::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_ref_data_if_present(_serialiser, element);

	result &= serialise_data_if_present(_serialiser, stringFormatterParams);
	result &= serialise_data(_serialiser, usingFormat);

	return result;
}


//

String TextBlockInstance::Element::format_custom(tchar const * _format, StringFormatterParams & _params) const
{
	int initialParamsCount = _params.get_param_count();
	for_every(param, params)
	{
		_params.add(param->paramId, param->textBlockInstance.get());
	}
	int addedParamsCount = _params.get_param_count() - initialParamsCount;
	String result = textBlock->format_text_for_instance(_format, _params);
	if (addedParamsCount > 0)
	{
		// remove just params we added now
		_params.remove_params_at(initialParamsCount, addedParamsCount);
	}
	return result;
}

bool TextBlockInstance::Element::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, textBlock);
	result &= serialise_data(_serialiser, params);

	return result;
}

//

bool TextBlockInstance::Param::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, paramId);
	result &= serialise_ref_data_if_present(_serialiser, textBlockInstance);

	return result;
}

//

String TextBlockInstances::format(tchar const * _newLine, int _emptyLinesBetween) const
{
	String result;
	String fakeNewLine(TXT("\x1")); // kind of hacky but we don't want nice sentence to intervene
	for_every(tbi, textBlocks)
	{
		if (auto const * t = tbi->get())
		{
			result += t->format().nice_sentence(_emptyLinesBetween); // allow max empty lines between
		}
		else if (for_everys_index(tbi) != textBlocks.get_size() - 1)
		{
			result += fakeNewLine;
		}
	}
	result = result.nice_sentence(_emptyLinesBetween); // allow max empty lines between
	String paragraphBreak;
	for_range(int, i, 0, _emptyLinesBetween)
	{
		paragraphBreak += _newLine;
	}
	result = result.replace(fakeNewLine, paragraphBreak); // to allow multiple lines between if requested but keep nice_sentence's work
	result = result.nice_sentence(_emptyLinesBetween); // allow max empty lines between
	return result;
}

bool TextBlockInstances::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, textBlocks);

	return result;
}

void TextBlockInstances::add_instance_of(TextBlock* _tb, tchar const * _format, StringFormatterParams & _params, Random::Generator & _rg)
{
	if (_tb)
	{
		add(_tb->create_instance(_format, _params, _rg));
	}
}
