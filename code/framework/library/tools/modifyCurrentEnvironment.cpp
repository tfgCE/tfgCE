#include "libraryTools.h"

#include "..\..\..\core\mainSettings.h"
#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\types\names.h"

#include "..\..\game\tweakingContext.h"
#include "..\..\world\environment.h"

#ifdef AN_CLANG
#include "..\usedLibraryStored.inl"
#endif

using namespace ::Framework;

bool LibraryTools::modify_current_environment(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

#ifdef AN_TWEAKS
	if (void* currentEnvironmentVoid = TweakingContext::get(Names::currentEnviornment))
	{
		Environment* environment = plain_cast<Environment>(currentEnvironmentVoid);

		todo_note(TXT("do more environment params"));

		for_every(paramsNode, _node->children_named(TXT("params")))
		{
			for_every(paramNode, paramsNode->all_children())
			{
				EnvironmentParam param;
				if (param.load_from_xml(paramNode, _lc))
				{
					todo_note(TXT("prepare param?"));
					environment->access_info().access_params().tweak(param);
				}
				else
				{
					result = false;
				}
			}
		}
	}
#endif

	return result;
}
