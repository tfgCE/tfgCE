#include "gse_blendEmissivesForRoom.h"

#include "..\..\utils.h"

#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// variable
DEFINE_STATIC_NAME(timeLeft);
DEFINE_STATIC_NAME(timeSpent);

//

bool BlendEmissivesForRoom::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	XML_LOAD(_node, emissiveColour);
	XML_LOAD(_node, emissiveBaseColour);
	XML_LOAD(_node, emissivePower);

	XML_LOAD_FLOAT_ATTR(_node, forTime);

	XML_LOAD_FLOAT_ATTR(_node, blendTime);
	XML_LOAD_NAME_ATTR(_node, curve);

	XML_LOAD_NAME_ATTR(_node, roomVar);

	return result;
}

bool BlendEmissivesForRoom::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type BlendEmissivesForRoom::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	float& timeLeft = _execution.access_variables().access<float>(NAME(timeLeft));
	float& timeSpent = _execution.access_variables().access<float>(NAME(timeSpent));

	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		timeLeft = forTime;
		timeSpent = 0.0f;
	}

	float deltaTime = TutorialSystem::check_active() ? ::System::Core::get_raw_delta_time() : ::System::Core::get_delta_time();

	float prevTimeSpent = timeSpent;

	timeLeft -= deltaTime;
	timeSpent += deltaTime;

	float blendAmount;
	if (blendTime < 0.0f)
	{
		blendAmount = 1.0f;
	}
	else
	{
		if (forTime > 0.0f)
		{
			blendAmount = clamp(deltaTime / blendTime, 0.0f, 1.0f);
		}
		else
		{
			float prevT = clamp(prevTimeSpent / blendTime, 0.0f, 1.0f);
			float curT = clamp(timeSpent / blendTime, 0.0f, 1.0f);
			blendAmount = prevT < 1.0f ? (curT - prevT) / (1.0f - prevT) : 1.0f;
		}
	}

	if (roomVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(roomVar))
		{
			if (auto* room = exPtr->get())
			{
				TeaForGodEmperor::Utils::force_blend_emissives_for_room(room, emissiveColour, emissiveBaseColour, emissivePower, blendAmount);
			}
		}
	}

	if (forTime > 0.0f && timeLeft < 0.0f)
	{
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}

	if (forTime <= 0.0f && timeSpent >= blendTime)
	{
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}

	return Framework::GameScript::ScriptExecutionResult::Repeat;
}
