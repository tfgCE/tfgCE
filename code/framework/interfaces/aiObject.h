#pragma once

#include "..\..\core\cachedPointer.h"
#include "..\..\core\fastCast.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\other\registeredType.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\..\core\types\string.h"
#endif

struct Vector3;
struct String;
struct Transform;

namespace Framework
{
	class Room;
	class IModulesOwner;
	class Object;

	namespace AI
	{
		class Message;
		typedef Array<Message const *> MessagesToProcess;
		class Hunch;
		typedef Array<RefCountObjectPtr<Hunch>> Hunches;
	};

	/**
	 *	This class is for any object that might be referred by AI
	 */
	interface_class IAIObject
	: public SafeObject<IAIObject>
	{
		FAST_CAST_DECLARE(IAIObject);
		FAST_CAST_END();
	public:
		IAIObject();
		virtual ~IAIObject() {}

	public:
		void cache_ai_object_pointers();

		IModulesOwner * get_ai_object_as_modules_owner() { return asIModulesOwner.get(); }
		IModulesOwner const * get_ai_object_as_modules_owner() const { return asIModulesOwner.get(); }
		Object * get_ai_object_as_object() { return asObject.get(); }
		Object const * get_ai_object_as_object() const { return asObject.get(); }

#ifdef AN_DEVELOPMENT_OR_PROFILER
		String const& ai_get_additional_info() const { return aiAdditionalInfo; }
		void ai_set_additional_info(String const& _info) { aiAdditionalInfo = _info; }
#endif

	public:

		virtual bool does_handle_ai_message(Name const& _messageName) const = 0;
		virtual AI::MessagesToProcess * access_ai_messages_to_process() = 0;

		virtual Transform ai_get_placement() const = 0;
		//virtual Rotator2 ai_get_direction();
		virtual Room* ai_get_room() const = 0;
		virtual String const& ai_get_name() const = 0;
		virtual IAIObject const * ai_instigator() const { return this; } // mind behind object (projectile is aiobject and projectile's instigator is someone who shot it)

	private:
		TypedCachedPointer<IModulesOwner> asIModulesOwner;
		TypedCachedPointer<Object> asObject;

#ifdef AN_DEVELOPMENT_OR_PROFILER
		String aiAdditionalInfo;
#endif
	};
};

DECLARE_REGISTERED_TYPE(Framework::IAIObject*);
DECLARE_REGISTERED_TYPE(SafePtr<Framework::IAIObject>);
