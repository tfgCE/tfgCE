#include "latentStackVariables.h"

#include "..\math\math.h"

using namespace Latent;

StackDataBuffer::StackDataBuffer(StackVariablesBase* _inStackVariables)
: inStackVariables(_inStackVariables)
, prev(nullptr)
, next(nullptr)
{
	SET_EXTRA_DEBUG_INFO(data, TXT("StackDataBuffer.data"));

	if (! inStackVariables->dataBuffer)
	{
		inStackVariables->dataBuffer = this;
	}
	else
	{
		StackDataBuffer* lastOne = inStackVariables->dataBuffer;
		while (lastOne->next)
		{
			lastOne = lastOne->next;
		}
		lastOne->next = this;
		prev = lastOne;
	}
}

StackDataBuffer::~StackDataBuffer()
{
	an_assert(data.get_size() == 0, TXT("should be empty!"));
	if (prev)
	{
		prev->next = nullptr;
	}
	else if (inStackVariables && inStackVariables->dataBuffer == this)
	{
		inStackVariables->dataBuffer = nullptr;
	}
	if (next)
	{
		delete next;
	}
}

//

void* StackVariableEntry::get_data() const
{
	if (inDataBuffer->data.is_index_valid(at))
	{
		return &inDataBuffer->data[at];
	}
	else
	{
		return nullptr;
	}
}

void StackVariableEntry::destroy()
{
	an_assert(inDataBuffer != nullptr);
	an_assert(inDataBuffer->data.get_size() == at + size, TXT("we should be removing last variable!"));
	if (destructFunc)
	{
		void* ptr = get_data();
		an_assert(ptr != nullptr);
		destructFunc(ptr);
	}
	inDataBuffer->data.set_size(at);
	if (at == 0)
	{
		delete inDataBuffer;
	}
#if AN_DEBUG
	inDataBuffer = nullptr;
#endif
}

void StackVariableEntry::copy_from(StackVariableEntry const & _variable)
{
	an_assert(inDataBuffer);
	an_assert(at == NONE);
	at = _variable.at;
	size = _variable.size;
	type = _variable.type;
#ifdef AN_INSPECT_LATENT
	typeId = _variable.typeId;
#endif
	destructFunc = _variable.destructFunc;
	copyConstructFunc = _variable.copyConstructFunc;
	inDataBuffer->inStackVariables->find_place_for(size, inDataBuffer, at);
	if (copyConstructFunc)
	{
		copyConstructFunc(get_data(), _variable.get_data());
	}
}

//

StackVariablesBase::StackVariablesBase()
: dataBuffer(nullptr)
{
}

StackVariablesBase::~StackVariablesBase()
{
	delete_and_clear(dataBuffer);
}

void StackVariablesBase::find_place_for(int _size, OUT_ StackDataBuffer* & _dataBuffer, OUT_ int & _at)
{
	an_assert(_size <= StackDataBuffer::DATA_SIZE);

	StackDataBuffer* lastDataBuffer = dataBuffer;
	if (!lastDataBuffer)
	{
		lastDataBuffer = new StackDataBuffer(this);
	}
	else
	{
		while (lastDataBuffer->next)
		{
			lastDataBuffer = lastDataBuffer->next;
		}
	}
	if (lastDataBuffer->data.get_size() + _size <= lastDataBuffer->data.get_max_size())
	{
		_dataBuffer = lastDataBuffer;
	}
	else
	{
		_dataBuffer = new StackDataBuffer(this);
	}
	_at = _dataBuffer->data.get_size();
	_dataBuffer->data.grow_size(_size);
}

