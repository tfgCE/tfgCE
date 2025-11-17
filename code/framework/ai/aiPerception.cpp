#include "aiPerception.h"

#include "aiPerceptionRequest.h"

#include "..\object\object.h"

#include "..\..\core\concurrency\scopedSpinLock.h"

using namespace Framework;
using namespace AI;

//

DEFINE_STATIC_NAME(aiIgnore);

//

bool Perception::is_valid_for_perception(Object const* _object)
{
	if (_object->is_sub_object() ||
		_object->get_tags().get_tag(NAME(aiIgnore)))
	{
		return false;
	}
	return true;
}

Perception::Perception(MindInstance* _mind)
#ifndef AN_CLANG
#ifdef AN_DEVELOPMENT
: mind(_mind)
#endif
#endif
{

}

Perception::~Perception()
{
}

void Perception::advance()
{
	if (requests.is_empty())
	{
		return;
	}

	// one request per frame!
	if (PerceptionRequest* request = requests.get_first().get())
	{
		if (request->process())
		{
			requests.remove_at(0);
		}
	}
}

PerceptionRequest* Perception::add_request(PerceptionRequest* _request)
{
	Concurrency::ScopedSpinLock lock(requestsAdditionLock);
	requests.push_back(PerceptionRequestPtr(_request));
	return _request;
}
