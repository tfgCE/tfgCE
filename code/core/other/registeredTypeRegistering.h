#ifndef AN_REGISTER_TYPES_OFFSET
#error define AN_REGISTER_TYPES_OFFSET as valid number! make sure there are no errors when running game as numbers don't offset each other, for easier use enter all offsets here!
#endif

//	based on:	http://stackoverflow.com/questions/16591642/combining-two-defined-symbols-in-c-preprocessor
//	quote:		All problems in computer science can be solved by an extra level of indirection:
//	note:		This is something that I used for for_every macros too

#define COMBINE_TWO_(a, b) a##b
#define COMBINE_TWO(a, b) COMBINE_TWO_(a,b)
#define STRINGIFY(a) #a

/**
 *		AN_REGISTER_TYPES_OFFSET
 *			core		1000
 *			framework	2000
 *			scavengers	3000
 */

#include "registeredType.h"

#include "parserUtils.h"

#include "..\casts.h"

#include "..\io\xml.h"
#include "..\memory\objectHelpers.h"
#include "..\memory\refCountObjectPtr.h"
#include "..\memory\safeObject.h"
#include "..\types\name.h"
#include "..\types\string.h"

#include <functional>

#define PARSE_VALUE_FN(_var) void (*_var)(String const & _value, void * _into)
#define SAVE_TO_XML_FN(_var) bool (*_var)(IO::XML::Node * _node, void const * _value)
#define LOAD_FROM_XML_FN(_var) bool (*_var)(IO::XML::Node const * _node, void * _into)
#define LOG_VALUE_FN(_var) void (*_var)(LogInfoContext & _log, void const * _value, tchar const * _name)
#define CONSTRUCTOR_DESTRUCTOR_FN(_var) void (*_var)(void* _value)
#define COPY_FN(_var) void (*_var)(void* _value, void const* _from)

struct LogInfoContext;

// this struct holds data about registered types and  allows as to have all types defined in continous list without dynamic allocations - it will be built when starting
struct RegisteredTypeInfo;

struct RegisteredSubField
{
	Name name;
	int id;
	size_t offset;

	RegisteredSubField() {}
	RegisteredSubField(Name const & _name, int _id, size_t _offset) : name(_name), id(_id), offset(_offset) {}
};

struct RegisteredTypeInfo
{
	static RegisteredTypeInfo* firstRegisteredTypeInfo;

	tchar const * const name;
	int id;
	size_t size;
	size_t alignment;
	bool plainData = false; // doesn't require constructor nor destructor, can be copied with memory_copy
	void const * initialValue = nullptr;
	RegisteredTypeInfo * next;
	Array<RegisteredSubField> subFields;
	PARSE_VALUE_FN(parse_value);
	SAVE_TO_XML_FN(save_to_xml);
	LOAD_FROM_XML_FN(load_from_xml);
	LOG_VALUE_FN(log_value);
	CONSTRUCTOR_DESTRUCTOR_FN(constructor);
	CONSTRUCTOR_DESTRUCTOR_FN(destructor);
	COPY_FN(copy);

	RegisteredTypeInfo(tchar const * const _name, int _id, size_t _size, size_t _alignment, bool _plainData, void const * _initialValue, CONSTRUCTOR_DESTRUCTOR_FN(_constructor), CONSTRUCTOR_DESTRUCTOR_FN(_destructor), COPY_FN(_copy))
	: name(_name)
	, id(_id)
	, size(_size)
	, alignment(_alignment)
	, plainData(_plainData)
	, initialValue(_initialValue)
	, parse_value(nullptr)
	, load_from_xml(nullptr)
	, log_value(nullptr)
	, constructor(_constructor)
	, destructor(_destructor)
	, copy(_copy)
	{
#ifdef AN_DEVELOPMENT
		RegisteredTypeInfo* check = firstRegisteredTypeInfo;
		while (check)
		{
			an_assert(check->id != id, TXT("types duplicate, please use different offset"));
			check = check->next;
		}
#endif
		// rearrange list
		next = firstRegisteredTypeInfo;
		firstRegisteredTypeInfo = this;
	}

	RegisteredSubField const * find_sub_field(Name const & _name) const
	{
		for_every(subField, subFields)
		{
			if (subField->name == _name)
			{
				return subField;
			}
		}

		return nullptr;
	}

	RegisteredTypeInfo & add_sub_field(Name const & _name, int _id, size_t _offset)
	{
		// only if we have not registered
		if (find_sub_field(_name) == nullptr)
		{
			subFields.push_back(RegisteredSubField(_name, _id, _offset));
		}
		return *this;
	}

