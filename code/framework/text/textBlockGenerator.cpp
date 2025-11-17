#include "textBlockGenerator.h"

#include "textBlock.h"

using namespace Framework;

//

String TextBlockGenerator::PieceDef::get_desc() const
{
	return textBlock->get_name().to_string();
}

//

bool TextBlockGenerator::Utils::should_use_connector(PieceCombiner::Generator<TextBlockGenerator> const & _generator, PieceCombiner::PieceInstance<TextBlockGenerator> const & _pieceInstance, PieceCombiner::Connector<TextBlockGenerator> const & _connector)
{
	if (_pieceInstance.get_piece()->def.textBlock.get())
	{
		an_assert(_generator.get_generation_context().params);
		if (LocalisedStringForm const * locStrForm = _pieceInstance.get_piece()->def.textBlock->get_text().get_form(LocalisedStringRequestedForm().with_params(*_generator.get_generation_context().params)))
		{
			if (_connector.def.paramId.is_valid())
			{
				for_every(element, locStrForm->get_string_formatter_elements())
				{
					if (element->isParam &&
						_connector.def.paramId == element->param)
					{
						return true;
					}
				}

				// connector is for param but there is no such parameter in localised string form
				return false;
			}
		}
	}

	return true;
}
