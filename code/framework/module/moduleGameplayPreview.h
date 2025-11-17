#pragma once

#include "moduleGameplay.h"

namespace Framework
{
	class ActorType;
	class ItemType;
	class ModuleGameplayPreviewData;
	struct CollisionInfo;

	struct ModuleGameplayPreviewControllerTransform
	{
		Name id;
		Transform placement = Transform::identity;
	};

	struct ModuleGameplayPreviewAttach
	{
		Name id;
		Name checkVar;

		UsedLibraryStored<ActorType> actorType;
		UsedLibraryStored<ItemType> itemType;
		Name attachToAttachedId;
		Name attachToSocket;
		Name attachViaSocket;

		Tags tags;
		
		SimpleVariableStorage variables;
		struct CopyVariable
		{
			Name from;
			Name to;
		};
		Array<CopyVariable> copyVariables;
	};

	class ModuleGameplayPreview
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleGameplayPreview);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static RegisteredModule<ModuleGameplay> & register_itself();

		ModuleGameplayPreview(IModulesOwner* _owner);
		virtual ~ModuleGameplayPreview();

	public: // Module
		override_ void setup_with(ModuleData const * _moduleData);
		override_ void initialise();

	public:
		int get_controller_transform_num() const { return controllerTransforms.get_size(); }
		int find_controller_transform_index(Name const & _id) const;
		Name const & get_controller_transform_id(int _idx) const;
		Transform const& get_controller_transform(int _idx) const;
		void set_controller_transform(int _idx, Transform const& _placement);

	private:
		ModuleGameplayPreviewData const* moduleGameplayPreviewData = nullptr;

		Array<ModuleGameplayPreviewControllerTransform> controllerTransforms;

		struct Attached
		{
			Name id;
			SafePtr<IModulesOwner> attached;
		};
		Array<Attached> attached;
	};

	class ModuleGameplayPreviewData
	: public ModuleData
	{
		FAST_CAST_DECLARE(ModuleGameplayPreviewData);
		FAST_CAST_BASE(ModuleData);
		FAST_CAST_END();
		typedef ModuleData base;
	public:
		ModuleGameplayPreviewData(LibraryStored* _inLibraryStored);

	protected:
		override_ bool read_parameter_from(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

	private:
		Array<ModuleGameplayPreviewControllerTransform> controllerTransforms;
		List<ModuleGameplayPreviewAttach> attaches;

		friend class ModuleGameplayPreview;
	};
};

