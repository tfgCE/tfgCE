#pragma once

#include "..\..\framework\game\gameObject.h"

#include <functional>

struct Name;

namespace TeaForGodEmperor
{
	//class BulletinBoard;

	//typedef Framework::GameObjectPtr<BulletinBoard> BulletinBoardPtr;

	bool resolve_pending_game_objects();

	// this is actual game objects serialisation, not only pointers to them
	template<typename Class, typename Container>
	bool serialise_game_objects_container(Serialiser & _serialiser, REF_ Container & _container);
	//
	template<typename Class>
	bool serialise_game_objects(Serialiser & _serialiser, REF_ Array<Framework::GameObjectPtr<Class>> & _array);
	//
	template<typename Class>
	bool serialise_game_objects_list(Serialiser & _serialiser, REF_ List<Framework::GameObjectPtr<Class>> & _list);

	template<typename Class, typename Container>
	bool serialise_game_objects_container_with_class_name(Serialiser & _serialiser, REF_ Container & _container,
		std::function<Name(Class* _obj)> _get_class_name, std::function<Class*(Name& _className)> _create_object);
	//
	template<typename Class>
	bool serialise_game_objects_with_class_name(Serialiser & _serialiser, REF_ Array<Framework::GameObjectPtr<Class>> & _array,
		std::function<Name(Class* _obj)> _get_class_name, std::function<Class*(Name& _className)> _create_object);
	//
	template<typename Class>
	bool serialise_game_objects_list_with_class_name(Serialiser & _serialiser, REF_ List<Framework::GameObjectPtr<Class>> & _list,
		std::function<Name(Class* _obj)> _get_class_name, std::function<Class*(Name& _className)> _create_object);

	template<typename Class>
	bool serialise_ref_count_objects_with_class_name(Serialiser & _serialiser, REF_ Array<RefCountObjectPtr<Class>> & _array,
		std::function<Name(Class* _obj)> _get_class_name, std::function<Class*(Name& _className)> _create_object);

#ifdef AN_DEVELOPMENT
	int log_game_objects();
#endif

};

#include "gameObjectPtrs.inl"
