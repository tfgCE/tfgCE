#pragma once

#include "moduleData.h"

#include "..\..\core\containers\map.h"
#include "..\..\core\random\random.h"

namespace Framework
{
	class TemporaryObjectType;

	struct ModuleTemporaryObjectsSingle
	: public RefCountObject
	{
		mutable Random::Generator rg;
		struct Type
		{
			Framework::UsedLibraryStored<Framework::TemporaryObjectType> type;
			float probCoef = 1.0f;
			Type() {}
			explicit Type(Framework::UsedLibraryStored<Framework::TemporaryObjectType> const & _type, float _probCoef = 1.0f) : type(_type), probCoef(_probCoef) {}
		};
		Name id;
		Tags tags; // to know what kind of object this is
		TagCondition systemTagRequired;
		RangeInt companionCount = RangeInt(1); // only for companions
		Array<Type> types;
		Array<Name> copyVariables;
		SimpleVariableStorage variables;
		Array<Name> modulesAffectedByVariables;
		Name socket;
		Name socketPrefix;
		Name bone;
		Transform placement = Transform::identity; // relative placement
		Range randomDirAngle = Range::zero; // rotate placement, keeping up dir same
		Range3 randomOffsetLocation = Range3(Range::zero, Range::zero, Range::zero);
		Range3 randomOffsetRotation = Range3(Range::zero, Range::zero, Range::zero);
		Optional<bool> autoSpawn;		
		bool spawnDetached = false;

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		Transform ready_placement(OPTIONAL_ Random::Generator * _rg) const;

		Framework::TemporaryObjectType* get_type(Random::Generator & rng = Random::Generator::get_universal()) const;
	};

	class ModuleTemporaryObjectsData
	: public ModuleData
	{
		FAST_CAST_DECLARE(ModuleTemporaryObjectsData);
		FAST_CAST_BASE(ModuleData);
		FAST_CAST_END();

		typedef ModuleData base;
	public:
		ModuleTemporaryObjectsData(LibraryStored* _inLibraryStored);

		bool should_exist() const { return !temporaryObjects.is_empty() || forceExistence; }

	public:
		Array<RefCountObjectPtr<ModuleTemporaryObjectsSingle>> const& get_temporary_objects() const { return temporaryObjects; }

	public: // ModuleData
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected: // ModuleData
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:
		Array<RefCountObjectPtr<ModuleTemporaryObjectsSingle>> temporaryObjects;

		bool forceExistence = false;
	};
};


