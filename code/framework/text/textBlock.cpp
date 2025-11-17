#include "textBlock.h"

#include "textBlockInstance.h"

#include "stringFormatter.h"

#include "..\library\library.h"
#include "..\library\libraryLoadingContext.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"

using namespace Framework;

//

DEFINE_STATIC_NAME_STR(ptb, TXT("private text block"));

//

REGISTER_FOR_FAST_CAST(TextBlock);
LIBRARY_STORED_DEFINE_TYPE(TextBlock, textBlock);

TextBlock::TextBlock(Library * _library, LibraryName const & _name)
: base(_library, _name)
, textBlockPiece( new PieceCombiner::Piece<TextBlockGenerator>() )
{
	textBlockPiece->def.textBlock.lock(this);
}

bool TextBlock::create_private_from_xml(bool _main, IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _id, int _idx, OUT_ RefCountObjectPtr<TextBlock> & _created)
{
	// build just using 
	String idIdx = String::printf(TXT("%S [%i]"), _id, _idx);
	ForLibraryLoadingContext flc(_lc);
	flc.push_id(idIdx.to_char());

	// this is full id
	String ptbName = String::printf(TXT("%S [%i]"), _lc.build_id().to_char(), _idx);
	LibraryName useName(NAME(ptb), Name(ptbName));
	bool result = true;

	result &= _lc.get_library()->get_text_blocks().load_from_xml(_node, _lc, useName);

	_created = _lc.get_library()->get_text_blocks().find_may_fail(useName);
	if (_created.is_set())
	{
		_created->be_private();
		_created->isMain.set_if_not_set(_main); // assume if not set otherwise
	}

	return result;
}

bool TextBlock::load_private_into_array_from_xml(bool _main, IO::XML::Node const * _groupNode, LibraryLoadingContext & _lc, tchar const * _id, REF_ Array<TextBlockPtr> & _array, std::function<void(TextBlock*)> _perform_on_loaded_text_block)
{
	bool result = true;

	for_every(node, _groupNode->children_named(TXT("textBlock")))
	{
		RefCountObjectPtr<TextBlock> newTextBlock;
		result &= create_private_from_xml(_main, node, _lc, _id, _array.get_size(), newTextBlock);
		if (newTextBlock.is_set())
		{
			_array.push_back(TextBlockPtr(newTextBlock.get()));
			if (_perform_on_loaded_text_block)
			{
				_perform_on_loaded_text_block(newTextBlock.get());
			}
		}
	}

	return result;
}