	static RegisteredTypeInfo * find_by_name(tchar const * const _type)
	{
		RegisteredTypeInfo * iter = firstRegisteredTypeInfo;
		while (iter)
		{
			if (tstrcmpi(iter->name, _type) == 0)
			{
				return iter;
			}
			iter = iter->next;
		}
		return nullptr;
	}

	static RegisteredTypeInfo * find_by_id(int _id)
	{
		RegisteredTypeInfo * iter = firstRegisteredTypeInfo;
		while (iter)
		{
			if (iter->id == _id)
			{
				return iter;
			}
			iter = iter->next;
		}
		return nullptr;
	}
};

// parsing

#define ADD_SPECIALISED_PARSE_VALUE(Class, _function) \
	SetParseValueFunction temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_parse_value)(TXT(#Class), _function);

#ifdef AN_CLANG
	#define ADD_SPECIALISED_PARSE_VALUE_USING(Class, Function) \
		void parse__##Class(String const & _from, void * _into) \
		{ \
			CAST_TO_VALUE(Class, _into).Function(_from); \
		} \
		ADD_SPECIALISED_PARSE_VALUE(Class, parse__##Class);
#else
	#define ADD_SPECIALISED_PARSE_VALUE_USING(Class, Function) \
		void parse__##Class(String const & _from, void * _into) \
		{ \
			CAST_TO_VALUE(Class, _into).##Function(_from); \
		} \
		ADD_SPECIALISED_PARSE_VALUE(Class, parse__##Class);
#endif

struct SetParseValueFunction
{
	SetParseValueFunction(tchar const * const _type, PARSE_VALUE_FN(_function))
	{
		if (RegisteredTypeInfo * foundType = RegisteredTypeInfo::find_by_name(_type))
		{
			an_assert(foundType->parse_value == nullptr);
			foundType->parse_value = _function;
		}
		else
		{
			todo_important(TXT("couldn't find type \"%S\""), _type);
		}
	}
};

// loading from xml

#define ADD_SPECIALISED_LOAD_FROM_XML(Class, _function) \
	SetLoadFromXmlFunction temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_load_from_xml)(TXT(#Class), _function);

struct SetLoadFromXmlFunction
{
	SetLoadFromXmlFunction(tchar const * const _type, LOAD_FROM_XML_FN(_function))
	{
		if (RegisteredTypeInfo * foundType = RegisteredTypeInfo::find_by_name(_type))
		{
			an_assert(foundType->load_from_xml == nullptr);
			foundType->load_from_xml = _function;
		}
		else
		{
			todo_important(TXT("couldn't find type \"%S\""), _type);
		}
	}
};

#ifdef AN_CLANG
	#define ADD_SPECIALISED_LOAD_FROM_XML_USING(Class, Function) \
		bool temp_function_named(load_from_xml)(IO::XML::Node const * _node, void * _into) \
		{ \
			return CAST_TO_VALUE(Class, _into).Function(_node); \
		} \
		ADD_SPECIALISED_LOAD_FROM_XML(Class, temp_function_named(load_from_xml));
	#define ADD_SPECIALISED_LOAD_FROM_XML_USING_NAMED(Class, Name, Function) \
		bool temp_function_named(load_from_xml)(IO::XML::Node const * _node, void * _into) \
		{ \
			return CAST_TO_VALUE(Class, _into).Function(_node); \
		} \
		ADD_SPECIALISED_LOAD_FROM_XML(Name, temp_function_named(load_from_xml));
#else
	#define ADD_SPECIALISED_LOAD_FROM_XML_USING(Class, Function) \
		bool temp_function_named(load_from_xml)(IO::XML::Node const * _node, void * _into) \
		{ \
			return CAST_TO_VALUE(Class, _into).##Function(_node); \
		} \
		ADD_SPECIALISED_LOAD_FROM_XML(Class, temp_function_named(load_from_xml));
	#define ADD_SPECIALISED_LOAD_FROM_XML_USING_NAMED(Class, Name, Function) \
		bool temp_function_named(load_from_xml)(IO::XML::Node const * _node, void * _into) \
		{ \
			return CAST_TO_VALUE(Class, _into).##Function(_node); \
		} \
		ADD_SPECIALISED_LOAD_FROM_XML(Name, temp_function_named(load_from_xml));
#endif

// saving to xml

#define ADD_SPECIALISED_SAVE_TO_XML(Class, _function) \
	SetSaveToXmlFunction temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_save_to_xml)(TXT(#Class), _function);

#define ADD_SPECIALISED_SAVE_TO_XML_NAMED(Class, Name, _function) \
	SetSaveToXmlFunction temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_save_to_xml_named)(TXT(#Name), _function);

struct SetSaveToXmlFunction
{
	SetSaveToXmlFunction(tchar const * const _type, SAVE_TO_XML_FN(_function))
	{
		if (RegisteredTypeInfo * foundType = RegisteredTypeInfo::find_by_name(_type))
		{
			an_assert(foundType->save_to_xml == nullptr);
			foundType->save_to_xml = _function;
		}
		else
		{
			todo_important(TXT("couldn't find type \"%S\""), _type);
		}
	}
};

#ifdef AN_CLANG
	#define ADD_SPECIALISED_SAVE_TO_XML_USING(Class, Function) \
		bool temp_function_named(save_to_xml)(IO::XML::Node * _node, void const * _value) \
		{ \
			return CAST_TO_VALUE(Class, _value).Function(_node); \
		} \
		ADD_SPECIALISED_SAVE_TO_XML(Class, temp_function_named(save_to_xml));
	#define ADD_SPECIALISED_SAVE_TO_XML_USING_NAMED(Class, Name, Function) \
		bool temp_function_named(save_to_xml)(IO::XML::Node * _node, void const * _value) \
		{ \
			return CAST_TO_VALUE(Class, _value).Function(_node); \
		} \
		ADD_SPECIALISED_SAVE_TO_XML(Name, temp_function_named(save_to_xml));
#else
	#define ADD_SPECIALISED_SAVE_TO_XML_USING(Class, Function) \
		bool temp_function_named(save_to_xml)(IO::XML::Node * _node, void const * _value) \
		{ \
			return CAST_TO_VALUE(Class, _value).##Function(_node); \
		} \
		ADD_SPECIALISED_SAVE_TO_XML(Class, temp_function_named(save_to_xml));
	#define ADD_SPECIALISED_SAVE_TO_XML_USING_NAMED(Class, Name, Function) \
		bool temp_function_named(save_to_xml)(IO::XML::Node * _node, void const * _value) \
		{ \
			return CAST_TO_VALUE(Class, _value).##Function(_node); \
		} \
		ADD_SPECIALISED_SAVE_TO_XML(Name, temp_function_named(save_to_xml));
#endif

// logging values

#define ADD_SPECIALISED_LOG_VALUE(Class, _function) \
	SetLogValueFunction temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_log_value)(::type_id<Class>(), _function);

struct SetLogValueFunction
{
	SetLogValueFunction(int _typeId, LOG_VALUE_FN(_function))
	{
		if (RegisteredTypeInfo * foundType = RegisteredTypeInfo::find_by_id(_typeId))
		{
			an_assert(foundType->log_value == nullptr);
			foundType->log_value = _function;
		}
		else
		{
			todo_important(TXT("couldn't find type %i"), _typeId);
		}
	}
};

#define ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Class) \
	void COMBINE_TWO(log_value__, __LINE__)(LogInfoContext & _log, void const * _value, tchar const * _name) \
	{ \
		if (_name) \
		{ \
			_log.log(TXT("%S [%S] : %S"), _name, TXT(#Class), CAST_TO_VALUE(Class, _value).to_string().to_char()); \
		} \
		else \
		{ \
			_log.log(TXT("[%S] : %S"), TXT(#Class), CAST_TO_VALUE(Class, _value).to_string().to_char()); \
		} \
	} \
	ADD_SPECIALISED_LOG_VALUE(Class, COMBINE_TWO(log_value__, __LINE__));

#define ADD_SPECIALISED_LOG_VALUE_USING_LOG(Class) \
	void COMBINE_TWO(log_value__, __LINE__)(LogInfoContext & _log, void const * _value, tchar const * _name) \
	{ \
		if (_name) \
		{ \
			_log.log(TXT("%S [%S]"), _name, TXT(#Class)); \
		} \
		else \
		{ \
			_log.log(TXT("[%S]"), TXT(#Class)); \
		} \
		LOG_INDENT(_log); \
		CAST_TO_VALUE(Class, _value).log(_log); \
	} \
	ADD_SPECIALISED_LOG_VALUE(Class, COMBINE_TWO(log_value__, __LINE__));

#define ADD_SPECIALISED_LOG_VALUE_USING_LOG_PTR(Class) \
	void COMBINE_TWO(log_value__, __LINE__)(LogInfoContext & _log, void const * _value, tchar const * _name) \
	{ \
		if (_name) \
		{ \
			_log.log(TXT("%S [%S*]"), _name, TXT(#Class)); \
		} \
		else \
		{ \
			_log.log(TXT("[%S*]"), TXT(#Class)); \
		} \
		LOG_INDENT(_log); \
		if (auto* val = CAST_TO_VALUE(Class*, _value)) \
		{ \
			val->log(_log); \
		} \
	} \
	ADD_SPECIALISED_LOG_VALUE(Class*, COMBINE_TWO(log_value__, __LINE__));

#define ADD_SPECIALISED_LOG_VALUE_USING(Class, Function) \
	void COMBINE_TWO(log_value__, __LINE__)(LogInfoContext & _log, void const * _value, tchar const * _name) \
	{ \
		if (_name) \
		{ \
			_log.log(TXT("%S [%S]"), _name, TXT(#Class)); \
		} \
		else \
		{ \
			_log.log(TXT("[%S]"), TXT(#Class)); \
		} \
		LOG_INDENT(_log); \
		Function(_log, CAST_TO_VALUE(Class, _value)); \
	} \
	ADD_SPECIALISED_LOG_VALUE(Class, COMBINE_TWO(log_value__, __LINE__));

// type definitions

// remember to put DECLARE_REGISTERED_TYPE in headers
#define DEFINE_TYPE_GET_IDS(type) \
	template <> \
	int RegisteredType::get_id<type>() { return COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__); } \

#define DEFINE_TYPE_LOOK_UP(type) \
	DEFINE_TYPE_GET_IDS(type); \
	RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (TXT(#type), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(type), std::alignment_of<type>(), true, nullptr, nullptr, nullptr, nullptr);

#define DEFINE_TYPE_LOOK_UP_PLAIN_DATA(type, initialValuePtr) \
	DEFINE_TYPE_GET_IDS(type); \
	RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (TXT(#type), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(type), std::alignment_of<type>(), true, initialValuePtr, nullptr, nullptr, nullptr);

#define DEFINE_TYPE_LOOK_UP_PLAIN_DATA_NAMED(type, name, initialValuePtr) \
	DEFINE_TYPE_GET_IDS(type); \
	RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (TXT(#name), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(type), std::alignment_of<type>(), true, initialValuePtr, nullptr, nullptr, nullptr);

#define DEFINE_TYPE_LOOK_UP_OBJECT(type, initialValuePtr) \
	DEFINE_TYPE_GET_IDS(type); \
	RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (TXT(#type), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(type), std::alignment_of<type>(), false /* not plain data! */, initialValuePtr, ObjectHelpers<type>::call_constructor_on, ObjectHelpers<type>::call_destructor_on, ObjectHelpers<type>::call_copy_on);

// pointers are plain data without default value (it will be zeroes)
#ifdef AN_CLANG
	#define DEFINE_TYPE_LOOK_UP_PTR(type) \
		DEFINE_TYPE_GET_IDS(type*); \
		RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (TXT("Ptr::"#type), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(type*), std::alignment_of<type*>(), true, nullptr, nullptr, nullptr, nullptr);

	#define DEFINE_TYPE_LOOK_UP_SAFE_PTR(type) \
		DEFINE_TYPE_GET_IDS(SafePtr<type>); \
		RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (TXT("SafePtr::"#type), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(SafePtr<type>), std::alignment_of<SafePtr<type>>(), false /* not plain data! */, nullptr, ObjectHelpers<SafePtr<type>>::call_constructor_on, ObjectHelpers<SafePtr<type>>::call_destructor_on, ObjectHelpers<SafePtr<type>>::call_copy_on);

	#define DEFINE_TYPE_LOOK_UP_REF_COUNT_OBJECT_PTR(type) \
		DEFINE_TYPE_GET_IDS(RefCountObjectPtr<type>); \
		RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (TXT("RefCountObjectPtr::"#type), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(RefCountObjectPtr<type>), std::alignment_of<RefCountObjectPtr<type>>(), false /* not plain data! */, nullptr, ObjectHelpers<RefCountObjectPtr<type>>::call_constructor_on, ObjectHelpers<RefCountObjectPtr<type>>::call_destructor_on, ObjectHelpers<RefCountObjectPtr<type>>::call_copy_on);
#else
	#define PTR_PREFIX Ptr::
	#define DEFINE_TYPE_LOOK_UP_PTR(type) \
		DEFINE_TYPE_GET_IDS(type*); \
		RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (STRINGIZE(COMBINE_TWO(PTR_PREFIX,type)), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(type*), std::alignment_of<type*>(), true, nullptr, nullptr, nullptr, nullptr);

	#define SAFE_PTR_PREFIX SafePtr::
	#define DEFINE_TYPE_LOOK_UP_SAFE_PTR(type) \
		DEFINE_TYPE_GET_IDS(SafePtr<type>); \
		RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (STRINGIZE(COMBINE_TWO(SAFE_PTR_PREFIX,type)), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(SafePtr<type>), std::alignment_of<SafePtr<type>>(), false /* not plain data! */, nullptr, ObjectHelpers<SafePtr<type>>::call_constructor_on, ObjectHelpers<SafePtr<type>>::call_destructor_on, ObjectHelpers<SafePtr<type>>::call_copy_on);

	#define REF_COUNT_OBJECT_PTR_PREFIX RefCountObjectPtr::
	#define DEFINE_TYPE_LOOK_UP_REF_COUNT_OBJECT_PTR(type) \
		DEFINE_TYPE_GET_IDS(RefCountObjectPtr<type>); \
		RegisteredTypeInfo temp_container_named_two(AN_REGISTER_TYPES_OFFSET, register_type_look_up) (STRINGIZE(COMBINE_TWO(REF_COUNT_OBJECT_PTR_PREFIX,type)), COMBINE_TWO(AN_REGISTER_TYPES_OFFSET, __LINE__), sizeof(RefCountObjectPtr<type>), std::alignment_of<RefCountObjectPtr<type>>(), false /* not plain data! */, nullptr, ObjectHelpers<RefCountObjectPtr<type>>::call_constructor_on, ObjectHelpers<RefCountObjectPtr<type>>::call_destructor_on, ObjectHelpers<RefCountObjectPtr<type>>::call_copy_on);
#endif

//

// combined for enums, using "to_char"
#define ADD_SPECIALISATIONS_FOR_ENUM_USING_TO_CHAR(_enumSuperType) \
	void COMBINE_TWO(parse_value__, __LINE__)(String const& _from, void* _into) \
	{ \
		ParserUtils::parse_using_to_char<_enumSuperType>(_from, CAST_TO_VALUE(_enumSuperType::Type, _into)); \
	} \
	ADD_SPECIALISED_PARSE_VALUE(_enumSuperType::Type, COMBINE_TWO(parse_value__, __LINE__)); \
	\
	bool COMBINE_TWO(save_to_xml__, __LINE__)(IO::XML::Node* _node, void const * _value) \
	{ \
		_node->add_text(_enumSuperType::to_char(CAST_TO_VALUE(_enumSuperType::Type, _value))); \
		return true; \
	} \
	ADD_SPECIALISED_SAVE_TO_XML(_enumSuperType::Type, COMBINE_TWO(save_to_xml__, __LINE__)); \
	\
	bool COMBINE_TWO(load_from_xml__, __LINE__)(IO::XML::Node const * _node, void* _into) \
	{ \
		ParserUtils::parse_using_to_char<_enumSuperType>(_node->get_internal_text(), CAST_TO_VALUE(_enumSuperType::Type, _into)); \
		return true; \
	} \
	ADD_SPECIALISED_LOAD_FROM_XML(_enumSuperType::Type, COMBINE_TWO(load_from_xml__, __LINE__)); \
	\
	void COMBINE_TWO(log_value__, __LINE__)(LogInfoContext& _log, void const* _value, tchar const* _name) \
	{ \
		if (_name) \
		{ \
			_log.log(TXT("%S [%S] : %S"), _name, TXT(#_enumSuperType), _enumSuperType::to_char(CAST_TO_VALUE(_enumSuperType::Type, _value))); \
		} \
		else \
		{ \
			_log.log(TXT("[%S] : %S"), TXT(#_enumSuperType), _enumSuperType::to_char(CAST_TO_VALUE(_enumSuperType::Type, _value))); \
		} \
	} \
	ADD_SPECIALISED_LOG_VALUE(_enumSuperType::Type, COMBINE_TWO(log_value__, __LINE__));

//

#define ADD_SUB_FIELD(_class, _fieldClass, _fieldName) \
{ \
	_class object; \
	DEFINE_STATIC_NAME(_fieldName); \
	(*RegisteredTypeInfo::find_by_id(::type_id<_class>())). \
		add_sub_field(NAME(_fieldName), type_id<_fieldClass>(), (int8*)&object._fieldName - (int8*)&object); \
}

//

#define CAST_TO_VALUE(Type, what) (*plain_cast<Type>(what))
