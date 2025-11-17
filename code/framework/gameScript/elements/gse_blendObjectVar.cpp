#include "gse_blendObjectVar.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\system\core.h"

#include "..\..\modulesOwner\modulesOwner.h"

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
DEFINE_STATIC_NAME(blendTimeLeft);

//

bool BlendObjectVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	blendTime = _node->get_float_attribute(TXT("blendTime"), blendTime);
	blendCurve = _node->get_name_attribute(TXT("blendCurve"), blendCurve);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	result &= params.load_from_xml(_node);

	mayFail = _node->get_bool_attribute_or_from_child_presence(TXT("mayFail"), mayFail);

	return result;
}

ScriptExecutionResult::Type BlendObjectVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	auto& blendTimeLeft = _execution.access_variables().access<float>(NAME(blendTimeLeft));

	float prevBlendTimeLeft = blendTimeLeft;
	if (is_flag_set(_flags, ScriptExecution::Flag::Entered))
	{
		prevBlendTimeLeft = blendTime;
		blendTimeLeft = blendTime;
	}

	{
		float deltaTime = ::System::Core::get_delta_time();
		blendTimeLeft = max(0.0f, blendTimeLeft - deltaTime);
	}

	float prevBlendPt = 0.0f;
	float blendPt = 1.0f;
	if (blendTime != 0.0f)
	{
		prevBlendPt = 1.0f - prevBlendTimeLeft / blendTime;
		blendPt = 1.0f - blendTimeLeft / blendTime;
	}

	prevBlendPt = BlendCurve::apply(blendCurve, prevBlendPt);
	blendPt = BlendCurve::apply(blendCurve, blendPt);

	if (blendTimeLeft != prevBlendTimeLeft || blendTimeLeft == 0.0f)
	{
		float blendNow = prevBlendPt < 1.0f ? 1.0f - (1.0f - blendPt) / (1.0f - prevBlendPt) : 1.0f;
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				Concurrency::ScopedSpinLock lock(imo->access_lock());
				params.do_for_every([imo, blendNow](SimpleVariableInfo const& _info)
					{
						if (_info.type_id() == type_id<float>())
						{
							float src = _info.get<float>();
							float & dest = imo->access_variables().access<float>(_info.get_name());
							dest = lerp(blendNow, dest, src);
						}
						else
						{
							error_dev_investigate(TXT("implement support"));
						}
					});
			}
		}
		else if (!mayFail)
		{
			warn(TXT("missing object"));
		}
	}

	return blendTimeLeft > 0? ScriptExecutionResult::Repeat : ScriptExecutionResult::Continue;
}