bool TextBlock::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	ForLibraryLoadingContext flc(_lc);
	flc.push_owner(this);

	allowEmptyLines = _node->get_int_attribute_or_from_child(TXT("allowEmptyLines"), allowEmptyLines);

	text.load_from_xml_child_required(_node, TXT("text"), _lc, TXT("text block"));

	if (auto const * node = _node->first_child_named(TXT("asTextBlockPiece")))
	{
		TextBlockGenerator::LoadingContext loadingContext;
		result &= textBlockPiece->load_from_xml(node, &loadingContext);
	}
	
	{
		float probability = textBlockPiece->get_probability();
		probability = _node->get_float_attribute_or_from_child(TXT("probability"), probability);
		probability = _node->get_float_attribute_or_from_child(TXT("probCoef"), probability);
		probability = _node->get_float_attribute_or_from_child(TXT("probabilityCoef"), probability);
		probability = _node->get_float_attribute_or_from_child(TXT("chance"), probability);
		textBlockPiece->set_probability(probability);
	}

	{
		bool loadedAnything = false;
		result &= textBlockPiece->load_plug_connectors_from_xml(_node, nullptr, TXT("asParam"), &loadedAnything);
		if (loadedAnything)
		{
			isMain = false; // assume it is not main if has plug connectors
		}
	}

	// note: can't use piece combiner load sub connectors as we have sub text blocks here
	// auto connectors + sub text blocks
	for_every(groupNode, _node->children_named(TXT("paramConnectors")))
	{
		for_every(paramNode, groupNode->children_named(TXT("param")))
		{
			textOnly = false; // we can't be text only then
			Name paramId = paramNode->get_name_attribute(TXT("id"));
			Name paramConnector = paramNode->get_name_attribute(TXT("connector")); // this is connector for both internal text blocks, but is also used for external
			if (!paramId.is_valid())
			{
				error_loading_xml(paramNode, TXT("no id provided for param in paramConnectors"));
				result = false;
				continue;
			}
			bool ownParamConnector = false;
			if (!paramConnector.is_valid())
			{
				// use internal connector
				paramConnector = Name(_lc.build_id() + TXT("; ") + paramId.to_char());
				ownParamConnector = true;
			}

			// load private text blocks
			bool loadedPrivateTextBlocks = false;
			load_private_into_array_from_xml(false, paramNode, _lc, paramId.to_char(), subTextBlocks,
				[&paramConnector, &loadedPrivateTextBlocks](TextBlock* _subTextBlock)
				{	// we will connect to parent through this
					PieceCombiner::Connector<TextBlockGenerator> connector;
					connector.be_normal_connector();
					connector.make_required_single_only();
					connector.make_can_create_new_pieces(false);
					connector.set_link_ps_as_plug(paramConnector.to_char());
					_subTextBlock->textBlockPiece->add_connector(connector);
					loadedPrivateTextBlocks = true;
				}
				);

			Array<int> connectorIndices;
			{
				// these connectors are used for external ones but we also accept "connector" attribute for that
				Array<Name> useConnectors;
				for_every(useNode, paramNode->children_named(TXT("use")))
				{
					Name useConnector = useNode->get_name_attribute(TXT("connector"));
					if (useConnector.is_valid())
					{
						useConnectors.push_back(useConnector);
					}
					else
					{
						error_loading_xml(useNode, TXT("missing connector for use"));
						result = false;
					}
				}
				if (paramConnector.is_valid())
				{
					useConnectors.push_back(paramConnector);
				}
				for_every(useConnector, useConnectors)
				{	// we will create children through this
					PieceCombiner::Connector<TextBlockGenerator> connector;
					connector.be_normal_connector();
					connector.make_required_single_only();
					connector.make_can_create_new_pieces(true);
					connector.set_link_ps_as_socket(useConnector->to_char());
					connector.def.paramId = paramId; // this will mark where we connect in text
					connectorIndices.push_back(textBlockPiece->add_connector(connector));
				}
			}

			if (!loadedPrivateTextBlocks && ownParamConnector)
			{
				error_loading_xml(paramNode, TXT("no private text blocks defined here and no connector defined, we will be using \"%S\" to connect stuff, which is unlikely to happen"), paramConnector.to_char());
				result = false;
			}

			if (loadedPrivateTextBlocks || ! ownParamConnector)
			{	// we will create children through this
				PieceCombiner::Connector<TextBlockGenerator> connector;
				connector.be_normal_connector();
				connector.make_required_single_only();
				connector.make_can_create_new_pieces(true);
				connector.set_link_ps_as_socket(paramConnector.to_char());
				connector.def.paramId = paramId; // this will mark where we connect in text
				connectorIndices.push_back(textBlockPiece->add_connector(connector));
			}

			textBlockPiece->make_connectors_block_each_other(connectorIndices);
		}
	}

	if (_node->first_child_named(TXT("mainTextBlock")) ||
		_node->first_child_named(TXT("isMainTextBlock")))
	{
		isMain = true;
	}

	result &= include.load_from_xml(_node, TXT("include"), _lc, TXT("include"));

	return result;
}

bool TextBlock::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		if (is_main()) // cache just for main includes
		{
			result &= include.cache_everything(_library, true); // skip mains, we won't be able to use them anyway
		}
	}

	return result;
}

void TextBlock::prepare_to_unload()
{
	base::prepare_to_unload();

	include.prepare_to_unload();

	textBlockPiece->def.textBlock.unlock_and_clear();

	// subTextBlocks are prepared to unload on their own as they are stored in main table
	subTextBlocks.clear();
}

