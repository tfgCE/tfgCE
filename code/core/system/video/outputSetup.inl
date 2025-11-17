template <typename OutputTextureDefinitionClass>
void OutputSetup<OutputTextureDefinitionClass>::copy_output_textures_from(OutputSetup const & _source)
{
	clear_output_textures();

	for_every(output, _source.outputs)
	{
		outputs.push_back(*output);
	}
}

template <typename OutputTextureDefinitionClass>
bool OutputSetup<OutputTextureDefinitionClass>::load_from_xml(IO::XML::Node const * _node, tchar const * _outputNodeName, OutputTextureDefinitionClass const & _defaultOutputTextureDefinition)
{
	bool result = true;
	if (!_node)
	{
		return false;
	}
	for_every(outputNode, _node->children_named(_outputNodeName))
	{
		OutputTextureDefinitionClass output = _defaultOutputTextureDefinition;
		if (output.load_from_xml(outputNode))
		{
			outputs.push_back(output);
		}
		else
		{
			result = false;
		}
	}
	return result;
}
