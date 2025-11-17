#include "shaderFunctionLib.h"

#include "..\library\library.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(ShaderFunctionLib);
LIBRARY_STORED_DEFINE_TYPE(ShaderFunctionLib, shaderFunctionLib);

ShaderFunctionLib::ShaderFunctionLib(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, shaderSetup(new ::System::ShaderSetup())
{
}

bool ShaderFunctionLib::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("file")))
	{
		shaderSetup = ::System::ShaderSetup::from_file(::System::ShaderType::FunctionLib, _lc.get_path_in_dir(attr->get_value()));
	}
	else if (IO::XML::Node const * source = _node->first_child_named(TXT("rawSource")))
	{
		shaderSetup = ::System::ShaderSetup::from_string(::System::ShaderType::FunctionLib, source->get_text());
	}
	else if (IO::XML::Node const * source = _node->first_child_named(TXT("source")))
	{
		shaderSetup = ::System::ShaderSetup::will_use_source(::System::ShaderType::FunctionLib);
		shaderSetup->shaderSource.load_from_xml(source);
	}

	if (IO::XML::Node const * dependsOnNode = _node->first_child_named(TXT("dependsOn")))
	{
		for_every(shaderFunctionLibNode, dependsOnNode->children_named(TXT("shaderFunctionLib")))
		{
			LibraryName shaderFunctionLib;
			if (shaderFunctionLib.load_from_xml(shaderFunctionLibNode, TXT("name"), _lc))
			{
				dependsOnShaderFunctionLibs.push_back(shaderFunctionLib);
			}
			else
			{
				error_loading_xml(dependsOnNode, TXT("problem loading shader lib reference"));
			}
		}
	}

	return result;
}

bool ShaderFunctionLib::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	todo_note(TXT("check if libs exist?"));

	return result;
}