String TextBlock::format_text_for_instance(tchar const * _format, StringFormatterParams & _params) const
{
	return StringFormatter::format_loc_str(text, _format, _params);
}

void TextBlock::add_sub_text_blocks(REF_ PieceCombiner::Generator<TextBlockGenerator> & _generator) const
{
	for_every(subTextBlock, subTextBlocks)
	{
		_generator.add_piece_type(subTextBlock->get()->textBlockPiece.get());
		subTextBlock->get()->add_sub_text_blocks(REF_ _generator);
	}
}

TextBlockInstancePtr TextBlock::create_instance(tchar const * _format, StringFormatterParams & _params, Random::Generator _usingGenerator) const
{
	if (textBlockPiece->connectors.is_empty())
	{
		// we do not have any connectors, we can skip using generator and just create instance
		// there's a hack to get non const verions of this, so instead "this" we have "textBlockPiece.def.textBlock.get()"
		return TextBlockInstance::build_from_text_block(textBlockPiece->def.textBlock.get(), _format, _params);
	}

	// setup generator
	PieceCombiner::Generator<TextBlockGenerator> generator;
	generator.access_generation_context().params = &_params;
	generator.use_should_use_connector(TextBlockGenerator::Utils::should_use_connector);

	if (include.is_empty())
	{
		for_available_text_blocks(textBlock, Library::get_current())
		{
			generator.add_piece_type(textBlock->textBlockPiece.get());
		}
	}
	else
	{
		for_every(textBlock, include.get_all())
		{
			generator.add_piece_type(textBlock->get()->textBlockPiece.get());
		}
	}

	add_sub_text_blocks(REF_ generator);

	// add main starting piece - this
	generator.add_piece(textBlockPiece.get(), TextBlockGenerator::PieceInstanceData(true));

	// alright, generate!
	PieceCombiner::GenerationResult::Type generationResult = generator.generate(&_usingGenerator);

	an_assert(generationResult == PieceCombiner::GenerationResult::Success);
	if (generationResult != PieceCombiner::GenerationResult::Success)
	{
		return TextBlockInstancePtr();
	}

	PieceCombiner::PieceInstance<TextBlockGenerator>* startingPiece = nullptr;
	for_every_ptr(piece, generator.access_all_piece_instances())
	{
		if (piece->get_data().isStarting)
		{
			startingPiece = piece;
			break;
		}
	}

	TextBlockInstancePtr result;
	if (startingPiece)
	{
		result = TextBlockInstance::build_from_piece(startingPiece, _format, _params);
	}

	return result;
}

TextBlockInstancePtr TextBlock::create_instance(tchar const * _format, IStringFormatterParamsProvider const * _provider, Random::Generator _usingGenerator) const
{
	return create_instance(_format, StringFormatterParams().add_params_provider(_provider), _usingGenerator);
}

TextBlockInstancePtr TextBlock::create_instance(tchar const * _format, ProvideStringFormatterParams _provideFunc, Random::Generator _usingGenerator) const
{
	return create_instance(_format, StringFormatterParams().add_params_provider(_provideFunc), _usingGenerator);
}

String TextBlock::format(tchar const * _format, StringFormatterParams & _params) const
{
	TextBlockInstancePtr tbi = create_instance(_format, _params);
	if (tbi.get())
	{
		return tbi->format();
	}
	else
	{
		error_stop(TXT("<ERROR FORMATTING TEXT BLOCK>"));
		return String(TXT("<ERROR FORMATTING TEXT BLOCK>"));
	}
}

String TextBlock::format(tchar const * _format, IStringFormatterParamsProvider const * _provider) const
{
	return format(_format, StringFormatterParams().add_params_provider(_provider));
}

String TextBlock::format(tchar const * _format, ProvideStringFormatterParams _provideFunc) const
{
	return format(_format, StringFormatterParams().add_params_provider(_provideFunc));
}
