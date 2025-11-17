#include "gse_collision.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\collision\shape.h"
#include "..\..\module\moduleCollision.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool Elements::Collision::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	disableCollidesWithReason.load_from_xml(_node, TXT("disableCollidesWithReason"));
	enableCollidesWithReason.load_from_xml(_node, TXT("enableCollidesWithReason"));

	return result;
}

bool Elements::Collision::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type Elements::Collision::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				if (auto* c = imo->get_collision())
				{
					bool updateCollidableObject = false;
					if (disableCollidesWithReason.is_set())
					{
						c->push_collides_with_flags(disableCollidesWithReason.get(), ::Collision::Flags::none());
						updateCollidableObject = true;
					}
					if (enableCollidesWithReason.is_set())
					{
						c->pop_collides_with_flags(enableCollidesWithReason.get());
						updateCollidableObject = true;
					}
					if (updateCollidableObject)
					{
						c->update_collidable_object();
					}
				}
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
