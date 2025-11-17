#pragma once

#include "..\..\core\collision\iCollidableObject.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\types\names.h"
#include "..\..\core\tags\tag.h"

#include "..\interfaces\aiMindOwner.h"
#include "..\interfaces\destroyable.h"
#include "..\modulesOwner\modulesOwner.h"

#include "..\world\worldObject.h"

#include "objectType.h"

namespace Framework
{
	class World;
	class SubWorld;
	class ModuleAppearance;
	class ModulePresence;
	class ModuleGameplay;
	class Object;

	typedef std::function< bool(Object* _object) > OnFoundObject; // true to stop further looking

	/**
	 *	Call Init after creation.
	 *
	 *
	 */
	class Object
	: public IModulesOwner // +IAIObject
	, public IAIMindOwner
	, public Collision::ICollidableObject
	, public IDestroyable
	, public WorldObject
	//	IDebugableObject,
	//	IInteractiveObject,
	//	IInAdventureObject,
	//	IEventHandlerObject,
	//	IStatsModifiersProvider,
	//	IRatedObject,
	//	IAIObject,
	//	IRaterObject,
	//	ILibrarySolver,
	//	ILibraryCacheOwner,
	//	IAttacker,
	//	IAIMindOwner,
	//	IRenderableObject,
	//	ICollidableObject,
	//	IAppearanceInstanceOwner
	{
		FAST_CAST_DECLARE(Object);
		FAST_CAST_BASE(IModulesOwner);
		FAST_CAST_BASE(IAIMindOwner);
		FAST_CAST_BASE(Collision::ICollidableObject);
		FAST_CAST_BASE(IDestroyable);
		FAST_CAST_BASE(WorldObject);
		FAST_CAST_END();
	public:
		Object(ObjectType const * _objectType, String const & _name); // requires initialisation: init()
		virtual ~Object(); // should use destroy()

		bool is_sub_object() const { return objectType->is_sub_object(); }

		void learn_from(SimpleVariableStorage & _parameters); // not const as it should be possible to modify this storage (for any reason)
		
		Tags & access_tags() { return tags; }
		Tags const& get_tags() const { return tags; }
		ObjectType const * get_object_type() const { return objectType; }
		String const & get_name() const { return name; }

		void mark_to_activate(TemporaryObject* _temporaryObject);

		World* get_in_world() const { return WorldObject::get_in_world(); }

	protected: // SafeObject
		void make_safe_object_available(Object* _object);
		void make_safe_object_unavailable();

	public: // WorldObject
		implement_ void ready_to_world_activate(); // not from WorldObject but part of world activation
		implement_ void world_activate();

		implement_ String get_world_object_name() const { return ai_get_name(); }

	public: // IModulesOwner
		override_ void initialise_modules(SetupModule _setup_module_pre = nullptr);
		implement_ void cease_to_exist(bool _removeFromWorldImmediately) { destroy(_removeFromWorldImmediately); }

		implement_ void init(SubWorld* _subWorld); // and place in sub world, requires calling initialise modules seperately
		implement_ void on_destruct(); // remove from world definitely

		implement_ bool does_allow_to_attach(IModulesOwner const * _imo) const { return true; }
		implement_ bool should_do_calculate_final_pose_attachment_with(IModulesOwner const * _imo) const { return true; } // check temporary object

		implement_ LibraryName const & get_library_name() const { return objectType ? objectType->get_name() : LibraryName::invalid(); }

#ifdef AN_PERFORMANCE_MEASURE
	public:
		implement_ String const & get_performance_context_info() const { return name; }
#endif

	public: // IAIObject
		implement_ bool does_handle_ai_message(Name const& _messageName) const;
		implement_ AI::MessagesToProcess* access_ai_messages_to_process();
		implement_ String const & ai_get_name() const { return name; }
		implement_ Transform ai_get_placement() const;
		implement_ Room* ai_get_room() const;

	protected:	// IDestroyable
		friend class World;
		friend class SubWorld;
		override_ void destruct_and_delete();
		override_ void on_destroy(bool _removeFromWorldImmediately);
		override_ void remove_destroyable_from_world();

	private:
		ObjectType const * objectType;

		Concurrency::SpinLock worldActivationLock = Concurrency::SpinLock(TXT("Framework.Object.worldActivationLock")); // to not get in conflict with temporary objects to activate 

		bool initialised;

		String name;
		Tags tags;
		float individualityFactor;

		Array<TemporaryObject*> temporaryObjectsToActivate; // on world activate

	};
};

DECLARE_REGISTERED_TYPE(Framework::Object*);
