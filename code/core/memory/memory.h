#pragma once

#define MEMORY_H_INLCUDED

#include "..\globalDefinitions.h"
#include "..\globalInclude.h"

/**
 *	Those methods should be used to allocated and deallocate memory
 */
inline void* allocate_memory(size_t _size)
{
	// TODO throw exception when no memory available?
	return (void*)malloc(_size);
}

inline void* align_memory(void* ptr, size_t alignment)
{
	return (void*)(((size_t)ptr + alignment - 1) & (~(alignment - 1)));
}

// this patches of memory are not aligned. They are mostly useful for raw data
#ifdef AN_CLANG
#define allocate_stack(_size) alloca(_size)
#else
#define allocate_stack(_size) _alloca(_size)
#endif

// these macros give aligned memory on stack
#define allocate_stack_var(_type, _name, _amount) \
	size_t alignment##_name = std::alignment_of<_type>(); \
	size_t size##_name = (sizeof(_type));/* + (alignment##_name - 1)) & (~(alignment##_name - 1));*/ /* maybe it's not needed? */ \
	_type* _name = (_type*)allocate_stack(size##_name * (_amount) + (alignment##_name) - 1); \
	_name = (_type*)align_memory(_name, alignment##_name);

#define allocate_stack_var_size_align(_name, _size, _align) \
	void* _name = allocate_stack(_size + _align - 1); \
	_name = align_memory(_name, _align);

inline void free_memory(void* _pointer)
{
	free(_pointer);
}

inline void memory_copy(void* _dest, void const * _src, size_t _size)
{
	memcpy(_dest, _src, _size);
}

inline void memory_zero(void* _dest, size_t _size)
{
	memset(_dest, 0, _size);
}

template <typename Class>
inline void memory_zero(Class& _dest)
{
	memset(&_dest, 0, sizeof(_dest));
}

inline void memory_set(void* _dest, int _value, size_t _size)
{
	memset(_dest, _value, _size);
}

inline void memory_swap(void* _a, void * _b, size_t _size)
{
	byte temp[4];
	for(size_t i = 0; i < _size;)
	{
		if (i <= _size - 4)
		{
			memcpy(temp, _a, 4);
			memcpy(_a, _b, 4);
			memcpy(_b, temp, 4);
			i += 4;
		}
		else
		{
			memcpy(temp, _a, 1);
			memcpy(_a, _b, 1);
			memcpy(_b, temp, 1);
			i += 1;
		}
	}
}

inline void memory_invalidate(void* _dest, size_t _size)
{
	memset(_dest, 0xbabafeed, _size);
}

template <typename Class>
inline void variable_invalidate(Class & _var)
{
	memory_invalidate(&_var, sizeof(_var));
}

inline bool memory_compare_8(void const * _a, void const * _b, size_t _size)
{
	int8 const * a = (int8 const *)(_a);
	int8 const * b = (int8 const *)(_b);
	while (_size > 0)
	{
		if (*a != *b)
		{
			return false;
		}
		++a;
		++b;
		--_size;
	}
	return true;
}

inline bool memory_compare(void const * _a, void const * _b, size_t _size)
{
	return memcmp(_a, _b, _size) == 0;

	int32 const * a = (int32 const *)(_a);
	int32 const * b = (int32 const *)(_b);
	size_t sizeLeft = _size % 4;
	_size -= sizeLeft;
	while (_size > 0)
	{
		if (*a != *b)
		{
			return false;
		}
		++a;
		++b;
		_size -= 4;
	}
	return sizeLeft == 0 || memory_compare_8(a, b, sizeLeft);
}

inline bool memory_compare_8_is_zero(void const * _a, size_t _size)
{
	int8 const * a = (int8 const *)(_a);
	int8 const b = 0;
	while (_size > 0)
	{
		if (*a != b)
		{
			return false;
		}
		++a;
		--_size;
	}
	return true;
}

inline bool memory_compare_is_zero(void const * _a, size_t _size)
{
	int32 const * a = (int32 const *)(_a);
	int32 const b = 0;
	size_t sizeLeft = _size % 4;
	_size -= sizeLeft;
	while (_size > 0)
	{
		if (*a != b)
		{
			return false;
		}
		++a;
		_size -= 4;
	}
	return sizeLeft == 0 || memory_compare_8_is_zero(a, sizeLeft);
}

#include "..\debug\detectMemoryLeaks.h" // they include each other
