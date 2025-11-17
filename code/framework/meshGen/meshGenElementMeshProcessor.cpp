#include "meshGenElementMeshProcessor.h"

#include "meshGenGenerationContext.h"

#include "..\..\core\io\xml.h"

//

using namespace Framework;
using namespace MeshGeneration;

//

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			void add_operators();
		};
	};
};

//

REGISTER_FOR_FAST_CAST(ElementMeshProcessorOperator);

//

bool ElementMeshProcessorOperatorSet::load_from_xml_child_node(IO::XML::Node const* _node, LibraryLoadingContext& _lc, tchar const* _childNodeName)
{
	bool result = true;

	for_every(node, _node->children_named(_childNodeName))
	{
		result &= load_from_xml(node, _lc);
	}

	return result;
}

bool ElementMeshProcessorOperatorSet::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("generationCondition") ||
			node->get_name() == TXT("wheresMyPoint") ||
			node->get_name() == TXT("parameters") ||
			node->get_name() == TXT("optionalParameters") ||
			node->get_name() == TXT("requiredParameters"))
		{
			continue;
		}
		if (auto* op = RegisteredElementMeshProcessorOperators::create(node->get_name()))
		{
			RefCountObjectPtr<ElementMeshProcessorOperator> opPtr(op);
			if (opPtr->load_from_xml(node, _lc))
			{
				operators.push_back(opPtr);
			}
			else
			{
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("could not recognise mesh processor operator \"%S\""), node->get_name().to_char());
			result = false;
		}
	}

	return result;
}

bool ElementMeshProcessorOperatorSet::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every_ref(op, operators)
	{
		result &= op->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool ElementMeshProcessorOperatorSet::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = true;

	Array<int> mpIndices = _actOnMeshPartIndices;

	Array<int> newMPIndices;

	for_every_ref(op, operators)
	{
		newMPIndices.clear();
		result &= op->process(_context, _instance, mpIndices, newMPIndices);

		// we should continue with working on all added so far
		mpIndices.add_from(newMPIndices);
		_newMeshPartIndices.add_from(newMPIndices);
	}

	/*
	{
		LogInfoContext log;
		_context.log(log);
		log.output_to_output();
	}
	*/

	return result;
}

//

REGISTER_FOR_FAST_CAST(ElementMeshProcessor);

ElementMeshProcessor::~ElementMeshProcessor()
{
}

bool ElementMeshProcessor::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	operators.clear();
	result &= operators.load_from_xml(_node, _lc);

	againstCheckpointsUp = _node->get_int_attribute_or_from_child(TXT("againstCheckpointsUp"), againstCheckpointsUp);

	return result;
}

bool ElementMeshProcessor::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= operators.prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementMeshProcessor::process(GenerationContext & _context, ElementInstance & _instance) const
{
	Array<int> meshPartIndices;

	{
		Checkpoint checkpoint(_context);

		checkpoint = _context.get_checkpoint(1 + againstCheckpointsUp); // of parent

		Checkpoint now(_context);

		meshPartIndices.make_space_for(now.partsSoFarCount - checkpoint.partsSoFarCount);
		for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i)
		{
			meshPartIndices.push_back(i);
		}
	}

	Array<int> newMeshPartIndices;

	bool result = operators.process(_context, _instance, meshPartIndices, OUT_ newMeshPartIndices);

	if (!result)
	{
		error_generating_mesh_element(this, TXT("could not process mesh"));
	}

	return result;
}

//

void RegisteredElementMeshProcessorOperators::initialise_static()
{
	base::initialise_static();

	MeshProcessorOperators::add_operators();
}

void RegisteredElementMeshProcessorOperators::close_static()
{
	base::close_static();
}
