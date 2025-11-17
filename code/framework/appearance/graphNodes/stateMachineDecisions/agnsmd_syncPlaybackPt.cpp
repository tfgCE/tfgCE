#include "agnsmd_syncPlaybackPt.h"

#include "..\..\..\module\moduleMovement.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;
using namespace StateMachineDecisionsLib;

//--

REGISTER_FOR_FAST_CAST(SyncPlaybackPt);

IStateMachineDecision* SyncPlaybackPt::create_decision()
{
	return new SyncPlaybackPt();
}

bool SyncPlaybackPt::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD_NAME_ATTR(_node, id);
	XML_LOAD(_node, is);

	return result;
}

bool SyncPlaybackPt::prepare_runtime(AnimationGraphRuntime& _runtime)
{
	bool result = base::prepare_runtime(_runtime);

	return result;
}

bool SyncPlaybackPt::execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const
{
	bool result = true;
	{
		bool anyOk = false;
		bool anyFailed = false;

		if (id.is_valid())
		{
			if (auto* syncPt = _runtime.get_any_set_sync_playback(id))
			{
				if (is.is_set())
				{
					an_assert(syncPt->is_set());
					bool ok = is.get().does_contain(syncPt->get());
					anyOk |= ok;
					anyFailed |= !ok;
				}
			}
		}

		result = anyOk && ! anyFailed;
	}

	if (result)
	{
		result = base::execute(_stateMachine, _runtime);
	}

	return result;
}
