#include "pipelines.h"

#include "primitivesPipeline.h"

using namespace System;

void Pipelines::initialise_static()
{
	PrimitivesPipeline::initialise_static();
}

void Pipelines::setup_static()
{
	PrimitivesPipeline::setup_static();
}

void Pipelines::close_static()
{
	PrimitivesPipeline::close_static();
}

bool Pipelines::load_source_from(REF_ String & _source, IO::XML::Node const * _node, tchar const * _childName, tchar const * _errorWhenEmpty)
{
	if (IO::XML::Node const * child = _node->first_child_named(_childName))
	{
		_source = child->get_text();
		_source += String(TXT("\n"));
		return true;
	}
	else
	{
		error_loading_xml(_node, _errorWhenEmpty);
		return false;
	}
}


