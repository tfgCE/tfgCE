#include "utils_loadingForSystemShader.h"

#include "..\mainSettings.h"
#include "..\system\core.h"
#include "..\tags\tagCondition.h"

//

bool CoreUtils::Loading::should_load_for_system_or_shader_option(IO::XML::Node const* _forNode)
{
	bool okToLoadAnyOk = false;
	bool okToLoadAnyFailed = false;
	{
		TagCondition systemTagRequired;
		systemTagRequired.load_from_xml_attribute(_forNode, TXT("systemTagRequired"));
		if (!systemTagRequired.is_empty())
		{
			bool ok = systemTagRequired.check(::System::Core::get_system_tags());
			okToLoadAnyOk |= ok;
			okToLoadAnyFailed |= !ok;
		}
	}
	{
		TagCondition shaderOptionRequired;
		shaderOptionRequired.load_from_xml_attribute(_forNode, TXT("shaderOptionRequired"));
		if (!shaderOptionRequired.is_empty())
		{
			bool ok = shaderOptionRequired.check(::MainSettings::global().get_shader_options());
			okToLoadAnyOk |= ok;
			okToLoadAnyFailed |= !ok;
		}
	}
	return okToLoadAnyOk && !okToLoadAnyFailed;
}

